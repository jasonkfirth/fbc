#!/usr/bin/env bash

##############################################################################
# FreeBASIC illumos package test runner
##############################################################################
#
# Purpose:
#
#   Run native illumos FreeBASIC smoke checks, fbctests, and exampleageddon
#   against a locally built FreeBASIC installation or an IPS package repository
#   from the illumos build artifacts.
#
# Responsibilities:
#
#   * locate the project root and validate the FreeBASIC source tree
#   * optionally install freebasic from a local IPS repository
#   * run fbctests (unit-tests + log-tests) with the configured job count
#   * run exampleageddon and fail on self-contained problems
#
# This script intentionally does not contain:
#
#   * illumos package/VM build logic
#   * host bootstrap dependency installation
#   * source-tree bootstrap of the compiler
#
##############################################################################

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Environment
##############################################################################

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
while :; do
	if [ -f "$ROOT/mk/version.mk" ] && [ -f "$ROOT/GNUmakefile" ]; then
		break
	fi
	[ "$ROOT" = "/" ] && break
	ROOT="$(dirname "$ROOT")"
done

[ -f "$ROOT/mk/version.mk" ] && [ -f "$ROOT/GNUmakefile" ] || {
	echo "ERROR: could not locate the FreeBASIC source root"
	exit 1
}

FBVERSION="$(awk -F':=' '/^FBVERSION/ {gsub(/[ \t]/,"",$2); print $2}' "$ROOT/mk/version.mk")"
REV="$(awk -F':=' '/^REV/ {gsub(/[ \t]/,"",$2); print $2}' "$ROOT/mk/version.mk")"
OSREL="$(uname -r 2>/dev/null || echo unknown)"
ARCH="$(uname -m 2>/dev/null || echo unknown)"

FBCTESTS_JOBS="${FBCTESTS_JOBS:-}"
FBCTESTS_UNIT_ARGS="${FBCTESTS_UNIT_ARGS:-}"
EXAMPLEAGEDDON_JOBS="${EXAMPLEAGEDDON_JOBS:-}"
EXAMPLEAGEDDON_COMPILE_TIMEOUT="${EXAMPLEAGEDDON_COMPILE_TIMEOUT:-180}"
EXAMPLEAGEDDON_RUN_TIMEOUT="${EXAMPLEAGEDDON_RUN_TIMEOUT:-10}"
PACKAGE_DIR=""
FBC="${FBC:-}"
SKIP_PACKAGE_INSTALL=0
OUTROOT="$ROOT/out/illumos-test-freebasic"
PKG_INSTALL_TIMEOUT="${PKG_INSTALL_TIMEOUT:-1200}"
PKG_PROXY_PORT="${PKG_PROXY_PORT:-}"
ILLUMOS_PKG_PROXY="${ILLUMOS_PKG_PROXY:-}"

##############################################################################
# helpers
##############################################################################

run() {
	echo "==> $*"
	"$@"
}

fail() {
	echo "ERROR: $*" >&2
	exit 1
}

msg() {
	echo ""
	echo "==> $*"
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

start_pkg_proxy() {
	local script="$OUTROOT/pkg_proxy.py"
	local pidfile="$OUTROOT/pkg_proxy.pid"

	if [ -z "$PKG_PROXY_PORT" ]; then
		PKG_PROXY_PORT="$(
			python3 - <<'PY'
import socket

for port in range(18080, 19080):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        try:
            sock.bind(("127.0.0.1", port))
        except OSError:
            continue
        print(port)
        raise SystemExit(0)
raise SystemExit("no free TCP port found")
PY
		)"
	fi

	if [ -z "$PKG_PROXY_PORT" ]; then
		fail "could not select a free TCP port for local pkg proxy"
	fi

	python3 - <<'PY' > "$script"
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
                        f"http://127.0.0.1:{PORT}"
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

	python3 -u "$script" "$PKG_PROXY_PORT" > "$OUTROOT/pkg-proxy.log" 2>&1 &
	echo "$!" > "$pidfile"
	sleep 1
	kill -0 "$(cat "$pidfile")" >/dev/null 2>&1 ||
		fail "package proxy failed to start; see $OUTROOT/pkg-proxy.log"

	ILLUMOS_PKG_PROXY="http://127.0.0.1:${PKG_PROXY_PORT}"
}

stop_pkg_proxy() {
	[ -f "$OUTROOT/pkg_proxy.pid" ] || return 0
	kill "$(cat "$OUTROOT/pkg_proxy.pid")" 2>/dev/null || true
	rm -f "$OUTROOT/pkg_proxy.pid"
}

