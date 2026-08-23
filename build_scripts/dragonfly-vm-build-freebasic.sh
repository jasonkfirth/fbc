#!/usr/bin/env bash

##############################################################################
# FreeBASIC DragonFly VM package builder
##############################################################################
#
# Purpose:
#
#   Build and test the DragonFly x86_64 FreeBASIC package from a Debian/Ubuntu
#   Linux host.
#
# Responsibilities:
#
#   * download or reuse an official DragonFly x86_64 live IMG
#   * boot the live image under QEMU with a serial console
#   * attach a writable work disk for packages, source, and build output
#   * build the DragonFly package with build_scripts/dragonfly-build-freebasic.sh
#   * install the package in a fresh live VM snapshot
#   * run binding, console, gfxlib, sfxlib, fbctests, and Exampleageddon checks
#   * capture QEMU audio output and verify that it is not silent
#
# This script intentionally does NOT contain:
#
#   * non-x86_64 DragonFly support
#   * DragonFly cross-compilation
#   * GUI installer automation
#
##############################################################################

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORKROOT="$ROOT/out/dragonfly-vm"
CACHE_DIR="$WORKROOT/cache"
RUN_DIR="$WORKROOT/run"
SERVE_DIR="$RUN_DIR/serve"
UPLOAD_DIR="$RUN_DIR/upload"
LOG_DIR="$WORKROOT/logs"
PACKAGE_DIR="$WORKROOT/packages"
ARCHIVE_DIR="$ROOT/out/dragonfly/x86-64"

RELEASE="6.4.2"
IMAGE_URL=""
IMAGE_FILE=""
RAW_IMAGE=""
PACKAGE_FILE=""
TEST_ONLY=0
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"
CPUS="$JOBS"
FBCTESTS_JOBS="$JOBS"
EXAMPLEAGEDDON_JOBS="$JOBS"
EXAMPLEAGEDDON_COMPILE_TIMEOUT="120"
EXAMPLEAGEDDON_RUN_TIMEOUT="10"
MEMORY="6144"
WORK_DISK_SIZE="32G"
HTTP_PORT=""
KEEP_VM=0
QEMU_ACCEL="${QEMU_ACCEL:-kvm}"
QEMU_CPU="${QEMU_CPU:-host}"

DEFAULT_IMAGE_URL="https://mirror-master.dragonflybsd.org/iso-images/dfly-x86_64-${RELEASE}_REL.img.bz2"

msg() { printf '\n==> %s\n' "$*"; }
warn() { printf '\nWARNING: %s\n' "$*" >&2; }
die() { printf '\nERROR: %s\n' "$*" >&2; exit 1; }

usage() {
	cat <<EOF
Usage: ./build_scripts/dragonfly-vm-build-freebasic.sh [options]

Options:
  --release N            DragonFly release. Default: 6.4.2
  --image-url URL        DragonFly x86_64 live .img.bz2 or .img URL.
  --image FILE           Existing DragonFly x86_64 live .img.bz2 or .img.
  --package FILE         Existing DragonFly .pkg to test.
  --test-only            Test --package without rebuilding FreeBASIC.
  --workroot DIR         Work directory. Default: out/dragonfly-vm
  --archive-dir DIR      Final archive directory. Default: out/dragonfly/x86-64
  --jobs N               Build jobs inside DragonFly. Default: host CPU count
  --cpus N               QEMU CPU count. Default: --jobs value
  --fbctests-jobs N      fbctests unit-test jobs. Default: --jobs value
  --exampleageddon-jobs N
                         Exampleageddon jobs. Default: --jobs value
  --exampleageddon-compile-timeout N
                         Per-example compile timeout. Default: 120
  --exampleageddon-run-timeout N
                         Per-example run timeout. Default: 10
  --memory MB            QEMU memory in MB. Default: 6144
  --work-disk-size SIZE  Per-VM writable work disk size. Default: 32G
  --http-port N          Host bootstrap HTTP port. Default: auto
  --keep-vm              Keep VM run artifacts on success.
  -h, --help             Show this help.

The script builds DragonFly x86_64 only.
EOF
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--release)
			RELEASE="$2"
			DEFAULT_IMAGE_URL="https://mirror-master.dragonflybsd.org/iso-images/dfly-x86_64-${RELEASE}_REL.img.bz2"
			shift 2
			;;
		--image-url) IMAGE_URL="$2"; shift 2 ;;
		--image) IMAGE_FILE="$2"; shift 2 ;;
		--package) PACKAGE_FILE="$2"; shift 2 ;;
		--test-only) TEST_ONLY=1; shift ;;
		--workroot)
			WORKROOT="$2"
			CACHE_DIR="$WORKROOT/cache"
			RUN_DIR="$WORKROOT/run"
			SERVE_DIR="$RUN_DIR/serve"
			UPLOAD_DIR="$RUN_DIR/upload"
			LOG_DIR="$WORKROOT/logs"
			PACKAGE_DIR="$WORKROOT/packages"
			shift 2
			;;
		--archive-dir) ARCHIVE_DIR="$2"; shift 2 ;;
		--jobs) JOBS="$2"; CPUS="$2"; FBCTESTS_JOBS="$2"; EXAMPLEAGEDDON_JOBS="$2"; shift 2 ;;
		--cpus) CPUS="$2"; shift 2 ;;
		--fbctests-jobs) FBCTESTS_JOBS="$2"; shift 2 ;;
		--exampleageddon-jobs) EXAMPLEAGEDDON_JOBS="$2"; shift 2 ;;
		--exampleageddon-compile-timeout) EXAMPLEAGEDDON_COMPILE_TIMEOUT="$2"; shift 2 ;;
		--exampleageddon-run-timeout) EXAMPLEAGEDDON_RUN_TIMEOUT="$2"; shift 2 ;;
		--memory) MEMORY="$2"; shift 2 ;;
		--work-disk-size) WORK_DISK_SIZE="$2"; shift 2 ;;
		--http-port) HTTP_PORT="$2"; shift 2 ;;
		--keep-vm) KEEP_VM=1; shift ;;
		-h|--help) usage; exit 0 ;;
		*) die "unknown option: $1" ;;
	esac
