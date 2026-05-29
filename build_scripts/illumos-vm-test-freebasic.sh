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
#   * launch a VM with the user-supplied or downloaded OmniOS cloud image
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
PACKAGE_REPO_DIR=""
RELEASE="r151058"
PKG_PROXY_PORT=""
PKG_PROXY_ENABLED=0
PKG_REPO_URL="https://pkg.omnios.org"
PKG_REPO_FALLBACK_URL="http://pkg.omnios.org"
IMAGE_URL=""
IMAGE_FILE=""
WORKROOT_PACKAGE="$ROOT/out/illumos/x86-64"
ARCHIVE_RESULTS="$RUN_DIR/results"
SSH_PORT=""
CPUS=""
MEMORY="6144"
DISK_SIZE="32G"
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"
FBCTESTS_JOBS=""
FBCTESTS_UNIT_ARGS=""
EXAMPLEAGEDDON_JOBS=""
EXAMPLEAGEDDON_COMPILE_TIMEOUT="180"
EXAMPLEAGEDDON_RUN_TIMEOUT="10"
PKG_INSTALL_TIMEOUT="1200"
SSH_TIMEOUT="${SSH_TIMEOUT:-7200}"
SKIP_PACKAGE_INSTALL=0
KEEP_VM=0
GUEST_IMAGE_DIR="/work"
WORK_ROOT_DIR="/work/freebasic-test"
PKG_CACHE_DIR="/work/freebasic-package"

DEFAULT_IMAGE_URL="https://downloads.omnios.org/media/stable/omnios-${RELEASE}.cloud.qcow2"

msg() { printf '\n==> %s\n' "$*"; }
die() { printf '\nERROR: %s\n' "$*" >&2; exit 1; }

usage() {
	cat <<EOF
Usage: ./build_scripts/illumos-vm-test-freebasic.sh [options]

Options:
  --package-dir DIR     Package artifact path. Default: out/illumos/x86-64
  --image-url URL       OmniOS cloud qcow2 URL.
  --image FILE          Existing OmniOS cloud qcow2 image.
  --release N           OmniOS release override. Default: ${RELEASE}
  --pkg-proxy-port N    Host package proxy port. Default: disabled
  --pkg-repo-url URL    Base OmniOS package URL for direct mode. Default: https://pkg.omnios.org
  --pkg-repo-fallback-url URL Optional fallback package URL for direct mode. Default: http://pkg.omnios.org
  --workroot DIR        Work directory. Default: out/illumos-vm
  --jobs N              Build/test concurrency hint. Default: host CPU count
  --cpus N              QEMU CPU count. Default: --jobs value
  --memory MB           VM memory in MB. Default: 6144
  --disk-size SIZE      Resized VM disk size. Default: 32G
  --ssh-port N          Host SSH forward port. Default: auto
  --fbctests-jobs N     fbctests job count. Default: --jobs value
  --fbctests-unit-args S Extra UNITTEST_RUN_ARGS for fbctests.
  --exampleageddon-jobs N  exampleageddon job count. Default: --jobs value
  --exampleageddon-compile-timeout N Example compile timeout.
  --exampleageddon-run-timeout N Example run timeout.
  --skip-package-install Skip package installation in guest.
  --pkg-install-timeout N  Seconds per package install attempt. Default: 1200.
  --keep-vm             Keep VM artifacts after success.
  -h, --help            Show this help text.
EOF
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--package-dir) PACKAGE_DIR="$2"; shift 2 ;;
		--image-url) IMAGE_URL="$2"; shift 2 ;;
		--image) IMAGE_FILE="$2"; shift 2 ;;
		--release) RELEASE="$2"; DEFAULT_IMAGE_URL="https://downloads.omnios.org/media/stable/omnios-${RELEASE}.cloud.qcow2"; shift 2 ;;
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
		--exampleageddon-jobs) EXAMPLEAGEDDON_JOBS="$2"; shift 2 ;;
		--exampleageddon-compile-timeout) EXAMPLEAGEDDON_COMPILE_TIMEOUT="$2"; shift 2 ;;
		--exampleageddon-run-timeout) EXAMPLEAGEDDON_RUN_TIMEOUT="$2"; shift 2 ;;
		--pkg-install-timeout) PKG_INSTALL_TIMEOUT="$2"; shift 2 ;;
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
		return
	fi

	[ -n "$IMAGE_URL" ] || IMAGE_URL="$DEFAULT_IMAGE_URL"
	IMAGE_FILE="$CACHE_DIR/$(basename "$IMAGE_URL")"
	[ -f "$IMAGE_FILE" ] && return

	download_url "$IMAGE_URL" "$IMAGE_FILE" ||
		die "failed to download $IMAGE_URL"
}