install_pkg_candidates() {
	local pkg
	local timeout_s
	local rc

	case "${PKG_INSTALL_TIMEOUT:-1200}" in
		''|*[!0-9]*|0) timeout_s="" ;;
		*) timeout_s="$PKG_INSTALL_TIMEOUT" ;;
	esac
	command -v timeout >/dev/null 2>&1 || timeout_s=""

	for pkg in "$@"; do
		if [ -n "$timeout_s" ]; then
			timeout "$timeout_s" pkg install --accept --no-refresh "$pkg" >/dev/null 2>&1
			rc=$?
		else
			pkg install --accept --no-refresh "$pkg" >/dev/null 2>&1
			rc=$?
		fi
		if [ "$rc" -eq 0 ]; then
			return 0
		fi
	done
	return 1
}

configure_pkg_publishers() {
	local release

	if [ -z "$ILLUMOS_PKG_PROXY" ]; then
		return 0
	fi

	release="$(uname -v | sed -n 's/^omnios-\(r[0-9][0-9]*\).*/\1/p')"
	[ -n "$release" ] || release="r151058"

	run pkg set-publisher --no-refresh -M '*' -O "${ILLUMOS_PKG_PROXY}/${release}/core/" omnios
	run pkg set-publisher --no-refresh -M '*' -O "${ILLUMOS_PKG_PROXY}/${release}/extra/" extra.omnios
}

run_illumos_package_install() {
	local package_repo="$1"
	local repo_uri
	local fmri

	repo_uri="file://$package_repo"
	fmri="pkg://local/lang/freebasic@${FBVERSION},${OSREL}-${REV}"

	prepare_tls_policy
	configure_pkg_publishers
	run pkg set-publisher --no-refresh -g "$repo_uri" local
	install_pkg_candidates "$fmri" "lang/freebasic" || fail "freebasic package unavailable in repositories"
}

fbctests_jobs() {
	case "${FBCTESTS_JOBS:-}" in
		''|*[!0-9]*|0) getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1 ;;
		*) echo "$FBCTESTS_JOBS" ;;
	esac
}

exampleageddon_jobs() {
	case "${EXAMPLEAGEDDON_JOBS:-}" in
		''|*[!0-9]*|0) getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1 ;;
		*) echo "$EXAMPLEAGEDDON_JOBS" ;;
	esac
}

run_fbctests() {
	local make
	local jobs
	local work
	local failed_log

	make="$(resolve_make)" || fail "missing gmake/make"
	jobs="$(fbctests_jobs)"
	work="$(mktemp -d "$OUTROOT/fbctests.XXXXXX")"

	msg "preparing fbctests source tree"
	mkdir -p "$work"
	cp -a "$ROOT/tests" "$work/tests"
	cp -a "$ROOT/inc" "$work/inc"

	msg "running fbctests with ${jobs} job(s)"

	# illumos /usr/bin/xargs does not support -r, which log-tests expects.
	if [ -x /usr/gnu/bin/xargs ]; then
		export PATH="/usr/gnu/bin:$PATH"
		export XARGS=/usr/gnu/bin/xargs
	fi
	cd "$work/tests"
	run "$make" clean FBC="$FBC"
	run "$make" check FBC="$FBC"
	run "$make" -j "$jobs" unit-tests FBC="$FBC" UNITTEST_RUN_ARGS="${FBCTESTS_UNIT_ARGS:-}"
	run "$make" -j "$jobs" log-tests FBC="$FBC"

	for failed_log in failed-fb.log failed-fblite.log failed-qb.log failed-deprecated.log; do
		[ -f "$failed_log" ] || fail "missing log-tests summary: $failed_log"
		if ! grep -qi 'None Found' "$failed_log"; then
			cat "$failed_log"
			fail "fbctests reported failures in $failed_log"
		fi
	done

	msg "fbctests passed"
	rm -rf "$work"
}

run_exampleageddon() {
	local jobs
	local compile_timeout
	local run_timeout
	local outdir

	jobs="$(exampleageddon_jobs)"

	case "$EXAMPLEAGEDDON_COMPILE_TIMEOUT" in
		''|*[!0-9]*|0) compile_timeout=180 ;;
		*) compile_timeout="$EXAMPLEAGEDDON_COMPILE_TIMEOUT" ;;
	esac

	case "$EXAMPLEAGEDDON_RUN_TIMEOUT" in
		''|*[!0-9]*|0) run_timeout=10 ;;
		*) run_timeout="$EXAMPLEAGEDDON_RUN_TIMEOUT" ;;
	esac

	outdir="$OUTROOT/exampleageddon"
	rm -rf "$outdir"
	mkdir -p "$outdir"

	msg "running exampleageddon with ${jobs} job(s)"
	run python3 "$ROOT/build_scripts/exampleageddon-freebasic.py" \
		--root "$ROOT" \
		--outdir "$outdir" \
		--prefix /usr/local \
		--include-dir "$ROOT/inc" \
		--fbc "$FBC" \
		--jobs "$jobs" \
		--compile-timeout "$compile_timeout" \
		--run-timeout "$run_timeout" \
		--fail-on-self-contained

	[ -f "$outdir/report.md" ] || fail "exampleageddon report was not generated"
	if ! grep -qx -- '- Self-contained problems: 0' "$outdir/report.md"; then
		fail "exampleageddon reported self-contained example problems"
	fi

	msg "exampleageddon passed"
}

