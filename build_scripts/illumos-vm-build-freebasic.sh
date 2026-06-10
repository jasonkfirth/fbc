#!/usr/bin/env bash

##############################################################################
# FreeBASIC illumos VM package builder
##############################################################################
#
# Purpose:
#
#   Build and test the illumos x86_64 FreeBASIC package from a Linux host.
#
# Responsibilities:
#
#   * download or reuse an OpenIndiana x86_64 cloud image
#   * inject a NoCloud seed ISO so the VM accepts an SSH key for root
#   * build the package with build_scripts/illumos-build-freebasic.sh
#   * copy the generated IPS repository and logs under out/illumos/x86-64
#
# This script intentionally does NOT contain:
#
#   * non-x86_64 illumos support
#   * GUI installer automation
#   * cross-compilation into illumos packages
#
##############################################################################

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORKROOT="$ROOT/out/illumos-vm"
CACHE_DIR="$WORKROOT/cache"
RUN_DIR="$WORKROOT/run"
LOG_DIR="$WORKROOT/logs"
ARCHIVE_DIR="$ROOT/out/illumos/x86-64"
AUDIO_WAV="$LOG_DIR/freebasic-illumos-audio.wav"
SERIAL_BOOT_LOG="$LOG_DIR/serial-boot.log"

RELEASE="20251026"
IMAGE_URL=""
IMAGE_FILE=""
HOST_CPUS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"
DEFAULT_JOBS="$(( (HOST_CPUS + 1) / 2 ))"
if [ "$DEFAULT_JOBS" -gt 1 ]; then
	DEFAULT_JOBS=1
fi
JOBS="$DEFAULT_JOBS"
CPUS="$JOBS"
MEMORY="6144"
DISK_SIZE="32G"
QEMU_ACCEL="${ILLUMOS_QEMU_ACCEL:-kvm:tcg}"
QEMU_CPU_MODEL="${ILLUMOS_QEMU_CPU:-qemu64,-svm}"
QEMU_DISK_IF="${ILLUMOS_QEMU_DISK_IF:-virtio}"
SSH_PORT=""
PKG_PROXY_PORT=""
HOST_FILE_SERVER_PORT=""
HOST_FILE_SERVER_PID=""
KEEP_VM=0
GUEST_DISPLAY=""
GUEST_XAUTHORITY=""
GUEST_HTTP_HOST_PORT=""
GUEST_HTTP_PORT="8000"
SERIAL_TTY_HOST="127.0.0.1"
SERIAL_TTY_PORT=""
USE_TTYA_BUILD=0
FORCE_TTYA_BUILD=0
WAIT_FOR_SSH_TRIES="${WAIT_FOR_SSH_TRIES:-30}"

DEFAULT_IMAGE_URL="https://dlc.openindiana.org/isos/hipster/${RELEASE}/OI-hipster-cloudimage.img.zst"

msg() { printf '\n==> %s\n' "$*"; }
die() { printf '\nERROR: %s\n' "$*" >&2; exit 1; }

usage() {
	cat <<EOF
Usage: ./build_scripts/illumos-vm-build-freebasic.sh [options]

Options:
  --release N          OpenIndiana release/date directory. Default: 20251026
  --image-url URL      OpenIndiana cloud image URL.
  --image FILE         Existing OpenIndiana cloud image.
  --workroot DIR       Work directory. Default: out/illumos-vm
  --archive-dir DIR    Final archive directory. Default: out/illumos/x86-64
  --jobs N             Build jobs inside illumos. Default: half host CPU count, capped at 1
  --cpus N             QEMU CPU count. Default: --jobs value
  --memory MB          QEMU memory in MB. Default: 6144
  --disk-size SIZE     Resized VM disk size. Default: 32G
  --ssh-port N         Host SSH forward port. Default: auto
  --pkg-proxy-port N   Host package proxy port. Default: auto
  --keep-vm            Keep VM run artifacts on success.
  --ttya               Force ttya build path (use console command channel).
  -h, --help           Show this help.
EOF
}

while [ "$#" -gt 0 ]; do
		case "$1" in
			--release)
				RELEASE="$2"
				DEFAULT_IMAGE_URL="https://dlc.openindiana.org/isos/hipster/${RELEASE}/OI-hipster-cloudimage.img.zst"
				shift 2
				;;
			--image-url) IMAGE_URL="$2"; shift 2 ;;
			--image) IMAGE_FILE="$2"; shift 2 ;;
			--workroot)
				WORKROOT="$2"
				CACHE_DIR="$WORKROOT/cache"
				RUN_DIR="$WORKROOT/run"
				LOG_DIR="$WORKROOT/logs"
				AUDIO_WAV="$LOG_DIR/freebasic-illumos-audio.wav"
				SERIAL_BOOT_LOG="$LOG_DIR/serial-boot.log"
				SERIAL_TTY_HOST="127.0.0.1"
				SERIAL_TTY_PORT=""
				shift 2
				;;
		--archive-dir) ARCHIVE_DIR="$2"; shift 2 ;;
		--jobs) JOBS="$2"; CPUS="$2"; shift 2 ;;
		--cpus) CPUS="$2"; shift 2 ;;
		--memory) MEMORY="$2"; shift 2 ;;
			--disk-size) DISK_SIZE="$2"; shift 2 ;;
		--ssh-port) SSH_PORT="$2"; shift 2 ;;
		--pkg-proxy-port) PKG_PROXY_PORT="$2"; shift 2 ;;
		--ttya) FORCE_TTYA_BUILD=1; shift ;;
		--keep-vm) KEEP_VM=1; shift ;;
		-h|--help) usage; exit 0 ;;
		*) die "unknown option: $1" ;;
	esac
done

case "$JOBS" in ''|*[!0-9]*|0) die "--jobs must be a positive integer" ;; esac
case "$CPUS" in ''|*[!0-9]*|0) die "--cpus must be a positive integer" ;; esac

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
	require_tool scp
	require_tool sha256sum
	require_tool ssh
	require_tool ssh-keygen
	require_tool timeout
	require_tool xorriso
}

download_url() {
	local url="$1"
	local dest="$2"
	local tmp="${dest}.tmp"

	rm -f "$tmp"

	if curl -fL --retry 3 --retry-delay 5 \
		--connect-timeout 20 --speed-time 20 --speed-limit 1024 \
		-o "$tmp" "$url"; then
		mv "$tmp" "$dest"
		return 0
	fi

	rm -f "$tmp"

	if command -v wget >/dev/null 2>&1; then
		if wget --timeout=30 --tries=3 -O "$tmp" "$url"; then
			mv "$tmp" "$dest"
			return 0
		fi
		rm -f "$tmp"
	fi

	return 1
}

verify_image_checksum() {
	local image="$1"
	local url="$2"
	local checksum_urls=()
	local checksum_file=""
	local expected
	local actual

	checksum_urls=(
		"${url}.sha256"
		"${url}.sha256sum"
	)

	for checksum_url in "${checksum_urls[@]}"; do
		checksum_file="$CACHE_DIR/$(basename "$checksum_url")"
		if download_url "$checksum_url" "$checksum_file"; then
			break
		fi
	done

	if [ ! -f "$checksum_file" ]; then
		msg "SHA256 file unavailable, keeping downloaded image without checksum verification"
		return 0
	fi

	expected="$(awk '{print $1; exit}' "$checksum_file")"
	[ -n "$expected" ] || die "empty SHA256 file: $checksum_file"

	actual="$(sha256sum "$image" | awk '{print $1}')"
	[ "$actual" = "$expected" ] || die "SHA256 mismatch for $image"
}

download_image() {
	mkdir -p "$CACHE_DIR" "$LOG_DIR"

	if [ -n "$IMAGE_FILE" ]; then
		[ -f "$IMAGE_FILE" ] || die "image not found: $IMAGE_FILE"
		IMAGE_FILE="$(cd "$(dirname "$IMAGE_FILE")" && pwd)/$(basename "$IMAGE_FILE")"
		IMAGE_FILE="$(decompress_image_if_needed "$IMAGE_FILE")"
		msg "Using illumos image $IMAGE_FILE"
		return 0
	fi

	if [ -z "$IMAGE_URL" ]; then
		IMAGE_URL="$DEFAULT_IMAGE_URL"
	fi

	IMAGE_FILE="$CACHE_DIR/$(basename "$IMAGE_URL")"
	IMAGE_FILE="$(cd "$CACHE_DIR" && pwd)/$(basename "$IMAGE_URL")"
	if [ ! -s "$IMAGE_FILE" ]; then
		msg "Downloading $IMAGE_URL"
		download_url "$IMAGE_URL" "$IMAGE_FILE" ||
			die "failed to download $IMAGE_URL; pass --image FILE to reuse a local illumos image"
		verify_image_checksum "$IMAGE_FILE" "$IMAGE_URL"
	else
		msg "Using cached image $IMAGE_FILE"
	fi
	IMAGE_FILE="$(decompress_image_if_needed "$IMAGE_FILE")"
}

