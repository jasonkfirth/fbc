#!/usr/bin/env bash

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

START_DIR="$(pwd)"
SEARCH_DIR="$START_DIR"
ROOT=""

while :; do
    if [ -d "$SEARCH_DIR/build_scripts" ] && { [ -f "$SEARCH_DIR/GNUmakefile" ] || [ -f "$SEARCH_DIR/makefile" ] || [ -f "$SEARCH_DIR/Makefile" ]; }; then
        ROOT="$SEARCH_DIR"
        break
    fi
    [ "$SEARCH_DIR" = "/" ] && break
    SEARCH_DIR="$(dirname "$SEARCH_DIR")"
done

[ -n "$ROOT" ] || { echo "ERROR: could not locate FreeBASIC root"; exit 1; }
cd "$ROOT"

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }
msg() { echo ""; echo "==> $1"; }

run_root() {
    if [ "$(id -u)" -eq 0 ]; then
        run "$@"
    elif command -v sudo >/dev/null 2>&1; then
        run sudo "$@"
    else
        die "this step requires root privileges; rerun as root or install sudo"
    fi
}

usage() {
    cat <<EOF
Usage: ./build_scripts/debianubuntu-test-freebasic.sh [options]

Options:
  --package-dir DIR   Directory containing Debian package artifacts
  --image IMAGE       Docker image to use (default: debian:stable-slim)
  --docker-cmd CMD    Docker command to use (default: docker)
  --skip-host-deps    Skip Docker host dependency installation
  --help              Show this help text

The test starts a fresh Debian/Ubuntu-style container, installs the local
freebasic .deb package with only package dependencies, compiles a tiny
program with fbc, then runs the native output.
EOF
}

PACKAGE_DIR=""
IMAGE="${IMAGE:-debian:stable-slim}"
DOCKER_CMD="${DOCKER_CMD:-docker}"
SKIP_HOST_DEPS=0

while [ $# -gt 0 ]; do
    case "$1" in
        --package-dir) PACKAGE_DIR="$2"; shift 2 ;;
        --image) IMAGE="$2"; shift 2 ;;
        --docker-cmd) DOCKER_CMD="$2"; shift 2 ;;
        --skip-host-deps) SKIP_HOST_DEPS=1; shift ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            die "unknown option: $1"
            ;;
    esac
done

if [ -z "$PACKAGE_DIR" ]; then
    ARCH="$(dpkg --print-architecture 2>/dev/null || true)"
    DISTRO_ID="unknown"
    CODENAME="unknown"
    if [ -f /etc/os-release ]; then
        # shellcheck disable=SC1091
        . /etc/os-release
        DISTRO_ID="${ID:-unknown}"
        CODENAME="${VERSION_CODENAME:-unknown}"
    fi
    PACKAGE_DIR="$ROOT/out/linux/$DISTRO_ID/$CODENAME/$ARCH"
fi

[ -d "$PACKAGE_DIR" ] || die "package directory not found: $PACKAGE_DIR"
PACKAGE_DIR="$(cd "$PACKAGE_DIR" && pwd -P)"
ls "$PACKAGE_DIR"/freebasic_*.deb >/dev/null 2>&1 || die "missing freebasic .deb in $PACKAGE_DIR"

install_host_deps() {
    [ "$SKIP_HOST_DEPS" -eq 0 ] || return 0

    if command -v "${DOCKER_CMD%% *}" >/dev/null 2>&1; then
        return 0
    fi

    if command -v apt-get >/dev/null 2>&1; then
        msg "installing Docker host dependency via apt"
        run_root apt-get update -y
        run_root apt-get install -y --no-install-recommends docker.io ca-certificates
        return 0
    fi

    die "Docker is required; install it or rerun with --skip-host-deps after installing Docker"
}

TEST_RUNNER="$(mktemp -t fb-deb-package-test.XXXXXX.sh)"
cleanup() {
    rm -f "$TEST_RUNNER"
}
trap cleanup EXIT

chmod 755 "$TEST_RUNNER"
cat > "$TEST_RUNNER" <<'TEST_RUNNER_EOF'
#!/bin/sh

set -eu

run() {
    echo "==> $*"
    "$@"
}

fail() {
    echo "ERROR: $*" >&2
    exit 1
}

export DEBIAN_FRONTEND=noninteractive

run apt-get update -y
run apt-get install -y --no-install-recommends /packages/freebasic_*.deb

command -v fbc >/dev/null 2>&1 || fail "fbc was not installed"
fbc -print host
fbc -print target

mkdir -p /tmp/fb-smoke
cat > /tmp/fb-smoke/hello.bas <<'EOF'
print "freebasic package smoke ok"
EOF

cd /tmp/fb-smoke
run fbc hello.bas

[ -x hello ] || fail "hello executable was not produced"
run ./hello > output.txt
cat output.txt
grep -q "freebasic package smoke ok" output.txt || fail "compiled executable output was wrong"

echo "freebasic package smoke test passed"
TEST_RUNNER_EOF

install_host_deps

msg "testing freebasic package in $IMAGE"
run ${DOCKER_CMD} run --rm \
    -v "$PACKAGE_DIR:/packages:ro" \
    -v "$TEST_RUNNER:/tmp/test-freebasic.sh:ro" \
    "$IMAGE" \
    /bin/sh /tmp/test-freebasic.sh
