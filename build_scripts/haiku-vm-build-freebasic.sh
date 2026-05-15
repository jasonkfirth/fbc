#!/usr/bin/env bash

##############################################################################
# FreeBASIC Haiku VM package builder
##############################################################################
#
# Purpose:
#
#   Build and test the Haiku x86_64 FreeBASIC package from a Debian/Ubuntu
#   Linux host.
#
# Responsibilities:
#
#   * download or reuse an official Haiku x86_64 anyboot image
#   * patch the live image so it starts sshd on first boot
#   * build FreeBASIC inside a Haiku QEMU VM using haiku-build-freebasic.sh
#   * install the resulting .hpkg in a separate clean Haiku VM
#   * run console, gfxlib, sfxlib, and fbctests checks
#
# This script intentionally does NOT contain:
#
#   * non-x86_64 Haiku support
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
ARCHIVE_DIR="$ROOT/out/haiku/x86-64"

ARCH="x86_64"
IMAGE_URL=""
IMAGE_FILE=""
ISO_FILE=""
PACKAGE_FILE=""
TEST_ONLY=0
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"
CPUS="$JOBS"
MEMORY="4096"
WORK_DISK_SIZE="24G"
KEEP_VMS=0
SSH_PORT=""
HTTP_PORT=""
VNC_DISPLAY=""
FBCTESTS_JOBS="$JOBS"
FBCTESTS_UNIT_ARGS=""
BOOT_SCRIPT_BYTES=884

NIGHTLY_INDEX_URL="https://download.haiku-os.org/nightly-images/x86_64/"

msg() { printf '\n==> %s\n' "$*"; }
warn() { printf '\nWARNING: %s\n' "$*" >&2; }
die() { printf '\nERROR: %s\n' "$*" >&2; exit 1; }

usage() {
	cat <<EOF
Usage: ./build_scripts/haiku-vm-build-freebasic.sh [options]

Options:
  --image-url URL          Haiku x86_64 anyboot .zip or .iso URL.
  --image FILE             Existing Haiku x86_64 anyboot .zip or .iso.
  --package FILE           Existing .hpkg to test.
  --test-only              Test --package without rebuilding FreeBASIC.
  --workroot DIR           Work directory. Default: out/haiku-vm
  --archive-dir DIR        Final archive directory. Default: out/haiku/x86-64
  --jobs N                 Build jobs inside Haiku. Default: host CPU count
  --cpus N                 QEMU CPU count. Default: --jobs value
  --memory MB              QEMU memory in MB. Default: 4096
  --work-disk-size SIZE    Per-VM BFS work disk size. Default: 24G
  --ssh-port N             Host SSH forward port. Default: auto
  --http-port N            Host bootstrap HTTP port. Default: auto
  --vnc-display N          VNC display number. Default: auto
  --fbctests-jobs N        fbctests make jobs. Default: --jobs value
  --fbctests-unit-args S   Extra UNITTEST_RUN_ARGS for fbctests.
  --keep-vms               Do not delete VM run directories on success.
  -h, --help               Show this help.

The script builds x86_64 only.  A fresh Haiku VM is used for package testing.
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
			PACKAGE_DIR="$WORKROOT/packages"
			LOG_DIR="$WORKROOT/logs"
			shift 2
			;;
		--archive-dir) ARCHIVE_DIR="$2"; shift 2 ;;
		--jobs) JOBS="$2"; CPUS="$2"; FBCTESTS_JOBS="$2"; shift 2 ;;
		--cpus) CPUS="$2"; shift 2 ;;
		--memory) MEMORY="$2"; shift 2 ;;
		--work-disk-size) WORK_DISK_SIZE="$2"; shift 2 ;;
		--ssh-port) SSH_PORT="$2"; shift 2 ;;
		--http-port) HTTP_PORT="$2"; shift 2 ;;
		--vnc-display) VNC_DISPLAY="$2"; shift 2 ;;
		--fbctests-jobs) FBCTESTS_JOBS="$2"; shift 2 ;;
		--fbctests-unit-args) FBCTESTS_UNIT_ARGS="$2"; shift 2 ;;
		--keep-vms) KEEP_VMS=1; shift ;;
		-h|--help) usage; exit 0 ;;
		*) die "unknown option: $1" ;;
	esac
