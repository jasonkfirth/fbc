#!/usr/bin/env bash

##############################################################################
# FreeBASIC FreeBSD VM package builder
##############################################################################
#
# Purpose:
#
#   Build and test the FreeBSD x86_64 FreeBASIC package from a Debian/Ubuntu
#   Linux host.
#
# Responsibilities:
#
#   * download or reuse an official FreeBSD amd64 BASIC-CLOUDINIT VM image
#   * inject a NoCloud seed ISO so the VM accepts an SSH key for root
#   * build the FreeBSD package with build_scripts/freebsd-build-freebasic.sh
#   * install the package and run console, gfxlib, sfxlib, and fbctests checks
#   * archive the package and logs under out/freebsd/x86-64
#
# This script intentionally does NOT contain:
#
#   * non-x86_64 FreeBSD support
#   * GUI installer automation
#   * cross-compilation into FreeBSD packages
#
##############################################################################

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORKROOT="$ROOT/out/freebsd-vm"
CACHE_DIR="$WORKROOT/cache"
RUN_DIR="$WORKROOT/run"
LOG_DIR="$WORKROOT/logs"
ARCHIVE_DIR="$ROOT/out/freebsd/x86-64"

RELEASE="15.1-RELEASE"
IMAGE_URL=""
IMAGE_FILE=""
PACKAGE_FILE=""
TEST_ONLY=0
GUEST_USER="freebsd"
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"
CPUS="$JOBS"
MEMORY="6144"
DISK_SIZE="32G"
SSH_PORT=""
VNC_DISPLAY=""
KEEP_VM=0
FBCTESTS_JOBS="$JOBS"
FBCTESTS_UNIT_ARGS=""
QEMU_ACCEL="${QEMU_ACCEL:-kvm}"
QEMU_CPU="${QEMU_CPU:-host}"
SSH_READY_ATTEMPTS="${FREEBSD_VM_SSH_ATTEMPTS:-600}"

DEFAULT_IMAGE_URL="https://download.freebsd.org/releases/VM-IMAGES/${RELEASE}/amd64/Latest/FreeBSD-${RELEASE}-amd64-BASIC-CLOUDINIT-ufs.qcow2.xz"

msg() { printf '\n==> %s\n' "$*"; }
warn() { printf '\nWARNING: %s\n' "$*" >&2; }
die() { printf '\nERROR: %s\n' "$*" >&2; exit 1; }

usage() {
	cat <<EOF
Usage: ./build_scripts/freebsd-vm-build-freebasic.sh [options]

Options:
  --image-url URL          FreeBSD amd64 BASIC-CLOUDINIT qcow2.xz URL.
  --image FILE             Existing FreeBSD qcow2 or qcow2.xz image.
  --package FILE           Existing FreeBSD package to test.
  --test-only              Test --package without rebuilding FreeBASIC.
  --workroot DIR           Work directory. Default: out/freebsd-vm
  --archive-dir DIR        Final archive directory. Default: out/freebsd/x86-64
  --jobs N                 Build jobs inside FreeBSD. Default: host CPU count
  --cpus N                 QEMU CPU count. Default: --jobs value
  --memory MB              QEMU memory in MB. Default: 6144
  --disk-size SIZE         Resized VM disk size. Default: 32G
  --ssh-port N             Host SSH forward port. Default: auto
  --vnc-display N          VNC display number. Default: auto
  --fbctests-jobs N        fbctests gmake jobs. Default: --jobs value
  --fbctests-unit-args S   Extra UNITTEST_RUN_ARGS for fbctests.
  --keep-vm                Do not delete VM run directory on success.
  -h, --help               Show this help.
EOF
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--image-url) IMAGE_URL="$2"; shift 2 ;;
		--image) IMAGE_FILE="$2"; shift 2 ;;
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
		--vnc-display) VNC_DISPLAY="$2"; shift 2 ;;
		--fbctests-jobs) FBCTESTS_JOBS="$2"; shift 2 ;;
		--fbctests-unit-args) FBCTESTS_UNIT_ARGS="$2"; shift 2 ;;
		--keep-vm) KEEP_VM=1; shift ;;
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