decompress_image_if_needed() {
	local image="$1"
	local decompressed
	local expected_size
	local actual_size

	case "$image" in
		*.zst)
			decompressed="${image%.zst}"
			expected_size="$(
				zstd -lv "$image" 2>/dev/null |
					sed -n 's/.*Decompressed Size:.*(\([0-9][0-9]*\) B).*/\1/p'
			)"
			actual_size=""
			if [ -f "$decompressed" ]; then
				actual_size="$(wc -c < "$decompressed" | tr -d '[:space:]')"
			fi
			if [ -f "$decompressed" ] &&
				[ "$decompressed" -nt "$image" ] &&
				{ [ -z "$expected_size" ] || [ "$actual_size" = "$expected_size" ]; }; then
				printf '%s\n' "$decompressed"
				return 0
			fi
			if ! command -v zstd >/dev/null 2>&1; then
				die "zstd is required to decompress ${image}"
			fi
			printf '==> Decompressing %s\n' "$image" >&2
			zstd -d -f -o "$decompressed" "$image" ||
				die "failed to decompress $image"
			printf '%s\n' "$decompressed"
			;;
		*) printf '%s\n' "$image" ;;
	esac
}

make_ssh_key() {
	local key="$1"
	local pub="$key.pub"
	local pub_type

	if [ -f "$key" ] && [ -f "$pub" ]; then
		pub_type="$(ssh-keygen -l -f "$pub" 2>/dev/null || true)"
		if [ -n "$pub_type" ] && (echo "$pub_type" | grep -q ' ED25519 '); then
			rm -f "$key" "$pub"
		fi
	fi

	if [ ! -f "$key" ]; then
		ssh-keygen -q -t rsa -b 3072 -N '' -f "$key"
	fi
}

prepare_disk() {
	local disk="$RUN_DIR/illumos.qcow2"
	local src="$IMAGE_FILE"
	local src_format="raw"

	case "$src" in
		*.qcow2) src_format="qcow2" ;;
		*.zst|*.img|*.raw|*.iso) src_format="raw" ;;
		*) src_format="qcow2" ;;
	esac

	rm -f "$disk"
	qemu-img create -f qcow2 -F "$src_format" -b "$src" "$disk" "$DISK_SIZE" >/dev/null
	printf '%s\n' "$disk"
}

write_seed_iso() {
	local key_pub="$1.pub"
	local seed_dir="$RUN_DIR/seed"
	local iso="$RUN_DIR/seed.iso"

	rm -rf "$seed_dir" "$iso"
	mkdir -p "$seed_dir"

	{
		printf '#cloud-config\n'
		printf 'ssh_pwauth: false\n'
		printf 'disable_root: false\n'
		printf 'users:\n'
		printf '  - name: root\n'
		printf '    ssh_authorized_keys:\n'
		printf '      - %s\n' "$(cat "$key_pub")"
		printf 'runcmd:\n'
		printf '  - [ svcadm, enable, ssh ]\n'
	} > "$seed_dir/user-data"

	printf 'instance-id: freebasic-illumos\nlocal-hostname: freebasic-illumos\n' > "$seed_dir/meta-data"

	xorriso -as mkisofs -quiet -volid cidata -joliet -rock \
		-output "$iso" "$seed_dir/user-data" "$seed_dir/meta-data"

	printf '%s\n' "$iso"
}

start_vm() {
	local disk="$1"
	local iso="$2"
	local pidfile="$RUN_DIR/qemu.pid"

	if [ -z "$SSH_PORT" ]; then
		SSH_PORT="$(find_free_port 2222)"
	fi
	if [ -z "$SERIAL_TTY_PORT" ]; then
		SERIAL_TTY_PORT="$(find_free_port 4300)"
	fi
	msg "Starting illumos VM on SSH port $SSH_PORT"
	qemu-system-x86_64 \
		-m "$MEMORY" \
		-smp "$CPUS" \
		-machine "accel=$QEMU_ACCEL" \
		-cpu "$QEMU_CPU_MODEL" \
		-drive "file=$disk,if=$QEMU_DISK_IF,format=qcow2" \
		-drive "file=$iso,media=cdrom,if=ide,readonly=on" \
		-boot order=c \
	-netdev "user,id=net0,hostfwd=tcp:127.0.0.1:${SSH_PORT}-:22" \
		-device virtio-net-pci,netdev=net0 \
		-audiodev "wav,id=audio0,path=$AUDIO_WAV" \
		-device "ES1370,audiodev=audio0" \
		-display none \
		-chardev "socket,id=serial_ttya,host=$SERIAL_TTY_HOST,port=$SERIAL_TTY_PORT,server=on,wait=off,logfile=$SERIAL_BOOT_LOG,logappend=off" \
		-serial "chardev:serial_ttya" \
		-pidfile "$pidfile" \
		-daemonize
}