done

case "$ARCH" in
	x86_64) ;;
	*) die "only x86_64 is supported by this script" ;;
esac

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
	require_tool qemu-system-x86_64
	require_tool rsync
	require_tool ssh
	require_tool scp
	require_tool tar
	require_tool sha256sum

	if ! command -v 7z >/dev/null 2>&1 && ! command -v unzip >/dev/null 2>&1; then
		die "required tool not found: 7z or unzip"
	fi
}

latest_haiku_url() {
	curl -fsSL "$NIGHTLY_INDEX_URL" |
		sed -n 's/.*href="\([^"]*x86_64-anyboot\.zip\)".*/\1/p' |
		head -n 1
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
		msg "Resolving latest Haiku x86_64 nightly"
		IMAGE_URL="$(latest_haiku_url)"
		[ -n "$IMAGE_URL" ] || die "could not locate latest Haiku nightly image"
	fi

	local name
	name="$(basename "$IMAGE_URL")"
	IMAGE_FILE="$CACHE_DIR/$name"

	if [ ! -f "$IMAGE_FILE" ]; then
		msg "Downloading $IMAGE_URL"
		curl -fL --retry 3 --retry-delay 5 -o "$IMAGE_FILE" "$IMAGE_URL"
	else
		msg "Using cached image $IMAGE_FILE"
	fi

	if curl -fsSL "$IMAGE_URL.sha256" -o "$IMAGE_FILE.sha256" 2>/dev/null; then
		local expected actual
		expected="$(sed -n 's/^.*= //p' "$IMAGE_FILE.sha256" | head -n 1)"
		actual="$(sha256sum "$IMAGE_FILE" | awk '{print $1}')"
		[ "$expected" = "$actual" ] ||
			die "checksum mismatch for $IMAGE_FILE"
	fi
}