usage() {
	cat <<EOF
Usage: ./build_scripts/illumos-test-freebasic.sh [options]

Options:
  --package-dir DIR         Path to illumos package artifact directory.
  --fbc PATH                Path to fbc executable. Default: resolved from PATH or /usr/local/bin/fbc.
  --skip-package-install    Do not install from --package-dir and use local compiler only.
  --pkg-install-timeout N   Seconds per package install attempt. Default: 1200.
  --pkg-proxy-port N        Start a local TLS 1.2 proxy on host port for pkg downloads.
  --fbctests-jobs N         Number of jobs for fbctests. Default: cpu count.
  --fbctests-unit-args S    Extra UNITTEST_RUN_ARGS for fbctests.
  --exampleageddon-jobs N    Number of jobs for exampleageddon. Default: cpu count.
  --exampleageddon-compile-timeout N Compile timeout in seconds for exampleageddon. Default: 180.
  --exampleageddon-run-timeout N     Run timeout in seconds for exampleageddon. Default: 10.
  -h, --help                Show this help text.
EOF
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--package-dir) PACKAGE_DIR="$2"; shift 2 ;;
		--fbc) FBC="$2"; shift 2 ;;
		--skip-package-install) SKIP_PACKAGE_INSTALL=1; shift ;;
		--pkg-install-timeout) PKG_INSTALL_TIMEOUT="$2"; shift 2 ;;
		--pkg-proxy-port) PKG_PROXY_PORT="$2"; shift 2 ;;
		--fbctests-jobs) FBCTESTS_JOBS="$2"; shift 2 ;;
		--fbctests-unit-args) FBCTESTS_UNIT_ARGS="$2"; shift 2 ;;
		--exampleageddon-jobs) EXAMPLEAGEDDON_JOBS="$2"; shift 2 ;;
		--exampleageddon-compile-timeout) EXAMPLEAGEDDON_COMPILE_TIMEOUT="$2"; shift 2 ;;
		--exampleageddon-run-timeout) EXAMPLEAGEDDON_RUN_TIMEOUT="$2"; shift 2 ;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			fail "unknown option: $1"
			;;
	esac
done

mkdir -p "$OUTROOT"
trap stop_pkg_proxy EXIT

if [ "$SKIP_PACKAGE_INSTALL" -eq 0 ] && [ -n "$PACKAGE_DIR" ]; then
	if [ ! -d "$PACKAGE_DIR" ]; then
		fail "package directory not found: $PACKAGE_DIR"
	fi
fi

if [ -z "$PACKAGE_DIR" ]; then
	for guess in \
		"$ROOT/out/illumos/$OSREL/$ARCH" \
		"$ROOT/out/illumos/x86-64" \
		"$ROOT/out/illumos"
	do
		if [ -d "$guess" ]; then
			PACKAGE_DIR="$guess"
			break
		fi
	done
fi

if [ -z "$FBC" ]; then
	if command -v fbc >/dev/null 2>&1; then
		FBC="fbc"
	elif [ -x /usr/local/bin/fbc ]; then
		FBC="/usr/local/bin/fbc"
	else
		fail "fbc not found; pass --fbc or install the freebasic package"
	fi
fi

if [ -n "$PKG_PROXY_PORT" ]; then
	start_pkg_proxy
fi

if [ "$SKIP_PACKAGE_INSTALL" -eq 0 ] && [ -n "$PACKAGE_DIR" ]; then
	msg "installing freebasic from package repository in $PACKAGE_DIR"
	PACKAGE_REPO="$(resolve_repo_dir "$PACKAGE_DIR" || true)"
	if [ -n "$PACKAGE_REPO" ] && [ -d "$PACKAGE_REPO" ]; then
		run_illumos_package_install "$PACKAGE_REPO"
	else
		echo "WARN: no repository directory found under $PACKAGE_DIR; using existing compiler"
	fi
fi

command -v "$FBC" >/dev/null 2>&1 || fail "fbc not executable: $FBC"
msg "using fbc: $FBC"

msg "running fbctests"
run_fbctests

msg "running exampleageddon"
run_exampleageddon

msg "illumos test run succeeded"
msg "outputs: $OUTROOT"

# end of illumos-test-freebasic.sh