start_pkg_proxy() {
	local script="$RUN_DIR/pkg_proxy.py"
	local pidfile="$RUN_DIR/pkg_proxy.pid"

	if [ -z "$PKG_PROXY_PORT" ]; then
		PKG_PROXY_PORT="$(find_free_port 18080)"
	fi

	cat > "$script" <<'PY'
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
import http.client
import re
import sys
import threading
import time
from urllib.parse import quote, unquote

UPSTREAM = "mirror.math.princeton.edu"
UPSTREAM_LIMIT = threading.BoundedSemaphore(2)
LEGACY_DIRECTORIES = {"index", "file", "pkg", "tmp", "trans", "catalog"}
VERSION_PATHS = {
    "/pub/openindiana/hipster/versions/0",
    "/pub/openindiana/hipster/versions/0/",
    "/pub/openindiana/hipster/publisher/openindiana.org/versions/0",
    "/pub/openindiana/hipster/publisher/openindiana.org/versions/0/",
}

VERSIONS_RESPONSE = """pkg-server 0
catalog 1
file 0
index 0
info 0
manifest 0
pkg 0
p5i 0
publisher 0
search 0
status 0
tmp 0
trans 0
versions 0
"""

SECTION_VERSION_MAPS = {"catalog", "file", "index", "pkg", "tmp", "trans", "search", "status", "info", "manifest", "p5i"}

def quote_static_pkg_component(text):
    return quote(quote(text, safe=""), safe="")


def rewrite_manifest_path(path):
    if path.startswith("/pub/openindiana/hipster/publisher/openindiana.org/"):
        source_base = "/pub/openindiana/hipster/publisher/openindiana.org"
        target_base = "/pub/openindiana/hipster/publisher/openindiana.org"
    elif path.startswith("/pub/openindiana/hipster/"):
        source_base = "/pub/openindiana/hipster"
        target_base = "/pub/openindiana/hipster/publisher/openindiana.org"
    else:
        return path

    suffix = path[len(source_base):].lstrip("/")
    parts = suffix.split("/", 2)
    if len(parts) != 3 or parts[0] != "manifest" or not parts[1].isdigit():
        return path

    fmri = unquote(parts[2])
    if "@" not in fmri:
        return path

    name, version = fmri.rsplit("@", 1)
    if not name or not version:
        return path

    return (
        target_base +
        "/pkg/" +
        quote_static_pkg_component(name) +
        "/" +
        quote_static_pkg_component(version)
    )


def rewrite_file_path(path):
    if path.startswith("/pub/openindiana/hipster/publisher/openindiana.org/"):
        source_base = "/pub/openindiana/hipster/publisher/openindiana.org"
        target_base = "/pub/openindiana/hipster/publisher/openindiana.org"
    elif path.startswith("/pub/openindiana/hipster/"):
        source_base = "/pub/openindiana/hipster"
        target_base = "/pub/openindiana/hipster/publisher/openindiana.org"
    else:
        return path

    suffix = path[len(source_base):].lstrip("/")
    parts = suffix.split("/", 2)
    if len(parts) != 3 or parts[0] != "file" or not parts[1].isdigit():
        return path

    digest = parts[2].lower()
    if not re.fullmatch(r"[0-9a-f]{40}", digest):
        return path

    return target_base + "/file/" + digest[:2] + "/" + digest



def rewrite_versioned_section(path):
    rewritten = rewrite_manifest_path(path)
    if rewritten != path:
        return rewritten

    rewritten = rewrite_file_path(path)
    if rewritten != path:
        return rewritten

    if path.startswith("/pub/openindiana/hipster/publisher/openindiana.org/"):
        source_base = "/pub/openindiana/hipster/publisher/openindiana.org"
        target_base = "/pub/openindiana/hipster/publisher/openindiana.org"
    elif path.startswith("/pub/openindiana/hipster/"):
        source_base = "/pub/openindiana/hipster"
        target_base = "/pub/openindiana/hipster/publisher/openindiana.org"
    else:
        return path

    suffix = path[len(source_base):].lstrip("/")
    if not suffix:
        return path

    parts = suffix.split("/")
    if len(parts) >= 2 and parts[0] in SECTION_VERSION_MAPS and parts[1].isdigit():
        rest = "/".join(parts[2:])
        if rest:
            return target_base + "/" + parts[0] + "/" + rest
        if path.endswith("/"):
            return target_base + "/" + parts[0] + "/"
        return target_base + "/" + parts[0]
    return path


def rewrite_legacy_path(path):
    if path in VERSION_PATHS:
        return "/__fbc-versions-0"

    legacy_prefixes = [
        "/pub/openindiana/hipster/versions/0/",
        "/pub/openindiana/hipster/versions/0",
        "/pub/openindiana/hipster/publisher/openindiana.org/versions/0/",
        "/pub/openindiana/hipster/publisher/openindiana.org/versions/0",
    ]

    for legacy_prefix in legacy_prefixes:
        if path.startswith(legacy_prefix):
            suffix = path[len(legacy_prefix):].lstrip("/")
            if not suffix:
                return "/pub/openindiana/hipster/publisher/openindiana.org/"
            if "/" in suffix:
                return "/pub/openindiana/hipster/publisher/openindiana.org/" + suffix
            if suffix in LEGACY_DIRECTORIES:
                return "/pub/openindiana/hipster/publisher/openindiana.org/" + suffix
            return "/pub/openindiana/hipster/publisher/openindiana.org/catalog/" + suffix
    rewritten = rewrite_versioned_section(path)
    if rewritten != path:
        return rewritten
    return path

class Proxy(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def do_HEAD(self):
        self.proxy(False)

    def do_GET(self):
        self.proxy(True)

    def do_POST(self):
        self.proxy(True)

    def proxy(self, include_body):
        path = rewrite_legacy_path(self.path)
        if path == "/__fbc-versions-0":
            self.send_response(200, "OK")
            self.send_header("Content-Type", "text/plain")
            self.send_header("Content-Length", str(len(VERSIONS_RESPONSE.encode("utf-8"))))
            self.send_header("Connection", "close")
            self.end_headers()
            if include_body:
                self.wfile.write(VERSIONS_RESPONSE.encode("utf-8"))
            return

        length = int(self.headers.get("Content-Length", "0") or "0")
        body = self.rfile.read(length) if length else None

        conn = None
        headers = {
            k: v for k, v in self.headers.items()
            if k.lower() not in ("host", "connection", "proxy-connection")
        }
        headers["Host"] = UPSTREAM

        UPSTREAM_LIMIT.acquire()
        try:
            resp = None
            for attempt in range(1, 21):
                if conn is not None:
                    conn.close()
                conn = http.client.HTTPConnection(UPSTREAM, 80, timeout=120)
                conn.request(self.command, path, body=body, headers=headers)
                resp = conn.getresponse()
                if (
                    self.command in ("GET", "HEAD") and
                    resp.status in (500, 502, 503, 504) and
                    attempt < 20
                ):
                    resp.read()
                    time.sleep(min(attempt * 3, 30))
                    continue
                break

            self.send_response(resp.status, resp.reason)
            for key, value in resp.getheaders():
                lower = key.lower()
                if lower in ("connection", "transfer-encoding"):
                    continue
                if lower == "location":
                    value = value.replace(
                        "https://mirror.math.princeton.edu/pub/openindiana",
                        "http://10.0.2.2:%d/pub/openindiana" % PORT
                    )
                    value = value.replace(
                        "http://mirror.math.princeton.edu/pub/openindiana",
                        "http://10.0.2.2:%d/pub/openindiana" % PORT
                    )
                self.send_header(key, value)
            self.send_header("Connection", "close")
            self.end_headers()

            if include_body:
                while True:
                    chunk = resp.read(1024 * 1024)
                    if not chunk:
                        break
                    self.wfile.write(chunk)
        except Exception as ex:
            self.send_error(502, str(ex))
        finally:
            if conn is not None:
                conn.close()
            UPSTREAM_LIMIT.release()

    def log_message(self, fmt, *args):
        sys.stderr.write("%s %s\n" % (self.address_string(), fmt % args))

PORT = int(sys.argv[1])
server = ThreadingHTTPServer(("0.0.0.0", PORT), Proxy)
print("pkg proxy listening on 0.0.0.0:%d" % PORT, flush=True)
server.serve_forever()
PY

	msg "Starting OpenIndiana package proxy on port $PKG_PROXY_PORT"
	python3 -u "$script" "$PKG_PROXY_PORT" > "$LOG_DIR/pkg-proxy.log" 2>&1 &
	echo $! > "$pidfile"
	sleep 1
	kill -0 "$(cat "$pidfile")" >/dev/null 2>&1 ||
		die "package proxy failed to start; see $LOG_DIR/pkg-proxy.log"
}

start_host_file_server() {
	if [ -n "$HOST_FILE_SERVER_PID" ]; then
		return 0
	fi

	if [ -z "$HOST_FILE_SERVER_PORT" ]; then
		HOST_FILE_SERVER_PORT="$(find_free_port 9100)"
	fi

	msg "Starting host file server on http://$SERIAL_TTY_HOST:$HOST_FILE_SERVER_PORT"
	python3 -u -m http.server "$HOST_FILE_SERVER_PORT" \
		--bind "$SERIAL_TTY_HOST" \
		--directory "$RUN_DIR" \
		> "$LOG_DIR/host-file-server.log" 2>&1 &
	HOST_FILE_SERVER_PID="$!"
	sleep 1
	kill -0 "$HOST_FILE_SERVER_PID" >/dev/null 2>&1 ||
		die "host file server failed to start; see $LOG_DIR/host-file-server.log"
}

stop_host_file_server() {
	if [ -z "$HOST_FILE_SERVER_PID" ]; then
		return 0
	fi

	kill "$HOST_FILE_SERVER_PID" >/dev/null 2>&1 || true
	HOST_FILE_SERVER_PID=""
	HOST_FILE_SERVER_PORT=""
}

prepare_gnu_make_source() {
	local version="${FBC_GMAKE_BOOTSTRAP_VERSION:-4.4.1}"
	local tarball="$RUN_DIR/make-${version}.tar.gz"
	local tmp="${tarball}.tmp"
	local url

	if [ -s "$tarball" ] && tar -tzf "$tarball" >/dev/null 2>&1; then
		return 0
	fi

	rm -f "$tmp"
	msg "Fetching GNU Make bootstrap source"
	for url in \
		"http://ftp.gnu.org/gnu/make/make-${version}.tar.gz" \
		"http://ftpmirror.gnu.org/make/make-${version}.tar.gz" \
		"https://ftp.gnu.org/gnu/make/make-${version}.tar.gz"; do
		rm -f "$tmp"
		if command -v curl >/dev/null 2>&1; then
			curl -fL --retry 3 --connect-timeout 20 --max-time 600 \
				-o "$tmp" "$url" || true
		elif command -v wget >/dev/null 2>&1; then
			wget -O "$tmp" "$url" || true
		else
			break
		fi

		if [ -s "$tmp" ] && tar -tzf "$tmp" >/dev/null 2>&1; then
			mv "$tmp" "$tarball"
			return 0
		fi
	done

	rm -f "$tmp"
	msg "WARN: could not fetch GNU Make bootstrap source; guest will try its own network"
	return 1
}

prepare_crt_objects_archive() {
	local archive="$RUN_DIR/openindiana-crt-objects.tar.gz"

	if [ -s "$archive" ] &&
		tar -tzf "$archive" >/dev/null 2>&1 &&
		tar -tzf "$archive" | grep -qx 'usr/lib/crtbegin.o' &&
		tar -tzf "$archive" | grep -qx 'usr/lib/amd64/libgcc_s.so' &&
		tar -tzf "$archive" | grep -qx 'usr/lib/amd64/libgcc.a' &&
		tar -tzf "$archive" | grep -qx 'usr/lib/amd64/libstdc++.so' &&
		tar -tzf "$archive" | grep -qx 'usr/include/stdio.h' &&
		tar -tzf "$archive" | grep -qx 'usr/include/ffi.h' &&
		tar -tzf "$archive" | grep -qx 'usr/include/X11/X.h' &&
		tar -tzf "$archive" | grep -qx 'usr/include/sys/audioio.h' &&
		tar -tzf "$archive" | grep -qx 'usr/gcc/13/bin/gcc' &&
		tar -tzf "$archive" | grep -qx 'usr/gcc/13/lib/gcc/x86_64-pc-solaris2.11/13.4.0/cc1' &&
		tar -tzf "$archive" | grep -qx 'usr/gcc/13/lib/libgcc-unwind.map' &&
		tar -tzf "$archive" | grep -qx 'usr/gnu/bin/as' &&
		tar -tzf "$archive" | grep -qx 'usr/lib/amd64/libzstd.so.1.5.7' &&
		tar -tzf "$archive" | grep -qx 'var/tmp/freebasic-openindiana-startup-objects-v11'; then
		return 0
	fi

	rm -f "$archive"
	msg "Fetching OpenIndiana runtime and compiler startup objects"
	python3 - "$archive" <<'PY'
import gzip
import io
import json
import sys
import tarfile
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from urllib.parse import quote
from urllib.request import urlopen

archive_path = sys.argv[1]
base = "http://mirror.math.princeton.edu/pub/openindiana/hipster/publisher/openindiana.org"
package_specs = (
    ("system/header", "headers"),
    ("system/header/header-audio", "headers"),
    ("x11/header/x11-protocols", "headers"),
    ("system/library/c-runtime", "runtime"),
    ("library/libffi", "libffi"),
    ("system/library/gcc-13-runtime", "gcc-runtime"),
    ("system/library/g++-13-runtime", "gxx-runtime"),
    ("developer/gcc-13", "gcc-support"),
    ("developer/gnu-binutils", "binutils"),
    ("compress/zstd", "zstd"),
)

def quote_static(text):
    return quote(quote(text, safe=""), safe="")

with urlopen(base + "/catalog/catalog.base.C", timeout=120) as response:
    catalog = json.loads(response.read().decode("utf-8", "replace"))

objects = {}
links = {}
compiler_startup_names = {
    "crtbegin.o",
    "crtbeginS.o",
    "crtbeginT.o",
    "crtend.o",
    "crtendS.o",
}
compiler_library_names = {
    "libgcc.a",
    "libgcc_eh.a",
    "libgcc_s.so.1",
    "libstdc++.a",
    "libsupc++.a",
}
gcc_tool_names = {
    "cpp",
    "g++",
    "gcc",
}
gcc_private_prefix = "usr/gcc/13/lib/gcc/x86_64-pc-solaris2.11/13.4.0/"
binutils_tool_paths = {
    "usr/gnu/bin/ar",
    "usr/gnu/bin/as",
    "usr/gnu/bin/objcopy",
    "usr/gnu/bin/strip",
}

def add_object(digest, path):
    if path not in objects:
        objects[path] = digest

def add_link(path, target):
    if path not in objects and path not in links:
        links[path] = target

def is_32_bit_compiler_path(path):
    return "/32/" in path or path.endswith("/32")

def add_compiler_library_aliases(digest, basename):
    if basename == "libgcc_s.so.1":
        add_object(digest, "usr/lib/amd64/libgcc_s.so.1")
        add_object(digest, "usr/lib/amd64/libgcc_s.so")
        add_object(digest, "usr/lib/libgcc_s.so.1")
        add_object(digest, "usr/lib/libgcc_s.so")
    elif basename in ("libgcc.a", "libgcc_eh.a"):
        add_object(digest, "usr/lib/amd64/" + basename)
        add_object(digest, "usr/lib/" + basename)
    elif basename.startswith("libstdc++.so."):
        add_object(digest, "usr/lib/amd64/" + basename)
        add_object(digest, "usr/lib/amd64/libstdc++.so.6")
        add_object(digest, "usr/lib/amd64/libstdc++.so")
        add_object(digest, "usr/lib/" + basename)
        add_object(digest, "usr/lib/libstdc++.so.6")
        add_object(digest, "usr/lib/libstdc++.so")
    elif basename in ("libstdc++.a", "libsupc++.a"):
        add_object(digest, "usr/lib/amd64/" + basename)
        add_object(digest, "usr/lib/" + basename)

for package_name, package_kind in package_specs:
    entries = catalog.get("openindiana.org", {}).get(package_name, [])
    if not entries:
        raise SystemExit(package_name + " not found in catalog")

    version = entries[-1].get("version", "")
    if not version:
        raise SystemExit(package_name + " catalog entry has no version")

    manifest_url = base + "/pkg/" + quote_static(package_name) + "/" + quote_static(version)
    with urlopen(manifest_url, timeout=120) as response:
        manifest = response.read().decode("utf-8", "replace")

    for line in manifest.splitlines():
        parts = line.split()
        if not parts or parts[0] not in ("file", "link"):
            continue
        digest = ""
        path = ""
        target = ""
        for part in parts[1:]:
            if len(part) == 40 and all(ch in "0123456789abcdef" for ch in part):
                digest = part
            elif part.startswith("path="):
                path = part[5:]
            elif part.startswith("target="):
                target = part[7:]
        if not path:
            continue

        if package_kind == "headers" and path.startswith("usr/include/"):
            if parts[0] == "file" and digest:
                add_object(digest, path)
            elif parts[0] == "link" and target:
                add_link(path, target)
            continue

        if parts[0] != "file" or not digest:
            continue

        if package_kind == "runtime":
            if path.startswith("usr/lib/") and path.endswith(".o"):
                add_object(digest, path)
        elif package_kind == "libffi":
            if path.endswith("/include/ffi.h"):
                add_object(digest, path)
                add_object(digest, "usr/include/ffi.h")
            elif path.endswith("/include/ffitarget.h"):
                add_object(digest, path)
                add_object(digest, "usr/include/ffitarget.h")
        elif package_kind == "binutils":
            if path in binutils_tool_paths:
                add_object(digest, path)
        elif package_kind == "zstd":
            if path in ("usr/lib/amd64/libzstd.so.1.5.7", "usr/lib/libzstd.so.1.5.7"):
                add_object(digest, path)
        elif package_kind in ("gcc-support", "gcc-runtime", "gxx-runtime"):
            basename = path.rsplit("/", 1)[-1]
            if package_kind == "gcc-support" and path.startswith("usr/gcc/13/bin/") and basename in gcc_tool_names:
                add_object(digest, path)
            elif package_kind == "gcc-support" and path == "usr/gcc/13/lib/libgcc-unwind.map":
                add_object(digest, path)
            elif basename in compiler_startup_names:
                add_object(digest, path)
                if not is_32_bit_compiler_path(path):
                    add_object(digest, "usr/lib/" + basename)
                    add_object(digest, "usr/lib/amd64/" + basename)
            elif basename in compiler_library_names or basename.startswith("libstdc++.so."):
                add_object(digest, path)
                if not is_32_bit_compiler_path(path):
                    add_compiler_library_aliases(digest, basename)
            elif package_kind == "gcc-support" and path.startswith(gcc_private_prefix) and "/plugin/" not in path:
                add_object(digest, path)

add_link("usr/bin/cc", "../gcc/13/bin/gcc")
add_link("usr/bin/c++", "../gcc/13/bin/g++")
add_link("usr/bin/cpp", "../gcc/13/bin/cpp")
add_link("usr/bin/gcc", "../gcc/13/bin/gcc")
add_link("usr/bin/g++", "../gcc/13/bin/g++")
add_link("usr/bin/ar", "../gnu/bin/ar")
add_link("usr/bin/as", "../gnu/bin/as")
add_link("usr/bin/objcopy", "../gnu/bin/objcopy")
add_link("usr/bin/strip", "../gnu/bin/strip")
add_link("usr/lib/amd64/libzstd.so.1", "libzstd.so.1.5.7")
add_link("usr/lib/amd64/libzstd.so", "libzstd.so.1")
add_link("usr/lib/libzstd.so.1", "libzstd.so.1.5.7")
add_link("usr/lib/libzstd.so", "libzstd.so.1")

required_paths = (
    "usr/lib/crt1.o",
    "usr/lib/crtbegin.o",
    "usr/lib/crtend.o",
    "usr/lib/amd64/libgcc_s.so",
    "usr/lib/amd64/libgcc.a",
    "usr/lib/amd64/libstdc++.so",
    "usr/include/stdio.h",
    "usr/include/ffi.h",
    "usr/include/X11/X.h",
    "usr/include/sys/audioio.h",
    "usr/gcc/13/bin/gcc",
    gcc_private_prefix + "cc1",
    "usr/gcc/13/lib/libgcc-unwind.map",
    "usr/gnu/bin/as",
    "usr/lib/amd64/libzstd.so.1.5.7",
)
missing_paths = [path for path in required_paths if path not in objects]
if missing_paths:
    raise SystemExit("startup object archive is missing: " + ", ".join(missing_paths))

def fetch_payload(digest):
    payload_url = base + "/file/" + digest[:2] + "/" + digest
    last_error = None
    for attempt in range(5):
        try:
            with urlopen(payload_url, timeout=120) as response:
                payload = response.read()
            try:
                payload = gzip.decompress(payload)
            except OSError:
                pass
            return digest, payload
        except Exception as exc:
            last_error = exc
            time.sleep(min(2 ** attempt, 10))
    raise last_error

payloads = {}
with ThreadPoolExecutor(max_workers=8) as executor:
    futures = {
        executor.submit(fetch_payload, digest): digest
        for digest in sorted(set(objects.values()))
    }
    for future in as_completed(futures):
        digest, payload = future.result()
        payloads[digest] = payload

with tarfile.open(archive_path, "w:gz") as archive:
    for path, digest in sorted(objects.items()):
        payload = payloads[digest]
        info = tarfile.TarInfo(path)
        info.size = len(payload)
        info.mode = 0o644
        info.uid = 0
        info.gid = 0
        info.uname = "root"
        info.gname = "bin"
        archive.addfile(info, io.BytesIO(payload))

    for path, target in sorted(links.items()):
        info = tarfile.TarInfo(path)
        info.type = tarfile.SYMTYPE
        info.linkname = target
        info.mode = 0o777
        info.uid = 0
        info.gid = 0
        info.uname = "root"
        info.gname = "bin"
        archive.addfile(info)

    sentinel = b"decoded OpenIndiana headers, audio headers, X11 protocol headers, startup objects, libffi headers, GCC support libraries, GCC compiler files, GNU binutils tools, and GCC map files\n"
    info = tarfile.TarInfo("var/tmp/freebasic-openindiana-startup-objects-v11")
    info.size = len(sentinel)
    info.mode = 0o644
    info.uid = 0
    info.gid = 0
    info.uname = "root"
    info.gname = "bin"
    archive.addfile(info, io.BytesIO(sentinel))
PY
	tar -tzf "$archive" >/dev/null 2>&1 || {
		rm -f "$archive"
		msg "WARN: could not prepare C runtime startup object archive"
		return 1
	}
}

run_ttya_command() {
	local command="$1"
	local timeout_seconds="${2:-3600}"
	local log_file="${3:-$LOG_DIR/serial.log}"
	local command_b64 marker

	command_b64="$(printf '%s' "$command" | base64 -w0)"
	marker="__FBC_TTYA_CMD_DONE_$(date +%s)_${RANDOM}_$$"

	timeout "$((timeout_seconds + 120))" python3 - "$SERIAL_TTY_HOST" "$SERIAL_TTY_PORT" "$marker" "$command_b64" "$log_file" "$timeout_seconds" <<'PY'
import base64
import re
import socket
import sys
import time


host = sys.argv[1]
port = int(sys.argv[2])
marker = sys.argv[3]
command_b64 = sys.argv[4]
log_file = sys.argv[5]
command_timeout = int(sys.argv[6])
MAX_WAIT_BUFFER = 262144


def log_to_file(handle, text):
	if text:
		handle.write(text)
		handle.flush()


def wait_for(sock, handle, patterns, timeout_seconds):
	end = time.time() + timeout_seconds
	buffer = ""
	patterns = [p.lower() for p in patterns]
	while time.time() < end:
		try:
			chunk = sock.recv(4096)
		except socket.timeout:
			time.sleep(0.05)
			continue
		except OSError:
			break
		if not chunk:
			time.sleep(0.05)
			continue
		text = chunk.decode("utf-8", errors="replace")
		buffer += text
		if len(buffer) > MAX_WAIT_BUFFER:
			buffer = buffer[-MAX_WAIT_BUFFER:]
		log_to_file(handle, text)
		low = buffer.lower()
		for pattern in patterns:
			if pattern in low:
				return buffer
		time.sleep(0.05)
	return buffer


def send_line(sock, text):
	if not text.endswith("\n"):
		text += "\n"
	for line in text.splitlines(True):
		sock.sendall(line.encode("utf-8"))
		time.sleep(0.03)


def looks_like_prompt(text):
	for line in text.splitlines():
		line = line.rstrip("\r\n")
		if line.endswith("#") or line.endswith("$"):
			return True
	return "# " in text or "$ " in text


def looks_like_maintenance_prompt(text):
	return "enter user name for system maintenance" in text


def wait_for_login_prompt(sock, handle):
	attempts = 0
	state = ""
	while attempts < 12:
		attempts += 1
		state = wait_for(
			sock,
			handle,
			[
				"enter user name for system maintenance",
				"console login:",
				"login:",
				"assword:",
				"# ",
				"$ "
			],
			20
		)
		if looks_like_prompt(state):
			return state
		low = state.lower()
		if looks_like_maintenance_prompt(low):
			return state
		if "login:" in low:
			send_line(sock, "root")
			continue
		if "password:" in low or "assword:" in low:
			send_line(sock, "")
			continue
		send_line(sock, "\n")
	return state


sock = socket.create_connection((host, port), timeout=20)
sock.settimeout(1.0)
with open(log_file, "a", encoding="utf-8", errors="replace") as handle:
	try:
		state = wait_for_login_prompt(sock, handle)
		if looks_like_maintenance_prompt(state.lower()):
			send_line(sock, "\u0003")
			state = wait_for_login_prompt(sock, handle)
		if not looks_like_prompt(state):
			raise SystemExit("serial session did not reach a shell prompt")

		cmd_text = base64.b64decode(command_b64).decode("utf-8")
		cmd_text = cmd_text.replace("\t", "    ")
		wrapper_text = (
			"/bin/sh /tmp/freebasic-ttya-cmd.sh\n"
			"rc=$?\n"
			"rm -f /tmp/freebasic-ttya-cmd.sh /tmp/freebasic-ttya-wrapper.sh\n"
			f"printf '%s:%s\\n' '{marker}' \"$rc\"\n"
			"exit \"$rc\"\n"
		)
		shell_cmd = (
			f"cat > /tmp/freebasic-ttya-cmd.sh <<'EOF'\n"
			f"{cmd_text}\n"
			f"EOF\n"
			"cat > /tmp/freebasic-ttya-wrapper.sh <<'EOF'\n"
			f"{wrapper_text}"
			"EOF\n"
			"sh /tmp/freebasic-ttya-wrapper.sh\n"
		)
		send_line(sock, shell_cmd)
		completion = wait_for(sock, handle, [marker + ":"], command_timeout)
		match = re.search(r"%s:(\d+)" % re.escape(marker), completion)
		if not match:
			send_line(sock, "\u0003")
			wait_for(sock, handle, ["# ", "$ "], 30)
			raise SystemExit("serial command failed: completion marker not observed")
		else:
			rc = int(match.group(1))
		if rc != 0:
			raise SystemExit(f"guest command exited with code {rc}")
	finally:
		try:
			sock.close()
		except OSError:
			pass
PY
}

prepare_ttya_guest() {
	local attempts=0
	local max_attempts=8
	local prepare_log="$LOG_DIR/ttya-prepare.log"
	local prepare_cmd

	prepare_cmd="$(cat <<'CMD'
set -eu
if [ -x /usr/bin/stty ]; then
	/usr/bin/stty rows 40 columns 160 >/dev/null 2>&1 || true
fi

BOOTSTRAP_INTERFACES=$(
	/usr/sbin/dladm show-phys -p -o LINK 2>/dev/null \
		| /usr/bin/awk '$1 != "" && $1 != "lo0" { print $1 }'
)
if [ -z "$BOOTSTRAP_INTERFACES" ]; then
	BOOTSTRAP_INTERFACES=$(
		/usr/sbin/ifconfig -a 2>/dev/null \
			| /usr/bin/awk -F: '/^[a-zA-Z][a-zA-Z0-9_-]*:/{print $1}' \
			| /usr/bin/grep -Ev '^lo0$'
	)
fi
if [ -n "$BOOTSTRAP_INTERFACES" ]; then
	for bootstrap_iface in $BOOTSTRAP_INTERFACES; do
		/usr/sbin/ifconfig "$bootstrap_iface" plumb >/dev/null 2>&1 || true
		/usr/sbin/ifconfig "$bootstrap_iface" up >/dev/null 2>&1 || true
		/usr/sbin/ifconfig "$bootstrap_iface" inet 10.0.2.15 netmask 255.255.255.0 up >/dev/null 2>&1 || true
		break
	done
	/usr/sbin/route add default 10.0.2.2 >/dev/null 2>&1 || true
	/usr/sbin/route -p add net default 10.0.2.2 >/dev/null 2>&1 || true
fi
CMD
)"

	msg "Preparing illumos guest through ttya"
	while [ "$attempts" -lt "$max_attempts" ]; do
		attempts=$((attempts + 1))
		if run_ttya_command "$prepare_cmd" 600 "$prepare_log"; then
			return 0
		fi
		msg "ttya prepare attempt $attempts failed; waiting for console"
		sleep 20
	done

	return 1
}