find_free_vnc_display() {
	python3 - <<'PY'
import socket

for display in range(21, 99):
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
	require_tool qemu-img
	require_tool qemu-system-x86_64
	require_tool rsync
	require_tool scp
	require_tool ssh
	require_tool xorriso
	require_tool xz
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
	mkdir -p "$CACHE_DIR" "$LOG_DIR"

	if [ -n "$IMAGE_FILE" ]; then
		[ -f "$IMAGE_FILE" ] || die "image not found: $IMAGE_FILE"
		IMAGE_FILE="$(cd "$(dirname "$IMAGE_FILE")" && pwd)/$(basename "$IMAGE_FILE")"
		msg "Using FreeBSD image $IMAGE_FILE"
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

make_ssh_key() {
	local key="$1"

	if [ ! -f "$key" ]; then
		ssh-keygen -q -t ed25519 -N '' -f "$key"
	fi
}

prepare_disk() {
	local vm_dir="$1"
	local disk="$vm_dir/freebsd.qcow2"

	rm -f "$disk"

	case "$IMAGE_FILE" in
		*.xz)
			msg "Decompressing FreeBSD image"
			xz -dc "$IMAGE_FILE" > "$disk"
			;;
		*)
			cp --sparse=always "$IMAGE_FILE" "$disk"
			;;
	esac

	qemu-img resize "$disk" "$DISK_SIZE" >/dev/null
}

write_seed_iso() {
	local vm_dir="$1"
	local key_pub="$2"
	local seed_dir="$vm_dir/seed"
	local seed_iso="$vm_dir/seed.iso"
	local authorized_key

	authorized_key="$(cat "$key_pub")"
	rm -rf "$seed_dir"
	mkdir -p "$seed_dir"

	cat > "$seed_dir/meta-data" <<EOF
instance-id: freebasic-freebsd-${RELEASE}
local-hostname: freebasic-freebsd
EOF

cat > "$seed_dir/user-data" <<EOF
#cloud-config
hostname: freebasic-freebsd
fqdn: freebasic-freebsd.local
disable_root: false
ssh_pwauth: false
users:
  - name: freebsd
    shell: /bin/sh
    groups: wheel
    sudo: ALL=(ALL) NOPASSWD:ALL
    lock_passwd: false
    ssh_authorized_keys:
      - $authorized_key
  - name: root
    ssh_authorized_keys:
      - $authorized_key
packages:
  - sudo
network:
  ethernets:
    vtnet0:
      dhcp4: true
write_files:
  - path: /etc/rc.conf.d/sshd
    content: |
      sshd_enable="YES"
runcmd:
  - mkdir -p /etc/ssh/sshd_config.d
  - printf 'PermitRootLogin prohibit-password\nPubkeyAuthentication yes\n' > /etc/ssh/sshd_config.d/99-freebasic-root.conf
  - mkdir -p /work
  - chown freebsd:freebsd /work
  - service sshd restart || service sshd onestart
EOF

	xorriso -as mkisofs -quiet -output "$seed_iso" -volid cidata -joliet -rock "$seed_dir" >/dev/null
}

start_vm() {
	local vm_dir="$1"
	local ssh_port="$2"
	local vnc_display="$3"
	local disk="$vm_dir/freebsd.qcow2"
	local seed_iso="$vm_dir/seed.iso"
	local audio_wav="$vm_dir/freebasic-freebsd-audio.wav"

	qemu-system-x86_64 \
		-accel "$QEMU_ACCEL" \
		-cpu "$QEMU_CPU" \
		-m "$MEMORY" \
		-smp "$CPUS" \
		-drive "file=$disk,format=qcow2,if=virtio,index=0" \
		-drive "file=$seed_iso,format=raw,media=cdrom,readonly=on" \
		-netdev "user,id=net0,hostfwd=tcp:127.0.0.1:$ssh_port-:22" \
		-device virtio-net-pci,netdev=net0 \
		-audiodev "wav,id=audio0,path=$audio_wav" \
		-device intel-hda \
		-device hda-output,audiodev=audio0 \
		-vga std \
		-display "vnc=127.0.0.1:$vnc_display" \
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

	# The remaining words are the caller's intended guest command.
	# shellcheck disable=SC2029
	ssh -i "$key" \
		-o BatchMode=yes \
		-o StrictHostKeyChecking=no \
		-o UserKnownHostsFile=/dev/null \
		-o ConnectTimeout=5 \
		-p "$port" \
		"$GUEST_USER"@127.0.0.1 "$@"
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
		"$source" "$GUEST_USER@127.0.0.1:$target"
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
		"$GUEST_USER@127.0.0.1:$source" "$target"
}

