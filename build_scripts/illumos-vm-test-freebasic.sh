#!/usr/bin/env bash

##############################################################################
# FreeBASIC illumos VM test runner
##############################################################################
#
# Purpose:
#
#   Boot an illumos VM image and run FreeBASIC smoke checks, fbctests, and
#   exampleageddon using a package repository prepared by illumos-build scripts.
#
# Responsibilities:
#
#   * launch a VM with the user-supplied or downloaded OpenIndiana cloud image
#   * copy prepared package repository and source trees into the guest
#   * install the local freebasic IPS package
#   * execute fbctests and exampleageddon in the guest
#
# This script intentionally does not contain:
#
#   * illumos native build/package creation
#   * host image cache management outside this script
#   * full runtime device setup beyond test needs
#
##############################################################################

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Environment
##############################################################################

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORKROOT="$ROOT/out/illumos-vm"
RUN_DIR="$WORKROOT/run"
LOG_DIR="$WORKROOT/logs"
CACHE_DIR="$WORKROOT/cache"
PACKAGE_DIR=""
RELEASE="20251026"
PKG_PROXY_PORT=""
PKG_PROXY_ENABLED=1
PKG_REPO_URL="http://mirror.math.princeton.edu/pub/openindiana/hipster/publisher/openindiana.org"
PKG_REPO_FALLBACK_URL="http://mirror.math.princeton.edu/pub/openindiana/hipster/publisher/openindiana.org"
PKG_INSTALL_ATTEMPTS=3
PKG_INSTALL_RETRY_DELAY=6
IMAGE_URL=""
IMAGE_FILE=""
WORKROOT_PACKAGE="$ROOT/out/illumos/x86-64"
ARCHIVE_RESULTS="$RUN_DIR/results"
OFFLINE_TEST_MODE=0
SSH_PORT=""
CPUS=""
MEMORY="6144"
DISK_SIZE="32G"
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"
FBCTESTS_JOBS=""
FBCTESTS_UNIT_ARGS=""
FBCTESTS_FOCUSED_BMK=""
RUN_FBCTESTS=1
RUN_EXAMPLEAGEDDON=1
EXAMPLEAGEDDON_JOBS=""
EXAMPLEAGEDDON_COMPILE_TIMEOUT="180"
EXAMPLEAGEDDON_RUN_TIMEOUT="10"
PKG_INSTALL_TIMEOUT="1200"
SSH_TIMEOUT="${SSH_TIMEOUT:-7200}"
SKIP_PACKAGE_INSTALL=0
KEEP_VM=0
WORK_ROOT_DIR="/work/freebasic-test"
PKG_CACHE_DIR="/work/freebasic-package"
SERIAL_TTY_HOST="127.0.0.1"
SERIAL_TTY_PORT=""

DEFAULT_IMAGE_URL="https://dlc.openindiana.org/isos/hipster/${RELEASE}/OI-hipster-cloudimage.img.zst"

msg() { printf '\n==> %s\n' "$*"; }
die() { printf '\nERROR: %s\n' "$*" >&2; exit 1; }

usage() {
	cat <<EOF
Usage: ./build_scripts/illumos-vm-test-freebasic.sh [options]

Options:
  --package-dir DIR     Package artifact path. Default: out/illumos/x86-64
  --image-url URL       OpenIndiana cloud image URL.
  --image FILE          Existing OpenIndiana cloud image.
  --release N           OpenIndiana release/date directory override. Default: ${RELEASE}
  --pkg-proxy-port N    Host package proxy port. Default: auto-selected.
  --pkg-repo-url URL    Base OpenIndiana publisher URL for direct mode.
  --pkg-repo-fallback-url URL Optional fallback package URL for direct mode.
  --workroot DIR        Work directory. Default: out/illumos-vm
  --jobs N              Build/test concurrency hint. Default: host CPU count
  --cpus N              QEMU CPU count. Default: --jobs value
  --memory MB           VM memory in MB. Default: 6144
  --disk-size SIZE      Resized VM disk size. Default: 32G
  --ssh-port N          Host SSH forward port. Default: auto
  --fbctests-jobs N     fbctests job count. Default: --jobs value
  --fbctests-unit-args S Extra UNITTEST_RUN_ARGS for fbctests.
  --fbctests-focused BMK Run one multi-module .bmk test instead of all fbctests.
  --skip-fbctests       Skip fbctests for a resumed Exampleageddon run.
  --skip-exampleageddon Skip Exampleageddon for a focused diagnostic run.
  --exampleageddon-jobs N  exampleageddon job count. Default: --jobs value
  --exampleageddon-compile-timeout N Example compile timeout.
  --exampleageddon-run-timeout N Example run timeout.
  --skip-package-install Skip package installation in guest.
  --pkg-install-timeout N  Seconds per package install attempt. Default: 1200.
  --pkg-install-attempts N  Number of attempts per package install. Default: 3.
  --pkg-install-retry-delay N  Delay in seconds between package retries. Default: 6.
  --offline-test-mode     Use only local package repo for install and skip remote publisher refresh.
  --keep-vm             Keep VM artifacts after success.
  -h, --help            Show this help text.
EOF
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--package-dir) PACKAGE_DIR="$2"; shift 2 ;;
		--image-url) IMAGE_URL="$2"; shift 2 ;;
		--image) IMAGE_FILE="$2"; shift 2 ;;
		--release) RELEASE="$2"; DEFAULT_IMAGE_URL="https://dlc.openindiana.org/isos/hipster/${RELEASE}/OI-hipster-cloudimage.img.zst"; shift 2 ;;
		--pkg-proxy-port)
			PKG_PROXY_ENABLED=1
			PKG_PROXY_PORT="$2"
			shift 2
			;;
		--pkg-repo-url) PKG_REPO_URL="$2"; shift 2 ;;
		--pkg-repo-fallback-url) PKG_REPO_FALLBACK_URL="$2"; shift 2 ;;
		--workroot)
			WORKROOT="$2"
			RUN_DIR="$WORKROOT/run"
			LOG_DIR="$WORKROOT/logs"
			CACHE_DIR="$WORKROOT/cache"
			ARCHIVE_RESULTS="$WORKROOT/results"
			shift 2
			;;
		--jobs) JOBS="$2"; shift 2 ;;
		--cpus) CPUS="$2"; shift 2 ;;
		--memory) MEMORY="$2"; shift 2 ;;
		--disk-size) DISK_SIZE="$2"; shift 2 ;;
		--ssh-port) SSH_PORT="$2"; shift 2 ;;
		--fbctests-jobs) FBCTESTS_JOBS="$2"; shift 2 ;;
		--fbctests-unit-args) FBCTESTS_UNIT_ARGS="$2"; shift 2 ;;
		--fbctests-focused) FBCTESTS_FOCUSED_BMK="$2"; shift 2 ;;
		--skip-fbctests) RUN_FBCTESTS=0; shift ;;
		--skip-exampleageddon) RUN_EXAMPLEAGEDDON=0; shift ;;
		--exampleageddon-jobs) EXAMPLEAGEDDON_JOBS="$2"; shift 2 ;;
		--exampleageddon-compile-timeout) EXAMPLEAGEDDON_COMPILE_TIMEOUT="$2"; shift 2 ;;
		--exampleageddon-run-timeout) EXAMPLEAGEDDON_RUN_TIMEOUT="$2"; shift 2 ;;
		--pkg-install-attempts) PKG_INSTALL_ATTEMPTS="$2"; shift 2 ;;
		--pkg-install-retry-delay) PKG_INSTALL_RETRY_DELAY="$2"; shift 2 ;;
		--pkg-install-timeout) PKG_INSTALL_TIMEOUT="$2"; shift 2 ;;
		--offline-test-mode) OFFLINE_TEST_MODE=1; shift ;;
		--skip-package-install) SKIP_PACKAGE_INSTALL=1; shift ;;
		--keep-vm) KEEP_VM=1; shift ;;
		-h|--help)
			usage
			exit 0
			;;
		*) die "unknown option: $1" ;;
	esac