ssh_illumos() {
	timeout "${SSH_TIMEOUT:-30}" ssh -o StrictHostKeyChecking=no \
		-o UserKnownHostsFile=/dev/null \
		-o BatchMode=yes \
		-o NumberOfPasswordPrompts=0 \
		-o ConnectTimeout=10 \
		-o PubkeyAcceptedKeyTypes=+ssh-rsa \
		-o HostKeyAlgorithms=+ssh-rsa \
		-i "$RUN_DIR/id_ed25519" \
		-p "$SSH_PORT" \
		root@127.0.0.1 "$@"
}

scp_to_illumos() {
timeout 300 scp -o StrictHostKeyChecking=no \
		-o UserKnownHostsFile=/dev/null \
		-o BatchMode=yes \
		-o NumberOfPasswordPrompts=0 \
		-o PubkeyAcceptedKeyTypes=+ssh-rsa \
		-o HostKeyAlgorithms=+ssh-rsa \
		-i "$RUN_DIR/id_ed25519" \
		-P "$SSH_PORT" \
		"$@"
}

wait_for_ssh() {
	local tries="$WAIT_FOR_SSH_TRIES"
	local bootstrap_done=0
	local bootstrap_attempted=0
	local attempts_before_bootstrap

	attempts_before_bootstrap=$((tries - 2))
	if [ "$attempts_before_bootstrap" -lt 1 ]; then
		attempts_before_bootstrap=1
	fi

	msg "Waiting for SSH"
	while [ "$tries" -gt 0 ]; do
		if ssh_illumos 'uname -a' >/dev/null 2>&1; then
			return 0
		fi
		if [ "$bootstrap_done" -eq 0 ] && [ "$bootstrap_attempted" -eq 0 ] && [ "$tries" -le "$attempts_before_bootstrap" ]; then
			bootstrap_attempted=1
			msg "SSH not yet available; attempting console bootstrap via ttya"
			if bootstrap_illumos_via_ttya; then
				bootstrap_done=1
			else
				msg "ttya bootstrap failed; continuing to wait for SSH"
			fi
		fi
		tries=$((tries - 1))
		sleep 10
	done

	return 1
}