prepare_disk() {
	local disk="$RUN_DIR/illumos.qcow2"
	local src="$IMAGE_FILE"
	rm -f "$disk"
	qemu-img create -f qcow2 -F qcow2 -b "$src" "$disk" "$DISK_SIZE" >/dev/null
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
	local serial="$LOG_DIR/serial.log"

	msg "starting illumos VM on SSH port $SSH_PORT"
	qemu-system-x86_64 \
		-m "$MEMORY" \
		-smp "$CPUS" \
		-machine accel=kvm:tcg \
		-cpu max \
		-drive "file=$disk,if=virtio,format=qcow2" \
		-drive "file=$iso,media=cdrom,if=ide,readonly=on" \
		-netdev "user,id=net0,hostfwd=tcp:127.0.0.1:${SSH_PORT}-:22" \
		-device e1000,netdev=net0 \
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

	msg "Starting OmniOS package proxy on port $PKG_PROXY_PORT"
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
	while [ "$tries" -gt 0 ]; do
		if timeout 5 ssh -o BatchMode=yes -o StrictHostKeyChecking=no \
			-o UserKnownHostsFile=/dev/null -o ConnectTimeout=5 \
			-i "$RUN_DIR/id_ed25519" -p "$SSH_PORT" root@127.0.0.1 'uname -a' >/dev/null 2>&1; then
			return 0
		fi
		tries=$((tries - 1))
		sleep 10
	done
	die "timed out waiting for illumos SSH"
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
	if [ ! -f "$key" ]; then
		ssh-keygen -q -t ed25519 -N '' -f "$key"
	fi
}

send_test_payload() {
	local repo_dir="$1"

	msg "preparing guest test workspace"
	ssh_illumos "rm -rf '$WORK_ROOT_DIR' '$PKG_CACHE_DIR' && mkdir -p '$WORK_ROOT_DIR' '$PKG_CACHE_DIR' '$PKG_CACHE_DIR/repo'"

	if [ "$SKIP_PACKAGE_INSTALL" -eq 0 ] && [ -n "$repo_dir" ] && [ -d "$repo_dir" ]; then
		msg "copying package repository into VM"
		scp_to_illumos -r "$repo_dir" "root@127.0.0.1:$PKG_CACHE_DIR/"
		ssh_illumos "rm -f '$PKG_CACHE_DIR/repo/publisher/local/tmp/lock'"
	fi

	scp_to_illumos -r "$ROOT/tests" "root@127.0.0.1:$WORK_ROOT_DIR/"
	scp_to_illumos -r "$ROOT/inc" "root@127.0.0.1:$WORK_ROOT_DIR/"
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

resolve_make() {
	for make in gmake make; do
		command -v "$make" >/dev/null 2>&1 && {
			echo "$make"
			return 0
		}
	done
	return 1
}

install_pkg_candidates() {
	local pkg
	local timeout_s
	local rc
	local log
	local had_timeout
	local attempts
	local attempt
	local pkg_timeouted
	log="/tmp/illumos-vm-pkg-install.log"
	had_timeout=0

	case "${PKG_INSTALL_TIMEOUT:-1200}" in
		''|*[!0-9]*|0) timeout_s="" ;;
		*) timeout_s="$PKG_INSTALL_TIMEOUT" ;;
	esac
	command -v timeout >/dev/null 2>&1 || timeout_s=""

	attempts=2
	for pkg in "$@"; do
		attempt=1
		while [ "$attempt" -le "$attempts" ]; do
			: > "$log"
			pkg_timeouted=0
			if [ -n "$timeout_s" ]; then
				timeout "$timeout_s" /usr/bin/pkg install --accept --no-refresh "$pkg" > "$log" 2>&1
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
			sleep 3
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