wait_for_ssh() {
	local key="$1"
	local port="$2"
	local vm_dir="$3"
	local attempts="${4:-$SSH_READY_ATTEMPTS}"

	msg "Waiting for FreeBSD SSH on localhost:$port"
	for _ in $(seq 1 "$attempts"); do
		if timeout 5 ssh -i "$key" \
				-o BatchMode=yes \
				-o StrictHostKeyChecking=no \
				-o UserKnownHostsFile=/dev/null \
				-o ConnectTimeout=5 \
				-p "$port" \
				"$GUEST_USER"@127.0.0.1 \
				'echo freebsd-ready' \
				> "$vm_dir/ssh-ready.out" 2> "$vm_dir/ssh-ready.err"; then
			return 0
		fi
		sleep 3
	done

	tail -n 120 "$vm_dir/serial.log" >&2 || true
	cat "$vm_dir/ssh-ready.err" >&2 || true
	die "timed out waiting for FreeBSD SSH"
}

wait_for_guest_ready() {
	local key="$1"
	local port="$2"
	local attempt=1
	local stable_checks=0
	local max_attempts="${3:-$SSH_READY_ATTEMPTS}"

	msg "Waiting for FreeBSD first-boot update to finish"

	while [ "$attempt" -le "$max_attempts" ]; do
		if ssh_guest "$key" "$port" "pgrep -f '[f]reebsd-update' >/dev/null" >/dev/null 2>&1; then
			stable_checks=0
		elif ssh_guest "$key" "$port" true >/dev/null 2>&1; then
			stable_checks=$((stable_checks + 1))
			if [ "$stable_checks" -ge 6 ]; then
				return 0
			fi
		else
			stable_checks=0
		fi

		sleep 10
		attempt=$((attempt + 1))
	done

	die "FreeBSD did not reach a stable first-boot state"
}

prepare_vm() {
	local vm_dir="$RUN_DIR/freebsd"
	local key="$vm_dir/id_ed25519"

	rm -rf "$vm_dir"
	mkdir -p "$vm_dir"

	make_ssh_key "$key"
	prepare_disk "$vm_dir" >&2
	write_seed_iso "$vm_dir" "$key.pub" >&2
	start_vm "$vm_dir" "$SSH_PORT" "$VNC_DISPLAY"
	wait_for_ssh "$key" "$SSH_PORT" "$vm_dir" >&2
	wait_for_guest_ready "$key" "$SSH_PORT" >&2

	printf '%s\n' "$vm_dir"
}

install_guest_tools() {
	local key="$1"
	local port="$2"

	ssh_guest "$key" "$port" 'su -m root -c /bin/sh' <<'EOF'
set -eu

export ASSUME_ALWAYS_YES=yes
env ASSUME_ALWAYS_YES=yes pkg bootstrap || true
env IGNORE_OSVERSION=yes pkg update -f
env IGNORE_OSVERSION=yes pkg install -y \
	bash \
	binutils \
	gcc \
	gmake \
	libffi \
	libglvnd \
	libX11 \
	libXau \
	libXcursor \
	libXdmcp \
	libXext \
	libXi \
	libXinerama \
	libXpm \
	libXrandr \
	libXrender \
	libXxf86vm \
	libxcb \
	mesa-libs \
	ncurses \
	pkgconf \
	python312 \
	rsync \
	terminfo-db \
	xauth \
	xorg-vfbserver
mkdir -p /work
chown freebsd:freebsd /work
EOF
}