done

case "$JOBS" in ''|*[!0-9]*|0) die "--jobs must be a positive integer" ;; esac
case "$CPUS" in ''|*[!0-9]*|0) die "--cpus must be a positive integer" ;; esac
case "$FBCTESTS_JOBS" in ''|*[!0-9]*|0) die "--fbctests-jobs must be a positive integer" ;; esac
case "$EXAMPLEAGEDDON_JOBS" in ''|*[!0-9]*|0) die "--exampleageddon-jobs must be a positive integer" ;; esac
case "$EXAMPLEAGEDDON_COMPILE_TIMEOUT" in ''|*[!0-9]*|0) die "--exampleageddon-compile-timeout must be a positive integer" ;; esac
case "$EXAMPLEAGEDDON_RUN_TIMEOUT" in ''|*[!0-9]*|0) die "--exampleageddon-run-timeout must be a positive integer" ;; esac

if [ "$TEST_ONLY" -eq 1 ] && [ -z "$PACKAGE_FILE" ]; then
	die "--test-only requires --package"
fi

require_tool() {
	command -v "$1" >/dev/null 2>&1 || die "required tool not found: $1"
}

find_free_port() {
	python3 - "$1" <<'PY'
import socket
import sys

start = int(sys.argv[1])

for port in range(start, start + 2000):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:
            s.bind(("127.0.0.1", port))
        except OSError:
            continue
        print(port)
        raise SystemExit(0)

raise SystemExit("no free TCP port found")
PY
}

check_host_tools() {
	require_tool bash
	require_tool bunzip2
	require_tool curl
	require_tool python3
	require_tool qemu-img
	require_tool qemu-system-x86_64
	require_tool sha256sum
	require_tool tar

	python3 - <<'PY'
import pexpect
PY
}

configure_qemu_acceleration() {
	if [ ! -r /dev/kvm ] || [ ! -w /dev/kvm ]; then
		QEMU_ACCEL="tcg"
		QEMU_CPU="max"
	fi

	msg "QEMU acceleration: $QEMU_ACCEL (CPU: $QEMU_CPU)"
}

resolve_package_file() {
	[ -n "$PACKAGE_FILE" ] || return 0
	[ -f "$PACKAGE_FILE" ] || die "package not found: $PACKAGE_FILE"

	PACKAGE_FILE="$(cd "$(dirname "$PACKAGE_FILE")" && pwd)/$(basename "$PACKAGE_FILE")"
}

download_image() {
	mkdir -p "$CACHE_DIR" "$LOG_DIR" "$PACKAGE_DIR"

	if [ -n "$IMAGE_FILE" ]; then
		[ -f "$IMAGE_FILE" ] || die "image not found: $IMAGE_FILE"
		IMAGE_FILE="$(cd "$(dirname "$IMAGE_FILE")" && pwd)/$(basename "$IMAGE_FILE")"
		msg "Using DragonFly image $IMAGE_FILE"
		return 0
	fi

	if [ -z "$IMAGE_URL" ]; then
		IMAGE_URL="$DEFAULT_IMAGE_URL"
	fi

	IMAGE_FILE="$CACHE_DIR/$(basename "$IMAGE_URL")"
	if [ ! -f "$IMAGE_FILE" ]; then
		msg "Downloading $IMAGE_URL"
		curl -fL --retry 3 --retry-delay 5 -o "$IMAGE_FILE" "$IMAGE_URL"
	else
		msg "Using cached image $IMAGE_FILE"
	fi
}

extract_image() {
	case "$IMAGE_FILE" in
		*.bz2)
			RAW_IMAGE="$CACHE_DIR/$(basename "${IMAGE_FILE%.bz2}")"
			if [ ! -f "$RAW_IMAGE" ]; then
				msg "Extracting DragonFly live image"
				bunzip2 -kc "$IMAGE_FILE" > "$RAW_IMAGE"
			fi
			;;
		*)
			RAW_IMAGE="$IMAGE_FILE"
			;;
	esac
}

make_source_archive() {
	msg "Packing source tree for DragonFly"
	mkdir -p "$SERVE_DIR" "$UPLOAD_DIR" "$LOG_DIR" "$PACKAGE_DIR"
	rm -f "$SERVE_DIR/freebasic-source.tar.gz"

	tar -czf "$SERVE_DIR/freebasic-source.tar.gz" \
		--exclude='./.git' \
		--exclude='./out' \
		--exclude='./build' \
		--exclude='./.build*' \
		--exclude='./OMA' \
		--exclude='./OMA_old' \
		--exclude='./remote_probe_temp' \
		--exclude='./nuttx-suite-logs' \
		--exclude='./package-root' \
		--exclude='./bin/fbc*' \
		--exclude='./bootstrap/fbc*' \
		--exclude='*/obj' \
		--exclude='*.o' \
		--exclude='*.a' \
		-C "$ROOT" .
}

