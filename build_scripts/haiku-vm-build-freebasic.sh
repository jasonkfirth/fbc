#!/usr/bin/env bash

##############################################################################
# FreeBASIC Haiku VM package builder
##############################################################################
#
# Purpose:
#
#   Build and test the Haiku x86_64 or x86 (i386) FreeBASIC package from a
#   Debian/Ubuntu Linux host.
#
# Responsibilities:
#
#   * download or reuse an official Haiku x86_gcc2h or x86_64 anyboot image
#   * patch the live image so it starts sshd on first boot
#   * build FreeBASIC inside a Haiku QEMU VM using haiku-build-freebasic.sh
#   * install the resulting .hpkg in a separate clean Haiku VM
#   * run console, gfxlib, sfxlib, fbctests, and exampleageddon checks
#
# This script intentionally does NOT contain:
#
#   * cross-compilation into Haiku packages
#   * GUI automation of the Haiku installer
#
##############################################################################

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORKROOT="$ROOT/out/haiku-vm"
CACHE_DIR="$WORKROOT/cache"
RUN_DIR="$WORKROOT/run"
PACKAGE_DIR="$WORKROOT/packages"
LOG_DIR="$WORKROOT/logs"
ARCH="x86_64"
IMAGE_ARCH="x86_64"
TARGET_ARCH=""
ARCHIVE_DIR=""
IMAGE_URL=""
IMAGE_FILE=""
ISO_FILE=""
PACKAGE_FILE=""
PYTHON_HPKG=""
TEST_ONLY=0
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"
CPUS="$JOBS"
CPUS_EXPLICIT=0
MEMORY="4096"
WORK_DISK_SIZE="24G"
KEEP_VMS=0
SSH_PORT=""
HTTP_PORT=""
VNC_DISPLAY=""
FBCTESTS_JOBS="$JOBS"
FBCTESTS_UNIT_ARGS=""
QEMU_ACCEL="${QEMU_ACCEL:-kvm}"
QEMU_CPU="${QEMU_CPU:-host}"
EXAMPLEAGEDDON_JOBS="$JOBS"
EXAMPLEAGEDDON_COMPILE_TIMEOUT="120"
EXAMPLEAGEDDON_RUN_TIMEOUT="10"
PYTHON_INSTALL_TIMEOUT="1800"
BOOT_SCRIPT_BYTES=884

NIGHTLY_INDEX_URL="https://download.haiku-os.org/nightly-images/x86_64/"

msg() { printf '\n==> %s\n' "$*"; }
warn() { printf '\nWARNING: %s\n' "$*" >&2; }
die() { printf '\nERROR: %s\n' "$*" >&2; exit 1; }

usage() {
	cat <<EOF
Usage: ./build_scripts/haiku-vm-build-freebasic.sh [options]

Options:
  --image-url URL          Haiku x86_64 or x86_gcc2h anyboot .zip or .iso URL.
  --image FILE             Existing Haiku x86_64 or x86_gcc2h anyboot .zip or .iso.
  --package FILE           Existing .hpkg to test.
  --arch ARCH              Haiku architecture: x86_64, x86, or i386.
  --test-only              Test --package without rebuilding FreeBASIC.
  --workroot DIR           Work directory. Default: out/haiku-vm
  --archive-dir DIR        Final archive directory. Default: out/haiku/$ARCH
  --jobs N                 Build jobs inside Haiku. Default: host CPU count
  --cpus N                 QEMU CPU count. Default: --jobs value
  --memory MB              QEMU memory in MB. Default: 4096
  --work-disk-size SIZE    Per-VM BFS work disk size. Default: 24G
  --ssh-port N             Host SSH forward port. Default: auto
  --http-port N            Host bootstrap HTTP port. Default: auto
  --vnc-display N          VNC display number. Default: auto
  --fbctests-jobs N        fbctests make jobs. Default: --jobs value
  --fbctests-unit-args S   Extra UNITTEST_RUN_ARGS for fbctests.
  --exampleageddon-jobs N  exampleageddon jobs. Default: --jobs value
  --exampleageddon-compile-timeout N
                           Per-example compile timeout. Default: 120
  --exampleageddon-run-timeout N
                           Per-example run timeout. Default: 10
  --python-hpkg FILE       Optional local Haiku python3*.hpkg to stage for
                           exampleageddon.
  --python-install-timeout N
                           Python pkgman install timeout. Default: 1800
  --keep-vms               Do not delete VM run directories on success.
  -h, --help               Show this help.

The script builds x86_64 and x86 (i386) Haiku VMs. A fresh Haiku VM is used for package testing.
EOF
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--image-url) IMAGE_URL="$2"; shift 2 ;;
		--image) IMAGE_FILE="$2"; shift 2 ;;
		--package) PACKAGE_FILE="$2"; shift 2 ;;
		--python-hpkg) PYTHON_HPKG="$2"; shift 2 ;;
		--test-only) TEST_ONLY=1; shift ;;
		--workroot)
			WORKROOT="$2"
			CACHE_DIR="$WORKROOT/cache"
			RUN_DIR="$WORKROOT/run"
			PACKAGE_DIR="$WORKROOT/packages"
			LOG_DIR="$WORKROOT/logs"
			shift 2
			;;
		--archive-dir) ARCHIVE_DIR="$2"; shift 2 ;;
		--arch) TARGET_ARCH="$2"; shift 2 ;;
		--jobs)
			JOBS="$2"
			[ "$CPUS_EXPLICIT" -eq 1 ] || CPUS="$2"
			FBCTESTS_JOBS="$2"
			EXAMPLEAGEDDON_JOBS="$2"
			shift 2
			;;
		--cpus) CPUS="$2"; CPUS_EXPLICIT=1; shift 2 ;;
		--memory) MEMORY="$2"; shift 2 ;;
		--work-disk-size) WORK_DISK_SIZE="$2"; shift 2 ;;
		--ssh-port) SSH_PORT="$2"; shift 2 ;;
		--http-port) HTTP_PORT="$2"; shift 2 ;;
		--vnc-display) VNC_DISPLAY="$2"; shift 2 ;;
		--fbctests-jobs) FBCTESTS_JOBS="$2"; shift 2 ;;
		--fbctests-unit-args) FBCTESTS_UNIT_ARGS="$2"; shift 2 ;;
		--exampleageddon-jobs) EXAMPLEAGEDDON_JOBS="$2"; shift 2 ;;
		--exampleageddon-compile-timeout) EXAMPLEAGEDDON_COMPILE_TIMEOUT="$2"; shift 2 ;;
		--exampleageddon-run-timeout) EXAMPLEAGEDDON_RUN_TIMEOUT="$2"; shift 2 ;;
		--python-install-timeout) PYTHON_INSTALL_TIMEOUT="$2"; shift 2 ;;
		--keep-vms) KEEP_VMS=1; shift ;;
		-h|--help) usage; exit 0 ;;
		*) die "unknown option: $1" ;;
	esac
done

QEMU_BIN=""
case "$ARCH" in
	x86_64)
		IMAGE_ARCH="x86_64"
		QEMU_BIN="qemu-system-x86_64"
		;;
	x86)
		IMAGE_ARCH="x86_gcc2h"
		QEMU_BIN="qemu-system-i386"
		;;
esac

if [ -n "$TARGET_ARCH" ]; then
	case "$TARGET_ARCH" in
		x86_64|amd64)
			ARCH="x86_64"
			IMAGE_ARCH="x86_64"
			QEMU_BIN="qemu-system-x86_64"
			;;
		x86|i386|i486|i586|i686|x86_gcc2|x86_gcc2h)
			ARCH="x86"
			IMAGE_ARCH="x86_gcc2h"
			QEMU_BIN="qemu-system-i386"
			;;
		*)
			die "unsupported Haiku architecture: $TARGET_ARCH"
			;;
	esac
fi

case "$ARCH" in
	x86_64|x86) ;;
	*) die "only x86_64 and x86 (i386) are supported by this script" ;;
esac

NIGHTLY_INDEX_URL="https://download.haiku-os.org/nightly-images/$IMAGE_ARCH/"

if [ -z "$ARCHIVE_DIR" ]; then
    ARCHIVE_DIR="$ROOT/out/haiku/$ARCH"
fi