configure_omnios_publishers() {
	local release="$1"
	local mode="$2"
	local base_url="$3"
	local normalized_base
	local direct_url

	case "$mode" in
		proxy)
			if [ -z "${PKG_PROXY_PORT:-}" ]; then
				fail "pkg proxy requested but no PKG_PROXY_PORT provided"
			fi
			run /usr/bin/pkg set-publisher --no-refresh -M '*' -O "http://10.0.2.2:${PKG_PROXY_PORT}/${release}/core/" omnios
			run /usr/bin/pkg set-publisher --no-refresh -M '*' -O "http://10.0.2.2:${PKG_PROXY_PORT}/${release}/extra/" extra.omnios
			run /usr/bin/pkg refresh omnios extra.omnios
			;;
		direct)
			if [ -z "$base_url" ]; then
				fail "no package repository URL configured for direct mode"
			fi
			normalized_base="${base_url%/}"
			if [[ "$normalized_base" == */"${release}" ]]; then
				direct_url="$normalized_base"
			else
				direct_url="${normalized_base}/${release}"
			fi
			if ! /usr/bin/pkg set-publisher --no-refresh -M '*' -O "${direct_url}/core/" omnios; then
				return 1
			fi
			if ! /usr/bin/pkg set-publisher --no-refresh -M '*' -O "${direct_url}/extra/" extra.omnios; then
				return 1
			fi
			if ! /usr/bin/pkg refresh omnios extra.omnios; then
				return 1
			fi
			;;
		*)
			fail "unknown publisher mode: $mode"
			;;
	esac
	return 0
}