write_upload_server() {
	cat > "$RUN_DIR/upload-server.py" <<'PY'
import http.server
import os
import posixpath
import sys

serve_dir = os.path.abspath(sys.argv[1])
upload_dir = os.path.abspath(sys.argv[2])
port = int(sys.argv[3])

os.makedirs(upload_dir, exist_ok=True)

class Handler(http.server.SimpleHTTPRequestHandler):
    def translate_path(self, path):
        path = path.split("?", 1)[0].split("#", 1)[0]
        path = posixpath.normpath(path)
        words = [word for word in path.split("/") if word]

        if words and words[0] == "upload":
            return os.path.join(upload_dir, *words[1:])

        return os.path.join(serve_dir, *words)

    def do_PUT(self):
        path = self.translate_path(self.path)
        if not os.path.abspath(path).startswith(upload_dir + os.sep):
            self.send_error(403)
            return

        os.makedirs(os.path.dirname(path), exist_ok=True)
        length = int(self.headers.get("Content-Length", "0"))

        with open(path, "wb") as f:
            remaining = length
            while remaining > 0:
                chunk = self.rfile.read(min(1024 * 1024, remaining))
                if not chunk:
                    break
                f.write(chunk)
                remaining -= len(chunk)

        self.send_response(201)
        self.end_headers()

with http.server.ThreadingHTTPServer(("0.0.0.0", port), Handler) as httpd:
    httpd.serve_forever()
PY
}

start_upload_server() {
	write_upload_server
	python3 "$RUN_DIR/upload-server.py" "$SERVE_DIR" "$UPLOAD_DIR" "$HTTP_PORT" \
		> "$RUN_DIR/http.log" 2>&1 &
	echo $! > "$RUN_DIR/http.pid"
}

stop_upload_server() {
	if [ -f "$RUN_DIR/http.pid" ]; then
		kill "$(cat "$RUN_DIR/http.pid")" 2>/dev/null || true
		rm -f "$RUN_DIR/http.pid"
	fi
}

prepare_vm_disks() {
	local name="$1"
	local vm_dir="$RUN_DIR/$name"

	rm -rf "$vm_dir"
	mkdir -p "$vm_dir"
	cp --sparse=always "$RAW_IMAGE" "$vm_dir/dragonfly.img"
	truncate -s "$WORK_DISK_SIZE" "$vm_dir/work.raw"
}

qemu_args() {
	local vm_dir="$1"
	local audio_wav="${2:-}"

	printf '%s\n' \
		qemu-system-x86_64 \
		-accel "$QEMU_ACCEL" \
		-cpu "$QEMU_CPU" \
		-m "$MEMORY" \
		-smp "$CPUS" \
		-drive "file=$vm_dir/dragonfly.img,format=raw,if=ide,index=0" \
		-drive "file=$vm_dir/work.raw,format=raw,if=ide,index=1" \
		-netdev "user,id=net0" \
		-device "e1000,netdev=net0" \
		-nographic \
		-monitor none \
		-serial stdio

	if [ -n "$audio_wav" ]; then
		printf '%s\n' \
			-audiodev "wav,id=audio0,path=$audio_wav" \
			-device "ES1370,audiodev=audio0"
	fi
}

run_qemu_guest_script() {
	local console_log="$1"
	local guest_command="$2"
	shift 2

	python3 - "$console_log" "$guest_command" "$@" <<'PY'
import os
import pexpect
import signal
import sys
import time

console_log = sys.argv[1]
guest_command = sys.argv[2]
qemu_cmd = sys.argv[3:]

os.makedirs(os.path.dirname(console_log), exist_ok=True)
log = open(console_log, "w", encoding="utf-8", errors="replace")
child = pexpect.spawn(qemu_cmd[0], qemu_cmd[1:], encoding="utf-8", timeout=900)
child.logfile_read = log

def stop_qemu():
    if child.isalive():
        child.kill(signal.SIGTERM)
        time.sleep(2)
    if child.isalive():
        child.kill(signal.SIGKILL)

def send_loader_command(command):
    for character in command:
        child.send(character)
        time.sleep(0.03)
    child.send("\r")

try:
    child.expect("Booting in", timeout=90)
    child.send("9")
    child.expect("OK", timeout=60)

    # Keep loader input on the channel that pexpect is already using, but ask
    # the kernel to use COM1 after boot.  Switching the loader's own console to
    # comconsole here would also move its input before we can submit "boot".
    send_loader_command("set boot_serial=YES")
    child.expect("OK", timeout=60)
    send_loader_command("boot")
    child.expect("login:", timeout=240)
    child.sendline("root")
    child.expect("#", timeout=60)
    child.sendline("exec /bin/sh")
    child.expect("#", timeout=60)
    child.sendline(guest_command)
    child.expect(r"exit_status=([0-9]+)=", timeout=None)
    status = int(child.match.group(1))
    try:
        child.expect(pexpect.EOF, timeout=120)
    except pexpect.TIMEOUT:
        stop_qemu()
    if status != 0:
        raise SystemExit(f"guest command failed with exit_status={status}")
except Exception:
    stop_qemu()
    raise
finally:
    log.close()
PY
}

collect_guest_logs() {
	local name

	mkdir -p "$LOG_DIR"

	for name in freebasic-dragonfly-build.log freebasic-dragonfly-test.log; do
		if [ -f "$UPLOAD_DIR/$name" ]; then
			cp -f "$UPLOAD_DIR/$name" "$LOG_DIR/$name"
		fi
	done
}

