#!/usr/bin/env bash

##############################################################################
# FreeBASIC OpenBSD VM package builder
##############################################################################
#
# Purpose:
#
#   Build and test the OpenBSD x86_64 FreeBASIC package from a Debian/Ubuntu
#   Linux host.
#
# Responsibilities:
#
#   * PXE-boot the official OpenBSD amd64 ramdisk installer in QEMU
#   * install a clean OpenBSD VM with SSH enabled
#   * build the OpenBSD package with build_scripts/openbsd-build-freebasic.sh
#   * install the package in a separate clean VM overlay
#   * run console, gfxlib, sfxlib, and fbctests checks
#   * archive the package and logs under out/openbsd/x86-64
#
# This script intentionally does NOT contain:
#
#   * non-x86_64 OpenBSD support
#   * cross-compilation into OpenBSD packages
#   * GUI installer automation
#
##############################################################################

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORKROOT="$ROOT/out/openbsd-vm"
CACHE_DIR="$WORKROOT/cache"
RUN_DIR="$WORKROOT/run"
LOG_DIR="$WORKROOT/logs"
ARCHIVE_DIR="$ROOT/out/openbsd/x86-64"

RELEASE="7.8"
ARCH="amd64"
GUEST_USER="root"
PXEBOOT_URL=""
BSDRD_URL=""
PACKAGE_FILE=""
TEST_ONLY=0
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"
CPUS="$JOBS"
MEMORY="6144"
DISK_SIZE="64G"
SSH_PORT=""
KEEP_VMS=0
FBCTESTS_JOBS="$JOBS"
FBCTESTS_UNIT_ARGS=""

DEFAULT_BASE_URL="https://cdn.openbsd.org/pub/OpenBSD/${RELEASE}/${ARCH}"
DEFAULT_PACKAGE_PATH="https://cdn.openbsd.org/pub/OpenBSD/${RELEASE}/packages/${ARCH}"

msg() { printf '\n==> %s\n' "$*"; }
warn() { printf '\nWARNING: %s\n' "$*" >&2; }
die() { printf '\nERROR: %s\n' "$*" >&2; exit 1; }

usage() {
	cat <<EOF
Usage: ./build_scripts/openbsd-vm-build-freebasic.sh [options]

Options:
  --pxeboot-url URL        OpenBSD pxeboot URL. Default: ${DEFAULT_BASE_URL}/pxeboot
  --bsd-rd-url URL         OpenBSD bsd.rd URL. Default: ${DEFAULT_BASE_URL}/bsd.rd
  --package FILE           Existing OpenBSD package to test.
  --test-only              Test --package without rebuilding FreeBASIC.
  --workroot DIR           Work directory. Default: out/openbsd-vm
  --archive-dir DIR        Final archive directory. Default: out/openbsd/x86-64
  --jobs N                 Build jobs inside OpenBSD. Default: host CPU count
  --cpus N                 QEMU CPU count. Default: --jobs value
  --memory MB              QEMU memory in MB. Default: 6144
  --disk-size SIZE         Installed VM disk size. Default: 64G
  --ssh-port N             Host SSH forward port. Default: auto
  --fbctests-jobs N        fbctests gmake jobs. Default: --jobs value
  --fbctests-unit-args S   Extra UNITTEST_RUN_ARGS for fbctests.
  --keep-vms               Do not delete VM run directories on success.
  -h, --help               Show this help.
EOF
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--pxeboot-url) PXEBOOT_URL="$2"; shift 2 ;;
		--bsd-rd-url) BSDRD_URL="$2"; shift 2 ;;
		--package) PACKAGE_FILE="$2"; shift 2 ;;
		--test-only) TEST_ONLY=1; shift ;;
		--workroot)
			WORKROOT="$2"
			CACHE_DIR="$WORKROOT/cache"
			RUN_DIR="$WORKROOT/run"
			LOG_DIR="$WORKROOT/logs"
			shift 2
			;;
		--archive-dir) ARCHIVE_DIR="$2"; shift 2 ;;
		--jobs) JOBS="$2"; CPUS="$2"; FBCTESTS_JOBS="$2"; shift 2 ;;
		--cpus) CPUS="$2"; shift 2 ;;
		--memory) MEMORY="$2"; shift 2 ;;
		--disk-size) DISK_SIZE="$2"; shift 2 ;;
		--ssh-port) SSH_PORT="$2"; shift 2 ;;
		--fbctests-jobs) FBCTESTS_JOBS="$2"; shift 2 ;;
		--fbctests-unit-args) FBCTESTS_UNIT_ARGS="$2"; shift 2 ;;
		--keep-vms) KEEP_VMS=1; shift ;;
		-h|--help) usage; exit 0 ;;
		*) die "unknown option: $1" ;;
	esac