configure_omnios_publishers_with_fallback() {
	local release="$1"
	local allow_proxy="${2:-1}"

	if [ "$allow_proxy" -eq 1 ] && [ -n "${PKG_PROXY_PORT:-}" ]; then
		if configure_omnios_publishers "$release" proxy; then
			return 0
		fi
		echo "==> local package proxy unreachable, trying direct repositories" >&2
	fi

	if configure_omnios_publishers "$release" direct "${PKG_REPO_URL}"; then
		return 0
	fi

	if [ -n "$PKG_REPO_FALLBACK_URL" ] && [ "$PKG_REPO_FALLBACK_URL" != "$PKG_REPO_URL" ]; then
		if configure_omnios_publishers "$release" direct "$PKG_REPO_FALLBACK_URL"; then
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

run_fbctests() {
	local jobs
	local make
	jobs="$(fbctests_jobs)"
	make="$(resolve_make)" || fail "missing gmake/make"

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
	local release

	release="$(uname -v | sed -n 's/^omnios-\(r[0-9][0-9]*\).*/\1/p')"
	[ -n "$release" ] || release="r151058"

	prepare_tls_policy

	echo "==> installing test dependencies"
	run echo "==> proxy port in guest: ${PKG_PROXY_PORT:-<unset>}"
	if ! configure_omnios_publishers_with_fallback "$release" 1; then
		fail "unable to configure OmniOS package repositories"
	fi

	if ! command -v python3 >/dev/null 2>&1; then
		install_pkg_candidates runtime/python-313 runtime/python-311 runtime/python-310 runtime/python-39 runtime/python-27 ||
			fail "python package unavailable in OmniOS repositories"
	fi
	if ! command -v make >/dev/null 2>&1; then
		install_pkg_candidates developer/build/make ||
			fail "make package unavailable in OmniOS repositories"
	fi
	if ! command -v gmake >/dev/null 2>&1; then
		install_pkg_candidates developer/build/gnu-make ||
			fail "gnu-make package unavailable in OmniOS repositories"
	fi

	if ! find_cmd_or_path gcc cc /usr/gnu/bin/gcc /usr/bin/cc >/dev/null 2>&1; then
		PKG_INSTALL_HAD_TIMEOUT=0
		if ! install_pkg_candidates developer/gcc15 developer/gcc14 developer/gcc13 developer/gcc10; then
			if [ "$PKG_INSTALL_HAD_TIMEOUT" -ne 0 ] && [ -n "${PKG_PROXY_PORT:-}" ]; then
				echo "==> compiler install timed out via local proxy, retrying with only direct repositories"
			else
				echo "==> compiler install unavailable from initial publisher, trying direct repositories"
			fi

			if ! configure_omnios_publishers_with_fallback "$release" 0; then
				if ! configure_omnios_publishers_with_fallback "$release" 1; then
					fail "unable to configure OmniOS package repositories for compiler install"
				fi
			fi

			install_pkg_candidates developer/gcc15 developer/gcc14 developer/gcc13 developer/gcc10 ||
				fail "no compiler package found from configured repository"
		fi
	fi

	CC_PATH="$(find_cmd_or_path gcc cc /usr/gnu/bin/gcc /usr/bin/gcc /usr/local/bin/gcc /usr/gnu/bin/cc /usr/bin/cc /usr/local/bin/cc)"
	if [ -z "${CC_PATH:-}" ]; then
		fail "no compiler toolchain found after package/install attempts"
	fi
	export CC="$CC_PATH"
	case "$CC_PATH" in
		/*) export PATH="$(dirname "$CC_PATH"):$PATH" ;;
	esac
	echo "==> using CC=$CC"

	CXX_PATH="$(find_cmd_or_path g++ c++ /usr/gnu/bin/g++ /usr/bin/g++ /usr/local/bin/g++ /usr/gnu/bin/c++ /usr/bin/c++)"
	if [ -n "${CXX_PATH:-}" ]; then
		export CXX="$CXX_PATH"
		case "$CXX_PATH" in
			/*) export PATH="$(dirname "$CXX_PATH"):$PATH" ;;
		esac
		echo "==> using CXX=$CXX"
	fi

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
		[ -d "${PKG_REPO_DIR:-}" ] && \
		[ -z "${FBC_BIN:-}" ]; then
		PKG_URI="file://${PKG_REPO_DIR}"
		FBVERSION="${FBVERSION:-1.20.1}"
		PKG_OSREL="${PKG_OSREL:-5.11}"
		PKG_REV="${PKG_REV:-1}"
		run /usr/bin/pkg set-publisher --no-refresh -g "$PKG_URI" local
		install_pkg_candidates "pkg://local/lang/freebasic@${FBVERSION},${PKG_OSREL}-${PKG_REV}" "lang/freebasic" || fail "freebasic package unavailable in repositories"
		FBC_BIN="$(command -v fbc || true)"
		if [ -z "${FBC_BIN:-}" ] && [ -x /usr/local/bin/fbc ]; then
			FBC_BIN="/usr/local/bin/fbc"
		fi
		command -v "$FBC_BIN" >/dev/null 2>&1 || fail "fbc not installed after package install"
	elif [ "${SKIP_PACKAGE_INSTALL:-0}" -eq 0 ] && [ -n "${FBC_BIN:-}" ]; then
		echo "==> using existing fbc in guest, skipping package install"
	fi
	command -v "$FBC_BIN" >/dev/null 2>&1 || fail "fbc not installed"

	run_fbctests
	run_exampleageddon
	echo "==> TEST PASSED"
}

run_main_tests
EOF
chmod +x "$RUN_DIR/illumos-vm-test-runner.sh"
}

run_test_in_guest() {
	local runner="/work/freebasic-vm-test-runner.sh"
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
EXAMPLEAGEDDON_JOBS=${EXAMPLEAGEDDON_JOBS:-} \
EXAMPLEAGEDDON_COMPILE_TIMEOUT=$EXAMPLEAGEDDON_COMPILE_TIMEOUT \
EXAMPLEAGEDDON_RUN_TIMEOUT=$EXAMPLEAGEDDON_RUN_TIMEOUT \
FBVERSION=$fbversion \
PKG_OSREL=$pkg_osrel \
PKG_REV=$pkg_rev \
PKG_PROXY_PORT=$proxy_port \
PKG_INSTALL_TIMEOUT=$PKG_INSTALL_TIMEOUT \
PKG_REPO_URL=$PKG_REPO_URL \
PKG_REPO_FALLBACK_URL=$PKG_REPO_FALLBACK_URL \
PKG_REPO_DIR=$PKG_CACHE_DIR/repo \
bash $runner > '$guest_log' 2>&1"
	then
		msg "guest tests passed"
	else
		scp_from_illumos "root@127.0.0.1:$guest_log" "$host_log" || true
		ssh_illumos '[ -f /work/freebasic-test/exampleageddon/report.md ]' >/dev/null 2>&1 &&
			scp_from_illumos "root@127.0.0.1:/work/freebasic-test/exampleageddon/report.md" "$ARCHIVE_RESULTS/exampleageddon-report.md" || true
		ssh_illumos '[ -f /work/freebasic-test/exampleageddon/results.csv ]' >/dev/null 2>&1 &&
			scp_from_illumos "root@127.0.0.1:/work/freebasic-test/exampleageddon/results.csv" "$ARCHIVE_RESULTS/exampleageddon-results.csv" || true
		tail -n 120 "$host_log" >&2 || true
		die "illumos VM test run failed"
	fi

	scp_from_illumos "root@127.0.0.1:$guest_log" "$host_log"
	ssh_illumos '[ -f /work/freebasic-test/exampleageddon/report.md ]' >/dev/null 2>&1 &&
		scp_from_illumos "root@127.0.0.1:/work/freebasic-test/exampleageddon/report.md" "$ARCHIVE_RESULTS/exampleageddon-report.md" || true
	ssh_illumos '[ -f /work/freebasic-test/exampleageddon/results.csv ]' >/dev/null 2>&1 &&
		scp_from_illumos "root@127.0.0.1:/work/freebasic-test/exampleageddon/results.csv" "$ARCHIVE_RESULTS/exampleageddon-results.csv" || true
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