write_guest_build_script() {
	cat > "$SERVE_DIR/dragonfly-build-run.sh" <<EOF
#!/bin/sh
set -eu

PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:\$PATH"
export PATH
export JOBS="$JOBS"
export PKG_CACHEDIR="/work/pkg-cache"
export TMPDIR="/work/tmp"

log=/tmp/freebasic-dragonfly-build.log
rm -f "\$log"

upload() {
	file="\$1"
	name="\$2"
	if command -v curl >/dev/null 2>&1 && [ -f "\$file" ]; then
		curl -fsS -T "\$file" "http://10.0.2.2:$HTTP_PORT/upload/\$name" || true
	fi
}

finish() {
	rc=\$?
	upload "\$log" "freebasic-dragonfly-build.log"
	exit "\$rc"
}
trap finish EXIT

(
set -eu

hostname fbc-dragonfly
dhclient em0 || true

newfs -U /dev/ad1s0
mkdir -p /work
mount /dev/ad1s0 /work

mkdir -p /work/local /work/tmp /work/pkg-cache /work/freebasic-source
chmod 1777 /work/tmp
cp -a /usr/local/. /work/local/
mount_null /work/local /usr/local || mount_nullfs /work/local /usr/local

pkg update -f
pkg install -y curl

fetch -o /tmp/freebasic-source.tar.gz "http://10.0.2.2:$HTTP_PORT/freebasic-source.tar.gz"
tar -xzf /tmp/freebasic-source.tar.gz -C /work/freebasic-source

cd /work/freebasic-source
BUILDROOT=/work/freebasic-build \\
STAGE=/work/freebasic-build/stage \\
PKGROOT=/work/freebasic-build/pkgroot \\
PKGMETA=/work/freebasic-build/pkgmeta \\
OUT=/work/freebasic-source/out \\
	/bin/sh ./build_scripts/dragonfly-build-freebasic.sh

pkg="\$(find /work/freebasic-source/out -maxdepth 1 -type f -name 'freebasic-*.pkg' | sort | tail -n 1)"
[ -n "\$pkg" ] && [ -f "\$pkg" ]
upload "\$pkg" "\$(basename "\$pkg")"

echo "==> uploaded package \$(basename "\$pkg")"
) > "\$log" 2>&1 &
pid=\$!

while kill -0 "\$pid" 2>/dev/null; do
	sleep 60
	if kill -0 "\$pid" 2>/dev/null; then
		upload "\$log" "freebasic-dragonfly-build.log"
		printf 'DragonFly build still running: '
		tail -n 1 "\$log" 2>/dev/null | tr '\000' ' ' | cut -c 1-160 || true
	fi
done

if ! wait "\$pid"; then
	echo "DragonFly package build failed; final guest log follows:" >&2
	tail -n 160 "\$log" >&2 || true
	exit 1
fi
EOF
}

write_guest_test_script() {
	cat > "$SERVE_DIR/dragonfly-test-run.sh" <<EOF
#!/bin/sh
set -eu

PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:\$PATH"
export PATH
export FBCTESTS_JOBS="$FBCTESTS_JOBS"
export EXAMPLEAGEDDON_JOBS="$EXAMPLEAGEDDON_JOBS"
export EXAMPLEAGEDDON_COMPILE_TIMEOUT="$EXAMPLEAGEDDON_COMPILE_TIMEOUT"
export EXAMPLEAGEDDON_RUN_TIMEOUT="$EXAMPLEAGEDDON_RUN_TIMEOUT"
export PKG_CACHEDIR="/work/pkg-cache"
export TMPDIR="/work/tmp"
export SFXLIB_OSS_DEVICE="/dev/dsp"

log=/tmp/freebasic-dragonfly-test.log
rm -f "\$log"

upload() {
	file="\$1"
	name="\$2"
	if command -v curl >/dev/null 2>&1 && [ -f "\$file" ]; then
		curl -fsS -T "\$file" "http://10.0.2.2:$HTTP_PORT/upload/\$name" || true
	fi
}

finish() {
	rc=\$?
	upload "\$log" "freebasic-dragonfly-test.log"
	exit "\$rc"
}

run() {
	echo "==> \$*"
	"\$@"
}

fail() {
	echo "ERROR: \$*" >&2
	exit 1
}

fbctests_jobs() {
	case "\${FBCTESTS_JOBS:-}" in
		''|*[!0-9]*|0) echo 1 ;;
		*) echo "\$FBCTESTS_JOBS" ;;
	esac
}

exampleageddon_jobs() {
	case "\${EXAMPLEAGEDDON_JOBS:-}" in
		''|*[!0-9]*|0) echo 1 ;;
		*) echo "\$EXAMPLEAGEDDON_JOBS" ;;
	esac
}

run_gfx_smoke() {
	out="\$1"
	err="\$2"
	shift 2

	if timeout 30 "\$@" > "\$out" 2> "\$err"; then
		cat "\$out" || true
		[ ! -s "\$err" ] || {
			cat "\$err"
			fail "gfx smoke wrote stderr"
		}
		return 0
	fi

	cat "\$out" || true
	cat "\$err" || true
	fail "gfx smoke failed"
}

start_xvfb() {
	Xvfb :99 -screen 0 800x600x24 > /tmp/freebasic-xvfb.log 2>&1 &
	echo \$! > /tmp/freebasic-xvfb.pid
	export DISPLAY=:99
	sleep 2

	if ! kill -0 "\$(cat /tmp/freebasic-xvfb.pid)" 2>/dev/null; then
		cat /tmp/freebasic-xvfb.log || true
		fail "Xvfb failed to start"
	fi
}