case "$JOBS" in ''|*[!0-9]*|0) die "--jobs must be a positive integer" ;; esac
case "$CPUS" in ''|*[!0-9]*|0) die "--cpus must be a positive integer" ;; esac
case "$FBCTESTS_JOBS" in ''|*[!0-9]*|0) die "--fbctests-jobs must be a positive integer" ;; esac
case "$EXAMPLEAGEDDON_JOBS" in ''|*[!0-9]*|0) die "--exampleageddon-jobs must be a positive integer" ;; esac
case "$EXAMPLEAGEDDON_COMPILE_TIMEOUT" in ''|*[!0-9]*|0) die "--exampleageddon-compile-timeout must be a positive integer" ;; esac
case "$EXAMPLEAGEDDON_RUN_TIMEOUT" in ''|*[!0-9]*|0) die "--exampleageddon-run-timeout must be a positive integer" ;; esac
case "$PYTHON_INSTALL_TIMEOUT" in ''|*[!0-9]*|0) die "--python-install-timeout must be a positive integer" ;; esac

if [ "$TEST_ONLY" -eq 1 ] && [ -z "$PACKAGE_FILE" ]; then
	die "--test-only requires --package"
fi

if [ -n "$PYTHON_HPKG" ]; then
	[ -f "$PYTHON_HPKG" ] || die "Python package not found: $PYTHON_HPKG"
	PYTHON_HPKG="$(cd "$(dirname "$PYTHON_HPKG")" && pwd)/$(basename "$PYTHON_HPKG")"
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

find_free_vnc_display() {
	python3 - <<'PY'
import socket

for display in range(11, 99):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:
            s.bind(("127.0.0.1", 5900 + display))
        except OSError:
            continue
        print(display)
        raise SystemExit(0)

raise SystemExit("no free VNC display found")
PY
}

check_host_tools() {
	require_tool bash
	require_tool curl
	require_tool python3
	require_tool "$QEMU_BIN"
	require_tool ssh
	require_tool scp
	require_tool tar
	require_tool sha256sum

	if ! command -v 7z >/dev/null 2>&1 && ! command -v unzip >/dev/null 2>&1; then
		die "required tool not found: 7z or unzip"
	fi
}

configure_qemu_acceleration() {
	# Current x86_gcc2h anyboot images reset at the kernel handoff with multiple
	# virtual CPUs under both KVM and TCG.  An untouched copy of the same image
	# boots under KVM with one conservative 32-bit CPU, so use that proven legacy
	# profile unless the caller chose a different accelerator explicitly.
	if [ "$ARCH" = "x86" ] && [ "$QEMU_ACCEL" = "kvm" ]; then
		QEMU_CPU="qemu32"
		CPUS=1
	elif [ ! -r /dev/kvm ] || [ ! -w /dev/kvm ]; then
		QEMU_ACCEL="tcg"
		QEMU_CPU="max"
	fi

	msg "QEMU acceleration: $QEMU_ACCEL (CPU: $QEMU_CPU)"
}

latest_haiku_url() {
	curl -fsSL "$NIGHTLY_INDEX_URL" |
		sed -n "s/.*href=\"\([^\"]*${IMAGE_ARCH}-anyboot\.zip\)\".*/\1/p" |
		awk 'NF && !found { print; found = 1 }'
}

download_image() {
	mkdir -p "$CACHE_DIR" "$PACKAGE_DIR" "$LOG_DIR"

	if [ -n "$IMAGE_FILE" ]; then
		[ -f "$IMAGE_FILE" ] || die "image not found: $IMAGE_FILE"
		IMAGE_FILE="$(cd "$(dirname "$IMAGE_FILE")" && pwd)/$(basename "$IMAGE_FILE")"
		msg "Using Haiku image $IMAGE_FILE"
		return 0
	fi

	if [ -z "$IMAGE_URL" ]; then
		msg "Resolving latest Haiku $ARCH nightly"
		IMAGE_URL="$(latest_haiku_url)"
		[ -n "$IMAGE_URL" ] || die "could not locate latest Haiku nightly image"
	fi

	local name partial_file
	name="$(basename "$IMAGE_URL")"
	IMAGE_FILE="$CACHE_DIR/$name"
	partial_file="$IMAGE_FILE.part"

	if [ ! -f "$IMAGE_FILE" ]; then
		msg "Downloading $IMAGE_URL"
		if [ -f "$partial_file" ]; then
			curl -fL --retry 3 --retry-delay 5 -C - -o "$partial_file" "$IMAGE_URL"
		else
			curl -fL --retry 3 --retry-delay 5 -o "$partial_file" "$IMAGE_URL"
		fi
		mv -f "$partial_file" "$IMAGE_FILE"
	else
		msg "Using cached image $IMAGE_FILE"
	fi

	if curl -fsSL "$IMAGE_URL.sha256" -o "$IMAGE_FILE.sha256" 2>/dev/null; then
		local expected actual extract_base
		expected="$(sed -n 's/^.*= //p' "$IMAGE_FILE.sha256" | head -n 1)"
		actual="$(sha256sum "$IMAGE_FILE" | awk '{print $1}')"
		if [ "$expected" != "$actual" ]; then
			extract_base="${name%.*}"
			rm -f "$IMAGE_FILE"
			rm -rf "$CACHE_DIR/extracted/$extract_base"
			die "checksum mismatch for $IMAGE_FILE"
		fi
	fi
}

extract_iso() {
	local ext
	ext="${IMAGE_FILE##*.}"

	if [ "$ext" = "iso" ]; then
		ISO_FILE="$IMAGE_FILE"
		return 0
	fi

	local image_base extract_dir
	image_base="$(basename "$IMAGE_FILE")"
	image_base="${image_base%.*}"
	extract_dir="$CACHE_DIR/extracted/$image_base"
	mkdir -p "$extract_dir"

	ISO_FILE="$(find "$extract_dir" -maxdepth 1 -type f -name "*${IMAGE_ARCH}*anyboot*.iso" | head -n 1)"
	if [ -n "$ISO_FILE" ] && [ -f "$ISO_FILE" ]; then
		msg "Using extracted ISO $ISO_FILE"
		return 0
	fi

	msg "Extracting Haiku anyboot ISO"
	rm -rf "$extract_dir"
	mkdir -p "$extract_dir"

	if command -v 7z >/dev/null 2>&1; then
		7z x -y -o"$extract_dir" "$IMAGE_FILE" >/dev/null
	else
		unzip -q "$IMAGE_FILE" -d "$extract_dir"
	fi

	ISO_FILE="$(find "$extract_dir" -maxdepth 1 -type f -name "*${IMAGE_ARCH}*anyboot*.iso" | head -n 1)"
	[ -n "$ISO_FILE" ] && [ -f "$ISO_FILE" ] ||
		die "could not find $IMAGE_ARCH anyboot ISO in $IMAGE_FILE"
}

resolve_package_file() {
	[ -n "$PACKAGE_FILE" ] || return 0
	[ -f "$PACKAGE_FILE" ] || die "package not found: $PACKAGE_FILE"

	PACKAGE_FILE="$(cd "$(dirname "$PACKAGE_FILE")" && pwd)/$(basename "$PACKAGE_FILE")"
}

archive_results() {
	local hpkg="$1"
	local base

	mkdir -p "$ARCHIVE_DIR"

	# The archive directory is reused across reruns. Clear generated files first
	# so optional logs from an earlier run cannot describe the current package.
	rm -f "$ARCHIVE_DIR"/freebasic*.hpkg
	rm -f "$ARCHIVE_DIR"/freebasic-haiku-*.log
	rm -f "$ARCHIVE_DIR"/exampleageddon-report.md
	rm -f "$ARCHIVE_DIR"/exampleageddon-results.csv
	rm -f "$ARCHIVE_DIR"/freebasic-haiku-audio.wav
	rm -f "$ARCHIVE_DIR"/SHA256SUMS

	cp -f "$hpkg" "$ARCHIVE_DIR/"

	for log in "$LOG_DIR"/freebasic-haiku-*.log; do
		[ -f "$log" ] || continue
		cp -f "$log" "$ARCHIVE_DIR/"
	done

	if [ -f "$RUN_DIR/test/exampleageddon-report.md" ]; then
		cp -f "$RUN_DIR/test/exampleageddon-report.md" "$ARCHIVE_DIR/"
	fi

	if [ -f "$RUN_DIR/test/exampleageddon-results.csv" ]; then
		cp -f "$RUN_DIR/test/exampleageddon-results.csv" "$ARCHIVE_DIR/"
	fi

	if [ -f "$RUN_DIR/test/freebasic-haiku-audio.wav" ]; then
		cp -f "$RUN_DIR/test/freebasic-haiku-audio.wav" "$ARCHIVE_DIR/"
	fi

	base="$(basename "$hpkg")"
	(
		cd "$ARCHIVE_DIR"
		sha256sum "$base" > SHA256SUMS
	)
}

make_ssh_key() {
	local key="$1"

	if [ ! -f "$key" ]; then
		ssh-keygen -q -t ed25519 -N '' -f "$key"
	fi
}