extract_iso() {
	local ext
	ext="${IMAGE_FILE##*.}"

	if [ "$ext" = "iso" ]; then
		ISO_FILE="$IMAGE_FILE"
		return 0
	fi

	local extract_dir="$CACHE_DIR/extracted"
	mkdir -p "$extract_dir"

	ISO_FILE="$(find "$extract_dir" -maxdepth 1 -type f -name '*x86_64*anyboot*.iso' | head -n 1)"
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

	ISO_FILE="$(find "$extract_dir" -maxdepth 1 -type f -name '*x86_64*anyboot*.iso' | head -n 1)"
	[ -n "$ISO_FILE" ] && [ -f "$ISO_FILE" ] ||
		die "could not find x86_64 anyboot ISO in $IMAGE_FILE"
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

	cp -f "$hpkg" "$ARCHIVE_DIR/"

	for log in "$LOG_DIR"/freebasic-haiku-*.log; do
		[ -f "$log" ] || continue
		cp -f "$log" "$ARCHIVE_DIR/"
	done

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

	cat > "$serve_dir/haiku-bootstrap.sh" <<EOF
#!/bin/sh
LOG="\$HOME/config/settings/boot/freebasic-bootstrap-inner.log"
(
export PATH=/boot/system/bin:/bin:\$PATH
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
) >> "\$LOG" 2>&1
EOF
	chmod +x "$serve_dir/haiku-bootstrap.sh"

	cat > "$boot_script" <<EOF
#!/bin/sh
LOG="\$HOME/config/settings/boot/freebasic-bootstrap.log"
URL="http://10.0.2.2:$http_port/haiku-bootstrap.sh"
(
export PATH=/boot/system/bin:/bin:\$PATH
for n in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40; do
    curl -fsS "\$URL" -o "\$HOME/config/settings/boot/freebasic-bootstrap.sh" && break
    sleep 2
done
chmod +x "\$HOME/config/settings/boot/freebasic-bootstrap.sh"
"\$HOME/config/settings/boot/freebasic-bootstrap.sh" &
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
	local boot_image="$vm_dir/haiku-boot.raw"
	local work_disk="$vm_dir/work.raw"

	qemu-system-x86_64 \
		-enable-kvm \
		-cpu host \
		-m "$MEMORY" \
		-smp "$CPUS" \
		-drive "file=$boot_image,format=raw,if=ide,index=0" \
		-drive "file=$work_disk,format=raw,if=ide,index=1" \
		-netdev "user,id=net0,hostfwd=tcp:127.0.0.1:$ssh_port-:22" \
		-device e1000,netdev=net0 \
		-vga std \
		-display "vnc=127.0.0.1:$vnc_display" \
		-serial "file:$vm_dir/serial.log" \
		-pidfile "$vm_dir/qemu.pid" \
		-daemonize
}

stop_vm() {
	local vm_dir="$1"

	if [ -f "$vm_dir/qemu.pid" ]; then
		kill "$(cat "$vm_dir/qemu.pid")" 2>/dev/null || true
		rm -f "$vm_dir/qemu.pid"
	fi

	if [ -f "$vm_dir/http.pid" ]; then
		kill "$(cat "$vm_dir/http.pid")" 2>/dev/null || true
		rm -f "$vm_dir/http.pid"
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

wait_for_ssh() {
	local key="$1"
	local port="$2"
	local vm_dir="$3"
	local attempts="${4:-120}"

	msg "Waiting for Haiku SSH on localhost:$port"
	for _ in $(seq 1 "$attempts"); do
		if timeout 4 ssh $(ssh_opts "$key" "$port") user@127.0.0.1 \
				'test -f ~/config/settings/boot/freebasic-ssh-ready' \
				> "$vm_dir/ssh-ready.out" 2> "$vm_dir/ssh-ready.err"; then
			return 0
		fi
		sleep 2
	done

	return 1
}

print_vm_failure_logs() {
	local vm_dir="$1"

	tail -n 80 "$vm_dir/serial.log" >&2 || true
	cat "$vm_dir/http.log" >&2 || true
	cat "$vm_dir/ssh-ready.err" >&2 || true
}

ssh_guest() {
	local key="$1"
	local port="$2"
	shift 2

	ssh $(ssh_opts "$key" "$port") user@127.0.0.1 "$@"
}

rsync_ssh() {
	local key="$1"
	local port="$2"

	printf 'ssh -i %s -o BatchMode=yes -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=5 -p %s' \
		"$key" "$port"
}

scp_to_guest() {
	local key="$1"
	local port="$2"
	local source="$3"
	local target="$4"

	scp $(scp_opts "$key" "$port") "$source" "user@127.0.0.1:$target"
}

scp_from_guest() {
	local key="$1"
	local port="$2"
	local source="$3"
	local target="$4"

	scp $(scp_opts "$key" "$port") "user@127.0.0.1:$source" "$target"
}

prepare_vm() {
	local name="$1"
	local vm_dir="$RUN_DIR/$name"
	local key="$vm_dir/id_ed25519"
	local ssh_port="$2"
	local http_port="$3"
	local vnc_display="$4"

	rm -rf "$vm_dir"
	mkdir -p "$vm_dir"

	make_ssh_key "$key"
	write_bootstrap_files "$vm_dir" "$http_port" "$key.pub"
	patch_boot_image "$vm_dir"
	create_blank_work_disk "$vm_dir"
	start_http_server "$vm_dir" "$http_port"
	start_vm "$vm_dir" "$ssh_port" "$vnc_display"
	if ! wait_for_ssh "$key" "$ssh_port" "$vm_dir" 45 >&2; then
		warn "first Haiku boot did not reach SSH; restarting the warmed image" >&2
		if [ -f "$vm_dir/qemu.pid" ]; then
			kill "$(cat "$vm_dir/qemu.pid")" 2>/dev/null || true
			rm -f "$vm_dir/qemu.pid"
		fi
		sleep 2
		mv "$vm_dir/serial.log" "$vm_dir/serial-first-boot.log" 2>/dev/null || true
		start_vm "$vm_dir" "$ssh_port" "$vnc_display"
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
export PATH=/boot/system/bin:/bin:$PATH

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
export PATH=/boot/system/bin:/bin:$PATH

pkgman_install_retry() {
	for n in 1 2 3 4 5; do
		pkgman install -y "$@" && return 0
		echo "pkgman install failed; retrying ($n/5)" >&2
		sleep 10
	done

	return 1
}

pkgman_install_retry \
	haiku_devel \
	make \
	gcc \
	binutils \
	pkgconf \
	rsync \
	libffi_devel \
	zstd \
	ncurses6 \
	ncurses6_devel

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

pkgman_install_retry() {
	for n in 1 2 3 4 5; do
		pkgman install -y "$@" && return 0
		echo "pkgman install failed; retrying ($n/5)" >&2
		sleep 10
	done

	return 1
}

pkgman_install_retry rsync
EOF
}

send_source_tree() {
	local key="$1"
	local port="$2"
	local target="$3"

	msg "Copying source tree to Haiku"
	ssh_guest "$key" "$port" "rm -rf '$target' && mkdir -p '$target'"

	rsync -a --delete \
		--exclude='.git/' \
		--exclude='out/' \
		--exclude='.build*/' \
		--exclude='package-root/' \
		--exclude='*.hpkg' \
		--exclude='*.deb' \
		--exclude='*.ddeb' \
		--exclude='*.rpm' \
		--exclude='*.txz' \
		--exclude='*.tar' \
		--exclude='*.tar.gz' \
		--exclude='*.tar.xz' \
		--exclude='*.zip' \
		--exclude='*.install_manifest' \
		--exclude='package-root.install_manifest' \
		--exclude='bin/fbc*' \
		--exclude='bootstrap/fbc*' \
		--exclude='*/obj/' \
		--exclude='*.o' \
		--exclude='*.a' \
		-e "$(rsync_ssh "$key" "$port")" \
		"$ROOT/" "user@127.0.0.1:$target/"
}

send_tests_tree() {
	local key="$1"
	local port="$2"

	msg "Copying fbctests source to Haiku"
	ssh_guest "$key" "$port" "rm -rf /Work/fbctests-source && mkdir -p /Work/fbctests-source/tests /Work/fbctests-source/inc"

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
		"$ROOT/tests/" "user@127.0.0.1:/Work/fbctests-source/tests/"

	rsync -a --delete \
		-e "$(rsync_ssh "$key" "$port")" \
		"$ROOT/inc/" "user@127.0.0.1:/Work/fbctests-source/inc/"
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

./build_scripts/haiku-build-freebasic.sh --noinstall > "$log" 2>&1 &
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
	remote_hpkg="$(ssh_guest "$key" "$port" "cd '$source_dir' && ls -1 freebasic-*.hpkg | sort | tail -n 1")"
	[ -n "$remote_hpkg" ] || die "Haiku package was not created"
	scp_from_guest "$key" "$port" "$source_dir/$remote_hpkg" "$PACKAGE_DIR/"

	local hpkg
	hpkg="$(find "$PACKAGE_DIR" -maxdepth 1 -type f -name 'freebasic-*.hpkg' | sort | tail -n 1)"
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
	for n in 1 2 3 4 5; do
		pkgman install -y "$@" && return 0
		echo "pkgman install failed; retrying ($n/5)" >&2
		sleep 10
	done

	return 1
}

fbctests_jobs() {
	case "${FBCTESTS_JOBS:-}" in
		''|*[!0-9]*|0) echo 1 ;;
		*) echo "$FBCTESTS_JOBS" ;;
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

	[ -d /Work/fbctests-source/tests ] || fail "tests source was not staged"
	[ -d /Work/fbctests-source/inc ] || fail "inc source was not staged"

	cd /Work/fbctests-source/tests

	echo "==> cleaning fbctests tree"
	run make clean FBC=fbc

	echo "==> checking installed compiler through fbctests"
	run make check FBC=fbc

	echo "==> running unit-tests with ${jobs} job(s)"
	run make -j "$jobs" unit-tests FBC=fbc UNITTEST_RUN_ARGS="${FBCTESTS_UNIT_ARGS:-}"

	echo "==> running log-tests with ${jobs} job(s)"
	run make -j "$jobs" log-tests FBC=fbc

	for failed_log in failed-fb.log failed-fblite.log failed-qb.log failed-deprecated.log; do
		[ -f "$failed_log" ] || fail "missing log-tests summary: $failed_log"
		if ! grep -qi 'None Found' "$failed_log"; then
			cat "$failed_log"
			fail "log-tests reported failures in $failed_log"
		fi
	done

	echo "==> fbctests passed"
}