bootstrap_illumos_via_ttya() {
	local key_pub="$RUN_DIR/id_ed25519.pub"
	local key_b64

	if [ -z "$SERIAL_TTY_PORT" ]; then
		msg "No ttya serial port configured; cannot bootstrap via console"
		return 1
	fi
	if [ ! -f "$key_pub" ]; then
		msg "Public SSH key missing: $key_pub"
		return 1
	fi

	key_b64="$(python3 - "$key_pub" <<'PY'
import base64
import sys

with open(sys.argv[1], "r", encoding="utf-8") as f:
    data = f.read()

print(base64.b64encode(data.encode("utf-8")).decode("ascii"), end="")
PY
)"

	timeout 120 python3 - "$SERIAL_TTY_HOST" "$SERIAL_TTY_PORT" "$key_b64" "$LOG_DIR/serial.log" "__FBC_TTYA_BOOTSTRAP_DONE__" <<'PY'
import base64
import socket
import sys
import time


def log_to_file(handle, text):
	if text:
		handle.write(text)
		handle.flush()


def wait_for(sock, handle, patterns, timeout_seconds):
	end = time.time() + timeout_seconds
	buffer = ""
	patterns = [p.lower() for p in patterns]
	while time.time() < end:
		try:
			chunk = sock.recv(4096)
		except socket.timeout:
			time.sleep(0.1)
			continue
		except OSError:
			break
		if not chunk:
			time.sleep(0.1)
			continue
		text = chunk.decode("utf-8", errors="replace")
		buffer += text
		log_to_file(handle, text)
		low = buffer.lower()
		for pattern in patterns:
			if pattern in low:
				return buffer
		time.sleep(0.05)
	return buffer