send_source_tree() {
	local key="$1"
	local port="$2"
	local target="$3"

	case "$target" in
		/work/*) ;;
		*) die "refusing to replace source directory outside /work: $target" ;;
	esac

	msg "Copying source tree to FreeBSD"
	ssh_guest "$key" "$port" \
		"su -m root -c 'rm -rf \"$target\" && mkdir -p \"$target\" && chown freebsd:freebsd \"$target\"'"

	rsync -a --delete \
		--exclude-from "$ROOT/mk/source-copy-excludes.rsync" \
		-e "$(rsync_ssh "$key" "$port")" \
		"$ROOT/" "$GUEST_USER@127.0.0.1:$target/"
}

send_tests_tree() {
	local key="$1"
	local port="$2"

	msg "Copying fbctests source to FreeBSD"
	ssh_guest "$key" "$port" \
		"su -m root -c 'rm -rf /work/fbctests-source && mkdir -p /work/fbctests-source/tests /work/fbctests-source/inc /work/fbctests-source/src/sfxlib && chown -R freebsd:freebsd /work/fbctests-source'"

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
		"$ROOT/tests/" "$GUEST_USER@127.0.0.1:/work/fbctests-source/tests/"

	rsync -a --delete \
		-e "$(rsync_ssh "$key" "$port")" \
		"$ROOT/inc/" "$GUEST_USER@127.0.0.1:/work/fbctests-source/inc/"

	rsync -a --delete \
		--exclude='obj' \
		--exclude='*.o' \
		--exclude='*.a' \
		-e "$(rsync_ssh "$key" "$port")" \
		"$ROOT/src/sfxlib/" "$GUEST_USER@127.0.0.1:/work/fbctests-source/src/sfxlib/"
}

build_package_in_vm() {
	local vm_dir="$1"
	local key="$vm_dir/id_ed25519"
	local port="$2"
	local source_dir="/work/freebasic-source"

	msg "Preparing FreeBSD build VM"
	install_guest_tools "$key" "$port"
	ssh_guest "$key" "$port" "mkdir -p /work"
	send_source_tree "$key" "$port" "$source_dir"

	msg "Building FreeBSD package"
	mkdir -p "$LOG_DIR"
	if ! ssh_guest "$key" "$port" "su -m root -c '/usr/local/bin/bash -s'" <<EOF
set -euo pipefail

mkdir -p /work/packages
cd "$source_dir"
rm -f /work/freebasic-freebsd-build.log

env JOBS="$JOBS" OUT=/work/packages bash ./build_scripts/freebsd-build-freebasic.sh > /work/freebasic-freebsd-build.log 2>&1 &
pid=\$!

while kill -0 "\$pid" 2>/dev/null; do
	sleep 60
	if kill -0 "\$pid" 2>/dev/null; then
		printf 'FreeBSD build still running: '
		tail -n 1 /work/freebasic-freebsd-build.log 2>/dev/null | tr '\000' ' ' | cut -c 1-160 || true
	fi
done

wait "\$pid"
chmod a+r /work/freebasic-freebsd-build.log /work/packages/freebasic-*.pkg
chown -R freebsd:freebsd /work/packages /work/freebasic-freebsd-build.log
EOF
	then
		rm -f "$LOG_DIR/freebasic-freebsd-build.log"
		scp_from_guest "$key" "$port" "/work/freebasic-freebsd-build.log" "$LOG_DIR/" || true
		tail -n 160 "$LOG_DIR/freebasic-freebsd-build.log" >&2 || true
		die "FreeBSD package build failed"
	fi

	rm -f "$LOG_DIR/freebasic-freebsd-build.log"
	scp_from_guest "$key" "$port" "/work/freebasic-freebsd-build.log" "$LOG_DIR/"

	local remote_pkg
	remote_pkg="$(ssh_guest "$key" "$port" "ls -1 /work/packages/freebasic-*.pkg | sort | tail -n 1")"
	[ -n "$remote_pkg" ] || die "FreeBSD package was not created"

	mkdir -p "$WORKROOT/packages"
	scp_from_guest "$key" "$port" "$remote_pkg" "$WORKROOT/packages/"

	local pkg
	pkg="$(find "$WORKROOT/packages" -maxdepth 1 -type f -name 'freebasic-*.pkg' | sort | tail -n 1)"
	[ -n "$pkg" ] && [ -f "$pkg" ] || die "FreeBSD package was not copied out"
	printf '%s\n' "$pkg" > "$vm_dir/package.path"
}

write_test_runner() {
	local path="$1"

	cat > "$path" <<'EOF'
#!/usr/bin/env bash

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

resolve_gnu_cxx() {
	local cxx

	for cxx in g++ g++14 g++13 g++12; do
		if command -v "$cxx" >/dev/null 2>&1; then
			command -v "$cxx"
			return 0
		fi
	done

	command -v c++
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
	cd /work/fbctests-source/tests

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

run_exampleageddon() {
	local python

	python="/usr/local/bin/python3.12"
	[ -x "$python" ] || fail "python312 is required for exampleageddon"

	echo "==> running exampleageddon"
	rm -rf /work/exampleageddon
	run "$python" /work/freebasic-source/build_scripts/exampleageddon-freebasic.py \
		--root /work/freebasic-source \
		--outdir /work/exampleageddon \
		--prefix /usr/local \
		--include-dir /work/freebasic-source/inc \
		--fbc fbc \
		--jobs 1 \
		--compile-timeout 180 \
		--run-timeout 10 \
		--fail-on-self-contained

	[ -f /work/exampleageddon/report.md ] || fail "exampleageddon report was not created"
	[ -f /work/exampleageddon/results.csv ] || fail "exampleageddon results CSV was not created"
	grep -qx -- '- Self-contained problems: 0' /work/exampleageddon/report.md || {
		cat /work/exampleageddon/report.md
		fail "exampleageddon reported self-contained example problems"
	}

	echo "==> exampleageddon passed"
}

trap stop_xvfb EXIT

export PATH=/usr/local/bin:/usr/local/sbin:/bin:/sbin:/usr/bin:/usr/sbin:$PATH
export CXX="${CXX:-$(resolve_gnu_cxx)}"

prepare_audio_device() {
	echo "==> preparing FreeBSD audio device"
	su -m root -c 'kldload snd_hda >/dev/null 2>&1 || kldload snd_driver >/dev/null 2>&1 || true'
	su -m root -c 'sysctl hw.snd.default_unit=0 >/dev/null 2>&1 || true'
	cat /dev/sndstat || true
	ls -l /dev/dsp* || true
}

echo "==> installing package test dependencies"
export ASSUME_ALWAYS_YES=yes
su -m root -c 'env ASSUME_ALWAYS_YES=yes IGNORE_OSVERSION=yes pkg install -y \
	bash \
	binutils \
	gcc \
	gmake \
	libffi \
	libglvnd \
	libX11 \
	libXau \
	libXcursor \
	libXdmcp \
	libXext \
	libXi \
	libXinerama \
	libXpm \
	libXrandr \
	libXrender \
	libXxf86vm \
	libxcb \
	mesa-libs \
	ncurses \
	pkgconf \
	rsync \
	terminfo-db \
	xauth \
	xorg-vfbserver'

echo "==> reinstalling FreeBASIC package"
su -m root -c 'pkg delete -y freebasic >/dev/null 2>&1 || true'
run su -m root -c 'env ASSUME_ALWAYS_YES=yes pkg install -y /work/package/freebasic-*.pkg'

echo "==> verifying fbc"
command -v fbc
fbc -version
echo "CXX=$CXX"

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

cat > /work/smoke/gfx-screen-modes.bas <<'FBEOF'
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
run fbc /work/smoke/console.bas -x /work/smoke/console

echo "==> running console smoke"
console_output="$(/work/smoke/console)"
echo "$console_output"
[ "$console_output" = "Hello world" ] || fail "unexpected console output"

echo "==> compiling crt/sys/socket.bi API smoke"
run fbc /work/fbctests-source/tests/crt/socket.bas -x /work/smoke/socket-bi

echo "==> running crt/sys/socket.bi API smoke"
run /work/smoke/socket-bi

echo "==> compiling curses.bi API smoke"
run fbc /work/fbctests-source/tests/crt/curses.bas -x /work/smoke/curses-bi

echo "==> running curses.bi API smoke"
run /work/smoke/curses-bi

echo "==> compiling TCP loopback smoke"
run fbc -mt /work/fbctests-source/tests/file/tcp.bas -x /work/smoke/tcp

echo "==> running TCP loopback smoke"
timeout 60 /work/smoke/tcp

start_xvfb

echo "==> compiling gfxlib truecolor smoke"
run fbc /work/smoke/gfx-truecolor.bas -x /work/smoke/gfx-truecolor

echo "==> compiling gfxlib SCREEN mode smoke"
run fbc -lang fblite -exx /work/smoke/gfx-screen-modes.bas -x /work/smoke/gfx-screen-modes

echo "==> running gfxlib truecolor smoke"
run_gfx_smoke /work/smoke/gfx-truecolor.out /work/smoke/gfx-truecolor.err /work/smoke/gfx-truecolor

echo "==> running gfxlib SCREEN mode smoke"
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

prepare_audio_device

echo "==> running sfxlib real audio smoke"
SFXLIB_DRIVER="FreeBSD OSS" timeout 20 /work/smoke/sfx > /work/smoke/sfx.out 2> /work/smoke/sfx.err || {
	cat /work/smoke/sfx.out || true
	cat /work/smoke/sfx.err || true
	fail "sfx smoke failed"
}
cat /work/smoke/sfx.out || true
grep -qx 'sfx-start' /work/smoke/sfx.out || fail "sfx smoke did not start"
grep -qx 'sfx-end' /work/smoke/sfx.out || fail "sfx smoke did not finish"
grep -qi '^sfx-driver=freebsd oss' /work/smoke/sfx.out || fail "sfx smoke did not use FreeBSD OSS"
[ ! -s /work/smoke/sfx.err ] || {
	cat /work/smoke/sfx.err
	fail "sfx smoke wrote stderr"
}

run_fbctests
run_exampleageddon

echo "==> TEST PASSED"
EOF

	chmod +x "$path"
}

test_package_in_vm() {
	local vm_dir="$1"
	local key="$vm_dir/id_ed25519"
	local port="$2"
	local pkg="$3"
	local runner="$vm_dir/test-freebasic-freebsd.sh"

	msg "Preparing FreeBSD package test"
	install_guest_tools "$key" "$port"
	ssh_guest "$key" "$port" "mkdir -p /work/package"
	scp_to_guest "$key" "$port" "$pkg" "/work/package/"
	send_source_tree "$key" "$port" "/work/freebasic-source"
	send_tests_tree "$key" "$port"

	write_test_runner "$runner"
	scp_to_guest "$key" "$port" "$runner" "/work/test-freebasic-freebsd.sh"

	msg "Running FreeBSD package smoke tests and fbctests"
	if ! ssh_guest "$key" "$port" \
			"FBCTESTS_JOBS='$FBCTESTS_JOBS' FBCTESTS_UNIT_ARGS='$FBCTESTS_UNIT_ARGS' /usr/local/bin/bash -s" <<'EOF'
set -euo pipefail

log=/work/freebasic-freebsd-test.log
rm -f "$log"

/usr/local/bin/bash /work/test-freebasic-freebsd.sh > "$log" 2>&1 &
pid=$!

while kill -0 "$pid" 2>/dev/null; do
	sleep 60
	if kill -0 "$pid" 2>/dev/null; then
		printf 'FreeBSD tests still running: '
		tail -n 1 "$log" 2>/dev/null | tr '\000' ' ' | cut -c 1-160 || true
	fi
done

wait "$pid"
EOF
	then
		rm -f "$LOG_DIR/freebasic-freebsd-test.log"
		scp_from_guest "$key" "$port" "/work/freebasic-freebsd-test.log" "$LOG_DIR/" || true
		tail -n 200 "$LOG_DIR/freebasic-freebsd-test.log" >&2 || true
		die "FreeBSD package smoke tests or fbctests failed"
	fi

	rm -f "$LOG_DIR/freebasic-freebsd-test.log"
	scp_from_guest "$key" "$port" "/work/freebasic-freebsd-test.log" "$LOG_DIR/"
	scp_from_guest "$key" "$port" "/work/exampleageddon/report.md" "$vm_dir/exampleageddon-report.md"
	scp_from_guest "$key" "$port" "/work/exampleageddon/results.csv" "$vm_dir/exampleageddon-results.csv"
}

verify_audio_capture() {
	local wav="$1"
	local log="$2"

	msg "Verifying FreeBSD QEMU audio capture"
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

	for log in "$LOG_DIR"/freebasic-freebsd-*.log; do
		[ -f "$log" ] || continue
		cp -f "$log" "$ARCHIVE_DIR/"
	done

	if [ -f "$RUN_DIR/freebsd/freebasic-freebsd-audio.wav" ]; then
		cp -f "$RUN_DIR/freebsd/freebasic-freebsd-audio.wav" "$ARCHIVE_DIR/"
	fi
	if [ -f "$RUN_DIR/freebsd/exampleageddon-report.md" ]; then
		cp -f "$RUN_DIR/freebsd/exampleageddon-report.md" "$ARCHIVE_DIR/"
	fi
	if [ -f "$RUN_DIR/freebsd/exampleageddon-results.csv" ]; then
		cp -f "$RUN_DIR/freebsd/exampleageddon-results.csv" "$ARCHIVE_DIR/"
	fi

	base="$(basename "$pkg")"
	(
		cd "$ARCHIVE_DIR"
		sha256sum "$base" > SHA256SUMS
	)
}

cleanup() {
	stop_vm "$RUN_DIR/freebsd" || true
}

trap cleanup EXIT

main() {
	check_host_tools
	configure_qemu_acceleration
	resolve_package_file

	if [ -z "$SSH_PORT" ]; then SSH_PORT="$(find_free_port 11022)"; fi
	if [ -z "$VNC_DISPLAY" ]; then VNC_DISPLAY="$(find_free_vnc_display)"; fi

	msg "FreeBSD VM ports: ssh=$SSH_PORT vnc=127.0.0.1:$VNC_DISPLAY"

	download_image

	rm -rf "$RUN_DIR"
	mkdir -p "$RUN_DIR" "$LOG_DIR"

	local vm_dir pkg

	vm_dir="$(prepare_vm)"

	if [ "$TEST_ONLY" -eq 1 ]; then
		pkg="$PACKAGE_FILE"
	else
		build_package_in_vm "$vm_dir" "$SSH_PORT"
		pkg="$(cat "$vm_dir/package.path")"
		[ -n "$pkg" ] && [ -f "$pkg" ] || die "FreeBSD package was not copied out"
	fi

	test_package_in_vm "$vm_dir" "$SSH_PORT" "$pkg"
	stop_vm "$vm_dir"
	verify_audio_capture "$vm_dir/freebasic-freebsd-audio.wav" "$LOG_DIR/freebasic-freebsd-audio.log"
	archive_results "$pkg"

	if [ "$KEEP_VM" -eq 0 ]; then
		rm -rf "$RUN_DIR"
	fi

	msg "FreeBSD package build and fbctests completed"
	echo "Package: $pkg"
	echo "Archive: $ARCHIVE_DIR"
	echo "Build log: $LOG_DIR/freebasic-freebsd-build.log"
	echo "Test log:  $LOG_DIR/freebasic-freebsd-test.log"
	echo "Audio log: $LOG_DIR/freebasic-freebsd-audio.log"
}

main "$@"

##############################################################################
# end of freebsd-vm-build-freebasic.sh
##############################################################################
