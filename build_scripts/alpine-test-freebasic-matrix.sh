#!/usr/bin/env bash

set -euo
# pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Locate project root
##############################################################################

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

##############################################################################
# Helpers
##############################################################################

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }
msg() { echo ""; echo "==> $1"; }
need_cmd() { command -v "$1" >/dev/null 2>&1 || die "missing command: $1"; }

log_has_missing_manifest() {
    local log="$1"

    [ -f "$log" ] || return 1
    grep -Eq 'no matching manifest|manifest unknown|not found: manifest' "$log"
}

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
Usage: ./build_scripts/alpine-test-freebasic-matrix.sh [options]

Options:
  --distro NAME     Limit tests to one apk-based distro family (alpine, postmarketos)
  --arch ARCH       Limit tests to one Alpine-style CPU arch
  --keep-going      Continue after per-entry failures
  --skip-host-deps  Skip host dependency installation
  --list            Show package directories that would be tested
  --help            Show this help text

The script discovers package artifacts under out/linux/<distro>/<release>/<arch>,
starts a fresh Docker container for each target, installs that target's .apk
packages, then compiles/runs console, gfxlib, and sfxlib smoke programs.

Inside the guest, the only package install command is:
  apk add --allow-untrusted /packages/*.apk

Any additional packages must be pulled as declared dependencies of the .apk.
EOF
}

##############################################################################
# Options
##############################################################################

DISTRO_FILTER=""
ARCH_FILTER=""
KEEP_GOING=0
SKIP_HOST_DEPS=0
LIST_ONLY=0

while [ $# -gt 0 ]; do
    case "$1" in
        --distro) DISTRO_FILTER="$2"; shift 2 ;;
        --arch) ARCH_FILTER="$2"; shift 2 ;;
        --keep-going) KEEP_GOING=1; shift ;;
        --skip-host-deps) SKIP_HOST_DEPS=1; shift ;;
        --list) LIST_ONLY=1; shift ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            die "unknown option: $1"
            ;;
    esac
done

if [ -n "$DISTRO_FILTER" ] && [ "$DISTRO_FILTER" != "alpine" ] && [ "$DISTRO_FILTER" != "postmarketos" ]; then
    die "unsupported apk-based distro: $DISTRO_FILTER"
fi

##############################################################################
# Host dependency installation
##############################################################################

install_host_deps() {
    [ "$SKIP_HOST_DEPS" -eq 0 ] || return 0

    if command -v apt-get >/dev/null 2>&1; then
        msg "installing host dependencies via apt"
        run_root apt-get update -y
        run_root apt-get install -y --no-install-recommends \
            docker.io \
            qemu-user-static \
            binfmt-support \
            ca-certificates
        return 0
    fi

    if command -v pacman >/dev/null 2>&1; then
        msg "installing host dependencies via pacman"
        run_root pacman -Sy --noconfirm \
            docker qemu-user-static binfmt-qemu-static ca-certificates
        return 0
    fi

    if command -v dnf >/dev/null 2>&1; then
        msg "installing host dependencies via dnf"
        run_root dnf install -y docker qemu-user-static ca-certificates
        return 0
    fi

    if command -v yum >/dev/null 2>&1; then
        msg "installing host dependencies via yum"
        run_root yum install -y docker qemu-user-static ca-certificates
        return 0
    fi

    die "unsupported host package manager; install Docker, qemu-user-static, and binfmt manually"
}

##############################################################################
# Matrix definition
##############################################################################

APK_TARGETS=(
    "alpine|3.23|alpine:3.23"
    "alpine|3.22|alpine:3.22"
    "alpine|3.21|alpine:3.21"
    "alpine|edge|alpine:edge"
    "postmarketos|edge|adamthiede/postmarketos:edge"
)

docker_platform_for_arch() {
    case "$1" in
        x86_64) echo "linux/amd64" ;;
        x86) echo "linux/386" ;;
        aarch64) echo "linux/arm64" ;;
        armv7|armhf) echo "linux/arm/v7" ;;
        ppc64le) echo "linux/ppc64le" ;;
        s390x) echo "linux/s390x" ;;
        riscv64) echo "linux/riscv64" ;;
        *)
            die "unsupported Docker platform arch: $1"
            ;;
    esac
}

image_for_target() {
    local distro="$1"
    local release="$2"
    local entry
    local entry_distro
    local entry_release
    local image

    for entry in "${APK_TARGETS[@]}"; do
        IFS="|" read -r entry_distro entry_release image <<EOF
$entry
EOF
        if [ "$entry_distro" = "$distro" ] && [ "$entry_release" = "$release" ]; then
            echo "$image"
            return 0
        fi
    done

    return 1
}

target_matches_filters() {
    local distro="$1"
    local arch="$2"

    if [ -n "$DISTRO_FILTER" ] && [ "$DISTRO_FILTER" != "$distro" ]; then
        return 1
    fi

    if [ -n "$ARCH_FILTER" ] && [ "$ARCH_FILTER" != "$arch" ]; then
        return 1
    fi

    return 0
}

##############################################################################
# Test runner script
##############################################################################

TEST_RUNNER="$(mktemp -t fb-apk-package-test.XXXXXX.sh)"
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

run_timeout() {
    if command -v timeout >/dev/null 2>&1; then
        timeout 20s "$@"
    else
        "$@"
    fi
}

run_gfx_smoke() {
    local out="$1"
    local err="$2"
    local combined="/tmp/fb-package-smoke/gfx.combined"

    if run_timeout /tmp/fb-package-smoke/gfx > "$out" 2> "$err"; then
        cat "$out" || true
        if [ -s "$err" ]; then
            cat "$err"
            cat "$out" "$err" > "$combined" 2>/dev/null || true
            if grep -Eiq 'error loading shared library|relocation error|undefined symbol|cannot execute|exec format error|no such file or directory|ld-linux|ld-musl' "$combined"; then
                fail "gfx binary wrote a loader/linker error"
            fi
            if grep -Eiq 'display|x11|x server|screenres|graphics' "$combined"; then
                echo "HEADLESS-RUN: gfx binary started, but no usable display is available"
                return 0
            fi
            fail "gfx binary wrote stderr"
        fi
        return 0
    fi

    cat "$out" || true
    cat "$err" || true
    cat "$out" "$err" > "$combined" 2>/dev/null || true

    if grep -Eiq 'error loading shared library|relocation error|undefined symbol|cannot execute|exec format error|no such file or directory|ld-linux|ld-musl' "$combined"; then
        fail "gfx binary failed with a loader/linker error"
    fi

    if grep -Eiq 'display|x11|x server|screenres|graphics' "$combined"; then
        echo "HEADLESS-RUN: gfx binary started, but no usable display is available"
        return 0
    fi

    fail "gfx binary failed before proving headless/display handling"
}

apk update

set -- /packages/*.apk
[ -e "$1" ] || fail "no .apk packages mounted at /packages"

echo "==> installing FreeBASIC packages"
run apk add --allow-untrusted "$@"

echo "==> verifying fbc"
command -v fbc
fbc -version

mkdir -p /tmp/fb-package-smoke

cat > /tmp/fb-package-smoke/console.bas <<'FBEOF'
print "Hello world"
FBEOF

cat > /tmp/fb-package-smoke/gfx.bas <<'FBEOF'
screenres 160, 100, 32
color rgb(255, 255, 255), rgb(0, 0, 0)
cls
draw string (8, 8), "Hello world"
line (8, 28)-(120, 70), rgb(0, 200, 255), bf
print "Hello world"
sleep 50
screen 0
FBEOF

cat > /tmp/fb-package-smoke/sfx.bas <<'FBEOF'
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
run fbc /tmp/fb-package-smoke/console.bas -x /tmp/fb-package-smoke/console -v
[ -x /tmp/fb-package-smoke/console ] || fail "console binary was not created"
if command -v readelf >/dev/null 2>&1; then
    readelf -l /tmp/fb-package-smoke/console | sed -n 's/.*Requesting program interpreter: //p'
fi

echo "==> running console smoke"
console_output="$(/tmp/fb-package-smoke/console)"
echo "$console_output"
[ "$console_output" = "Hello world" ] || fail "unexpected console output: $console_output"

echo "==> compiling gfxlib smoke"
run fbc /tmp/fb-package-smoke/gfx.bas -x /tmp/fb-package-smoke/gfx -v
[ -x /tmp/fb-package-smoke/gfx ] || fail "gfx binary was not created"

echo "==> running gfxlib smoke"
run_gfx_smoke /tmp/fb-package-smoke/gfx.out /tmp/fb-package-smoke/gfx.err

echo "==> compiling sfxlib smoke"
run fbc /tmp/fb-package-smoke/sfx.bas -x /tmp/fb-package-smoke/sfx -v
[ -x /tmp/fb-package-smoke/sfx ] || fail "sfx binary was not created"

echo "==> running sfxlib smoke"
if ! run_timeout /tmp/fb-package-smoke/sfx > /tmp/fb-package-smoke/sfx.out 2> /tmp/fb-package-smoke/sfx.err; then
    cat /tmp/fb-package-smoke/sfx.out || true
    cat /tmp/fb-package-smoke/sfx.err || true
    fail "sfx binary failed"
fi
cat /tmp/fb-package-smoke/sfx.out || true
grep -qx 'sfx-start' /tmp/fb-package-smoke/sfx.out || fail "sfx binary did not print sfx-start"
grep -qx 'sfx-end' /tmp/fb-package-smoke/sfx.out || fail "sfx binary did not print sfx-end"
if [ -s /tmp/fb-package-smoke/sfx.err ]; then
    cat /tmp/fb-package-smoke/sfx.err
    fail "sfx binary wrote stderr"
fi

echo "==> TEST PASSED"
TEST_RUNNER_EOF

##############################################################################
# Package discovery
##############################################################################

discover_targets() {
    local dir
    local rel
    local distro
    local release
    local arch

    [ -d "$ROOT/out/linux" ] || return 0

    while IFS= read -r dir; do
        rel="${dir#"$ROOT/out/linux/"}"
        IFS="/" read -r distro release arch <<EOF
$rel
EOF
        [ -n "$distro" ] && [ -n "$release" ] && [ -n "$arch" ] || continue
        [ "$distro" = "alpine" ] || [ "$distro" = "postmarketos" ] || continue
        target_matches_filters "$distro" "$arch" || continue
        echo "${distro}|${release}|${arch}|${dir}"
    done < <(find "$ROOT/out/linux" -mindepth 3 -maxdepth 3 -type d | sort)
}

TARGETS=()
while IFS= read -r target; do
    TARGETS+=("$target")
done < <(discover_targets)

if [ "$LIST_ONLY" -eq 1 ]; then
    for target in "${TARGETS[@]}"; do
        IFS="|" read -r distro release arch dir <<EOF
$target
EOF
        shopt -s nullglob
        apks=("$dir"/*.apk)
        shopt -u nullglob
        echo "${distro}|${release}|${arch}|${#apks[@]} apk(s)|${dir}"
    done
    exit 0
fi

[ "${#TARGETS[@]}" -gt 0 ] || die "no target directories found under out/linux"

##############################################################################
# Test prep
##############################################################################

install_host_deps

need_cmd docker
run_root docker run --rm --privileged tonistiigi/binfmt --install all

##############################################################################
# Test execution
##############################################################################

test_one() {
    local target="$1"
    local distro
    local release
    local arch
    local dir
    local platform
    local image
    local log

    IFS="|" read -r distro release arch dir <<EOF
$target
EOF

    if ! image="$(image_for_target "$distro" "$release")"; then
        echo "SKIPPED: ${distro}/${release} (${arch}) is not in the known Docker target map"
        return 0
    fi

    shopt -s nullglob
    apks=("$dir"/*.apk)
    shopt -u nullglob

    if [ "${#apks[@]}" -eq 0 ]; then
        echo "SKIPPED: ${distro}/${release} (${arch}) has no .apk packages in $dir"
        return 0
    fi

    platform="$(docker_platform_for_arch "$arch")"
    log="$dir/docker_test.log"

    echo
    echo "============================================================"
    echo "Testing ${distro}/${release} (${arch})"
    echo "Docker image: ${image}"
    echo "Docker platform: ${platform}"
    echo "Packages: ${#apks[@]}"
    echo "Log: ${log}"
    echo "============================================================"

    if ! {
        run_root docker pull --platform "$platform" "$image" &&
        run_root docker run --rm \
            --platform "$platform" \
            -v "$dir:/packages:ro" \
            -v "$TEST_RUNNER:/test-freebasic-packages.sh:ro" \
            "$image" \
            /bin/sh /test-freebasic-packages.sh
    } &> "$log"; then
        if log_has_missing_manifest "$log"; then
            echo "SKIPPED: ${distro}/${release} (${arch}) has no Docker image for ${platform}"
            echo "Log: $log"
            return 0
        fi

        echo "TEST FAILED: ${distro}/${release} (${arch})"
        echo "Log: $log"
        return 1
    fi

    echo "TEST PASSED: ${distro}/${release} (${arch})"
}

failures=0

for target in "${TARGETS[@]}"; do
    if ! test_one "$target"; then
        failures=$((failures + 1))
        if [ "$KEEP_GOING" -eq 0 ]; then
            break
        fi
    fi
done

if [ "$failures" -ne 0 ]; then
    echo
    echo "============================================================"
    echo "APK PACKAGE TESTS FINISHED WITH FAILURES: $failures"
    echo "============================================================"
    exit 1
fi

echo
echo "============================================================"
echo "ALL APK PACKAGE TESTS FINISHED"
echo "============================================================"