def send_line(sock, text):
	if not text.endswith("\n"):
		text += "\n"
	for line in text.splitlines(True):
		sock.sendall(line.encode("utf-8"))
		time.sleep(0.03)


def looks_like_prompt(text):
    for line in text.splitlines():
        line = line.rstrip("\r\n")
        if line.endswith("#") or line.endswith("$"):
            return True
    return "# " in text or "$ " in text


def looks_like_maintenance_prompt(text):
    return "enter user name for system maintenance" in text


def wait_for_login_prompt(sock, handle):
	attempts = 0
	state = ""
	while attempts < 8:
		attempts += 1
		state = wait_for(
			sock,
			handle,
			[
				"enter user name for system maintenance",
				"console login:",
				"login:",
				"assword:",
				"# ",
				"$ "
			],
			40
		)
		if looks_like_prompt(state):
			return state
		low = state.lower()
		if looks_like_maintenance_prompt(low):
			return state
		if "login:" in low:
			send_line(sock, "root")
			continue
		if "password:" in low or "assword:" in low:
			send_line(sock, "")
			continue
		send_line(sock, "\n")
	return state



host = sys.argv[1]
port = int(sys.argv[2])
key_b64 = sys.argv[3]
log_file = sys.argv[4]
done_marker = sys.argv[5]

public_key = base64.b64decode(key_b64.encode("ascii")).decode("utf-8").strip()

needs_reboot = 0

bootstrap_cmd_tmpl = """set -eu
mkdir -p /root/.ssh
chmod 700 /root/.ssh
cat > /root/.ssh/authorized_keys <<'FBC_KEY'
{key}
FBC_KEY
chmod 600 /root/.ssh/authorized_keys
chown root:root /root/.ssh /root/.ssh/authorized_keys || true

awk ' \
/^#?PermitRootLogin[[:space:]]+/ {{ print "PermitRootLogin yes"; permit_root=1; next }} \
/^#?PubkeyAuthentication[[:space:]]+/ {{ print "PubkeyAuthentication yes"; pubkey_auth=1; next }} \
/^#?PasswordAuthentication[[:space:]]+/ {{ print "PasswordAuthentication yes"; password_auth=1; next }} \
{{ print }} \
END {{ \
	if (!permit_root) print "PermitRootLogin yes"; \
	if (!pubkey_auth) print "PubkeyAuthentication yes"; \
	if (!password_auth) print "PasswordAuthentication yes"; \
}}' /etc/ssh/sshd_config > /etc/ssh/sshd_config.new && mv /etc/ssh/sshd_config.new /etc/ssh/sshd_config

if /usr/sbin/svcadm enable -r svc:/network/ssh:default >/dev/null 2>&1; then
    :
elif /usr/sbin/svcadm restart svc:/network/ssh:default >/dev/null 2>&1; then
    :
else
    :
fi
BOOTSTRAP_INTERFACES=$(
    /usr/sbin/dladm show-phys -p -o LINK 2>/dev/null \
        | /usr/bin/awk '$1 != "" && $1 != "lo0" {{ print $1 }}'
)
if [ -z "$BOOTSTRAP_INTERFACES" ]; then
    BOOTSTRAP_INTERFACES=$(
        /usr/sbin/ifconfig -a 2>/dev/null \
            | /usr/bin/awk -F: '/^[a-zA-Z][a-zA-Z0-9_-]*:/{{print $1}}' \
            | /usr/bin/grep -Ev '^lo0$'
    )
fi
if [ -n "$BOOTSTRAP_INTERFACES" ]; then
    for bootstrap_iface in $BOOTSTRAP_INTERFACES; do
        /usr/sbin/ifconfig "$bootstrap_iface" plumb >/dev/null 2>&1 || true
        /usr/sbin/ifconfig "$bootstrap_iface" up >/dev/null 2>&1 || true
        /usr/sbin/ifconfig "$bootstrap_iface" inet 10.0.2.15 netmask 255.255.255.0 up >/dev/null 2>&1 || true
        break
    done
    /usr/sbin/route -p add net default 10.0.2.2 >/dev/null 2>&1 || true
fi
echo "{done_marker}"
"""