done

case "$JOBS" in ''|*[!0-9]*|0) die "--jobs must be a positive integer" ;; esac
case "$CPUS" in ''|*[!0-9]*|0) die "--cpus must be a positive integer" ;; esac
case "$FBCTESTS_JOBS" in ''|*[!0-9]*|0) die "--fbctests-jobs must be a positive integer" ;; esac

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
	require_tool curl
	require_tool python3
	require_tool qemu-img
	require_tool qemu-system-x86_64
	require_tool rsync
	require_tool scp
	require_tool ssh
}

resolve_package_file() {
	[ -n "$PACKAGE_FILE" ] || return 0
	[ -f "$PACKAGE_FILE" ] || die "package not found: $PACKAGE_FILE"

	PACKAGE_FILE="$(cd "$(dirname "$PACKAGE_FILE")" && pwd)/$(basename "$PACKAGE_FILE")"
}

download_installer_files() {
	mkdir -p "$CACHE_DIR" "$LOG_DIR"

	if [ -z "$PXEBOOT_URL" ]; then
		PXEBOOT_URL="$DEFAULT_BASE_URL/pxeboot"
	fi

	if [ -z "$BSDRD_URL" ]; then
		BSDRD_URL="$DEFAULT_BASE_URL/bsd.rd"
	fi

	if [ ! -f "$CACHE_DIR/pxeboot" ]; then
		msg "Downloading $PXEBOOT_URL"
		curl -fL --retry 3 --retry-delay 5 -o "$CACHE_DIR/pxeboot" "$PXEBOOT_URL"
	else
		msg "Using cached $CACHE_DIR/pxeboot"
	fi

	if [ ! -f "$CACHE_DIR/bsd.rd" ]; then
		msg "Downloading $BSDRD_URL"
		curl -fL --retry 3 --retry-delay 5 -o "$CACHE_DIR/bsd.rd" "$BSDRD_URL"
	else
		msg "Using cached $CACHE_DIR/bsd.rd"
	fi
}

make_ssh_key() {
	local key="$1"

	if [ ! -f "$key" ]; then
		ssh-keygen -q -t ed25519 -N '' -f "$key"
	fi
}

write_install_files() {
	local vm_dir="$1"
	local key_pub="$2"
	local tftp_dir="$vm_dir/tftp"
	local install_conf="$vm_dir/install.conf"

	mkdir -p "$tftp_dir/etc"
	cp "$CACHE_DIR/pxeboot" "$tftp_dir/pxeboot"
	cp "$CACHE_DIR/bsd.rd" "$tftp_dir/bsd.rd"
	ln -sf pxeboot "$tftp_dir/auto_install"

	cat > "$tftp_dir/etc/boot.conf" <<'EOF'
stty com0 115200
set tty com0
boot tftp:/bsd.rd
EOF

	cat > "$install_conf" <<EOF
System hostname = fbc-openbsd
Password for root = *************
Public ssh key for root account = $(cat "$key_pub")
Start sshd(8) by default = yes
Do you expect to run the X Window System = yes
Do you want the X Window System to be started by xenodm(1) = no
Change the default console to com0 = yes
Which speed should com0 use = 115200
Setup a user = fbc
Full name for user fbc = fbc
Password for user fbc = *************
Public ssh key for user fbc = $(cat "$key_pub")
Allow root ssh login = prohibit-password
What timezone are you in = UTC
Default IPv4 route = 10.0.2.2
DNS domain name = local
DNS nameservers = 10.0.2.3
Which disk is the root disk = sd0
Encrypt the root disk with a (p)assphrase or (k)eydisk = no
Use (W)hole disk MBR, whole disk (G)PT or (E)dit = whole
URL to autopartitioning template for disklabel = none
Use (A)uto layout, (E)dit auto layout, or create (C)ustom layout = A
Location of sets = http
HTTP proxy URL = none
HTTP Server = cdn.openbsd.org
Server directory = pub/OpenBSD/${RELEASE}/${ARCH}
Set name(s) = -game* +x*
EOF
}