export PATH=/boot/system/bin:/bin:$PATH
export FBGFX="${FBGFX:-}"
export FB_GFX_DRIVER="${FB_GFX_DRIVER:-}"
export SFXLIB_DRIVER="${SFXLIB_DRIVER:-null}"

echo "==> installing package test dependencies"
run pkgman_install \
	haiku_devel \
	make \
	gcc \
	binutils \
	pkgconf \
	rsync \
	libffi_devel \
	zstd \
	ncurses6 \
	ncurses6_devel

echo "==> installing FreeBASIC package"
pkgman uninstall -y freebasic >/dev/null 2>&1 || true
run pkgman_install /Work/package/freebasic-*.hpkg

echo "==> verifying fbc"
command -v fbc
fbc -version

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
play "ABCDEFG"
dim as long sfx_device = fb_sfxDeviceCurrent()
dim as const zstring ptr sfx_driver = fb_sfxDeviceInfoName(sfx_device)
if sfx_driver <> 0 then
	print "sfx-driver="; *sfx_driver
else
	print "sfx-driver=<none>"
end if
print "sfx-end"
FBEOF

echo "==> compiling console smoke"
run fbc /Work/smoke/console.bas -x /Work/smoke/console

echo "==> running console smoke"
console_output="$(/Work/smoke/console)"
echo "$console_output"
[ "$console_output" = "Hello world" ] || fail "unexpected console output"