done

[ -d "$WORKROOT" ] || mkdir -p "$WORKROOT"
mkdir -p "$RUN_DIR" "$LOG_DIR" "$CACHE_DIR" "$ARCHIVE_RESULTS"

[ "$JOBS" -gt 0 ] 2>/dev/null || die "--jobs must be a positive integer"
[ -n "$CPUS" ] || CPUS="$JOBS"
[ "$CPUS" -gt 0 ] 2>/dev/null || die "--cpus must be a positive integer"

if [ -n "$FBCTESTS_FOCUSED_BMK" ]; then
	case "$FBCTESTS_FOCUSED_BMK" in
	/*|*..*|*[!A-Za-z0-9_./-]*)
		die "--fbctests-focused must name a relative .bmk file under tests"
		;;
	esac
	case "$FBCTESTS_FOCUSED_BMK" in
	*.bmk) ;;
	*) die "--fbctests-focused must name a .bmk file" ;;
	esac
	RUN_FBCTESTS=1
fi

if [ -z "$PACKAGE_DIR" ]; then
	PACKAGE_DIR="$WORKROOT_PACKAGE"
fi

if [ -n "$PACKAGE_DIR" ] && [ ! -d "$PACKAGE_DIR" ]; then
	die "package directory does not exist: $PACKAGE_DIR"
fi

[ -f "$ROOT/mk/version.mk" ] && [ -f "$ROOT/GNUmakefile" ] || die "could not locate project root at $ROOT"

require_tool() {
	command -v "$1" >/dev/null 2>&1 || die "required host tool not found: $1"
}

require_tool bash
require_tool curl
require_tool find
require_tool python3
require_tool qemu-img
require_tool qemu-system-x86_64
require_tool sha256sum
require_tool scp
require_tool ssh
require_tool timeout
require_tool xorriso

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

if [ -z "$SSH_PORT" ]; then
	SSH_PORT="$(find_free_port 2222)"
fi

download_url() {
	local url="$1"
	local dest="$2"
	local tmp="${dest}.tmp"

	rm -f "$tmp"
	if curl -fL --retry 3 --retry-delay 5 -o "$tmp" "$url"; then
		mv "$tmp" "$dest"
		return 0
	fi
	return 1
}

prepare_image() {
	if [ -n "$IMAGE_FILE" ]; then
		[ -f "$IMAGE_FILE" ] || die "image not found: $IMAGE_FILE"
		IMAGE_FILE="$(cd "$(dirname "$IMAGE_FILE")" && pwd)/$(basename "$IMAGE_FILE")"
		IMAGE_FILE="$(decompress_image_if_needed "$IMAGE_FILE")"
		return
	fi

	[ -n "$IMAGE_URL" ] || IMAGE_URL="$DEFAULT_IMAGE_URL"
	IMAGE_FILE="$CACHE_DIR/$(basename "$IMAGE_URL")"
	if [ -f "$IMAGE_FILE" ]; then
		IMAGE_FILE="$(decompress_image_if_needed "$IMAGE_FILE")"
		return
	fi

	download_url "$IMAGE_URL" "$IMAGE_FILE" ||
		die "failed to download $IMAGE_URL"
	IMAGE_FILE="$(decompress_image_if_needed "$IMAGE_FILE")"
}

decompress_image_if_needed() {
	local image="$1"
	local decompressed

	case "$image" in
		*.zst)
			decompressed="${image%.zst}"
			if [ -f "$decompressed" ] && [ "$decompressed" -nt "$image" ]; then
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
	echo "$iso"
}

start_vm() {
	local disk="$1"
	local iso="$2"
	local pidfile="$RUN_DIR/qemu.pid"

	if [ -z "$SERIAL_TTY_PORT" ]; then
		SERIAL_TTY_PORT="$(find_free_port 4300)"
	fi

	msg "starting illumos VM on SSH port $SSH_PORT"
	qemu-system-x86_64 \
		-m "$MEMORY" \
		-smp "$CPUS" \
		-machine accel=kvm:tcg \
		-cpu max \
		-drive "file=$disk,if=virtio,format=qcow2" \
		-drive "file=$iso,media=cdrom,if=ide,readonly=on" \
		-netdev "user,id=net0,hostfwd=tcp:127.0.0.1:${SSH_PORT}-:22" \
		-device virtio-net-pci,netdev=net0 \
		-display none \
		-chardev "socket,id=serial_ttya,host=$SERIAL_TTY_HOST,port=$SERIAL_TTY_PORT,server=on,wait=off" \
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

	msg "Starting OpenIndiana package proxy on port $PKG_PROXY_PORT"
	cat > "$script" <<'PY'
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
import http.client
import sys

UPSTREAM = "mirror.math.princeton.edu"
LEGACY_DIRECTORIES = {"index", "file", "pkg", "tmp", "trans", "catalog"}
VERSION_PATHS = {
    "/pub/openindiana/hipster/versions/0",
    "/pub/openindiana/hipster/versions/0/",
    "/pub/openindiana/hipster/publisher/openindiana.org/versions/0",
    "/pub/openindiana/hipster/publisher/openindiana.org/versions/0/",
}

VERSIONS_RESPONSE = """pkg-server 0
catalog 0
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

def rewrite_versioned_section(path):
    if path.startswith("/pub/openindiana/hipster/"):
        source_base = "/pub/openindiana/hipster"
        target_base = "/pub/openindiana/hipster/publisher/openindiana.org"
    elif path.startswith("/pub/openindiana/hipster/publisher/openindiana.org/"):
        source_base = "/pub/openindiana/hipster/publisher/openindiana.org"
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

        conn = http.client.HTTPConnection(UPSTREAM, 80, timeout=120)
        headers = {
            k: v for k, v in self.headers.items()
            if k.lower() not in ("host", "connection", "proxy-connection")
        }
        headers["Host"] = UPSTREAM

        try:
            conn.request(self.command, path, body=body, headers=headers)
            resp = conn.getresponse()
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
            conn.close()

    def log_message(self, fmt, *args):
        sys.stderr.write("%s %s\n" % (self.address_string(), fmt % args))


PORT = int(sys.argv[1])
server = ThreadingHTTPServer(("0.0.0.0", PORT), Proxy)
server.daemon_threads = True
server.request_queue_size = 128
print("pkg proxy listening on 0.0.0.0:%d" % PORT, flush=True)
server.serve_forever()
PY

	python3 -u "$script" "$PKG_PROXY_PORT" > "$LOG_DIR/pkg-proxy.log" 2>&1 &
	echo "$!" > "$pidfile"
	sleep 1
	kill -0 "$(cat "$pidfile")" >/dev/null 2>&1 ||
		die "package proxy failed to start; see $LOG_DIR/pkg-proxy.log"
}

stop_pkg_proxy() {
	if [ -f "$RUN_DIR/pkg_proxy.pid" ]; then
		kill "$(cat "$RUN_DIR/pkg_proxy.pid")" 2>/dev/null || true
		rm -f "$RUN_DIR/pkg_proxy.pid"
	fi
}

cleanup_vm() {
	stop_pkg_proxy
	if [ "$KEEP_VM" -eq 1 ]; then
		return 0
	fi

	if [ -f "$RUN_DIR/qemu.pid" ]; then
		pid="$(cat "$RUN_DIR/qemu.pid")"
		kill "$pid" 2>/dev/null || true
		rm -f "$RUN_DIR/qemu.pid"
	fi
}

wait_for_ssh() {
	local tries=90
	local bootstrap_done=0
	local bootstrap_attempted=0
	local attempts_before_bootstrap=88

	while [ "$tries" -gt 0 ]; do
		if timeout 5 ssh -o BatchMode=yes -o StrictHostKeyChecking=no \
			-o UserKnownHostsFile=/dev/null -o ConnectTimeout=5 \
			-i "$RUN_DIR/id_ed25519" -p "$SSH_PORT" root@127.0.0.1 'uname -a' >/dev/null 2>&1; then
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
	die "timed out waiting for illumos SSH"
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

	python3 - "$SERIAL_TTY_HOST" "$SERIAL_TTY_PORT" "$key_b64" "$LOG_DIR/serial.log" "__FBC_TTYA_BOOTSTRAP_DONE__" <<'PY'
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
	sock.sendall(text.encode("utf-8"))


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
				"$ ",
				"#",
				"$"
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

if command -v svcadm >/dev/null 2>&1; then
    if /usr/sbin/svcadm enable -r svc:/network/ssh:default >/dev/null 2>&1; then
        :
    elif /usr/sbin/svcadm restart svc:/network/ssh:default >/dev/null 2>&1; then
        :
    else
        :
    fi
fi
BOOTSTRAP_INTERFACES=$(
    dladm show-phys -p -o LINK 2>/dev/null \
        | awk '$1 != "" && $1 != "lo0" {{ print $1 }}'
)
if [ -z "$BOOTSTRAP_INTERFACES" ]; then
    BOOTSTRAP_INTERFACES=$(
        ifconfig -a 2>/dev/null \
            | awk -F: '/^[a-zA-Z][a-zA-Z0-9_-]*:/{{print $1}}' \
            | grep -Ev '^lo0$'
    )
fi
if [ -n "$BOOTSTRAP_INTERFACES" ]; then
    for bootstrap_iface in $BOOTSTRAP_INTERFACES; do
        ifconfig "$bootstrap_iface" plumb >/dev/null 2>&1 || true
        ifconfig "$bootstrap_iface" up >/dev/null 2>&1 || true
        ifconfig "$bootstrap_iface" inet 10.0.2.15 netmask 255.255.255.0 up >/dev/null 2>&1 || true
        break
    done
    route -p add net default 10.0.2.2 >/dev/null 2>&1 || true
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
			prompt_wait = wait_for(sock, handle, ["#", "$", "# ", "$ "], 120)
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

scp_from_illumos() {
	timeout 300 scp -o StrictHostKeyChecking=no \
		-o UserKnownHostsFile=/dev/null \
		-o BatchMode=yes \
		-o NumberOfPasswordPrompts=0 \
		-i "$RUN_DIR/id_ed25519" \
		-P "$SSH_PORT" \
		"$@"
}

copy_if_exists_in_guest() {
	local remote_path="$1"
	local local_path="$2"

	if ssh_illumos "[ -f \"$remote_path\" ]" >/dev/null 2>&1; then
		scp_from_illumos "root@127.0.0.1:$remote_path" "$local_path"
	fi
}

resolve_repo_dir() {
	local candidate="$1"
	local found

	[ -n "$candidate" ] || return 1
	if [ -d "$candidate/repo" ]; then
		echo "$candidate/repo"
		return 0
	fi
	if [ "$(basename "$candidate")" = "repo" ] && [ -d "$candidate" ]; then
		echo "$candidate"
		return 0
	fi
	found="$(find "$candidate" -type d -name repo 2>/dev/null | head -n 1 || true)"
	if [ -n "$found" ]; then
		echo "$found"
		return 0
	fi
	return 1
}

make_ssh_key() {
	local key="$RUN_DIR/id_ed25519"
	local pub="$key.pub"
	local key_info

	if [ -f "$key" ] && [ -f "$pub" ]; then
		key_info="$(ssh-keygen -l -f "$pub" 2>/dev/null || true)"
		if [ -n "$key_info" ] && echo "$key_info" | grep -q ' ED25519 '; then
			rm -f "$key" "$pub"
		fi
	fi

	if [ ! -f "$key" ]; then
		# Current OpenIndiana cloud images do not accept Ed25519 login keys.
		ssh-keygen -q -t rsa -b 3072 -N '' -f "$key"
	fi
}

send_test_payload() {
	local repo_dir="$1"

	msg "preparing guest test workspace"
	ssh_illumos "rm -rf '$WORK_ROOT_DIR' '$PKG_CACHE_DIR' && mkdir -p '$WORK_ROOT_DIR/src' '$PKG_CACHE_DIR' '$PKG_CACHE_DIR/repo'"

	if [ "$SKIP_PACKAGE_INSTALL" -eq 0 ] && [ -n "$repo_dir" ] && [ -d "$repo_dir" ]; then
		msg "copying package repository into VM"
		scp_to_illumos -r "$repo_dir" "root@127.0.0.1:$PKG_CACHE_DIR/"
		ssh_illumos "rm -f '$PKG_CACHE_DIR/repo/publisher/local/tmp/lock'"
	fi

	scp_to_illumos -r "$ROOT/tests" "root@127.0.0.1:$WORK_ROOT_DIR/"
	scp_to_illumos -r "$ROOT/inc" "root@127.0.0.1:$WORK_ROOT_DIR/"
	scp_to_illumos -r "$ROOT/src/sfxlib" "root@127.0.0.1:$WORK_ROOT_DIR/src/"
	scp_to_illumos -r "$ROOT/examples" "root@127.0.0.1:$WORK_ROOT_DIR/"
	scp_to_illumos "$ROOT/build_scripts/exampleageddon-freebasic.py" "root@127.0.0.1:$WORK_ROOT_DIR/exampleageddon-freebasic.py"
}

write_remote_runner() {
	cat > "$RUN_DIR/illumos-vm-test-runner.sh" <<'EOF'
#!/usr/bin/env bash
set -e

run() { echo "==> $*"; "$@"; }
fail() { echo "ERROR: $*" >&2; exit 1; }
PKG_INSTALL_HAD_TIMEOUT=0

find_cmd_or_path() {
	local f

	for f in "$@"; do
		command -v "$f" >/dev/null 2>&1 && {
			echo "$f"
			return 0
		}
	done
	for f in "$@"; do
		if [ -x "$f" ]; then
			echo "$f"
			return 0
		fi
	done
	return 1
}

resolve_timeout() {
	find_cmd_or_path timeout /usr/gnu/bin/timeout
}

run_with_timeout() {
	local seconds="$1"
	local timeout_cmd
	shift

	timeout_cmd="$(resolve_timeout)" || {
		"$@"
		return
	}
	"$timeout_cmd" "$seconds" "$@"
}

prepare_xargs() {
	local wrapper_dir="/var/tmp/freebasic-test-tool-wrappers"

	if printf '' | /usr/bin/xargs -r true >/dev/null 2>&1; then
		return 0
	fi

	mkdir -p "$wrapper_dir"
	cat > "$wrapper_dir/xargs" <<'XARGS_WRAPPER'
#!/usr/bin/env bash
no_run_if_empty=0
filtered=()

for arg in "$@"; do
	case "$arg" in
		-r|--no-run-if-empty) no_run_if_empty=1 ;;
		*) filtered+=("$arg") ;;
	esac
done

tmp="${TMPDIR:-/tmp}/freebasic-test-xargs.$$"
trap 'rm -f "$tmp"' EXIT
cat > "$tmp"

if [ "$no_run_if_empty" -eq 1 ] && [ ! -s "$tmp" ]; then
	exit 0
fi

exec /usr/bin/xargs "${filtered[@]}" < "$tmp"
XARGS_WRAPPER
	chmod +x "$wrapper_dir/xargs"
	export PATH="$wrapper_dir:$PATH"
}

resolve_make() {
	local make
	local resolved

	for make in \
		"${FBC_GMAKE:-}" \
		/var/tmp/freebasic-gnu-make/bin/make \
		/usr/gnu/bin/gmake \
		/usr/gnu/bin/make \
		gmake \
		make
	do
		[ -n "$make" ] || continue
		resolved="$(command -v "$make" 2>/dev/null)" || continue
		"$resolved" --version 2>/dev/null | grep -q '^GNU Make ' || continue
		echo "$resolved"
		return 0
	done
	return 1
}

grow_root_pool() {
	local disk
	local pool
	local short_vdev
	local vdev

	pool="$(zpool list -H -o name 2>/dev/null | sed -n '1p')" || return 0
	[ -n "$pool" ] || return 0

	echo "==> expanding root pool if the VM disk is larger than the image"
	zpool set autoexpand=on "$pool" >/dev/null 2>&1 || true

	zpool status -P "$pool" 2>/dev/null |
		awk '
			$1 ~ /^\/dev\/dsk\// { print $1; next }
			$1 ~ /^c[0-9].*[sp][0-9]+$/ { print $1; next }
		' |
		while read -r vdev; do
			[ -n "$vdev" ] || continue

			short_vdev="${vdev#/dev/dsk/}"
			disk="$short_vdev"
			case "$short_vdev" in
			*s[0-9]) disk="${short_vdev%s[0-9]}" ;;
			*p[0-9]) disk="${short_vdev%p[0-9]}" ;;
			esac

			if [ -n "$disk" ] && command -v format >/dev/null 2>&1; then
				printf 'partition\nexpand\nlabel\ny\nquit\nquit\n' |
					format -e "$disk" >/tmp/freebasic-format-grow.log 2>&1 || true
			fi

			zpool online -e "$pool" "$vdev" >/tmp/freebasic-zpool-grow.log 2>&1 ||
				zpool online -e "$pool" "$short_vdev" >>/tmp/freebasic-zpool-grow.log 2>&1 ||
				true
		done

	df -h / || true
}

bootstrap_gnu_make() {
	local prefix="/var/tmp/freebasic-gnu-make"
	local srcdir
	local tarball="/var/tmp/make-4.4.1.tar.gz"
	local workdir="/var/tmp/freebasic-gmake-bootstrap.$$"

	if resolve_make >/dev/null 2>&1; then
		return 0
	fi

	echo "==> bootstrapping GNU Make 4.4.1"
	rm -f "$tarball"
	if command -v curl >/dev/null 2>&1; then
		curl -fL --retry 3 --connect-timeout 20 --max-time 600 \
			-o "$tarball" http://ftp.gnu.org/gnu/make/make-4.4.1.tar.gz
	elif command -v wget >/dev/null 2>&1; then
		wget -O "$tarball" http://ftp.gnu.org/gnu/make/make-4.4.1.tar.gz
	else
		python3 - "$tarball" <<'PY_MAKE'
import sys
from urllib.request import urlopen

with urlopen("http://ftp.gnu.org/gnu/make/make-4.4.1.tar.gz", timeout=600) as source:
    with open(sys.argv[1], "wb") as destination:
        destination.write(source.read())
PY_MAKE
	fi

	tar -tzf "$tarball" >/dev/null 2>&1 || fail "downloaded GNU Make archive is invalid"
	rm -rf "$workdir"
	mkdir -p "$workdir" "$prefix"
	tar -xzf "$tarball" -C "$workdir"
	srcdir="$workdir/make-4.4.1"
	[ -d "$srcdir" ] || fail "GNU Make source directory was not extracted"
	(
		cd "$srcdir"
		./configure --prefix="$prefix" CC="$CC"
		./build.sh
		./make install
	)
	rm -rf "$workdir"
	resolve_make >/dev/null 2>&1 || fail "bootstrapped GNU Make did not pass validation"
}

install_pkg_candidates() {
	local pkg
	local timeout_s
	local timeout_cmd
	local rc
	local log
	local had_timeout
	local attempts
	local delay_s
	local attempt
	local pkg_timeouted
	log="/tmp/illumos-vm-pkg-install.log"
	had_timeout=0

	case "${PKG_INSTALL_TIMEOUT:-1200}" in
		''|*[!0-9]*|0) timeout_s="" ;;
		*) timeout_s="$PKG_INSTALL_TIMEOUT" ;;
	esac
	timeout_cmd="$(resolve_timeout 2>/dev/null || true)"
	[ -n "$timeout_cmd" ] || timeout_s=""

	case "${PKG_INSTALL_ATTEMPTS:-3}" in
		''|*[!0-9]*|0) attempts=3 ;;
		*) attempts="$PKG_INSTALL_ATTEMPTS" ;;
	esac
	case "${PKG_INSTALL_RETRY_DELAY:-6}" in
		''|*[!0-9]*|0) delay_s=6 ;;
		*) delay_s="$PKG_INSTALL_RETRY_DELAY" ;;
	esac
	for pkg in "$@"; do
		attempt=1
		while [ "$attempt" -le "$attempts" ]; do
			: > "$log"
			pkg_timeouted=0
			if [ -n "$timeout_s" ]; then
				"$timeout_cmd" "$timeout_s" /usr/bin/pkg install --accept --no-refresh "$pkg" > "$log" 2>&1
				rc=$?
			else
				/usr/bin/pkg install --accept --no-refresh "$pkg" > "$log" 2>&1
				rc=$?
			fi
			if [ "$rc" -eq 0 ]; then
				PKG_INSTALL_HAD_TIMEOUT=0
				return 0
			fi
			if [ -s "$log" ]; then
				if grep -q 'E_OPERATION_TIMEOUTED' "$log"; then
					pkg_timeouted=1
					had_timeout=1
				fi
				echo "ERROR: pkg install failed for $pkg (exit=$rc, attempt $attempt/$attempts)" >&2
				if [ -n "$timeout_s" ] && [ "$rc" -eq 124 ]; then
					echo "       install timed out after ${timeout_s}s; pass --pkg-install-timeout with a larger value" >&2
				fi
				echo "       --- install output tail for $pkg ---" >&2
				tail -n 20 "$log" >&2
				echo "       --- end tail ---" >&2
			else
				echo "ERROR: pkg install failed for $pkg (exit=$rc, attempt $attempt/$attempts)" >&2
			fi
			if [ "$pkg_timeouted" -eq 0 ] || [ "$attempt" -eq "$attempts" ]; then
				break
			fi
			sleep "$delay_s"
			attempt=$((attempt + 1))
		done
	done
	if [ "$had_timeout" -ne 0 ]; then
		PKG_INSTALL_HAD_TIMEOUT=1
	else
		PKG_INSTALL_HAD_TIMEOUT=0
	fi
	return 1
}

configure_openindiana_publishers() {
	local mode="$1"
	local base_url="$2"
	local normalized_base
	local direct_url

	case "$mode" in
		proxy)
			if [ -z "${PKG_PROXY_PORT:-}" ]; then
				fail "pkg proxy requested but no PKG_PROXY_PORT provided"
			fi
			run /usr/bin/pkg set-publisher --no-refresh -M '*' -O "http://10.0.2.2:${PKG_PROXY_PORT}/pub/openindiana/hipster" openindiana.org
			if ! run_with_timeout 90 /usr/bin/pkg refresh openindiana.org; then
				return 1
			fi
			;;
		direct)
			if [ -z "$base_url" ]; then
				fail "no package repository URL configured for direct mode"
			fi
			normalized_base="${base_url%/}"
			direct_url="$normalized_base"
			if ! /usr/bin/pkg set-publisher --no-refresh -M '*' -O "$direct_url" openindiana.org; then
				return 1
			fi
			if ! run_with_timeout 90 /usr/bin/pkg refresh openindiana.org; then
				return 1
			fi
			;;
		*)
			fail "unknown publisher mode: $mode"
			;;
	esac
	return 0
}

configure_openindiana_publishers_with_fallback() {
	local allow_proxy="${2:-1}"

	if [ "$allow_proxy" -eq 1 ] && [ -n "${PKG_PROXY_PORT:-}" ]; then
		if configure_openindiana_publishers proxy; then
			return 0
		fi
		echo "==> local package proxy unreachable, trying direct repositories" >&2
	fi

	if configure_openindiana_publishers direct "${PKG_REPO_URL}"; then
		return 0
	fi

	if [ -n "$PKG_REPO_FALLBACK_URL" ] && [ "$PKG_REPO_FALLBACK_URL" != "$PKG_REPO_URL" ]; then
		if configure_openindiana_publishers direct "$PKG_REPO_FALLBACK_URL"; then
			return 0
		fi
	fi

	return 1
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

run_socket_smoke() {
	echo "==> compiling crt/sys/socket.bi API smoke"
	run "$FBC_BIN" /work/freebasic-test/tests/crt/socket.bas -x /work/freebasic-test/socket-bi-smoke

	echo "==> running crt/sys/socket.bi API smoke"
	run /work/freebasic-test/socket-bi-smoke

	echo "==> compiling curses.bi API smoke"
	run "$FBC_BIN" /work/freebasic-test/tests/crt/curses.bas -x /work/freebasic-test/curses-bi-smoke

	echo "==> running curses.bi API smoke"
	run /work/freebasic-test/curses-bi-smoke

	echo "==> compiling TCP loopback smoke"
	run "$FBC_BIN" -mt /work/freebasic-test/tests/file/tcp.bas -x /work/freebasic-test/tcp-smoke

	echo "==> running TCP loopback smoke"
	if [ -x /usr/gnu/bin/timeout ]; then
		run /usr/gnu/bin/timeout 60 /work/freebasic-test/tcp-smoke
	else
		run /work/freebasic-test/tcp-smoke
	fi
}

run_fbctests() {
	local jobs
	local make
	jobs="$(fbctests_jobs)"
	make="$(resolve_make)" || fail "missing gmake/make"
	prepare_xargs

	cd /work/freebasic-test/tests
	run "$make" clean FBC="$FBC_BIN"
	run "$make" check FBC="$FBC_BIN"
	run "$make" -j "$jobs" unit-tests FBC="$FBC_BIN" UNITTEST_RUN_ARGS="${FBCTESTS_UNIT_ARGS:-}"
	run "$make" -j "$jobs" log-tests FBC="$FBC_BIN"

	for failed_log in failed-fb.log failed-fblite.log failed-qb.log failed-deprecated.log; do
		[ -f "$failed_log" ] || fail "missing log-tests summary: $failed_log"
		if ! grep -qi 'None Found' "$failed_log"; then
			cat "$failed_log"
			fail "log-tests reported failures in $failed_log"
		fi
	done
}

run_focused_fbctest() {
	local bmk
	local make

	bmk="$FBCTESTS_FOCUSED_BMK"
	make="$(resolve_make)" || fail "missing gmake/make"
	[ -f "/work/freebasic-test/tests/$bmk" ] ||
		fail "focused fbctest does not exist: $bmk"

	prepare_xargs
	cd /work/freebasic-test/tests
	run "$make" -f bmk-make.mk clean \
		BMK="$bmk" \
		TEST_MODE=MULTI_MODULE_OK
	run "$make" -f bmk-make.mk \
		BMK="$bmk" \
		TEST_MODE=MULTI_MODULE_OK \
		FB_LANG=fb \
		FBC="$FBC_BIN" \
		GCC="${CC:-gcc}"
}

run_exampleageddon() {
	local jobs
	local compile_timeout
	local run_timeout

	jobs="$(exampleageddon_jobs)"

	case "${EXAMPLEAGEDDON_COMPILE_TIMEOUT:-}" in
		''|*[!0-9]*|0) compile_timeout=180 ;;
		*) compile_timeout="${EXAMPLEAGEDDON_COMPILE_TIMEOUT}" ;;
	esac
	case "${EXAMPLEAGEDDON_RUN_TIMEOUT:-}" in
		''|*[!0-9]*|0) run_timeout=10 ;;
		*) run_timeout="${EXAMPLEAGEDDON_RUN_TIMEOUT}" ;;
	esac

	rm -rf /work/freebasic-test/exampleageddon
	mkdir -p /work/freebasic-test/exampleageddon

	run python3 /work/freebasic-test/exampleageddon-freebasic.py \
		--root /work/freebasic-test \
		--outdir /work/freebasic-test/exampleageddon \
		--prefix /usr/local \
		--include-dir /work/freebasic-test/inc \
		--fbc "$FBC_BIN" \
		--jobs "$jobs" \
		--compile-timeout "$compile_timeout" \
		--run-timeout "$run_timeout" \
		--fail-on-self-contained

	[ -f /work/freebasic-test/exampleageddon/report.md ] || fail "exampleageddon report missing"
	[ -f /work/freebasic-test/exampleageddon/results.csv ] || fail "exampleageddon results missing"
	if ! grep -qx -- '- Self-contained problems: 0' /work/freebasic-test/exampleageddon/report.md; then
		cat /work/freebasic-test/exampleageddon/report.md
		fail "exampleageddon reported self-contained example problems"
	fi
}

prepare_tls_policy() {
	cat > /tmp/freebasic-illumos-openssl.cnf <<'TLS_CFG'
openssl_conf = openssl_init

[openssl_init]
ssl_conf = ssl_section

[ssl_section]
system_default = system_default_section

[system_default_section]
MaxProtocol = TLSv1.2
TLS_CFG

	export OPENSSL_CONF=/tmp/freebasic-illumos-openssl.cnf
}

run_main_tests() {
	prepare_tls_policy
	grow_root_pool

	echo "==> installing test dependencies"
	run echo "==> proxy port in guest: ${PKG_PROXY_PORT:-<unset>}"
	if [ "$OFFLINE_TEST_MODE" -eq 1 ]; then
		if [ -z "${PKG_REPO_DIR:-}" ] || [ ! -d "$PKG_REPO_DIR" ]; then
			fail "offline test mode requires a local package repository; set PACKAGE_DIR accordingly"
		fi
		run echo "==> offline mode: configuring local package repository only"
		run /usr/bin/pkg set-publisher --no-refresh -G '*' -M '*' -g "file://${PKG_REPO_DIR}" local
		run /usr/bin/pkg refresh local
	else
		if ! configure_openindiana_publishers_with_fallback 1; then
			fail "unable to configure OpenIndiana package repositories"
		fi
	fi

	if ! command -v python3 >/dev/null 2>&1; then
		if [ "$OFFLINE_TEST_MODE" -eq 1 ]; then
			fail "python3 missing and offline-test-mode disables remote package refresh installs"
		else
			install_pkg_candidates runtime/python-313 runtime/python-311 runtime/python-310 runtime/python-39 runtime/python-27 ||
				fail "python package unavailable in OpenIndiana repositories"
		fi
	fi
	if ! command -v make >/dev/null 2>&1; then
		if [ "$OFFLINE_TEST_MODE" -eq 1 ]; then
			fail "make missing and offline-test-mode disables remote package refresh installs"
		else
			install_pkg_candidates developer/build/make ||
				fail "make package unavailable in OpenIndiana repositories"
		fi
	fi
	if ! find_cmd_or_path gcc cc clang /usr/gnu/bin/gcc /usr/gnu/bin/cc /usr/bin/cc /usr/local/bin/cc /usr/bin/clang /usr/local/bin/clang >/dev/null 2>&1; then
		PKG_INSTALL_HAD_TIMEOUT=0
		if [ "$OFFLINE_TEST_MODE" -eq 1 ]; then
			echo "==> offline mode: skipping compiler package install attempts"
		elif install_pkg_candidates developer/gcc15 developer/gcc14 developer/gcc13 developer/gcc10; then
			echo "==> installed compiler from configured package repositories"
		else
			if [ "$PKG_INSTALL_HAD_TIMEOUT" -ne 0 ] && [ -n "${PKG_PROXY_PORT:-}" ]; then
				echo "==> compiler install timed out via local proxy, retrying with only direct repositories"
			else
				echo "==> compiler install unavailable from initial publisher, trying direct repositories"
			fi

			if ! configure_openindiana_publishers_with_fallback 0; then
				if ! configure_openindiana_publishers_with_fallback 1; then
					fail "unable to configure OpenIndiana package repositories for compiler install"
				fi
			fi

			install_pkg_candidates developer/gcc15 developer/gcc14 developer/gcc13 developer/gcc10 ||
				fail "no compiler package found from configured repository"
		fi
	fi

	CC_PATH="$(find_cmd_or_path gcc cc clang /usr/gnu/bin/gcc /usr/bin/gcc /usr/local/bin/gcc /usr/gnu/bin/clang /usr/bin/clang /usr/local/bin/clang /usr/gnu/bin/cc /usr/bin/cc /usr/local/bin/cc)"
	if [ -z "${CC_PATH:-}" ]; then
		fail "no compiler toolchain found after package/install attempts"
	fi
	export CC="$CC_PATH"
	case "$CC_PATH" in
		/*) export PATH="$(dirname "$CC_PATH"):$PATH" ;;
	esac
	echo "==> using CC=$CC"

	CXX_PATH="$(find_cmd_or_path g++ c++ clang++ /usr/gnu/bin/g++ /usr/bin/g++ /usr/local/bin/g++ /usr/gnu/bin/c++ /usr/bin/c++ /usr/gnu/bin/clang++ /usr/bin/clang++)"
	if [ -n "${CXX_PATH:-}" ]; then
		export CXX="$CXX_PATH"
		case "$CXX_PATH" in
			/*) export PATH="$(dirname "$CXX_PATH"):$PATH" ;;
		esac
		echo "==> using CXX=$CXX"
	fi

	bootstrap_gnu_make

	if ! command -v gmake >/dev/null 2>&1 && command -v make >/dev/null 2>&1; then
		export PATH="/usr/gnu/bin:$PATH"
	fi
	if [ -x /usr/gnu/bin/xargs ]; then
		export XARGS=/usr/gnu/bin/xargs
		export PATH="/usr/gnu/bin:$PATH"
	fi

	if [ -z "${FBC_BIN:-}" ]; then
		FBC_BIN="$(command -v fbc || true)"
		if [ -z "${FBC_BIN:-}" ] && [ -x /usr/local/bin/fbc ]; then
			FBC_BIN="/usr/local/bin/fbc"
		fi
	fi

	if [ "${SKIP_PACKAGE_INSTALL:-0}" -eq 0 ] && \
		[ -d "${PKG_REPO_DIR:-}" ]; then
		PKG_URI="file://${PKG_REPO_DIR}"
		FBVERSION="${FBVERSION:-1.20.1}"
		PKG_OSREL="${PKG_OSREL:-5.11}"
		PKG_REV="${PKG_REV:-1}"
		run /usr/bin/pkg set-publisher --no-refresh -G '*' -M '*' -g "$PKG_URI" local
		run /usr/bin/pkg refresh local
		if /usr/bin/pkg info lang/freebasic >/dev/null 2>&1; then
			run /usr/bin/pkg uninstall lang/freebasic
		fi
		install_pkg_candidates "pkg://local/lang/freebasic@${FBVERSION},${PKG_OSREL}-${PKG_REV}" "lang/freebasic" || fail "freebasic package unavailable in repositories"
		FBC_BIN="$(command -v fbc || true)"
		if [ -z "${FBC_BIN:-}" ] && [ -x /usr/local/bin/fbc ]; then
			FBC_BIN="/usr/local/bin/fbc"
		fi
		command -v "$FBC_BIN" >/dev/null 2>&1 || fail "fbc not installed after package install"
	fi
	command -v "$FBC_BIN" >/dev/null 2>&1 || fail "fbc not installed"

	run_socket_smoke
	if [ "${RUN_FBCTESTS:-1}" -eq 1 ]; then
		if [ -n "${FBCTESTS_FOCUSED_BMK:-}" ]; then
			run_focused_fbctest
		else
			run_fbctests
		fi
	else
		echo "==> fbctests skipped for resumed validation"
	fi
	if [ "${RUN_EXAMPLEAGEDDON:-1}" -eq 1 ]; then
		run_exampleageddon
	else
		echo "==> exampleageddon skipped for focused validation"
	fi
	echo "==> TEST PASSED"
}

run_main_tests
EOF
chmod +x "$RUN_DIR/illumos-vm-test-runner.sh"
}

run_test_in_guest() {
	local runner="/work/illumos-vm-test-runner.sh"
	local host_log="$ARCHIVE_RESULTS/freebasic-illumos-vm-test.log"
	local guest_log="/work/freebasic-illumos-vm-test.log"
	local escaped_unit_args
	local repo_dir="$1"
	local fbversion="${2:-1.20.1}"
	local pkg_rev="${3:-1}"
	local pkg_osrel="${4:-5.11}"
	local proxy_port="${5:-${PKG_PROXY_PORT}}"

	msg "copying guest test runner"
	scp_to_illumos "$RUN_DIR/illumos-vm-test-runner.sh" "root@127.0.0.1:$runner"

	msg "starting fbctests/exampleageddon run in guest"
	escaped_unit_args="$(printf '%q' "$FBCTESTS_UNIT_ARGS")"
	if ssh_illumos \
		"SKIP_PACKAGE_INSTALL=$SKIP_PACKAGE_INSTALL \
FBCTESTS_JOBS=${FBCTESTS_JOBS:-} \
FBCTESTS_UNIT_ARGS=$escaped_unit_args \
FBCTESTS_FOCUSED_BMK=$FBCTESTS_FOCUSED_BMK \
RUN_FBCTESTS=$RUN_FBCTESTS \
RUN_EXAMPLEAGEDDON=$RUN_EXAMPLEAGEDDON \
EXAMPLEAGEDDON_JOBS=${EXAMPLEAGEDDON_JOBS:-} \
EXAMPLEAGEDDON_COMPILE_TIMEOUT=$EXAMPLEAGEDDON_COMPILE_TIMEOUT \
EXAMPLEAGEDDON_RUN_TIMEOUT=$EXAMPLEAGEDDON_RUN_TIMEOUT \
FBVERSION=$fbversion \
PKG_OSREL=$pkg_osrel \
PKG_REV=$pkg_rev \
PKG_PROXY_PORT=$proxy_port \
OFFLINE_TEST_MODE=$OFFLINE_TEST_MODE \
PKG_INSTALL_TIMEOUT=$PKG_INSTALL_TIMEOUT \
PKG_INSTALL_ATTEMPTS=$PKG_INSTALL_ATTEMPTS \
PKG_INSTALL_RETRY_DELAY=$PKG_INSTALL_RETRY_DELAY \
PKG_REPO_URL=$PKG_REPO_URL \
PKG_REPO_FALLBACK_URL=$PKG_REPO_FALLBACK_URL \
PKG_REPO_DIR=$PKG_CACHE_DIR/repo \
bash $runner > '$guest_log' 2>&1"
	then
		msg "guest tests passed"
	else
		scp_from_illumos "root@127.0.0.1:$guest_log" "$host_log" || true
		copy_if_exists_in_guest /work/freebasic-test/exampleageddon/report.md "$ARCHIVE_RESULTS/exampleageddon-report.md"
		copy_if_exists_in_guest /work/freebasic-test/exampleageddon/results.csv "$ARCHIVE_RESULTS/exampleageddon-results.csv"
		tail -n 120 "$host_log" >&2 || true
		die "illumos VM test run failed"
	fi

	scp_from_illumos "root@127.0.0.1:$guest_log" "$host_log"
	copy_if_exists_in_guest /work/freebasic-test/exampleageddon/report.md "$ARCHIVE_RESULTS/exampleageddon-report.md"
	copy_if_exists_in_guest /work/freebasic-test/exampleageddon/results.csv "$ARCHIVE_RESULTS/exampleageddon-results.csv"
}

main() {
	local disk
	local iso
	local repo
	local fbversion
	local rev
	local pkg_osrel

	make_ssh_key
	prepare_image
	disk="$(prepare_disk)"
	iso="$(write_seed_iso "$RUN_DIR/id_ed25519")"
	if [ "$PKG_PROXY_ENABLED" -eq 1 ]; then
		start_pkg_proxy
	fi

	trap cleanup_vm EXIT
	start_vm "$disk" "$iso"
	wait_for_ssh

	repo=""
	fbversion="$(sed -n 's/^FBVERSION[[:space:]]*:[[:space:]]*=[[:space:]]*//p' "$ROOT/mk/version.mk")"
	rev="$(sed -n 's/^REV[[:space:]]*:[[:space:]]*=[[:space:]]*//p' "$ROOT/mk/version.mk")"
	[ -n "$fbversion" ] || fbversion="1.20.1"
	[ -n "$rev" ] || rev="1"
	if [ "$SKIP_PACKAGE_INSTALL" -eq 0 ] && [ -n "$PACKAGE_DIR" ]; then
		repo="$(resolve_repo_dir "$PACKAGE_DIR" || true)"
	fi
	pkg_osrel="5.11"
	if [ -n "$repo" ]; then
		pkg_osrel="$(echo "$repo" | sed -n 's#^.*/\([0-9][0-9]*\.[0-9][0-9]*\)/amd64/repo$#\1#p')"
	fi
	[ -n "$pkg_osrel" ] || pkg_osrel="5.11"
	send_test_payload "$repo"
	write_remote_runner
	run_test_in_guest "$repo" "$fbversion" "$rev" "$pkg_osrel" "$PKG_PROXY_PORT"

	msg "Tests complete"
	msg "Results: $ARCHIVE_RESULTS"

	if [ "$KEEP_VM" -eq 0 ]; then
		cleanup_vm
		rm -rf "$RUN_DIR"
	fi
}

main "$@"

# end of illumos-vm-test-freebasic.sh