install_base_vm() {
	local vm_dir="$RUN_DIR/base"
	local disk="$vm_dir/openbsd-base.qcow2"
	local key="$RUN_DIR/id_ed25519"
	local install_log="$LOG_DIR/freebasic-openbsd-install.log"

	rm -rf "$vm_dir"
	mkdir -p "$vm_dir" "$LOG_DIR"

	make_ssh_key "$key"
	write_install_files "$vm_dir" "$key.pub"

	msg "Creating OpenBSD base VM disk"
	qemu-img create -f qcow2 "$disk" "$DISK_SIZE" >/dev/null

	msg "Installing OpenBSD ${RELEASE} ${ARCH}"
	python3 - "$disk" "$vm_dir/tftp" "$vm_dir/install.conf" "$install_log" "$CPUS" "$MEMORY" "$SSH_PORT" <<'PY'
import os
import pty
import re
import select
import signal
import subprocess
import sys
import time

disk, tftp_dir, conf_path, log_path, cpus, memory, ssh_port = sys.argv[1:]

cmd = [
    "qemu-system-x86_64",
    "-enable-kvm",
    "-m", memory,
    "-smp", cpus,
    "-drive", f"file={disk},if=virtio,format=qcow2",
    "-netdev",
    f"user,id=net0,hostname=fbc-openbsd,tftp={tftp_dir},bootfile=auto_install,hostfwd=tcp:127.0.0.1:{ssh_port}-:22",
    "-device", "e1000,netdev=net0",
    "-boot", "n",
    "-nographic",
]

install_conf = open(conf_path, "rb").read().rstrip(b"\n")
master_fd, slave_fd = pty.openpty()

log = open(log_path, "wb")
proc = subprocess.Popen(
    cmd,
    stdin=slave_fd,
    stdout=slave_fd,
    stderr=slave_fd,
    close_fds=True,
)
os.close(slave_fd)

buffer = b""

def cleanup():
    try:
        os.close(master_fd)
    except OSError:
        pass
    log.close()
    if proc.poll() is None:
        proc.terminate()
        try:
            proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait(timeout=10)

def read_until(pattern, timeout, reject=()):
    global buffer
    deadline = time.monotonic() + timeout
    compiled = re.compile(pattern, re.S)
    rejects = [re.compile(r, re.S) for r in reject]

    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise SystemExit(f"qemu exited during install with status {proc.returncode}")

        ready, _, _ = select.select([master_fd], [], [], 1)
        if not ready:
            continue

        data = os.read(master_fd, 4096)
        if not data:
            continue

        log.write(data)
        log.flush()
        buffer += data
        buffer = buffer[-262144:]

        for r in rejects:
            if r.search(buffer):
                raise SystemExit("OpenBSD installer hit an unanswered prompt")

        if compiled.search(buffer):
            return

    raise SystemExit(f"timed out waiting for {pattern!r}")

def send(data):
    if isinstance(data, str):
        data = data.encode()
    os.write(master_fd, data)

try:
    read_until(br"\(I\)nstall, \(U\)pgrade, \(A\)utoinstall or \(S\)hell\?", 300)
    send("S\n")
    read_until(br"# ", 120)

    send(b"cat > /tmp/install.conf <<'__FREEBASIC_OPENBSD_INSTALL__'\n")
    send(install_conf + b"\n")
    send(b"__FREEBASIC_OPENBSD_INSTALL__\n")
    send(b"/install -af /tmp/install.conf\n")

    read_until(
        br"CONGRATULATIONS! Your OpenBSD install has been successfully completed!",
        3600,
        reject=(br"Question has no answer in response file", br"non-interactive mode aborted"),
    )
    send("halt -p\n")
    read_until(br"The operating system has halted|Please press any key to reboot", 180)
finally:
    cleanup()
PY

	[ -f "$disk" ] || die "OpenBSD base disk was not created"
}