echo "==> compiling gfxlib truecolor smoke"
run fbc /Work/smoke/gfx-truecolor.bas -x /Work/smoke/gfx-truecolor

echo "==> compiling gfxlib SCREEN mode smoke"
run fbc -lang fblite -exx /Work/smoke/gfx-screen-modes.bas -x /Work/smoke/gfx-screen-modes

echo "==> running gfxlib truecolor smoke"
run_gfx_smoke /Work/smoke/gfx-truecolor.out /Work/smoke/gfx-truecolor.err /Work/smoke/gfx-truecolor

echo "==> running gfxlib SCREEN mode smoke"
run_gfx_smoke /Work/smoke/gfx-screen-modes.out /Work/smoke/gfx-screen-modes.err /Work/smoke/gfx-screen-modes

echo "==> compiling sfxlib smoke"
run fbc /Work/smoke/sfx.bas -x /Work/smoke/sfx

echo "==> compiling sfxlib showcase"
[ -f /boot/system/data/freebasic/examples/sfxlib/showcase.bas ] ||
	fail "sfxlib showcase example is not installed"
rm -rf /Work/smoke/sfxlib-showcase
mkdir -p /Work/smoke/sfxlib-showcase
cp -R /boot/system/data/freebasic/examples/sfxlib/. /Work/smoke/sfxlib-showcase/
(
	cd /Work/smoke/sfxlib-showcase
	run fbc showcase.bas -x /Work/smoke/sfx-showcase
)
[ -x /Work/smoke/sfx-showcase ] || fail "sfxlib showcase binary was not created"