prepare_audio() {
	echo "==> preparing DragonFly audio device"

	kldload snd_es137x >/tmp/freebasic-sound.log 2>&1 || true

	for n in 1 2 3 4 5; do
		[ -c /dev/dsp ] && {
			ls -l /dev/dsp* /dev/mixer* 2>/dev/null || true
			return 0
		}
		sleep 1
	done

	cat /tmp/freebasic-sound.log || true
	kldstat || true
	dmesg | tail -n 80 || true
	fail "DragonFly OSS audio device was not created"
}

stop_xvfb() {
	if [ -f /tmp/freebasic-xvfb.pid ]; then
		kill "\$(cat /tmp/freebasic-xvfb.pid)" 2>/dev/null || true
		rm -f /tmp/freebasic-xvfb.pid
	fi
}

finish_with_xvfb() {
	rc=\$?
	stop_xvfb
	upload "\$log" "freebasic-dragonfly-test.log"
	exit "\$rc"
}

run_fbctests() {
	jobs="\$(fbctests_jobs)"

	cd /work/freebasic-source/tests

	echo "==> cleaning fbctests tree"
	run gmake clean FBC=fbc

	echo "==> checking installed compiler through fbctests"
	run gmake check FBC=fbc

	echo "==> running unit-tests with \${jobs} job(s)"
	run gmake -j "\$jobs" unit-tests FBC=fbc

	echo "==> running log-tests serially"
	run gmake log-tests FBC=fbc

	for failed_log in failed-fb.log failed-fblite.log failed-qb.log failed-deprecated.log; do
		[ -f "\$failed_log" ] || fail "missing log-tests summary: \$failed_log"
		if ! grep -qi 'None Found' "\$failed_log"; then
			cat "\$failed_log"
			grep ': RESULT=FAILED' "\$failed_log" | while IFS=: read -r test_log rest; do
				[ -f "\$test_log" ] || continue
				echo "==> failed test log: \$test_log"
				cat "\$test_log"
				upload "\$test_log" "fbctests-\$(basename "\$test_log")"
			done
			fail "log-tests reported failures in \$failed_log"
		fi
	done

	echo "==> fbctests passed"
}

run_exampleageddon() {
	jobs="\$(exampleageddon_jobs)"
	python=/usr/local/bin/python3.11

	[ -x "\$python" ] || fail "python311 is required for Exampleageddon"
	[ -f /work/freebasic-source/build_scripts/exampleageddon-freebasic.py ] ||
		fail "Exampleageddon runner was not staged"

	echo "==> running Exampleageddon with \${jobs} job(s)"
	rm -rf /work/exampleageddon
	run "\$python" /work/freebasic-source/build_scripts/exampleageddon-freebasic.py \
		--root /work/freebasic-source \
		--outdir /work/exampleageddon \
		--prefix /usr/local \
		--include-dir /work/freebasic-source/inc \
		--fbc /usr/local/bin/fbc \
		--jobs "\$jobs" \
		--compile-timeout "\$EXAMPLEAGEDDON_COMPILE_TIMEOUT" \
		--run-timeout "\$EXAMPLEAGEDDON_RUN_TIMEOUT" \
		--fail-on-self-contained

	[ -f /work/exampleageddon/report.md ] || fail "Exampleageddon report was not created"
	[ -f /work/exampleageddon/results.csv ] || fail "Exampleageddon results CSV was not created"
	grep -qx -- '- Self-contained problems: 0' /work/exampleageddon/report.md || {
		sed -n '1,80p' /work/exampleageddon/report.md
		fail "Exampleageddon reported self-contained example problems"
	}

	upload /work/exampleageddon/report.md exampleageddon-report.md
	upload /work/exampleageddon/results.csv exampleageddon-results.csv
	echo "==> Exampleageddon passed"
}

trap finish_with_xvfb EXIT