write_bootstrap_files() {
	local vm_dir="$1"
	local http_port="$2"
	local key_pub="$3"
	local serve_dir="$vm_dir/serve"
	local boot_script="$vm_dir/UserBootscript"

	mkdir -p "$serve_dir"

	# Keep Haiku's stock boot-folder launcher.  The anyboot image enters the
	# interactive first_boot target before this desktop UserBootscript runs.  The
	# host selects FirstBootPrompt's default "Try Haiku" action through QEMU, then
	# this script lets the stock desktop launch entries finish.  Package
	# transactions can deadlock if they begin while FirstBootPrompt is still
	# completing the user session, so SSH is advertised only after that process
	# exits and services settle.
	cat > "$boot_script" <<EOF
#!/bin/sh
LOG="\$HOME/config/settings/boot/freebasic-bootstrap.log"
for file in "\$HOME"/config/settings/boot/launch/*
do
	[ -e "\$file" ] && /bin/open "\$file" &
done

(
export PATH=/boot/system/bin:/bin:\$PATH
sleep 5
while ps | grep -q '[F]irstBootPrompt'; do sleep 1; done
sleep 15
mkdir -p "\$HOME/config/settings/ssh"
cat > "\$HOME/config/settings/ssh/authorized_keys" <<'KEYEOF'
$(cat "$key_pub")
KEYEOF
chmod 700 "\$HOME/config/settings/ssh"
chmod 600 "\$HOME/config/settings/ssh/authorized_keys"
useradd sshd >/dev/null 2>&1 || true
ssh-keygen -A >/dev/null 2>&1 || true
sshd >/dev/null 2>&1 || /boot/system/bin/sshd >/dev/null 2>&1 || true
touch "\$HOME/config/settings/boot/freebasic-ssh-ready"
) >> "\$LOG" 2>&1 &
#
EOF

	python3 - "$boot_script" "$BOOT_SCRIPT_BYTES" <<'PY'
import sys

path = sys.argv[1]
size = int(sys.argv[2])
data = open(path, "rb").read()

if len(data) > size:
    raise SystemExit("bootstrap UserBootscript is too large")

open(path, "wb").write(data + b"\n" + b"#" * (size - len(data) - 1))
PY
}

patch_boot_image() {
	local vm_dir="$1"
	local boot_script="$vm_dir/UserBootscript"
	local boot_image="$vm_dir/haiku-boot.raw"

	cp --sparse=always "$ISO_FILE" "$boot_image"

	python3 - "$boot_image" "$boot_script" <<'PY'
import sys

image = sys.argv[1]
script = sys.argv[2]
needle = b"#!/bin/sh\n\n# DO NOT EDIT!\n#====================================================================="
replacement = open(script, "rb").read()

with open(image, "r+b") as f:
    data = f.read()
    offset = data.find(needle)
    if offset < 0:
        raise SystemExit("could not find Haiku UserBootscript marker")
    f.seek(offset)
    f.write(replacement)
PY
}

create_blank_work_disk() {
	local vm_dir="$1"
	local work_disk="$vm_dir/work.raw"

	rm -f "$work_disk"
	truncate -s "$WORK_DISK_SIZE" "$work_disk"
}

start_http_server() {
	local vm_dir="$1"
	local port="$2"

	python3 -m http.server "$port" --bind 127.0.0.1 \
		--directory "$vm_dir/serve" > "$vm_dir/http.log" 2>&1 &
	echo $! > "$vm_dir/http.pid"
}

start_vm() {
	local vm_dir="$1"
	local ssh_port="$2"
	local vnc_display="$3"
	local audio_wav="${4:-}"
	local boot_image="$vm_dir/haiku-boot.raw"
	local work_disk="$vm_dir/work.raw"
	local monitor_socket="$vm_dir/monitor.sock"
	local qemu_args

	rm -f "$monitor_socket"

	qemu_args=(
		"$QEMU_BIN"
		-accel "$QEMU_ACCEL"
		-cpu "$QEMU_CPU"
		-m "$MEMORY"
		-smp "$CPUS"
		-drive "file=$boot_image,format=raw,if=ide,index=0"
		-drive "file=$work_disk,format=raw,if=ide,index=1"
		-netdev "user,id=net0,hostfwd=tcp:127.0.0.1:$ssh_port-:22"
		-device "e1000,netdev=net0"
	)

	if [ -n "$audio_wav" ]; then
		rm -f "$audio_wav"
		qemu_args+=(
			-audiodev "wav,id=audio0,path=$audio_wav"
			-device intel-hda
			-device "hda-duplex,audiodev=audio0"
		)
	fi

	qemu_args+=(
		-vga std
		-display "vnc=127.0.0.1:$vnc_display"
		-monitor "unix:$monitor_socket,server=on,wait=off"
		-serial "file:$vm_dir/serial.log"
		-pidfile "$vm_dir/qemu.pid"
		-daemonize
	)

	"${qemu_args[@]}"
}

select_haiku_live_desktop() {
	local vm_dir="$1"
	local monitor_socket="$vm_dir/monitor.sock"

	[ -S "$monitor_socket" ] || return 1

	# FirstBootPrompt makes "Try Haiku" the window's default button.  Reading the
	# monitor response ensures QEMU processed Enter before this short-lived
	# control connection closes.
	python3 - \
		"$monitor_socket" \
		"$vm_dir/first-boot-before.ppm" \
		"$vm_dir/first-boot-after.ppm" <<'PY'
import socket
import sys
import time


def run_command(monitor, command):
    monitor.sendall(command.encode("utf-8") + b"\n")
    response = bytearray()

    while b"(qemu)" not in response:
        chunk = monitor.recv(4096)
        if not chunk:
            raise RuntimeError("QEMU monitor closed before acknowledging command")
        response.extend(chunk)

    if b"unknown command" in response or b"Error" in response:
        raise RuntimeError(response.decode("utf-8", "replace"))

with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as monitor:
    monitor.settimeout(5)
    monitor.connect(sys.argv[1])

    banner = bytearray()
    while b"(qemu)" not in banner:
        chunk = monitor.recv(4096)
        if not chunk:
            raise RuntimeError("QEMU monitor closed during greeting")
        banner.extend(chunk)

    try:
        run_command(monitor, "screendump " + sys.argv[2])
    except RuntimeError:
        pass
    run_command(monitor, "sendkey ret")
    time.sleep(0.25)
    try:
        run_command(monitor, "screendump " + sys.argv[3])
    except RuntimeError:
        pass
PY
}

stop_vm() {
	local vm_dir="$1"
	local pid

	if [ -f "$vm_dir/qemu.pid" ]; then
		pid="$(cat "$vm_dir/qemu.pid")"
		kill "$pid" 2>/dev/null || true

		for _ in $(seq 1 30); do
			if ! kill -0 "$pid" 2>/dev/null; then
				break
			fi
			sleep 1
		done

		if kill -0 "$pid" 2>/dev/null; then
			kill -KILL "$pid" 2>/dev/null || true
		fi

		rm -f "$vm_dir/qemu.pid"
	fi

	if [ -f "$vm_dir/http.pid" ]; then
		kill "$(cat "$vm_dir/http.pid")" 2>/dev/null || true
		rm -f "$vm_dir/http.pid"
	fi
}

wait_for_ssh() {
	local key="$1"
	local port="$2"
	local vm_dir="$3"
	local attempts="${4:-120}"
	local attempt

	msg "Waiting for Haiku SSH on localhost:$port"
	for attempt in $(seq 1 "$attempts"); do
		if timeout 4 ssh -i "$key" \
				-o BatchMode=yes \
				-o StrictHostKeyChecking=no \
				-o UserKnownHostsFile=/dev/null \
				-o ConnectTimeout=5 \
				-p "$port" \
				user@127.0.0.1 \
				'test -f ~/config/settings/boot/freebasic-ssh-ready' \
				> "$vm_dir/ssh-ready.out" 2> "$vm_dir/ssh-ready.err"; then
			return 0
		fi

		# The live image cannot reach the desktop UserBootscript until its
		# interactive first-boot prompt is accepted.  Repeat conservatively
		# because package activation may delay the prompt by several minutes.
		if [ $((attempt % 10)) -eq 0 ]; then
			select_haiku_live_desktop "$vm_dir" >/dev/null 2>&1 || true
		fi
		sleep 2
	done

	return 1
}

print_vm_failure_logs() {
	local vm_dir="$1"
	local vm_name
	local source

	vm_name="$(basename "$vm_dir")"
	mkdir -p "$LOG_DIR"

	for source in serial-first-boot.log serial.log http.log ssh-ready.err \
		first-boot-before.ppm first-boot-after.ppm; do
		if [ -f "$vm_dir/$source" ]; then
			cp -f "$vm_dir/$source" \
				"$LOG_DIR/freebasic-haiku-$vm_name-$source"
		fi
	done

	tail -n 80 "$vm_dir/serial.log" >&2 || true
	cat "$vm_dir/http.log" >&2 || true
	cat "$vm_dir/ssh-ready.err" >&2 || true
}

ssh_guest() {
	local key="$1"
	local port="$2"
	shift 2

	# The remaining words are the caller's intended guest command.
	# shellcheck disable=SC2029
	ssh -i "$key" \
		-o BatchMode=yes \
		-o StrictHostKeyChecking=no \
		-o UserKnownHostsFile=/dev/null \
		-o ConnectTimeout=5 \
		-p "$port" \
		user@127.0.0.1 "$@"
}

scp_to_guest() {
	local key="$1"
	local port="$2"
	local source="$3"
	local target="$4"

	scp -i "$key" \
		-o BatchMode=yes \
		-o StrictHostKeyChecking=no \
		-o UserKnownHostsFile=/dev/null \
		-o ConnectTimeout=5 \
		-P "$port" \
		"$source" "user@127.0.0.1:$target"
}

scp_from_guest() {
	local key="$1"
	local port="$2"
	local source="$3"
	local target="$4"

	scp -i "$key" \
		-o BatchMode=yes \
		-o StrictHostKeyChecking=no \
		-o UserKnownHostsFile=/dev/null \
		-o ConnectTimeout=5 \
		-P "$port" \
		"user@127.0.0.1:$source" "$target"
}

tar_directory_to_guest() {
	local key="$1"
	local port="$2"
	local source="$3"
	local target="$4"
	shift 4

	ssh_guest "$key" "$port" "rm -rf '$target' && mkdir -p '$target'"

	(
		cd "$source"
		tar "$@" -cf - .
	) | ssh_guest "$key" "$port" "cd '$target' && tar -xf -"
}

prepare_vm() {
	local name="$1"
	local vm_dir="$RUN_DIR/$name"
	local key="$vm_dir/id_ed25519"
	local ssh_port="$2"
	local http_port="$3"
	local vnc_display="$4"
	local audio_wav="${5:-}"

	rm -rf "$vm_dir"
	mkdir -p "$vm_dir"

	make_ssh_key "$key" || die "could not create the Haiku VM SSH key"
	write_bootstrap_files "$vm_dir" "$http_port" "$key.pub" ||
		die "could not create the Haiku VM bootstrap"
	patch_boot_image "$vm_dir" || die "could not patch the Haiku boot image"
	create_blank_work_disk "$vm_dir" || die "could not create the Haiku work disk"
	start_http_server "$vm_dir" "$http_port" || die "could not start the Haiku file server"
	start_vm "$vm_dir" "$ssh_port" "$vnc_display" "$audio_wav" ||
		die "could not start the Haiku VM"
	# A current nightly can spend more than five minutes completing its initial
	# package activation and user-session setup.  Do not interrupt that work
	# with an early warm restart.
	if ! wait_for_ssh "$key" "$ssh_port" "$vm_dir" 120 >&2; then
		warn "first Haiku boot did not reach SSH; restarting the warmed image" >&2
		if [ -f "$vm_dir/qemu.pid" ]; then
			kill "$(cat "$vm_dir/qemu.pid")" 2>/dev/null || true
			rm -f "$vm_dir/qemu.pid"
		fi
		sleep 2
		mv "$vm_dir/serial.log" "$vm_dir/serial-first-boot.log" 2>/dev/null || true
		start_vm "$vm_dir" "$ssh_port" "$vnc_display" "$audio_wav"
		if ! wait_for_ssh "$key" "$ssh_port" "$vm_dir" 120 >&2; then
			print_vm_failure_logs "$vm_dir"
			die "timed out waiting for Haiku SSH"
		fi
	fi

	printf '%s\n' "$vm_dir"
}

format_and_mount_work() {
	local key="$1"
	local port="$2"

ssh_guest "$key" "$port" 'bash -s' <<'EOF'
set -e
export PATH=/Work/tools:/boot/system/bin:/bin:$PATH

mkfs -t bfs -q /dev/disk/ata/0/slave/raw Work
rmdir /Work >/dev/null 2>&1 || true
mountvolume Work

for n in 1 2 3 4 5 6 7 8 9 10; do
	[ -d /Work/trash ] && break
	sleep 1
done

[ -d /Work/trash ] || {
	echo "Work volume was not mounted at /Work" >&2
	exit 1
}

df -h
EOF
}

install_build_dependencies() {
	local key="$1"
	local port="$2"

ssh_guest "$key" "$port" 'bash -s' <<'EOF'
set -e
export PATH=/Work/tools:/boot/system/bin:/bin:$PATH

run_limited() {
	limit="$1"
	shift

	# Haiku can retain a terminated team in ps until its parent reaps it.  The
	# hand-written kill -0 loop therefore never completed after a pkgman timeout.
	timeout -k 5 "$limit" "$@"
}

install_image_package() {
	name="$1"
	package_file="$(find /boot/_packages_ -maxdepth 1 -type f -name "$name-*.hpkg" 2>/dev/null | sort | tail -n 1)"

	if [ -z "$package_file" ]; then
		return 1
	fi

	if [ -e "/boot/system/packages/$(basename "$package_file")" ]; then
		return 0
	fi

	echo "==> installing image package $package_file"
	run_limited 300 pkgman install -y "$package_file"
}

install_image_packages() {
	for package_name in "$@"; do
		install_image_package "$package_name" || true
	done
}

install_optional_network_packages() {
	run_limited 300 pkgman install -y "$@" || {
		echo "WARNING: optional pkgman install failed or timed out: $*" >&2
		return 0
	}
}

cleanup_package_states() {
	find /boot/system/packages/administrative -maxdepth 1 -type d -name 'state_*' -exec rm -rf {} + 2>/dev/null || true
}

install_image_packages \
	haiku_devel \
	make \
	binutils \
	mpc \
	mpfr \
	gcc \
	pkgconfig \
	zstd_devel

if [ "$(getarch 2>/dev/null || true)" = "x86_gcc2" ]; then
	install_image_packages \
		haiku_x86_devel \
		binutils_x86 \
		mpc_x86 \
		mpfr_x86 \
		gcc_x86 \
		gcc_x86_syslibs \
		zstd_x86_devel
fi

cleanup_package_states

if [ "$(getarch 2>/dev/null || true)" = "x86_gcc2" ]; then
	install_optional_network_packages libffi_x86_devel ncurses6_x86_devel
else
	install_optional_network_packages libffi_devel ncurses6_devel
fi
cleanup_package_states
find /boot/_packages_ -maxdepth 1 -type f -name '*.hpkg' -exec rm -f {} + 2>/dev/null || true

df -h
command -v gcc
command -v make
command -v package
EOF
}

install_test_staging_dependencies() {
	local key="$1"
	local port="$2"

ssh_guest "$key" "$port" 'bash -s' <<'EOF'
set -e
export PATH=/boot/system/bin:/bin:$PATH
command -v tar
EOF
}

install_python3_dependency() {
	local key="$1"
	local port="$2"

ssh_guest "$key" "$port" 'bash -s' <<'EOF'
set -e
export PATH=/boot/system/bin:/bin:$PATH

if command -v python3 >/dev/null 2>&1; then
	exit 0
fi

run_limited() {
	limit="$1"
	shift

	# Haiku can retain a terminated team in ps until its parent reaps it.  The
	# hand-written kill -0 loop therefore never completed after a pkgman timeout.
	timeout -k 5 "$limit" "$@"
}

for package in python3 python312 python311 python310; do
	if run_limited 300 pkgman install -y "$package"; then
		break
	fi
done

if ! command -v python3 >/dev/null 2>&1; then
	for exe in python3.12 python3.11 python3.10; do
		if command -v "$exe" >/dev/null 2>&1; then
			mkdir -p /Work/tools
			ln -sf "$(command -v "$exe")" /Work/tools/python3
			break
		fi
	done
fi

PATH=/Work/tools:$PATH command -v python3
EOF
}

send_source_tree() {
	local key="$1"
	local port="$2"
	local target="$3"

	msg "Copying source tree to Haiku"
	tar_directory_to_guest "$key" "$port" "$ROOT" "$target" \
		--exclude='./.git' \
		--exclude='./out' \
		--exclude='./build' \
		--exclude='./.build*' \
		--exclude='./OMA' \
		--exclude='./OMA_old' \
		--exclude='./remote_probe_temp' \
		--exclude='./nuttx-suite-logs' \
		--exclude='./package-root' \
		--exclude='./package-root*' \
		--exclude='./*.hpkg' \
		--exclude='./*.deb' \
		--exclude='./*.ddeb' \
		--exclude='./*.rpm' \
		--exclude='./*.txz' \
		--exclude='./*.tar' \
		--exclude='./*.tar.gz' \
		--exclude='./*.tar.xz' \
		--exclude='./*.zip' \
		--exclude='./*.install_manifest' \
		--exclude='./package-root.install_manifest' \
		--exclude='./bin/fbc*' \
		--exclude='./bootstrap/fbc*' \
		--exclude='./*/obj' \
		--exclude='./*.o' \
		--exclude='./*.a'
}

prepare_haiku_bootstrap_sources() {
	local bootstrap_dir
	local bootstrap_fbc
	local bootstrap_path
	local fbc_target

	case "$ARCH" in
		x86_64)
			bootstrap_dir='haiku-amd64'
			fbc_target='haiku-x86_64'
			;;
		x86)
			bootstrap_dir='haiku-i386'
			fbc_target='haiku-x86'
			;;
		*)
			die "unsupported Haiku bootstrap architecture: $ARCH"
			;;
	esac

	bootstrap_path="$ROOT/bootstrap/$bootstrap_dir"
	bootstrap_fbc="${BOOT_FBC:-}"

	if [ -z "$bootstrap_fbc" ] && command -v fbc >/dev/null 2>&1; then
		bootstrap_fbc="$(command -v fbc)"
	fi

	if [ -n "$bootstrap_fbc" ]; then
		[ -x "$bootstrap_fbc" ] ||
			die "Haiku bootstrap compiler is not executable: $bootstrap_fbc"

		case "$bootstrap_path" in
			"$ROOT"/bootstrap/*) ;;
			*) die "unsafe Haiku bootstrap path: $bootstrap_path" ;;
		esac

		msg "Emitting $ARCH Haiku bootstrap sources with $bootstrap_fbc"
		rm -rf -- "$bootstrap_path"
		make -C "$ROOT" -j1 \
			HAVE_PREREQS_MK= \
			BOOT_FBC="$bootstrap_fbc" \
			BUILD_FBC="$bootstrap_fbc" \
			FBC_TARGET="$fbc_target" \
			FBTARGET_DIR_OVERRIDE="$bootstrap_dir" \
			bootstrap-emit
	fi

	if ! [ -d "$bootstrap_path" ] ||
		! find "$bootstrap_path" -maxdepth 1 -type f \
			\( -name '*.c' -o -name '*.asm' \) -print -quit | grep -q .; then
		die "Haiku bootstrap sources are missing; install fbc or set BOOT_FBC"
	fi
}

send_tests_tree() {
	local key="$1"
	local port="$2"

	msg "Copying fbctests source to Haiku"
	ssh_guest "$key" "$port" "rm -rf /Work/fbctests-source && mkdir -p /Work/fbctests-source"

	tar_directory_to_guest "$key" "$port" "$ROOT/tests" "/Work/fbctests-source/tests" \
		--exclude='./*.o' \
		--exclude='./*.a' \
		--exclude='./fbc-tests' \
		--exclude='./unit-tests.inc' \
		--exclude='./unit-tests-obj.lst' \
		--exclude='./log-tests-*.inc' \
		--exclude='./failed-log-tests-*.inc' \
		--exclude='./log-tests-*.lst' \
		--exclude='./log-tests-results-*.log'

	tar_directory_to_guest "$key" "$port" "$ROOT/inc" "/Work/fbctests-source/inc"

	tar_directory_to_guest "$key" "$port" "$ROOT/src/sfxlib" "/Work/fbctests-source/src/sfxlib" \
		--exclude='./obj' \
		--exclude='./*.o' \
		--exclude='./*.a'
}

send_exampleageddon_tree() {
	local key="$1"
	local port="$2"

	msg "Copying exampleageddon source to Haiku"
	ssh_guest "$key" "$port" \
		"rm -rf /Work/exampleageddon-source && mkdir -p /Work/exampleageddon-source/build_scripts /Work/exampleageddon-source/examples /Work/exampleageddon-source/inc"

	scp_to_guest "$key" "$port" \
		"$ROOT/build_scripts/exampleageddon-freebasic.py" \
		"/Work/exampleageddon-source/build_scripts/exampleageddon-freebasic.py"

	tar_directory_to_guest "$key" "$port" "$ROOT/examples" "/Work/exampleageddon-source/examples" \
		--exclude='./*.o' \
		--exclude='./*.obj' \
		--exclude='./*.asm' \
		--exclude='./*.exe' \
		--exclude='./gmon.out' \
		--exclude='./prof-*.txt'

	tar_directory_to_guest "$key" "$port" "$ROOT/inc" "/Work/exampleageddon-source/inc"
}

build_package_in_vm() {
	local vm_dir="$1"
	local key="$vm_dir/id_ed25519"
	local port="$2"
	local source_dir="/Work/freebasic-source"

	msg "Preparing Haiku build VM"
	format_and_mount_work "$key" "$port"
	install_build_dependencies "$key" "$port"
	send_source_tree "$key" "$port" "$source_dir"

	msg "Building Haiku package"
	mkdir -p "$PACKAGE_DIR" "$LOG_DIR"
	if ! ssh_guest "$key" "$port" "SOURCE_DIR='$source_dir' /bin/sh -s" <<'EOF'
set -e

log=/Work/freebasic-haiku-build.log
cd "$SOURCE_DIR"
rm -f "$log"

HAIKU_SKIP_DEPS=1 HAIKU_SKIP_NET_DEPS=1 HAIKU_PRESERVE_HPKG=1 sh ./build_scripts/haiku-build-freebasic.sh --noinstall > "$log" 2>&1 &
pid=$!

while kill -0 "$pid" 2>/dev/null; do
	sleep 60
	if kill -0 "$pid" 2>/dev/null; then
		printf 'Haiku build still running: '
		tail -n 1 "$log" 2>/dev/null | tr '\000' ' ' | cut -c 1-160 || true
	fi
done

wait "$pid"
EOF
	then
		rm -f "$LOG_DIR/freebasic-haiku-build.log"
		scp_from_guest "$key" "$port" "/Work/freebasic-haiku-build.log" "$LOG_DIR/" || true
		tail -n 120 "$LOG_DIR/freebasic-haiku-build.log" >&2 || true
		die "Haiku package build failed"
	fi

	rm -f "$LOG_DIR/freebasic-haiku-build.log"
	scp_from_guest "$key" "$port" "/Work/freebasic-haiku-build.log" "$LOG_DIR/"

	local remote_hpkg
	remote_hpkg="$(ssh_guest "$key" "$port" "cd '$source_dir' && ls -1 freebasic*.hpkg | sort | tail -n 1")"
	[ -n "$remote_hpkg" ] || die "Haiku package was not created"
	scp_from_guest "$key" "$port" "$source_dir/$remote_hpkg" "$PACKAGE_DIR/"

	local hpkg
	hpkg="$PACKAGE_DIR/$remote_hpkg"
	[ -n "$hpkg" ] && [ -f "$hpkg" ] || die "Haiku package was not copied out"
	printf '%s\n' "$hpkg" > "$vm_dir/package.path"
}

write_test_runner() {
	local path="$1"

	cat > "$path" <<'EOF'
#!/bin/sh

set -e

run() {
	echo "==> $*"
	"$@"
}

fail() {
	echo "ERROR: $*" >&2
	exit 1
}

pkgman_install() {
	run_limited 300 pkgman install -y "$@"
}

run_limited() {
	limit="$1"
	shift

	# Haiku can retain a terminated team in ps until its parent reaps it.  The
	# hand-written kill -0 loop therefore never completed after a pkgman timeout.
	timeout -k 5 "$limit" "$@"
}

install_image_package() {
	name="$1"
	package_file="$(find /boot/_packages_ -maxdepth 1 -type f -name "$name-*.hpkg" 2>/dev/null | sort | tail -n 1)"

	if [ -z "$package_file" ]; then
		return 1
	fi

	if [ -e "/boot/system/packages/$(basename "$package_file")" ]; then
		return 0
	fi

	echo "==> installing image package $package_file"
	run_limited 300 pkgman install -y "$package_file"
}

install_image_packages() {
	for package_name in "$@"; do
		install_image_package "$package_name" || true
	done
}

install_optional_packages() {
	pkgman_install "$@" || {
		echo "WARNING: optional pkgman install failed or timed out: $*" >&2
		return 0
	}
}

cleanup_package_states() {
	find /boot/system/packages/administrative -maxdepth 1 -type d -name 'state_*' -exec rm -rf {} + 2>/dev/null || true
}

using_secondary_x86() {
	[ "$(getarch 2>/dev/null || true)" = "x86_gcc2" ]
}

fbc_make_command() {
	if using_secondary_x86; then
		echo "setarch x86 fbc"
	else
		echo "fbc"
	fi
}

fbc_command() {
	if using_secondary_x86; then
		setarch x86 fbc "$@"
	else
		fbc "$@"
	fi
}

print_secondary_x86_tools() {
	using_secondary_x86 || return 0

	echo "==> checking secondary x86 tool paths"
	find /boot/system -type f \( -path '*/x86/*/ld' -o -path '*/x86/*/as' -o -path '*/x86/*/gcc' \) -print | sort
	for tool in as ar ld; do
		printf '%s: ' "$tool"
		setarch x86 /boot/system/develop/tools/x86/bin/gcc "-print-prog-name=$tool"
	done
}

ensure_python3() {
	if command -v python3 >/dev/null 2>&1; then
		return 0
	fi

	if using_secondary_x86; then
		for exe in python3 python3.10 python3.12 python3.11 python3.13 python3.14; do
			if setarch x86 "$exe" --version >/dev/null 2>&1; then
				mkdir -p /Work/tools
				cat > /Work/tools/python3 <<PYEOF
#!/bin/sh
exec setarch x86 $exe "\$@"
PYEOF
				chmod +x /Work/tools/python3
				return 0
			fi
		done
	fi

	for package_file in /Work/package/python3*.hpkg; do
		[ -f "$package_file" ] || continue
		if run_limited "${PYTHON_INSTALL_TIMEOUT:-1800}" pkgman install -y "$package_file"; then
			break
		fi
	done

	if command -v python3 >/dev/null 2>&1; then
		return 0
	fi

	if using_secondary_x86; then
		python_packages="python310_x86 python312_x86 python311_x86 python313_x86 python314_x86 python310 python312 python311 python313 python314"
	else
		python_packages="python310 python312 python311 python313 python314 python3.10 python3.12 python3.11 python3.13 python3.14 python3"
	fi

	for package in $python_packages; do
		if run_limited "${PYTHON_INSTALL_TIMEOUT:-1800}" pkgman install -y "$package"; then
			break
		fi
	done

	if using_secondary_x86; then
		for exe in python3 python3.10 python3.12 python3.11 python3.13 python3.14; do
			if setarch x86 "$exe" --version >/dev/null 2>&1; then
				mkdir -p /Work/tools
				cat > /Work/tools/python3 <<PYEOF
#!/bin/sh
exec setarch x86 $exe "\$@"
PYEOF
				chmod +x /Work/tools/python3
				return 0
			fi
		done
	fi

	if ! command -v python3 >/dev/null 2>&1; then
		for exe in python3.14 python3.13 python3.12 python3.11 python3.10 python3.9; do
			if command -v "$exe" >/dev/null 2>&1; then
				mkdir -p /Work/tools
				ln -sf "$(command -v "$exe")" /Work/tools/python3
				break
			fi
		done
	fi

	PATH=/Work/tools:$PATH command -v python3
}

fbctests_jobs() {
	case "${FBCTESTS_JOBS:-}" in
		''|*[!0-9]*|0) echo 1 ;;
		*) echo "$FBCTESTS_JOBS" ;;
	esac
}

exampleageddon_jobs() {
	case "${EXAMPLEAGEDDON_JOBS:-}" in
		''|*[!0-9]*|0) echo 1 ;;
		*) echo "$EXAMPLEAGEDDON_JOBS" ;;
	esac
}

run_gfx_smoke() {
	out="$1"
	err="$2"
	shift 2

	if timeout 20 "$@" > "$out" 2> "$err"; then
		cat "$out" || true
		[ ! -s "$err" ] || {
			cat "$err"
			fail "gfx smoke wrote stderr"
		}
		return 0
	fi

	cat "$out" || true
	cat "$err" || true
	fail "gfx smoke failed"
}

run_fbctests() {
	jobs="$(fbctests_jobs)"
	fbc_cmd="$(fbc_make_command)"
	cc_cmd="cc"
	cxx_cmd="g++"

	# The x86_gcc2 image keeps GCC 2 as its primary compiler for ABI
	# compatibility and exposes the modern 32-bit toolchain through the
	# -x86 command aliases.  Multi-module tests use C and C++ helpers which
	# require options such as -m32 and therefore must use those aliases too.
	if using_secondary_x86; then
		cc_cmd="cc-x86"
		cxx_cmd="g++-x86"
		command -v "$cc_cmd" >/dev/null 2>&1 || fail "missing secondary x86 C compiler: $cc_cmd"
		command -v "$cxx_cmd" >/dev/null 2>&1 || fail "missing secondary x86 C++ compiler: $cxx_cmd"
	fi

	[ -d /Work/fbctests-source/tests ] || fail "tests source was not staged"
	[ -d /Work/fbctests-source/inc ] || fail "inc source was not staged"

	cd /Work/fbctests-source/tests

	echo "==> cleaning fbctests tree"
	run make clean FBC="$fbc_cmd" CC="$cc_cmd" CXX="$cxx_cmd" GCC="$cc_cmd"

	echo "==> checking installed compiler through fbctests"
	run make check FBC="$fbc_cmd" CC="$cc_cmd" CXX="$cxx_cmd" GCC="$cc_cmd"

	echo "==> running unit-tests with ${jobs} job(s)"
	run make -j "$jobs" unit-tests FBC="$fbc_cmd" CC="$cc_cmd" CXX="$cxx_cmd" GCC="$cc_cmd" UNITTEST_RUN_ARGS="${FBCTESTS_UNIT_ARGS:-}"

	echo "==> running log-tests with ${jobs} job(s)"
	run make -j "$jobs" log-tests FBC="$fbc_cmd" CC="$cc_cmd" CXX="$cxx_cmd" GCC="$cc_cmd"

	for failed_log in failed-fb.log failed-fblite.log failed-qb.log failed-deprecated.log; do
		[ -f "$failed_log" ] || fail "missing log-tests summary: $failed_log"
		if ! grep -qi 'None Found' "$failed_log"; then
			cat "$failed_log"
			fail "log-tests reported failures in $failed_log"
		fi
	done

	echo "==> fbctests passed"
}

run_exampleageddon() {
	jobs="$(exampleageddon_jobs)"
	fbc_cmd="$(fbc_make_command)"

	[ -d /Work/exampleageddon-source/examples ] || fail "examples source was not staged"
	[ -d /Work/exampleageddon-source/inc ] || fail "inc source was not staged for exampleageddon"
	[ -f /Work/exampleageddon-source/build_scripts/exampleageddon-freebasic.py ] ||
		fail "exampleageddon runner was not staged"

	rm -rf /Work/exampleageddon

	echo "==> running exampleageddon with ${jobs} job(s)"
	run python3 /Work/exampleageddon-source/build_scripts/exampleageddon-freebasic.py \
		--root /Work/exampleageddon-source \
		--prefix /boot/system \
		--include-dir /Work/exampleageddon-source/inc \
		--outdir /Work/exampleageddon \
		--fbc "$fbc_cmd" \
		--jobs "$jobs" \
		--compile-timeout "${EXAMPLEAGEDDON_COMPILE_TIMEOUT:-120}" \
		--run-timeout "${EXAMPLEAGEDDON_RUN_TIMEOUT:-10}"

	[ -f /Work/exampleageddon/report.md ] || fail "exampleageddon report was not created"
	[ -f /Work/exampleageddon/results.csv ] || fail "exampleageddon results CSV was not created"

	if ! grep -qx -- '- Self-contained problems: 0' /Work/exampleageddon/report.md; then
		sed -n '1,80p' /Work/exampleageddon/report.md
		fail "exampleageddon reported self-contained example problems"
	fi

	echo "==> exampleageddon passed"
}

export PATH=/Work/tools:/boot/system/bin:/bin:$PATH
export FBGFX="${FBGFX:-}"
export FB_GFX_DRIVER="${FB_GFX_DRIVER:-}"
export SFXLIB_DRIVER="${SFXLIB_DRIVER:-default}"

echo "==> installing package test dependencies"
install_image_packages \
	haiku_devel \
	make \
	binutils \
	mpc \
	mpfr \
	gcc \
	pkgconfig \
	zstd_devel

if using_secondary_x86; then
	install_image_packages \
		haiku_x86_devel \
		binutils_x86 \
		mpc_x86 \
		mpfr_x86 \
		gcc_x86 \
		gcc_x86_syslibs \
		zstd_x86_devel
fi

cleanup_package_states

if using_secondary_x86; then
	run install_optional_packages libffi_x86_devel ncurses6_x86_devel
else
	run install_optional_packages libffi_devel ncurses6_devel
fi
cleanup_package_states
find /boot/_packages_ -maxdepth 1 -type f -name '*.hpkg' -exec rm -f {} + 2>/dev/null || true

echo "==> installing FreeBASIC package"
pkgman uninstall -y freebasic freebasic_x86 >/dev/null 2>&1 || true
run pkgman_install /Work/package/freebasic*.hpkg

echo "==> verifying fbc"
fbc_command -version
print_secondary_x86_tools

mkdir -p /Work/smoke

cat > /Work/smoke/console.bas <<'FBEOF'
print "Hello world"
FBEOF

cat > /Work/smoke/gfx-truecolor.bas <<'FBEOF'
#include once "fbgfx.bi"

sub fail( byref message as string )
	print "gfx truecolor failure: "; message
	end 1
end sub

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
else
	screenset 0, 0
end if
line (0, 0)-(63, 63), rgb( 0, 0, 0 ), bf
line (8, 32)-(55, 55), rgb( 255, 255, 255 ), bf
expect_rgb 8, 32, rgb( 255, 255, 255 ), "screenset page"

screensync
sleep 50, 1
screen 0
FBEOF

cat > /Work/smoke/gfx-screen-modes.bas <<'FBEOF'
#include once "fbgfx.bi"

sub fail( byref message as string )
	print "gfx legacy failure: "; message
	end 1
end sub

sub expect_index( byval x as integer, byval y as integer, byval expected as ulong, byref label as string )
	dim as ulong actual = culng( point( x, y ) )

	if( actual <> expected ) then
		print "gfx legacy mismatch: "; label; " actual="; actual; " expected="; expected
		end 1
	end if
end sub

sub draw_mode( byval mode as integer )
	dim as integer w, h, depth, bpp, pitch

	screeninfo w, h, depth, bpp, pitch
	if( w < 48 or h < 32 ) then
		fail "mode " & str( mode ) & " reported an unexpectedly small framebuffer"
	end if

	screenset 0, 0
	cls

	if( depth <= 1 ) then
		palette 1, 255, 255, 255
		line (0, 0)-(47, 31), 0, bf
		line (8, 8)-(23, 23), 1, bf
		expect_index 8, 8, 1, "mode " & str( mode ) & " white block"
	else
		palette 1, 255, 0, 0
		palette 2, 0, 255, 0
		palette 3, 0, 0, 255
		line (0, 0)-(63, 31), 0, bf
		line (8, 8)-(23, 23), 1, bf
		line (24, 8)-(39, 23), 2, bf
		line (40, 8)-(55, 23), 3, bf
		expect_index 8, 8, 1, "mode " & str( mode ) & " red block"
		expect_index 24, 8, 2, "mode " & str( mode ) & " green block"
		expect_index 40, 8, 3, "mode " & str( mode ) & " blue block"
	end if

	screensync
	sleep 30, 1
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

end 0
FBEOF

cat > /Work/smoke/sfx.bas <<'FBEOF'
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
		end 2
	end if
else
	print "sfx-driver=<none>"
	end 3
end if
sound 440, 0.75
play "t120 o4 l4 c e g > c"
sleep 1600, 1
print "sfx-end"
FBEOF

echo "==> compiling console smoke"
run fbc_command /Work/smoke/console.bas -x /Work/smoke/console

echo "==> running console smoke"
console_output="$(/Work/smoke/console)"
echo "$console_output"
[ "$console_output" = "Hello world" ] || fail "unexpected console output"

echo "==> compiling crt/sys/socket.bi API smoke"
run fbc_command /Work/fbctests-source/tests/crt/socket.bas -x /Work/smoke/socket-bi

echo "==> running crt/sys/socket.bi API smoke"
run /Work/smoke/socket-bi

echo "==> compiling curses.bi API smoke"
run fbc_command /Work/fbctests-source/tests/crt/curses.bas -x /Work/smoke/curses-bi

echo "==> running curses.bi API smoke"
run /Work/smoke/curses-bi

echo "==> compiling TCP loopback smoke"
run fbc_command -mt /Work/fbctests-source/tests/file/tcp.bas -x /Work/smoke/tcp

echo "==> running TCP loopback smoke"
timeout 60 /Work/smoke/tcp

echo "==> compiling gfxlib truecolor smoke"
run fbc_command /Work/smoke/gfx-truecolor.bas -x /Work/smoke/gfx-truecolor

echo "==> compiling gfxlib SCREEN mode smoke"
run fbc_command -lang fblite -exx /Work/smoke/gfx-screen-modes.bas -x /Work/smoke/gfx-screen-modes

echo "==> running gfxlib truecolor smoke"
run_gfx_smoke /Work/smoke/gfx-truecolor.out /Work/smoke/gfx-truecolor.err /Work/smoke/gfx-truecolor

echo "==> running gfxlib SCREEN mode smoke"
run_gfx_smoke /Work/smoke/gfx-screen-modes.out /Work/smoke/gfx-screen-modes.err /Work/smoke/gfx-screen-modes

echo "==> compiling sfxlib smoke"
run fbc_command /Work/smoke/sfx.bas -x /Work/smoke/sfx

echo "==> compiling sfxlib showcase"
[ -f /boot/system/data/freebasic/examples/sfxlib/showcase.bas ] ||
	fail "sfxlib showcase example is not installed"
rm -rf /Work/smoke/sfxlib-showcase
mkdir -p /Work/smoke/sfxlib-showcase
cp -R /boot/system/data/freebasic/examples/sfxlib/. /Work/smoke/sfxlib-showcase/
(
	cd /Work/smoke/sfxlib-showcase
	run fbc_command showcase.bas -x /Work/smoke/sfx-showcase
)
[ -x /Work/smoke/sfx-showcase ] || fail "sfxlib showcase binary was not created"

echo "==> running sfxlib real audio smoke"
SFXLIB_DRIVER=Haiku timeout 20 /Work/smoke/sfx > /Work/smoke/sfx.out 2> /Work/smoke/sfx.err || {
	cat /Work/smoke/sfx.out || true
	cat /Work/smoke/sfx.err || true
	fail "sfx smoke failed"
}
cat /Work/smoke/sfx.out || true
grep -qx 'sfx-start' /Work/smoke/sfx.out || fail "sfx smoke did not start"
grep -qx 'sfx-end' /Work/smoke/sfx.out || fail "sfx smoke did not finish"
grep -qi '^sfx-driver=haiku' /Work/smoke/sfx.out || fail "sfx smoke did not use Haiku"
[ ! -s /Work/smoke/sfx.err ] || {
	cat /Work/smoke/sfx.err
	fail "sfx smoke wrote stderr"
}

echo "==> sfxlib smoke passed"

run_fbctests
if run ensure_python3; then
	run_exampleageddon
else
	echo "==> guest Python unavailable; host will run exampleageddon"
	touch /Work/exampleageddon-needs-host
fi

echo "==> TEST PASSED"
EOF

	chmod +x "$path"
}

run_host_exampleageddon() {
	local vm_dir="$1"
	local key="$2"
	local port="$3"
	local outdir="$vm_dir/exampleageddon-host"
	local log="$LOG_DIR/freebasic-haiku-exampleageddon-host.log"
	local remote_shell

	remote_shell="ssh $(ssh_opts "$key" "$port") user@127.0.0.1"

	msg "Running host-driven exampleageddon against Haiku"
	rm -rf "$outdir"
	if ! python3 "$ROOT/build_scripts/exampleageddon-freebasic.py" \
			--root "$ROOT" \
			--prefix /boot/system \
			--include-dir "$ROOT/inc" \
			--outdir "$outdir" \
			--fbc "setarch x86 fbc" \
			--jobs "$EXAMPLEAGEDDON_JOBS" \
			--compile-timeout "$EXAMPLEAGEDDON_COMPILE_TIMEOUT" \
			--run-timeout "$EXAMPLEAGEDDON_RUN_TIMEOUT" \
			--fail-on-self-contained \
			--remote-shell "$remote_shell" \
			--path-map "$ROOT=/Work/exampleageddon-source" \
			--path-map "$outdir=/Work/exampleageddon" \
			> "$log" 2>&1; then
		tail -n 160 "$log" >&2 || true
		return 1
	fi

	cp -f "$outdir/report.md" "$vm_dir/exampleageddon-report.md"
	cp -f "$outdir/results.csv" "$vm_dir/exampleageddon-results.csv"
}

test_package_in_vm() {
	local vm_dir="$1"
	local key="$vm_dir/id_ed25519"
	local port="$2"
	local hpkg="$3"
	local runner="$vm_dir/test-freebasic-haiku.sh"

	msg "Preparing Haiku test VM"
	format_and_mount_work "$key" "$port"
	install_test_staging_dependencies "$key" "$port"
	ssh_guest "$key" "$port" "mkdir -p /Work/package"
	scp_to_guest "$key" "$port" "$hpkg" "/Work/package/"
	if [ -n "$PYTHON_HPKG" ]; then
		scp_to_guest "$key" "$port" "$PYTHON_HPKG" "/Work/package/"
	fi
	send_tests_tree "$key" "$port"
	send_exampleageddon_tree "$key" "$port"

	write_test_runner "$runner"
	scp_to_guest "$key" "$port" "$runner" "/Work/test-freebasic-haiku.sh"

	msg "Running Haiku package smoke tests, fbctests, and exampleageddon"
	if ! ssh_guest "$key" "$port" \
			"FBCTESTS_JOBS='$FBCTESTS_JOBS' FBCTESTS_UNIT_ARGS='$FBCTESTS_UNIT_ARGS' EXAMPLEAGEDDON_JOBS='$EXAMPLEAGEDDON_JOBS' EXAMPLEAGEDDON_COMPILE_TIMEOUT='$EXAMPLEAGEDDON_COMPILE_TIMEOUT' EXAMPLEAGEDDON_RUN_TIMEOUT='$EXAMPLEAGEDDON_RUN_TIMEOUT' PYTHON_INSTALL_TIMEOUT='$PYTHON_INSTALL_TIMEOUT' /bin/sh -s" <<'EOF'
set -e

log=/Work/freebasic-haiku-test.log
rm -f "$log"

/bin/sh /Work/test-freebasic-haiku.sh > "$log" 2>&1 &
pid=$!

while kill -0 "$pid" 2>/dev/null; do
	sleep 60
	if kill -0 "$pid" 2>/dev/null; then
		printf 'Haiku tests still running: '
		tail -n 1 "$log" 2>/dev/null | tr '\000' ' ' | cut -c 1-160 || true
	fi
done

wait "$pid"
EOF
	then
		rm -f "$LOG_DIR/freebasic-haiku-test.log"
		scp_from_guest "$key" "$port" "/Work/freebasic-haiku-test.log" "$LOG_DIR/" || true
		scp_from_guest "$key" "$port" "/Work/exampleageddon/report.md" "$vm_dir/exampleageddon-report.md" || true
		scp_from_guest "$key" "$port" "/Work/exampleageddon/results.csv" "$vm_dir/exampleageddon-results.csv" || true
		tail -n 160 "$LOG_DIR/freebasic-haiku-test.log" >&2 || true
		die "Haiku package smoke tests, fbctests, or exampleageddon failed"
	fi

	rm -f "$LOG_DIR/freebasic-haiku-test.log"
	scp_from_guest "$key" "$port" "/Work/freebasic-haiku-test.log" "$LOG_DIR/"
	if ssh_guest "$key" "$port" "test -f /Work/exampleageddon-needs-host"; then
		run_host_exampleageddon "$vm_dir" "$key" "$port" ||
			die "host-driven exampleageddon failed"
	else
		scp_from_guest "$key" "$port" "/Work/exampleageddon/report.md" "$vm_dir/exampleageddon-report.md"
		scp_from_guest "$key" "$port" "/Work/exampleageddon/results.csv" "$vm_dir/exampleageddon-results.csv"
	fi
}

verify_audio_capture() {
	local wav="$1"
	local log="$2"

	msg "Verifying Haiku QEMU audio capture"
	python3 - "$wav" "$log" <<'PY'
import math
import struct
import sys
import time
import wave

wav_path = sys.argv[1]
log_path = sys.argv[2]

try:
    last_error = None
    for _ in range(20):
        try:
            with wave.open(wav_path, "rb") as w:
                channels = w.getnchannels()
                width = w.getsampwidth()
                rate = w.getframerate()
                frames = w.getnframes()
                data = w.readframes(frames)
            break
        except Exception as ex:
            last_error = ex
            time.sleep(0.5)
    else:
        raise last_error
except Exception as ex:
    raise SystemExit(f"failed to read audio capture: {ex}")

if width != 2:
    raise SystemExit(f"unexpected sample width: {width}")

sample_count = len(data) // 2
if sample_count <= 0:
    raise SystemExit("audio capture contains no samples")

samples = struct.unpack("<%dh" % sample_count, data)
peak = max(abs(sample) for sample in samples)
rms = math.sqrt(sum(sample * sample for sample in samples) / float(sample_count))
active = sum(1 for sample in samples if abs(sample) > 500)

with open(log_path, "w", encoding="utf-8") as f:
    f.write(f"file={wav_path}\n")
    f.write(f"rate={rate}\n")
    f.write(f"channels={channels}\n")
    f.write(f"frames={frames}\n")
    f.write(f"samples={sample_count}\n")
    f.write(f"peak={peak}\n")
    f.write(f"rms={rms:.2f}\n")
    f.write(f"active_samples={active}\n")

if frames < rate // 4:
    raise SystemExit("audio capture is too short")
if peak < 1000:
    raise SystemExit("audio capture peak is too low")
if rms < 50.0:
    raise SystemExit("audio capture RMS is too low")
if active < rate // 20:
    raise SystemExit("audio capture has too few active samples")

with open(log_path, "a", encoding="utf-8") as f:
    f.write("result=PASS\n")
PY
}

cleanup() {
	stop_vm "$RUN_DIR/build" || true
	stop_vm "$RUN_DIR/test" || true
}

trap cleanup EXIT

main() {
	check_host_tools
	configure_qemu_acceleration
	resolve_package_file

	if [ -z "$SSH_PORT" ]; then SSH_PORT="$(find_free_port 10022)"; fi
	if [ -z "$HTTP_PORT" ]; then HTTP_PORT="$(find_free_port 18080)"; fi
	if [ -z "$VNC_DISPLAY" ]; then VNC_DISPLAY="$(find_free_vnc_display)"; fi

	msg "Haiku VM ports: ssh=$SSH_PORT http=$HTTP_PORT vnc=127.0.0.1:$VNC_DISPLAY"

	download_image
	extract_iso
	if [ "$TEST_ONLY" -eq 0 ]; then
		prepare_haiku_bootstrap_sources
	fi

	rm -rf "$RUN_DIR"
	mkdir -p "$RUN_DIR" "$PACKAGE_DIR" "$LOG_DIR"
	rm -f "$LOG_DIR"/freebasic-haiku-*.log

	local build_dir test_dir hpkg

	if [ "$TEST_ONLY" -eq 1 ]; then
		hpkg="$PACKAGE_FILE"
	else
		build_dir="$(prepare_vm build "$SSH_PORT" "$HTTP_PORT" "$VNC_DISPLAY")"
		build_package_in_vm "$build_dir" "$SSH_PORT"
		hpkg="$(cat "$build_dir/package.path")"
		[ -n "$hpkg" ] && [ -f "$hpkg" ] || die "Haiku package was not copied out"
		stop_vm "$build_dir"
	fi

	SSH_PORT="$(find_free_port 10022)"
	HTTP_PORT="$(find_free_port 18080)"
	VNC_DISPLAY="$(find_free_vnc_display)"

	test_dir="$(prepare_vm test "$SSH_PORT" "$HTTP_PORT" "$VNC_DISPLAY" "$RUN_DIR/test/freebasic-haiku-audio.wav")"
	test_package_in_vm "$test_dir" "$SSH_PORT" "$hpkg"
	stop_vm "$test_dir"
	verify_audio_capture "$test_dir/freebasic-haiku-audio.wav" "$LOG_DIR/freebasic-haiku-audio.log"
	archive_results "$hpkg"

	if [ "$KEEP_VMS" -eq 0 ]; then
		rm -rf "$RUN_DIR"
	fi

	msg "Haiku package build, fbctests, and exampleageddon completed"
	echo "Package: $hpkg"
	echo "Archive: $ARCHIVE_DIR"
	echo "Build log: $LOG_DIR/freebasic-haiku-build.log"
	echo "Test log:  $LOG_DIR/freebasic-haiku-test.log"
	echo "Audio log: $LOG_DIR/freebasic-haiku-audio.log"
}

main "$@"

##############################################################################
# end of haiku-vm-build-freebasic.sh
##############################################################################