start_vm() {
	local name="$1"
	local port="$2"
	local vm_dir="$RUN_DIR/$name"
	local disk="$vm_dir/openbsd.qcow2"
	local base_disk="$RUN_DIR/base/openbsd-base.qcow2"
	local audio_wav="$vm_dir/freebasic-openbsd-audio.wav"

	rm -rf "$vm_dir"
	mkdir -p "$vm_dir"

	qemu-img create -f qcow2 -F qcow2 -b "$base_disk" "$disk" >/dev/null

	qemu-system-x86_64 \
		-enable-kvm \
		-m "$MEMORY" \
		-smp "$CPUS" \
		-drive "file=$disk,if=virtio,format=qcow2" \
		-netdev "user,id=net0,hostname=fbc-openbsd,hostfwd=tcp:127.0.0.1:$port-:22" \
		-device e1000,netdev=net0 \
		-audiodev "wav,id=audio0,path=$audio_wav" \
		-device ES1370,audiodev=audio0 \
		-display none \
		-serial "file:$vm_dir/serial.log" \
		-pidfile "$vm_dir/qemu.pid" \
		-daemonize
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
}

ssh_opts() {
	local key="$1"
	local port="$2"

	printf '%s\n' \
		-i "$key" \
		-o BatchMode=yes \
		-o StrictHostKeyChecking=no \
		-o UserKnownHostsFile=/dev/null \
		-o ConnectTimeout=5 \
		-p "$port"
}

scp_opts() {
	local key="$1"
	local port="$2"

	printf '%s\n' \
		-i "$key" \
		-o BatchMode=yes \
		-o StrictHostKeyChecking=no \
		-o UserKnownHostsFile=/dev/null \
		-o ConnectTimeout=5 \
		-P "$port"
}

rsync_ssh() {
	local key="$1"
	local port="$2"

	printf 'ssh -i %s -o BatchMode=yes -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=5 -p %s' \
		"$key" "$port"
}

ssh_guest() {
	local key="$1"
	local port="$2"
	shift 2

	ssh $(ssh_opts "$key" "$port") "$GUEST_USER"@127.0.0.1 "$@"
}

scp_to_guest() {
	local key="$1"
	local port="$2"
	local source="$3"
	local target="$4"

	scp $(scp_opts "$key" "$port") "$source" "$GUEST_USER@127.0.0.1:$target"
}

scp_from_guest() {
	local key="$1"
	local port="$2"
	local source="$3"
	local target="$4"

	scp $(scp_opts "$key" "$port") "$GUEST_USER@127.0.0.1:$source" "$target"
}

wait_for_ssh() {
	local key="$1"
	local port="$2"
	local vm_dir="$3"
	local attempts="${4:-160}"

	msg "Waiting for OpenBSD SSH on localhost:$port"
	for _ in $(seq 1 "$attempts"); do
		if timeout 5 ssh $(ssh_opts "$key" "$port") "$GUEST_USER"@127.0.0.1 \
				'uname -a' > "$vm_dir/ssh-ready.out" 2> "$vm_dir/ssh-ready.err"; then
			return 0
		fi
		sleep 3
	done

	tail -n 160 "$vm_dir/serial.log" >&2 || true
	cat "$vm_dir/ssh-ready.err" >&2 || true
	die "timed out waiting for OpenBSD SSH"
}

prepare_vm() {
	local name="$1"
	local port="$2"
	local vm_dir="$RUN_DIR/$name"
	local key="$RUN_DIR/id_ed25519"

	start_vm "$name" "$port"
	wait_for_ssh "$key" "$port" "$vm_dir" >&2

	printf '%s\n' "$vm_dir"
}

install_guest_tools() {
	local key="$1"
	local port="$2"

	ssh_guest "$key" "$port" 'sh -s' <<EOF
set -eu

export PKG_PATH="$DEFAULT_PACKAGE_PATH"
pkg_add -I \
	bash \
	g++-11.2.0p19 \
	gcc-11.2.0p19 \
	git \
	gmake \
	libffi \
	rsync-3.4.1

mkdir -p /home/fbc/work
chown -R fbc:fbc /home/fbc/work
EOF
}

send_source_tree() {
	local key="$1"
	local port="$2"
	local target="$3"

	msg "Copying source tree to OpenBSD"
	ssh_guest "$key" "$port" "rm -rf '$target' && mkdir -p '$target'"

	rsync -a --delete \
		--exclude-from "$ROOT/mk/source-copy-excludes.rsync" \
		-e "$(rsync_ssh "$key" "$port")" \
		"$ROOT/" "$GUEST_USER@127.0.0.1:$target/"
}