(
set -eu

hostname fbc-dragonfly
dhclient em0 || true

newfs -U /dev/ad1s0
mkdir -p /work
mount /dev/ad1s0 /work

mkdir -p /work/local /work/tmp /work/pkg-cache /work/freebasic-source /work/package
chmod 1777 /work/tmp
cp -a /usr/local/. /work/local/
mount_null /work/local /usr/local || mount_nullfs /work/local /usr/local

pkg update -f
pkg install -y \\
	bash \\
	curl \\
	gcc \\
	gmake \\
	libffi \\
	libX11 \\
	libXau \\
	libXcursor \\
	libXdmcp \\
	libXext \\
	libXi \\
	libXinerama \\
	libXpm \\
	libXrandr \\
	libXrender \\
	libXxf86vm \\
	libglvnd \\
	libxcb \\
	mesa-libs \\
	ncurses \\
	pkgconf \\
	python311 \\
	xorg-vfbserver \\
	xorgproto

fetch -o /tmp/freebasic-source.tar.gz "http://10.0.2.2:$HTTP_PORT/freebasic-source.tar.gz"
tar -xzf /tmp/freebasic-source.tar.gz -C /work/freebasic-source

pkg_url="http://10.0.2.2:$HTTP_PORT/upload/$DRAGONFLY_PACKAGE_BASENAME"
fetch -o "/work/package/$DRAGONFLY_PACKAGE_BASENAME" "\$pkg_url"

pkg delete -y freebasic >/dev/null 2>&1 || true
run pkg add -f "/work/package/$DRAGONFLY_PACKAGE_BASENAME"

command -v fbc
fbc -version

start_xvfb
prepare_audio

mkdir -p /work/smoke

cat > /work/smoke/console.bas <<'FBEOF'
print "Hello world"
FBEOF

cat > /work/smoke/gfx-truecolor.bas <<'FBEOF'
#include once "fbgfx.bi"

sub expect_rgb( byval x as integer, byval y as integer, byval expected as uinteger, byref label as string )
	dim as uinteger actual = cuint( point( x, y ) )

	if( actual <> expected ) then
		print "gfx truecolor mismatch: "; label; " actual=&h"; hex( actual, 8 ); " expected=&h"; hex( expected, 8 )
		end 1
	end if
end sub

dim as integer has_extra_page = 1

if( screenres( 64, 64, 32, 2 ) <> 0 ) then
	has_extra_page = 0
	if( screenres( 64, 64, 32 ) <> 0 ) then
		print "gfx truecolor skipped: screenres failed"
		end 77
	end if
end if

screenset 0, 0
line (0, 0)-(63, 63), rgb( 0, 0, 0 ), bf
line (8, 8)-(23, 23), rgb( 255, 0, 0 ), bf
line (24, 8)-(39, 23), rgb( 0, 255, 0 ), bf
line (40, 8)-(55, 23), rgb( 0, 0, 255 ), bf
expect_rgb 8, 8, rgb( 255, 0, 0 ), "red block"
expect_rgb 24, 8, rgb( 0, 255, 0 ), "green block"
expect_rgb 40, 8, rgb( 0, 0, 255 ), "blue block"

if( has_extra_page <> 0 ) then
	screenset 1, 1
	line (0, 0)-(63, 63), rgb( 255, 255, 255 ), bf
	line (8, 8)-(23, 23), rgb( 255, 0, 0 ), bf
	screenset 1, 1
	expect_rgb 8, 8, rgb( 255, 0, 0 ), "screenset red block"
end if

screen 0
print "gfx truecolor ok"
FBEOF

cat > /work/smoke/gfx-screen-modes.bas <<'FBEOF'
#include once "fbgfx.bi"

sub draw_mode( byval mode as integer )
	dim as integer max_color = 15

	if( mode = 1 ) then
		max_color = 3
	end if

	cls
	line (8, 8)-(32, 32), 1, bf
	line (40, 8)-(64, 32), iif( max_color >= 2, 2, 1 ), bf
	line (72, 8)-(96, 32), iif( max_color >= 4, 4, max_color ), bf
	pset (4, 4), max_color

	if( point( 4, 4 ) <> max_color ) then
		print "SCREEN "; mode; " point mismatch"
		end 1
	end if
end sub

sub test_mode( byval mode as integer )
	dim as integer stage = 0
	dim as integer unsupported_mode = 0

	if( mode = 0 ) then
		screen 0
		print "SCREEN 0 ok"
		exit sub
	end if

	on local error goto mode_error

	stage = 1
	screen mode
	if( unsupported_mode <> 0 ) then
		screen 0
		print "SCREEN "; mode; " unsupported, err="; err()
		exit sub
	end if

	stage = 2
	if( screenptr() = 0 ) then
		screen 0
		print "SCREEN "; mode; " unsupported"
		exit sub
	end if

	draw_mode mode
	screen 0
	print "SCREEN "; mode; " ok"
	exit sub

mode_error:
	if( stage = 1 ) then
		unsupported_mode = 1
		resume next
	end if

	print "SCREEN "; mode; " failed, err="; err()
	end 1
end sub

for mode as integer = 0 to 13
	test_mode mode
next
FBEOF

cat > /work/smoke/sfx.bas <<'FBEOF'
extern "C"
declare function fb_sfxDeviceCurrent() as long
declare function fb_sfxDeviceInfoName(byval id as long) as const zstring ptr
end extern

print "sfx-start"
dim as long sfx_device = fb_sfxDeviceCurrent()
dim as const zstring ptr sfx_driver = fb_sfxDeviceInfoName(sfx_device)
if sfx_driver <> 0 then
	print "sfx-driver="; *sfx_driver
	if instr(lcase(*sfx_driver), "null") > 0 then
		print "sfx-driver-null"
		end 1
	end if
else
	print "sfx-driver=<none>"
	end 1
end if

play "t120 o4 l8 c e g > c"
sleep 250
print "sfx-end"
FBEOF

echo "==> compiling console smoke"
run fbc /work/smoke/console.bas -x /work/smoke/console

echo "==> running console smoke"
console_output="\$(/work/smoke/console)"
echo "\$console_output"
[ "\$console_output" = "Hello world" ] || fail "unexpected console output"

echo "==> compiling crt/sys/socket.bi API smoke"
run fbc /work/freebasic-source/tests/crt/socket.bas -x /work/smoke/socket-bi

echo "==> running crt/sys/socket.bi API smoke"
run /work/smoke/socket-bi

echo "==> compiling curses.bi API smoke"
run fbc /work/freebasic-source/tests/crt/curses.bas -x /work/smoke/curses-bi

echo "==> running curses.bi API smoke"
run /work/smoke/curses-bi

echo "==> compiling threaded TCP runtime smoke"
run fbc -mt /work/freebasic-source/tests/file/tcp.bas -x /work/smoke/tcp

echo "==> running threaded TCP runtime smoke"
# The burst test performs one guest runtime call per byte.  DragonFly's generic
# KVM CPU is deliberately conservative, so keep the outer process guard beyond
# the test's documented two-minute internal deadlock bound.
run timeout 180 /work/smoke/tcp

echo "==> compiling gfxlib smoke"
run fbc /work/smoke/gfx-truecolor.bas -x /work/smoke/gfx-truecolor
run fbc -lang fblite -exx /work/smoke/gfx-screen-modes.bas -x /work/smoke/gfx-screen-modes

echo "==> running gfxlib smoke"
run_gfx_smoke /work/smoke/gfx-truecolor.out /work/smoke/gfx-truecolor.err /work/smoke/gfx-truecolor
run_gfx_smoke /work/smoke/gfx-screen-modes.out /work/smoke/gfx-screen-modes.err /work/smoke/gfx-screen-modes

echo "==> compiling sfxlib smoke"
run fbc /work/smoke/sfx.bas -x /work/smoke/sfx

echo "==> compiling sfxlib showcase"
[ -f /usr/local/share/freebasic/examples/sfxlib/showcase.bas ] ||
	fail "sfxlib showcase example is not installed"
rm -rf /work/smoke/sfxlib-showcase
mkdir -p /work/smoke/sfxlib-showcase
cp -R /usr/local/share/freebasic/examples/sfxlib/. /work/smoke/sfxlib-showcase/
(
	cd /work/smoke/sfxlib-showcase
	run fbc showcase.bas -x /work/smoke/sfx-showcase
)
[ -x /work/smoke/sfx-showcase ] || fail "sfxlib showcase binary was not created"

echo "==> running sfxlib real audio smoke"
SFXLIB_DRIVER="DRAGONFLY OSS" timeout 20 /work/smoke/sfx > /work/smoke/sfx.out 2> /work/smoke/sfx.err || {
	cat /work/smoke/sfx.out || true
	cat /work/smoke/sfx.err || true
	fail "sfx smoke failed"
}
cat /work/smoke/sfx.out || true
grep -qx 'sfx-start' /work/smoke/sfx.out || fail "sfx smoke did not start"
grep -qx 'sfx-end' /work/smoke/sfx.out || fail "sfx smoke did not finish"
grep -qi '^sfx-driver=dragonfly oss' /work/smoke/sfx.out || fail "sfx smoke did not use DragonFly OSS"
[ ! -s /work/smoke/sfx.err ] || {
	cat /work/smoke/sfx.err
	fail "sfx smoke wrote stderr"
}

run_fbctests
run_exampleageddon

echo "==> TEST PASSED"
) > "\$log" 2>&1
EOF
}

