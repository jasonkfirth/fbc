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
#   * download or reuse an OmniOS stable x86_64 cloud image
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

RELEASE="r151058"
IMAGE_URL=""
IMAGE_FILE=""
HOST_CPUS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"
JOBS="$(( (HOST_CPUS + 1) / 2 ))"
CPUS="$JOBS"
MEMORY="6144"
DISK_SIZE="32G"
SSH_PORT=""
PKG_PROXY_PORT=""
KEEP_VM=0
GUEST_DISPLAY=""
GUEST_XAUTHORITY=""

DEFAULT_IMAGE_URL="https://downloads.omnios.org/media/stable/omnios-${RELEASE}.cloud.qcow2"

msg() { printf '\n==> %s\n' "$*"; }
die() { printf '\nERROR: %s\n' "$*" >&2; exit 1; }

usage() {
	cat <<EOF
Usage: ./build_scripts/illumos-vm-build-freebasic.sh [options]

Options:
  --release N          OmniOS release. Default: r151058
  --image-url URL      OmniOS cloud qcow2 URL.
  --image FILE         Existing OmniOS cloud qcow2 image.
  --workroot DIR       Work directory. Default: out/illumos-vm
  --archive-dir DIR    Final archive directory. Default: out/illumos/x86-64
  --jobs N             Build jobs inside illumos. Default: half host CPU count
  --cpus N             QEMU CPU count. Default: --jobs value
  --memory MB          QEMU memory in MB. Default: 6144
  --disk-size SIZE     Resized VM disk size. Default: 32G
  --ssh-port N         Host SSH forward port. Default: auto
  --pkg-proxy-port N   Host package proxy port. Default: auto
  --keep-vm            Keep VM run artifacts on success.
  -h, --help           Show this help.
EOF
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--release)
			RELEASE="$2"
			DEFAULT_IMAGE_URL="https://downloads.omnios.org/media/stable/omnios-${RELEASE}.cloud.qcow2"
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
			shift 2
			;;
		--archive-dir) ARCHIVE_DIR="$2"; shift 2 ;;
		--jobs) JOBS="$2"; CPUS="$2"; shift 2 ;;
		--cpus) CPUS="$2"; shift 2 ;;
		--memory) MEMORY="$2"; shift 2 ;;
		--disk-size) DISK_SIZE="$2"; shift 2 ;;
		--ssh-port) SSH_PORT="$2"; shift 2 ;;
		--pkg-proxy-port) PKG_PROXY_PORT="$2"; shift 2 ;;
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
	local checksum_url="${url}.sha256"
	local checksum_file="$CACHE_DIR/$(basename "$checksum_url")"
	local expected
	local actual

	if ! download_url "$checksum_url" "$checksum_file"; then
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
		msg "Using illumos image $IMAGE_FILE"
		return 0
	fi

	if [ -z "$IMAGE_URL" ]; then
		IMAGE_URL="$DEFAULT_IMAGE_URL"
	fi

	IMAGE_FILE="$CACHE_DIR/$(basename "$IMAGE_URL")"
	if [ ! -s "$IMAGE_FILE" ]; then
		msg "Downloading $IMAGE_URL"
		download_url "$IMAGE_URL" "$IMAGE_FILE" ||
			die "failed to download $IMAGE_URL; pass --image FILE to reuse a local illumos image"
		verify_image_checksum "$IMAGE_FILE" "$IMAGE_URL"
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
	local disk="$RUN_DIR/illumos.qcow2"

	rm -f "$disk"
	qemu-img create -f qcow2 -F qcow2 -b "$IMAGE_FILE" "$disk" "$DISK_SIZE" >/dev/null
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
	local serial="$LOG_DIR/serial.log"

	if [ -z "$SSH_PORT" ]; then
		SSH_PORT="$(find_free_port 2222)"
	fi

	msg "Starting illumos VM on SSH port $SSH_PORT"
	qemu-system-x86_64 \
		-m "$MEMORY" \
		-smp "$CPUS" \
		-machine accel=kvm:tcg \
		-cpu max \
		-drive "file=$disk,if=virtio,format=qcow2" \
		-drive "file=$iso,media=cdrom,if=ide,readonly=on" \
		-netdev "user,id=net0,hostfwd=tcp:127.0.0.1:${SSH_PORT}-:22" \
		-device e1000,netdev=net0 \
		-audiodev "wav,id=audio0,path=$AUDIO_WAV" \
		-device "ES1370,audiodev=audio0" \
		-display none \
		-serial "file:$serial" \
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
import ssl
import sys

UPSTREAM = "pkg.omnios.org"