sock = socket.create_connection((host, port), timeout=20)
sock.settimeout(1.0)
with open(log_file, "a", encoding="utf-8", errors="replace") as handle:
	try:
		state = wait_for_login_prompt(sock, handle)
		if looks_like_maintenance_prompt(state.lower()):
			needs_reboot = 1
			send_line(sock, "\x04")
			state = wait_for_login_prompt(sock, handle)

		if not looks_like_prompt(state):
			send_line(sock, "root")
			state = wait_for_login_prompt(sock, handle)
			if "assword:" in state.lower():
				send_line(sock, "")
				state = wait_for_login_prompt(sock, handle)

		if not looks_like_prompt(state):
			raise SystemExit("serial session did not reach a shell prompt")

		bootstrap_cmd = bootstrap_cmd_tmpl.format(
			key=public_key,
			done_marker=done_marker
		)
		bootstrap_cmd = bootstrap_cmd.replace("\t", "    ")
		if needs_reboot:
			bootstrap_cmd += """
if [ -x /usr/sbin/reboot ]; then
    sync
    /usr/sbin/reboot
elif [ -x /sbin/reboot ]; then
    sync
    /sbin/reboot
else
    sync
    /usr/sbin/init 6
fi
"""
		send_line(sock, bootstrap_cmd)
		done_marker_lower = done_marker.lower()
		completion = wait_for(sock, handle, [done_marker], 600)
		if done_marker_lower not in completion.lower():
			prompt_wait = wait_for(sock, handle, ["# ", "$ "], 120)
			if not looks_like_prompt(prompt_wait):
				send_line(sock, "true")
				wait_for(sock, handle, ["# ", "$ "], 5)
	finally:
		try:
			sock.close()
		except OSError:
			pass
PY
}

pack_source() {
	local tarball="$RUN_DIR/freebasic-source.tar.gz"

	msg "Packing source tree"
	rm -f "$tarball"
	tar -czf "$tarball" \
		--no-same-owner \
		--no-same-permissions \
		--no-acls \
		--no-xattrs \
		--exclude='*/obj' \
		--exclude='*.o' \
		--exclude='*.a' \
		-C "$ROOT" \
		GNUmakefile \
		changelog.txt \
		readme.txt \
		todo.txt \
		build_scripts \
		examples \
		inc \
		lib \
		mk \
		src \
		tests \
		bootstrap/illumos-x86_64
}

run_guest_build() {
	local guest_env
	local guest_extract_cmd
	local guest_build_dir
	local guest_build_root
	local guest_mode="ssh"
	local build_log="$LOG_DIR/build.log"
	local download_url=""
	local prebuild_log="$LOG_DIR/ttya-prepare.log"
	local download_log="$LOG_DIR/ttya-download.log"
	local artifact_log="$LOG_DIR/ttya-artifacts.log"

	guest_env="ILLUMOS_PKG_PROXY=http://10.0.2.2:${PKG_PROXY_PORT} NATIVE_JOBS=${JOBS}"
	[ -n "$HOST_FILE_SERVER_PID" ] || start_host_file_server
	if prepare_gnu_make_source; then
		guest_env="$guest_env FBC_GMAKE_TARBALL_URL=http://10.0.2.2:${HOST_FILE_SERVER_PORT}/make-${FBC_GMAKE_BOOTSTRAP_VERSION:-4.4.1}.tar.gz"
	fi
	if prepare_crt_objects_archive; then
		guest_env="$guest_env FBC_CRT_OBJECTS_URL=http://10.0.2.2:${HOST_FILE_SERVER_PORT}/openindiana-crt-objects.tar.gz"
	fi

	if [ "$USE_TTYA_BUILD" -eq 1 ]; then
		guest_mode="ttya"
		guest_build_root="/var/tmp/freebasic-build"
	else
		guest_build_root="$(
			SSH_TIMEOUT=30 ssh_illumos '
				for base in /export/home /home /var/tmp /tmp /root; do
					if [ ! -d "$base" ] || [ ! -w "$base" ]; then
						continue
					fi
					available_k=$(df -Pk "$base" 2>/dev/null | awk "NR==2 {print \$4}")
					if [ -n "$available_k" ] && [ "$available_k" -gt 2000000 ]; then
						echo "$base/freebasic-build"
						exit 0
					fi
				done
				echo "/root/freebasic-build"
			'
		)"
	fi
	guest_build_dir="$guest_build_root"

	guest_extract_cmd="$(cat <<'CMD'
cd __GUEST_BUILD_DIR__
if ! tar -xzf source.tar.gz; then
	echo "ERROR: direct tar extraction failed; retrying into staging directory"
	tmpdir="__GUEST_BUILD_DIR__-extracted.$$"
	rm -rf "$tmpdir"
	mkdir -p "$tmpdir"
	tar -xzf source.tar.gz \
		--exclude="./OMA/android-output/qfak-overlay.apk" \
		--exclude="OMA/android-output/qfak-overlay.apk" \
		--exclude="./OMA/android-output/qfak-overlay.apk.idsig" \
		--exclude="OMA/android-output/qfak-overlay.apk.idsig" \
		-C "$tmpdir" || {
		echo "ERROR: tar extraction failed even with fallback"
		rm -rf "$tmpdir"
		exit 1
	}
	find __GUEST_BUILD_DIR__ -mindepth 1 ! -name "source.tar.gz" -exec rm -rf {} +
	cp -Rp "$tmpdir"/. __GUEST_BUILD_DIR__/
	rm -rf "$tmpdir"
fi
if [ ! -f __GUEST_BUILD_DIR__/build_scripts/illumos-build-freebasic.sh ]; then
	echo "ERROR: extracted tree is missing illumos build script"
	ls -la __GUEST_BUILD_DIR__ | sed -n 1,80p
else
	echo "INFO: illumos build script extracted successfully"
fi
CMD
)"
	guest_extract_cmd="${guest_extract_cmd//__GUEST_BUILD_DIR__/$guest_build_dir}"

	if [ "$guest_mode" = "ssh" ]; then
		msg "Uploading source"
		ssh_illumos "rm -rf ${guest_build_dir} && mkdir -p ${guest_build_dir}"
		scp_to_illumos "$RUN_DIR/freebasic-source.tar.gz" root@127.0.0.1:${guest_build_dir}/source.tar.gz
		SSH_TIMEOUT=14400 ssh_illumos "$guest_extract_cmd"
		SSH_TIMEOUT=14400 ssh_illumos "[ -f ${guest_build_dir}/build_scripts/illumos-build-freebasic.sh ] || mkdir -p ${guest_build_dir}/build_scripts"
		if ! SSH_TIMEOUT=14400 ssh_illumos "[ -f ${guest_build_dir}/build_scripts/illumos-build-freebasic.sh ]"; then
			msg "Illumos guest missing build script; copying build script tree from host"
			scp_to_illumos -r "$ROOT/build_scripts/" root@127.0.0.1:${guest_build_dir}/build_scripts/
			SSH_TIMEOUT=14400 ssh_illumos "chmod +x ${guest_build_dir}/build_scripts/illumos-build-freebasic.sh"
		fi
	else
		msg "Preparing source directory in guest via ttya"
		download_url="http://10.0.2.2:${HOST_FILE_SERVER_PORT}/freebasic-source.tar.gz"
		run_ttya_command "mkdir -p '$guest_build_dir'; cd '$guest_build_dir'; rm -rf ./* ./.[!.]* ./..?* 2>/dev/null || true" 120 "$prebuild_log"
		run_ttya_command "cd '$guest_build_dir'