send_tests_tree() {
	local key="$1"
	local port="$2"

	msg "Copying fbctests source to OpenBSD"
	ssh_guest "$key" "$port" "rm -rf /home/fbc/work/fbctests-source && mkdir -p /home/fbc/work/fbctests-source/tests /home/fbc/work/fbctests-source/inc"

	rsync -a --delete \
		--exclude='*.o' \
		--exclude='*.a' \
		--exclude='fbc-tests' \
		--exclude='unit-tests.inc' \
		--exclude='unit-tests-obj.lst' \
		--exclude='log-tests-*.inc' \
		--exclude='failed-log-tests-*.inc' \
		--exclude='log-tests-*.lst' \
		--exclude='log-tests-results-*.log' \
		-e "$(rsync_ssh "$key" "$port")" \
		"$ROOT/tests/" "$GUEST_USER@127.0.0.1:/home/fbc/work/fbctests-source/tests/"

	rsync -a --delete \
		-e "$(rsync_ssh "$key" "$port")" \
		"$ROOT/inc/" "$GUEST_USER@127.0.0.1:/home/fbc/work/fbctests-source/inc/"
}

build_package_in_vm() {
	local vm_dir="$1"
	local key="$RUN_DIR/id_ed25519"
	local port="$2"
	local source_dir="/home/fbc/work/freebasic-source"

	msg "Preparing OpenBSD build VM"
	install_guest_tools "$key" "$port"
	send_source_tree "$key" "$port" "$source_dir"

	msg "Building OpenBSD package"
	mkdir -p "$LOG_DIR"
	if ! ssh_guest "$key" "$port" "JOBS='$JOBS' SOURCE_DIR='$source_dir' bash -s" <<'EOF'
set -euo pipefail

mkdir -p /home/fbc/work/packages
cd "$SOURCE_DIR"
rm -f /home/fbc/work/freebasic-openbsd-build.log

env JOBS="$JOBS" OUT=/home/fbc/work/packages ./build_scripts/openbsd-build-freebasic.sh > /home/fbc/work/freebasic-openbsd-build.log 2>&1 &
pid=$!

while kill -0 "$pid" 2>/dev/null; do
	sleep 60
	if kill -0 "$pid" 2>/dev/null; then
		printf 'OpenBSD build still running: '
		tail -n 1 /home/fbc/work/freebasic-openbsd-build.log 2>/dev/null | tr '\000' ' ' | cut -c 1-160 || true
	fi
done

wait "$pid"
chmod a+r /home/fbc/work/freebasic-openbsd-build.log /home/fbc/work/packages/freebasic-*.tgz
EOF
	then
		rm -f "$LOG_DIR/freebasic-openbsd-build.log"
		scp_from_guest "$key" "$port" "/home/fbc/work/freebasic-openbsd-build.log" "$LOG_DIR/" || true
		tail -n 200 "$LOG_DIR/freebasic-openbsd-build.log" >&2 || true
		die "OpenBSD package build failed"
	fi

	rm -f "$LOG_DIR/freebasic-openbsd-build.log"
	scp_from_guest "$key" "$port" "/home/fbc/work/freebasic-openbsd-build.log" "$LOG_DIR/"

	local remote_pkg
	remote_pkg="$(ssh_guest "$key" "$port" "ls -1 /home/fbc/work/packages/freebasic-*.tgz | sort | tail -n 1")"
	[ -n "$remote_pkg" ] || die "OpenBSD package was not created"

	mkdir -p "$WORKROOT/packages"
	scp_from_guest "$key" "$port" "$remote_pkg" "$WORKROOT/packages/"

	local pkg
	pkg="$(find "$WORKROOT/packages" -maxdepth 1 -type f -name 'freebasic-*.tgz' | sort | tail -n 1)"
	[ -n "$pkg" ] && [ -f "$pkg" ] || die "OpenBSD package was not copied out"
	printf '%s\n' "$pkg" > "$vm_dir/package.path"
}

write_test_runner() {
	local path="$1"

	cat > "$path" <<'EOF'
#!/usr/local/bin/bash

set -euo pipefail

run() {
	echo "==> $*"
	"$@"
}

fail() {
	echo "ERROR: $*" >&2
	exit 1
}

fbctests_jobs() {
	case "${FBCTESTS_JOBS:-}" in
		''|*[!0-9]*|0) echo 1 ;;
		*) echo "$FBCTESTS_JOBS" ;;
	esac
}