echo "==> running sfxlib smoke"
SFXLIB_DRIVER=null timeout 20 /Work/smoke/sfx > /Work/smoke/sfx.out 2> /Work/smoke/sfx.err || {
	cat /Work/smoke/sfx.out || true
	cat /Work/smoke/sfx.err || true
	fail "sfx smoke failed"
}
cat /Work/smoke/sfx.out || true
grep -qx 'sfx-start' /Work/smoke/sfx.out || fail "sfx smoke did not start"
grep -qx 'sfx-end' /Work/smoke/sfx.out || fail "sfx smoke did not finish"
[ ! -s /Work/smoke/sfx.err ] || {
	cat /Work/smoke/sfx.err
	fail "sfx smoke wrote stderr"
}

echo "==> sfxlib smoke passed"

run_fbctests

echo "==> TEST PASSED"
EOF

	chmod +x "$path"
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
	send_tests_tree "$key" "$port"

	write_test_runner "$runner"
	scp_to_guest "$key" "$port" "$runner" "/Work/test-freebasic-haiku.sh"

	msg "Running Haiku package smoke tests and fbctests"
	if ! ssh_guest "$key" "$port" \
			"FBCTESTS_JOBS='$FBCTESTS_JOBS' FBCTESTS_UNIT_ARGS='$FBCTESTS_UNIT_ARGS' /bin/sh -s" <<'EOF'
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
		tail -n 160 "$LOG_DIR/freebasic-haiku-test.log" >&2 || true
		die "Haiku package smoke tests or fbctests failed"
	fi

	rm -f "$LOG_DIR/freebasic-haiku-test.log"
	scp_from_guest "$key" "$port" "/Work/freebasic-haiku-test.log" "$LOG_DIR/"
}

cleanup() {
	stop_vm "$RUN_DIR/build" || true
	stop_vm "$RUN_DIR/test" || true
}

trap cleanup EXIT

main() {
	check_host_tools
	resolve_package_file

	if [ -z "$SSH_PORT" ]; then SSH_PORT="$(find_free_port 10022)"; fi
	if [ -z "$HTTP_PORT" ]; then HTTP_PORT="$(find_free_port 18080)"; fi
	if [ -z "$VNC_DISPLAY" ]; then VNC_DISPLAY="$(find_free_vnc_display)"; fi

	msg "Haiku VM ports: ssh=$SSH_PORT http=$HTTP_PORT vnc=127.0.0.1:$VNC_DISPLAY"

	download_image
	extract_iso

	rm -rf "$RUN_DIR"
	mkdir -p "$RUN_DIR" "$PACKAGE_DIR" "$LOG_DIR"

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

	test_dir="$(prepare_vm test "$SSH_PORT" "$HTTP_PORT" "$VNC_DISPLAY")"
	test_package_in_vm "$test_dir" "$SSH_PORT" "$hpkg"
	stop_vm "$test_dir"

	if [ "$KEEP_VMS" -eq 0 ]; then
		rm -rf "$RUN_DIR"
	fi

	archive_results "$hpkg"

	msg "Haiku package build and fbctests completed"
	echo "Package: $hpkg"
	echo "Archive: $ARCHIVE_DIR"
	echo "Build log: $LOG_DIR/freebasic-haiku-build.log"
	echo "Test log:  $LOG_DIR/freebasic-haiku-test.log"
}

main "$@"

##############################################################################
# end of haiku-vm-build-freebasic.sh
##############################################################################