if command -v curl >/dev/null 2>&1; then
	curl -fL --retry 3 --connect-timeout 20 --max-time 1200 --retry-delay 5 -o source.tar.gz '$download_url'
elif command -v wget >/dev/null 2>&1; then
	wget -O source.tar.gz '$download_url'
else
	echo 'ERROR: guest needs curl or wget to fetch source archive'
	exit 1
fi
tar -tzf source.tar.gz >/dev/null 2>&1 || {
	echo 'ERROR: downloaded archive is invalid'
	exit 1
}
" 1200 "$download_log"
		run_ttya_command "$guest_extract_cmd" 1800 "$prebuild_log"
	fi

	if [ -n "$GUEST_DISPLAY" ]; then
		guest_env="$guest_env DISPLAY=$GUEST_DISPLAY XAUTHORITY=$GUEST_XAUTHORITY"
	fi

	msg "Running illumos build"
		if [ "$guest_mode" = "ssh" ]; then
			SSH_TIMEOUT=14400 ssh_illumos "cd ${guest_build_dir} && $guest_env bash build_scripts/illumos-build-freebasic.sh" \
				2>&1 | tee "$build_log"
		else
			if ! run_ttya_command "cd ${guest_build_dir} && $guest_env bash build_scripts/illumos-build-freebasic.sh" 14400 "$build_log"; then
				return 1
			fi
		fi

	if [ "$guest_mode" = "ssh" ]; then
		msg "Collecting package repository"
		rm -rf "$ARCHIVE_DIR"
		mkdir -p "$ARCHIVE_DIR" "$LOG_DIR"
		scp_to_illumos -r root@127.0.0.1:${guest_build_dir}/out/illumos/\* "$ARCHIVE_DIR/"
		cp "$build_log" "$ARCHIVE_DIR/build.log"
	else
		mkdir -p "$ARCHIVE_DIR"
		cp "$build_log" "$ARCHIVE_DIR/build.log"
		run_ttya_command "cd ${guest_build_dir} && ls -la out/illumos/x86-64 2>/dev/null || true; \
			[ -f out/illumos/x86-64/build.log ] && cat out/illumos/x86-64/build.log" 180 "$artifact_log" || true
		cp "$artifact_log" "$ARCHIVE_DIR/ttya-artifacts.log" 2>/dev/null || true
	fi
}

cleanup_vm() {
	if [ -n "$HOST_FILE_SERVER_PID" ]; then
		stop_host_file_server
	fi

	if [ -f "$RUN_DIR/x11-forward.pid" ]; then
		kill "$(cat "$RUN_DIR/x11-forward.pid")" >/dev/null 2>&1 || true
		rm -f "$RUN_DIR/x11-forward.pid"
	fi

	if [ -f "$RUN_DIR/pkg_proxy.pid" ]; then
		kill "$(cat "$RUN_DIR/pkg_proxy.pid")" >/dev/null 2>&1 || true
		rm -f "$RUN_DIR/pkg_proxy.pid"
	fi

	if [ -f "$RUN_DIR/qemu.pid" ]; then
		kill "$(cat "$RUN_DIR/qemu.pid")" >/dev/null 2>&1 || true
		rm -f "$RUN_DIR/qemu.pid"
	fi
}

start_x11_forward() {
	local display_number
	local socket
	local xauth_raw
	local xauth_file
	local remote_port

	[ -n "${DISPLAY:-}" ] || return 0

	display_number="$(printf '%s\n' "$DISPLAY" |
		sed -n 's/^.*:\([0-9][0-9]*\)\(\.[0-9][0-9]*\)\?$/\1/p')"
	[ -n "$display_number" ] || return 0

	socket="/tmp/.X11-unix/X${display_number}"
	[ -S "$socket" ] || return 0

	require_tool xauth

	xauth_raw="$RUN_DIR/xauth.raw"
	xauth_file="$RUN_DIR/Xauthority"
	rm -f "$xauth_raw" "$xauth_file"

	if ! xauth nlist "$DISPLAY" > "$xauth_raw" 2>/dev/null || [ ! -s "$xauth_raw" ]; then
		msg "Host DISPLAY=$DISPLAY has no xauth cookie; gfx smoke will need guest Xvfb"
		return 0
	fi

	: > "$xauth_file"
	sed -e 's/^..../ffff/' "$xauth_raw" | xauth -f "$xauth_file" nmerge -
	for _ in 1 2 3 4 5 6 7 8 9 10 11 12; do
		if scp_to_illumos "$xauth_file" root@127.0.0.1:/root/.freebasic-xauthority &&
			ssh_illumos 'chmod 600 /root/.freebasic-xauthority'; then
			break
		fi
		sleep 5
	done
	ssh_illumos 'test -f /root/.freebasic-xauthority' || {
		msg "Could not copy Xauthority into guest; gfx smoke will need guest Xvfb"
		return 0
	}

	remote_port="$((6000 + display_number))"
	msg "Forwarding guest X11 display 127.0.0.1:${display_number}.0 to host $DISPLAY"
	ssh -o StrictHostKeyChecking=no \
		-o UserKnownHostsFile=/dev/null \
		-o BatchMode=yes \
		-o NumberOfPasswordPrompts=0 \
		-o ConnectTimeout=10 \
		-i "$RUN_DIR/id_ed25519" \
		-p "$SSH_PORT" \
		-N \
		-R "127.0.0.1:${remote_port}:${socket}" \
		root@127.0.0.1 &
	echo $! > "$RUN_DIR/x11-forward.pid"
	sleep 1
	kill -0 "$(cat "$RUN_DIR/x11-forward.pid")" >/dev/null 2>&1 || {
		rm -f "$RUN_DIR/x11-forward.pid"
		msg "X11 SSH tunnel failed to start; gfx smoke will need guest Xvfb"
		return 0
	}

	GUEST_DISPLAY="127.0.0.1:${display_number}.0"
	GUEST_XAUTHORITY="/root/.freebasic-xauthority"
}

verify_audio_capture() {
	local wav="$1"
	local log="$2"

	msg "Verifying illumos QEMU audio capture"
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

main() {
	check_host_tools
	rm -rf "$RUN_DIR"
	mkdir -p "$RUN_DIR" "$LOG_DIR" "$ARCHIVE_DIR"
	rm -f \
		"$AUDIO_WAV" \
		"$LOG_DIR/build.log" \
		"$LOG_DIR/freebasic-illumos-audio.log" \
		"$LOG_DIR/host-file-server.log" \
		"$LOG_DIR/pkg-proxy.log" \
		"$LOG_DIR/serial.log" \
		"$SERIAL_BOOT_LOG" \
		"$LOG_DIR/ttya-artifacts.log" \
		"$LOG_DIR/ttya-download.log" \
		"$LOG_DIR/ttya-prepare.log"

	make_ssh_key "$RUN_DIR/id_ed25519"
	download_image
	local disk
	local iso
	disk="$(prepare_disk)"
	iso="$(write_seed_iso "$RUN_DIR/id_ed25519")"

	trap cleanup_vm EXIT
	start_pkg_proxy
	start_vm "$disk" "$iso"
	if [ "$FORCE_TTYA_BUILD" -eq 1 ]; then
		USE_TTYA_BUILD=1
		msg "Forcing ttya build path (--ttya)"
		prepare_ttya_guest
	elif wait_for_ssh; then
		msg "SSH is available; using SSH build path"
	else
		USE_TTYA_BUILD=1
		msg "SSH did not become available; using ttya build path"
		prepare_ttya_guest
	fi

	if [ "$USE_TTYA_BUILD" -eq 1 ]; then
		start_host_file_server
	else
		start_x11_forward
	fi
	pack_source
	run_guest_build || exit 1
	cleanup_vm
	verify_audio_capture "$AUDIO_WAV" "$LOG_DIR/freebasic-illumos-audio.log"
	cp "$AUDIO_WAV" "$ARCHIVE_DIR/freebasic-illumos-audio.wav"
	cp "$LOG_DIR/freebasic-illumos-audio.log" "$ARCHIVE_DIR/freebasic-illumos-audio.log"

	msg "SUCCESS"

	if [ "$KEEP_VM" -eq 0 ]; then
		cleanup_vm
		rm -rf "$RUN_DIR"
		trap - EXIT
	fi
}

main "$@"