class Proxy(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def do_HEAD(self):
        self.proxy(False)

    def do_GET(self):
        self.proxy(True)

    def do_POST(self):
        self.proxy(True)

    def proxy(self, include_body):
        length = int(self.headers.get("Content-Length", "0") or "0")
        body = self.rfile.read(length) if length else None

        ctx = ssl.create_default_context()
        ctx.maximum_version = ssl.TLSVersion.TLSv1_2
        conn = http.client.HTTPSConnection(UPSTREAM, 443, context=ctx, timeout=120)
        headers = {
            k: v for k, v in self.headers.items()
            if k.lower() not in ("host", "connection", "proxy-connection")
        }
        headers["Host"] = UPSTREAM

        try:
            conn.request(self.command, self.path, body=body, headers=headers)
            resp = conn.getresponse()
            self.send_response(resp.status, resp.reason)
            for key, value in resp.getheaders():
                lower = key.lower()
                if lower in ("connection", "transfer-encoding"):
                    continue
                if lower == "location":
                    value = value.replace(
                        "https://pkg.omnios.org",
                        "http://10.0.2.2:%d" % PORT
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
            conn.close()

    def log_message(self, fmt, *args):
        sys.stderr.write("%s %s\n" % (self.address_string(), fmt % args))

PORT = int(sys.argv[1])
server = ThreadingHTTPServer(("0.0.0.0", PORT), Proxy)
print("pkg proxy listening on 0.0.0.0:%d" % PORT, flush=True)
server.serve_forever()
PY

	msg "Starting OmniOS package proxy on port $PKG_PROXY_PORT"
	python3 -u "$script" "$PKG_PROXY_PORT" > "$LOG_DIR/pkg-proxy.log" 2>&1 &
	echo $! > "$pidfile"
	sleep 1
	kill -0 "$(cat "$pidfile")" >/dev/null 2>&1 ||
		die "package proxy failed to start; see $LOG_DIR/pkg-proxy.log"
}

ssh_illumos() {
	timeout "${SSH_TIMEOUT:-30}" ssh -o StrictHostKeyChecking=no \
		-o UserKnownHostsFile=/dev/null \
		-o BatchMode=yes \
		-o NumberOfPasswordPrompts=0 \
		-o ConnectTimeout=10 \
		-i "$RUN_DIR/id_ed25519" \
		-p "$SSH_PORT" \
		root@127.0.0.1 "$@"
}

scp_to_illumos() {
	timeout 300 scp -o StrictHostKeyChecking=no \
		-o UserKnownHostsFile=/dev/null \
		-o BatchMode=yes \
		-o NumberOfPasswordPrompts=0 \
		-i "$RUN_DIR/id_ed25519" \
		-P "$SSH_PORT" \
		"$@"
}

wait_for_ssh() {
	local tries=90

	msg "Waiting for SSH"
	while [ "$tries" -gt 0 ]; do
		if ssh_illumos 'uname -a' >/dev/null 2>&1; then
			return 0
		fi
		tries=$((tries - 1))
		sleep 10
	done

	die "timed out waiting for illumos SSH"
}

pack_source() {
	local tarball="$RUN_DIR/freebasic-source.tar.gz"

	msg "Packing source tree"
	rm -f "$tarball"
	tar -czf "$tarball" \
		--exclude='./.git' \
		--exclude='./out' \
		--exclude='./.build*' \
		--exclude='./package-root' \
		--exclude='./bin/fbc*' \
		--exclude='./bootstrap/fbc*' \
		--exclude='*/obj' \
		--exclude='*.o' \
		--exclude='*.a' \
		-C "$ROOT" .
}

run_guest_build() {
	local guest_env

	msg "Uploading source"
	ssh_illumos 'rm -rf /var/tmp/freebasic-build && mkdir -p /var/tmp/freebasic-build'
	scp_to_illumos "$RUN_DIR/freebasic-source.tar.gz" root@127.0.0.1:/var/tmp/freebasic-build/source.tar.gz

	msg "Running illumos build"
	guest_env="ILLUMOS_PKG_PROXY=http://10.0.2.2:${PKG_PROXY_PORT} NATIVE_JOBS=${JOBS}"
	if [ -n "$GUEST_DISPLAY" ]; then
		guest_env="$guest_env DISPLAY=$GUEST_DISPLAY XAUTHORITY=$GUEST_XAUTHORITY"
	fi

	SSH_TIMEOUT=14400 ssh_illumos "cd /var/tmp/freebasic-build && gzip -dc source.tar.gz | tar xf - && $guest_env bash build_scripts/illumos-build-freebasic.sh" \
		2>&1 | tee "$LOG_DIR/build.log"

	msg "Collecting package repository"
	rm -rf "$ARCHIVE_DIR"
	mkdir -p "$ARCHIVE_DIR" "$LOG_DIR"
	scp_to_illumos -r root@127.0.0.1:/var/tmp/freebasic-build/out/illumos/\* "$ARCHIVE_DIR/"
	cp "$LOG_DIR/build.log" "$ARCHIVE_DIR/build.log"
}

cleanup_vm() {
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

	make_ssh_key "$RUN_DIR/id_ed25519"
	download_image
	local disk
	local iso
	disk="$(prepare_disk)"
	iso="$(write_seed_iso "$RUN_DIR/id_ed25519")"

	trap cleanup_vm EXIT
	start_pkg_proxy
	start_vm "$disk" "$iso"
	wait_for_ssh
	start_x11_forward
	pack_source
	run_guest_build
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