start_xvfb() {
	if [ -n "${DISPLAY:-}" ]; then
		return 0
	fi

	Xvfb :99 -screen 0 800x600x24 > /tmp/freebasic-xvfb.log 2>&1 &
	echo $! > /tmp/freebasic-xvfb.pid
	export DISPLAY=:99
	sleep 2

	if ! kill -0 "$(cat /tmp/freebasic-xvfb.pid)" 2>/dev/null; then
		cat /tmp/freebasic-xvfb.log || true
		fail "Xvfb failed to start"
	fi
}

stop_xvfb() {
	if [ -f /tmp/freebasic-xvfb.pid ]; then
		kill "$(cat /tmp/freebasic-xvfb.pid)" 2>/dev/null || true
		rm -f /tmp/freebasic-xvfb.pid
	fi
}

run_gfx_smoke() {
	local out="$1"
	local err="$2"
	shift 2

	if timeout 30 "$@" > "$out" 2> "$err"; then
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
	local jobs

	jobs="$(fbctests_jobs)"
	cd /home/fbc/work/fbctests-source/tests

	echo "==> cleaning fbctests tree"
	run gmake clean FBC=fbc

	echo "==> checking installed compiler through fbctests"
	run gmake check FBC=fbc

	echo "==> running unit-tests with ${jobs} job(s)"
	run gmake -j "$jobs" unit-tests FBC=fbc UNITTEST_RUN_ARGS="${FBCTESTS_UNIT_ARGS:-}"

	echo "==> running log-tests with ${jobs} job(s)"
	run gmake -j "$jobs" log-tests FBC=fbc

	for failed_log in failed-fb.log failed-fblite.log failed-qb.log failed-deprecated.log; do
		[ -f "$failed_log" ] || fail "missing log-tests summary: $failed_log"
		if ! grep -qi 'None Found' "$failed_log"; then
			cat "$failed_log"
			awk -F: '/\.log:/ { print $1 }' "$failed_log" | sort -u | while read -r detail_log; do
				[ -f "$detail_log" ] || continue
				echo
				echo "==> failed detail: $detail_log"
				cat "$detail_log"
			done
			fail "log-tests reported failures in $failed_log"
		fi
	done

	echo "==> fbctests passed"
}

trap stop_xvfb EXIT

export PATH=/usr/local/bin:/usr/local/sbin:/usr/X11R6/bin:/bin:/sbin:/usr/bin:/usr/sbin:$PATH
export CC="${CC:-egcc}"
export CXX="${CXX:-eg++}"
export PKG_PATH="${PKG_PATH:-https://cdn.openbsd.org/pub/OpenBSD/7.8/packages/amd64}"
export SFXLIB_OPENBSD_SNDIO_DEVICE="${SFXLIB_OPENBSD_SNDIO_DEVICE:-rsnd/0}"