copy_package_to_server() {
	local package="$1"

	mkdir -p "$UPLOAD_DIR"
	cp -f "$package" "$UPLOAD_DIR/"
}

build_package() {
	local vm_dir="$RUN_DIR/build"
	local console_log="$LOG_DIR/freebasic-dragonfly-build-console.log"
	local guest_command
	local qemu_cmd

	prepare_vm_disks build
	write_guest_build_script

	guest_command="dhclient em0 || true; for n in 1 2 3 4 5 6 7 8 9 10; do ping -c 1 10.0.2.2 >/dev/null 2>&1 && break; sleep 2; done; fetch -o /tmp/dragonfly-build-run.sh http://10.0.2.2:$HTTP_PORT/dragonfly-build-run.sh && /bin/sh /tmp/dragonfly-build-run.sh; rc=\$?; echo exit_status=\${rc}=; halt -p"
	mapfile -t qemu_cmd < <(qemu_args "$vm_dir")

	msg "Building DragonFly package in QEMU"
	if ! run_qemu_guest_script \
		"$console_log" "$guest_command" "${qemu_cmd[@]}"; then
		collect_guest_logs
		return 1
	fi
	collect_guest_logs

	PACKAGE_FILE="$(find "$UPLOAD_DIR" -maxdepth 1 -type f -name 'freebasic-*.pkg' | sort | tail -n 1)"
	[ -n "$PACKAGE_FILE" ] && [ -f "$PACKAGE_FILE" ] || die "DragonFly package was not uploaded"
	cp -f "$PACKAGE_FILE" "$PACKAGE_DIR/"
	PACKAGE_FILE="$PACKAGE_DIR/$(basename "$PACKAGE_FILE")"
}

test_package() {
	local vm_dir="$RUN_DIR/test"
	local console_log="$LOG_DIR/freebasic-dragonfly-test-console.log"
	local audio_wav="$RUN_DIR/freebasic-dragonfly-audio.wav"
	local guest_command
	local qemu_cmd

	prepare_vm_disks test
	copy_package_to_server "$PACKAGE_FILE"

	DRAGONFLY_PACKAGE_BASENAME="$(basename "$PACKAGE_FILE")"
	write_guest_test_script

	guest_command="dhclient em0 || true; for n in 1 2 3 4 5 6 7 8 9 10; do ping -c 1 10.0.2.2 >/dev/null 2>&1 && break; sleep 2; done; fetch -o /tmp/dragonfly-test-run.sh http://10.0.2.2:$HTTP_PORT/dragonfly-test-run.sh && /bin/sh /tmp/dragonfly-test-run.sh; rc=\$?; echo exit_status=\${rc}=; halt -p"
	mapfile -t qemu_cmd < <(qemu_args "$vm_dir" "$audio_wav")

	msg "Running DragonFly package smoke tests and fbctests"
	if ! run_qemu_guest_script \
		"$console_log" "$guest_command" "${qemu_cmd[@]}"; then
		collect_guest_logs
		return 1
	fi
	collect_guest_logs
}

verify_audio_capture() {
	local wav="$1"
	local log="$2"

	msg "Verifying DragonFly QEMU audio capture"
	python3 - "$wav" "$log" <<'PY'
import math
import os
import struct
import sys
import wave

wav_path = sys.argv[1]
log_path = sys.argv[2]

if not os.path.exists(wav_path):
    raise SystemExit("audio capture was not created")

with wave.open(wav_path, "rb") as wav:
    data = wav.readframes(wav.getnframes())
    rate = wav.getframerate()
    channels = wav.getnchannels()
    width = wav.getsampwidth()
    count = wav.getnframes()

if not data or count <= 0:
    raise SystemExit("audio capture contains no samples")
if width != 2:
    raise SystemExit(f"unexpected sample width: {width}")

duration = count / float(rate)
sample_count = len(data) // 2
samples = struct.unpack("<%dh" % sample_count, data)
peak = max(abs(sample) for sample in samples)
rms = math.sqrt(sum(sample * sample for sample in samples) / float(sample_count))
active = 0

for i in range(0, sample_count, channels):
    frame = samples[i:i + channels]
    if max(abs(sample) for sample in frame) > 256:
        active += 1

if duration < 0.2:
    raise SystemExit("audio capture is too short")
if peak < 512:
    raise SystemExit("audio capture peak is too low")
if rms < 64:
    raise SystemExit("audio capture RMS is too low")
if active < rate // 20:
    raise SystemExit("audio capture has too few active samples")

with open(log_path, "w", encoding="utf-8") as f:
    f.write(f"path={wav_path}\n")
    f.write(f"rate={rate}\n")
    f.write(f"channels={channels}\n")
    f.write(f"sample_width={width}\n")
    f.write(f"frames={count}\n")
    f.write(f"samples={sample_count}\n")
    f.write(f"duration={duration:.3f}\n")
    f.write(f"peak={peak}\n")
    f.write(f"rms={rms:.2f}\n")
    f.write(f"active_samples={active}\n")
    f.write("result=PASS\n")
PY
}

archive_results() {
	mkdir -p "$ARCHIVE_DIR"

	cp -f "$PACKAGE_FILE" "$ARCHIVE_DIR/"
	cp -f "$LOG_DIR/freebasic-dragonfly-build-console.log" "$ARCHIVE_DIR/" 2>/dev/null || true
	cp -f "$LOG_DIR/freebasic-dragonfly-test-console.log" "$ARCHIVE_DIR/" 2>/dev/null || true
	cp -f "$UPLOAD_DIR/freebasic-dragonfly-build.log" "$ARCHIVE_DIR/" 2>/dev/null || true
	cp -f "$UPLOAD_DIR/freebasic-dragonfly-test.log" "$ARCHIVE_DIR/" 2>/dev/null || true
	cp -f "$UPLOAD_DIR/exampleageddon-report.md" "$ARCHIVE_DIR/" 2>/dev/null || true
	cp -f "$UPLOAD_DIR/exampleageddon-results.csv" "$ARCHIVE_DIR/" 2>/dev/null || true
	cp -f "$LOG_DIR/freebasic-dragonfly-audio.log" "$ARCHIVE_DIR/" 2>/dev/null || true
	cp -f "$RUN_DIR/freebasic-dragonfly-audio.wav" "$ARCHIVE_DIR/" 2>/dev/null || true

	(
		cd "$ARCHIVE_DIR"
		sha256sum freebasic-*.pkg > SHA256SUMS
	)
}

cleanup() {
	stop_upload_server
}

trap cleanup EXIT

main() {
	check_host_tools
	configure_qemu_acceleration
	resolve_package_file

	if [ -z "$HTTP_PORT" ]; then
		HTTP_PORT="$(find_free_port 19180)"
	fi

	msg "DragonFly VM HTTP port: $HTTP_PORT"

	download_image
	extract_image

	rm -rf "$RUN_DIR"
	mkdir -p "$RUN_DIR" "$SERVE_DIR" "$UPLOAD_DIR" "$LOG_DIR" "$PACKAGE_DIR"

	make_source_archive

	if [ "$TEST_ONLY" -eq 1 ]; then
		copy_package_to_server "$PACKAGE_FILE"
	else
		start_upload_server
		build_package
		stop_upload_server
	fi

	start_upload_server
	test_package
	stop_upload_server

	verify_audio_capture "$RUN_DIR/freebasic-dragonfly-audio.wav" "$LOG_DIR/freebasic-dragonfly-audio.log"
	archive_results

	if [ "$KEEP_VM" -eq 0 ]; then
		rm -rf "$RUN_DIR/build" "$RUN_DIR/test"
	fi

	msg "DragonFly package build, fbctests, and Exampleageddon completed"
	echo "Package: $ARCHIVE_DIR/$(basename "$PACKAGE_FILE")"
	echo "Archive: $ARCHIVE_DIR"
	if [ -f "$ARCHIVE_DIR/freebasic-dragonfly-build.log" ]; then
		echo "Build log: $ARCHIVE_DIR/freebasic-dragonfly-build.log"
	fi
	echo "Test log:  $ARCHIVE_DIR/freebasic-dragonfly-test.log"
	echo "Audio log: $ARCHIVE_DIR/freebasic-dragonfly-audio.log"
}

main "$@"

##############################################################################
# end of dragonfly-vm-build-freebasic.sh
##############################################################################