prepare_audio_device() {
	echo "==> preparing OpenBSD audio device"
	audioctl -a || true
	mixerctl -a || true
	case "$SFXLIB_OPENBSD_SNDIO_DEVICE" in
		rsnd/*)
			echo "using raw sndio device: $SFXLIB_OPENBSD_SNDIO_DEVICE"
			;;
		*)
			pkill sndiod >/dev/null 2>&1 || true
			sndiod >/tmp/freebasic-sndiod.log 2>&1 &
			echo $! > /tmp/freebasic-sndiod.pid
			sleep 1
			cat /tmp/freebasic-sndiod.log || true
			;;
	esac
}

echo "==> installing package test dependencies"
pkg_add -I \
	bash \
	g++-11.2.0p19 \
	gcc-11.2.0p19 \
	gmake \
	libffi \
	rsync-3.4.1

echo "==> reinstalling FreeBASIC package"
pkg_delete freebasic >/dev/null 2>&1 || true
run pkg_add -D unsigned /home/fbc/work/package/freebasic-*.tgz

echo "==> verifying fbc"
command -v fbc
fbc -version
echo "CC=$CC"
echo "CXX=$CXX"

mkdir -p /home/fbc/work/smoke

cat > /home/fbc/work/smoke/console.bas <<'FBEOF'
print "Hello world"
FBEOF

cat > /home/fbc/work/smoke/gfx-truecolor.bas <<'FBEOF'
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

cat > /home/fbc/work/smoke/gfx-screen-modes.bas <<'FBEOF'
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

cat > /home/fbc/work/smoke/sfx.bas <<'FBEOF'
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
play "L4 CDEFGAB"
sleep 1600, 1
print "sfx-end"
FBEOF

echo "==> compiling console smoke"
run fbc /home/fbc/work/smoke/console.bas -x /home/fbc/work/smoke/console

echo "==> running console smoke"
console_output="$(/home/fbc/work/smoke/console)"
echo "$console_output"
[ "$console_output" = "Hello world" ] || fail "unexpected console output"

start_xvfb

echo "==> compiling gfxlib truecolor smoke"
run fbc /home/fbc/work/smoke/gfx-truecolor.bas -x /home/fbc/work/smoke/gfx-truecolor

echo "==> compiling gfxlib SCREEN mode smoke"
run fbc -lang fblite -exx /home/fbc/work/smoke/gfx-screen-modes.bas -x /home/fbc/work/smoke/gfx-screen-modes

echo "==> running gfxlib truecolor smoke"
run_gfx_smoke /home/fbc/work/smoke/gfx-truecolor.out /home/fbc/work/smoke/gfx-truecolor.err /home/fbc/work/smoke/gfx-truecolor

echo "==> running gfxlib SCREEN mode smoke"
run_gfx_smoke /home/fbc/work/smoke/gfx-screen-modes.out /home/fbc/work/smoke/gfx-screen-modes.err /home/fbc/work/smoke/gfx-screen-modes

echo "==> compiling sfxlib smoke"
run fbc /home/fbc/work/smoke/sfx.bas -x /home/fbc/work/smoke/sfx

echo "==> compiling sfxlib showcase"
[ -f /usr/local/share/freebasic/examples/sfxlib/showcase.bas ] ||
	fail "sfxlib showcase example is not installed"
rm -rf /home/fbc/work/smoke/sfxlib-showcase
mkdir -p /home/fbc/work/smoke/sfxlib-showcase
cp -R /usr/local/share/freebasic/examples/sfxlib/. /home/fbc/work/smoke/sfxlib-showcase/
(
	cd /home/fbc/work/smoke/sfxlib-showcase
	run fbc showcase.bas -x /home/fbc/work/smoke/sfx-showcase
)
[ -x /home/fbc/work/smoke/sfx-showcase ] || fail "sfxlib showcase binary was not created"

prepare_audio_device

echo "==> running sfxlib real audio smoke"
SFXLIB_DRIVER="OpenBSD sndio" timeout 20 /home/fbc/work/smoke/sfx > /home/fbc/work/smoke/sfx.out 2> /home/fbc/work/smoke/sfx.err || {
	cat /home/fbc/work/smoke/sfx.out || true
	cat /home/fbc/work/smoke/sfx.err || true
	fail "sfx smoke failed"
}
cat /home/fbc/work/smoke/sfx.out || true
grep -qx 'sfx-start' /home/fbc/work/smoke/sfx.out || fail "sfx smoke did not start"
grep -qx 'sfx-end' /home/fbc/work/smoke/sfx.out || fail "sfx smoke did not finish"
grep -q '^sfx-driver=OpenBSD sndio' /home/fbc/work/smoke/sfx.out || fail "sfx smoke did not use OpenBSD sndio"
[ ! -s /home/fbc/work/smoke/sfx.err ] || {
	cat /home/fbc/work/smoke/sfx.err
	fail "sfx smoke wrote stderr"
}

run_fbctests

echo "==> TEST PASSED"
EOF

	chmod +x "$path"
}

test_package_in_vm() {
	local vm_dir="$1"
	local key="$RUN_DIR/id_ed25519"
	local port="$2"
	local pkg="$3"
	local runner="$vm_dir/test-freebasic-openbsd.sh"

	msg "Preparing OpenBSD package test"
	install_guest_tools "$key" "$port"
	ssh_guest "$key" "$port" "mkdir -p /home/fbc/work/package"
	scp_to_guest "$key" "$port" "$pkg" "/home/fbc/work/package/"
	send_tests_tree "$key" "$port"

	write_test_runner "$runner"
	scp_to_guest "$key" "$port" "$runner" "/home/fbc/work/test-freebasic-openbsd.sh"

	msg "Running OpenBSD package smoke tests and fbctests"
	if ! ssh_guest "$key" "$port" \
			"FBCTESTS_JOBS='$FBCTESTS_JOBS' FBCTESTS_UNIT_ARGS='$FBCTESTS_UNIT_ARGS' bash -s" <<'EOF'
set -euo pipefail

log=/home/fbc/work/freebasic-openbsd-test.log
rm -f "$log"

bash /home/fbc/work/test-freebasic-openbsd.sh > "$log" 2>&1 &
pid=$!

while kill -0 "$pid" 2>/dev/null; do
	sleep 60
	if kill -0 "$pid" 2>/dev/null; then
		printf 'OpenBSD tests still running: '
		tail -n 1 "$log" 2>/dev/null | tr '\000' ' ' | cut -c 1-160 || true
	fi
done

wait "$pid"
chmod a+r "$log"
EOF
	then
		rm -f "$LOG_DIR/freebasic-openbsd-test.log"
		scp_from_guest "$key" "$port" "/home/fbc/work/freebasic-openbsd-test.log" "$LOG_DIR/" || true
		tail -n 220 "$LOG_DIR/freebasic-openbsd-test.log" >&2 || true
		die "OpenBSD package smoke tests or fbctests failed"
	fi

	rm -f "$LOG_DIR/freebasic-openbsd-test.log"
	scp_from_guest "$key" "$port" "/home/fbc/work/freebasic-openbsd-test.log" "$LOG_DIR/"
}

verify_audio_capture() {
	local wav="$1"
	local log="$2"

	msg "Verifying OpenBSD QEMU audio capture"
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

archive_results() {
	local pkg="$1"
	local base

	mkdir -p "$ARCHIVE_DIR"
	cp -f "$pkg" "$ARCHIVE_DIR/"

	for log in "$LOG_DIR"/freebasic-openbsd-*.log; do
		[ -f "$log" ] || continue
		cp -f "$log" "$ARCHIVE_DIR/"
	done

	if [ -f "$RUN_DIR/test/freebasic-openbsd-audio.wav" ]; then
		cp -f "$RUN_DIR/test/freebasic-openbsd-audio.wav" "$ARCHIVE_DIR/"
	fi

	base="$(basename "$pkg")"
	(
		cd "$ARCHIVE_DIR"
		sha256sum "$base" > SHA256SUMS
	)
}

cleanup() {
	stop_vm "$RUN_DIR/build" || true
	stop_vm "$RUN_DIR/test" || true
}

trap cleanup EXIT

main() {
	check_host_tools
	resolve_package_file

	if [ -z "$SSH_PORT" ]; then SSH_PORT="$(find_free_port 12022)"; fi

	msg "OpenBSD VM ssh port: $SSH_PORT"

	download_installer_files

	rm -rf "$RUN_DIR"
	mkdir -p "$RUN_DIR" "$LOG_DIR"

	local build_dir test_dir pkg

	install_base_vm

	if [ "$TEST_ONLY" -eq 1 ]; then
		pkg="$PACKAGE_FILE"
	else
		build_dir="$(prepare_vm build "$SSH_PORT")"
		build_package_in_vm "$build_dir" "$SSH_PORT"
		pkg="$(cat "$build_dir/package.path")"
		[ -n "$pkg" ] && [ -f "$pkg" ] || die "OpenBSD package was not copied out"
		stop_vm "$build_dir"
	fi

	SSH_PORT="$(find_free_port 12022)"
	test_dir="$(prepare_vm test "$SSH_PORT")"
	test_package_in_vm "$test_dir" "$SSH_PORT" "$pkg"
	stop_vm "$test_dir"
	verify_audio_capture "$test_dir/freebasic-openbsd-audio.wav" "$LOG_DIR/freebasic-openbsd-audio.log"
	archive_results "$pkg"

	if [ "$KEEP_VMS" -eq 0 ]; then
		rm -rf "$RUN_DIR"
	fi

	msg "OpenBSD package build and fbctests completed"
	echo "Package: $ARCHIVE_DIR/$(basename "$pkg")"
	echo "Install log: $ARCHIVE_DIR/freebasic-openbsd-install.log"
	echo "Build log:   $ARCHIVE_DIR/freebasic-openbsd-build.log"
	echo "Test log:    $ARCHIVE_DIR/freebasic-openbsd-test.log"
	echo "Audio log:   $ARCHIVE_DIR/freebasic-openbsd-audio.log"
}

main "$@"

##############################################################################
# end of openbsd-vm-build-freebasic.sh
##############################################################################
