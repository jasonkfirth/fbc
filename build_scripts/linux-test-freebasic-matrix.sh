#!/usr/bin/env bash

set -euo pipefail
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
Usage: ./build_scripts/linux-test-freebasic-matrix.sh [options]

Options:
  --distro NAME     Limit tests to one distro family (for now: debian, ubuntu)
  --arch ARCH       Limit tests to one Debian-style CPU arch
  --keep-going      Continue after per-entry failures
  --skip-host-deps  Skip host dependency installation
  --list            Show package directories that would be tested
  --help            Show this help text

The script discovers package artifacts under out/linux/<distro>/<codename>/<arch>,
starts a fresh Docker container for each target, installs that target's .deb
packages, then compiles/runs console, gfxlib, and sfxlib smoke programs.
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

    if command -v zypper >/dev/null 2>&1; then
        msg "installing host dependencies via zypper"
        run_root zypper --non-interactive install docker qemu-user-static ca-certificates
        return 0
    fi

    die "unsupported host package manager; install Docker, qemu-user-static, and binfmt manually"
}

##############################################################################
# Matrix definition
##############################################################################

DISTRO_TARGETS=(
    "ubuntu|22.04|jammy"
    "ubuntu|24.04|noble"
    "ubuntu|24.10|oracular"
    "ubuntu|25.04|plucky"
    "ubuntu|25.10|questing"
    "ubuntu|26.04|resolute"
    "debian|12|bookworm"
    "debian|13|trixie"
    "debian|sid|sid"
)

docker_platform_for_arch() {
    case "$1" in
        amd64) echo "linux/amd64" ;;
        i386) echo "linux/386" ;;
        arm64) echo "linux/arm64" ;;
        armhf) echo "linux/arm/v7" ;;
        armel) echo "linux/arm/v6" ;;
        ppc64el) echo "linux/ppc64le" ;;
        s390x) echo "linux/s390x" ;;
        riscv64) echo "linux/riscv64" ;;
        loong64) echo "linux/loong64" ;;
        *)
            die "unsupported Docker platform arch: $1"
            ;;
    esac
}

image_tag_for_target() {
    local distro="$1"
    local codename="$2"
    local entry
    local entry_distro
    local tag
    local entry_codename

    for entry in "${DISTRO_TARGETS[@]}"; do
        IFS="|" read -r entry_distro tag entry_codename <<EOF
$entry
EOF
        if [ "$entry_distro" = "$distro" ] && [ "$entry_codename" = "$codename" ]; then
            echo "$tag"
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

TEST_RUNNER="$(mktemp -t fb-linux-package-test.XXXXXX.sh)"
cleanup() {
    rm -f "$TEST_RUNNER"
}
trap cleanup EXIT

chmod 755 "$TEST_RUNNER"
cat > "$TEST_RUNNER" <<'TEST_RUNNER_EOF'
#!/usr/bin/env bash

set -euo pipefail

run() { echo "==> $*"; "$@"; }
fail() { echo "ERROR: $*" >&2; exit 1; }

export DEBIAN_FRONTEND=noninteractive
export TERM=dumb

run apt-get update -y

shopt -s nullglob
debs=(/packages/*.deb)
[ "${#debs[@]}" -gt 0 ] || fail "no .deb packages mounted at /packages"

echo "==> installing FreeBASIC packages"
run apt-get install -y "${debs[@]}"

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
print "sfx-start"
play "ABCDEFG"
print "sfx-end"
FBEOF

echo "==> compiling console smoke"
run fbc /tmp/fb-package-smoke/console.bas -x /tmp/fb-package-smoke/console
[ -x /tmp/fb-package-smoke/console ] || fail "console binary was not created"

echo "==> running console smoke"
console_output="$(/tmp/fb-package-smoke/console)"
echo "$console_output"
[ "$console_output" = "Hello world" ] || fail "unexpected console output: $console_output"

echo "==> compiling gfxlib smoke"
run fbc /tmp/fb-package-smoke/gfx.bas -x /tmp/fb-package-smoke/gfx
[ -x /tmp/fb-package-smoke/gfx ] || fail "gfx binary was not created"

echo "==> running gfxlib smoke if a display helper is available"
if command -v xvfb-run >/dev/null 2>&1; then
    timeout 20s xvfb-run -a /tmp/fb-package-smoke/gfx > /tmp/fb-package-smoke/gfx.out 2> /tmp/fb-package-smoke/gfx.err || {
        cat /tmp/fb-package-smoke/gfx.out || true
        cat /tmp/fb-package-smoke/gfx.err || true
        fail "gfx binary failed under Xvfb"
    }
    cat /tmp/fb-package-smoke/gfx.out || true
    [ ! -s /tmp/fb-package-smoke/gfx.err ] || {
        cat /tmp/fb-package-smoke/gfx.err
        fail "gfx binary wrote stderr"
    }
elif [ -n "${DISPLAY:-}" ]; then
    timeout 20s /tmp/fb-package-smoke/gfx > /tmp/fb-package-smoke/gfx.out 2> /tmp/fb-package-smoke/gfx.err || {
        cat /tmp/fb-package-smoke/gfx.out || true
        cat /tmp/fb-package-smoke/gfx.err || true
        fail "gfx binary failed with DISPLAY=$DISPLAY"
    }
    cat /tmp/fb-package-smoke/gfx.out || true
    [ ! -s /tmp/fb-package-smoke/gfx.err ] || {
        cat /tmp/fb-package-smoke/gfx.err
        fail "gfx binary wrote stderr"
    }
else
    echo "SKIP-RUN: gfx binary compiled, but no DISPLAY or xvfb-run is available"
fi

echo "==> compiling sfxlib smoke"
run fbc /tmp/fb-package-smoke/sfx.bas -x /tmp/fb-package-smoke/sfx
[ -x /tmp/fb-package-smoke/sfx ] || fail "sfx binary was not created"

echo "==> running sfxlib smoke"
timeout 20s /tmp/fb-package-smoke/sfx > /tmp/fb-package-smoke/sfx.out 2> /tmp/fb-package-smoke/sfx.err || {
    cat /tmp/fb-package-smoke/sfx.out || true
    cat /tmp/fb-package-smoke/sfx.err || true
    fail "sfx binary failed"
}
cat /tmp/fb-package-smoke/sfx.out || true
[ ! -s /tmp/fb-package-smoke/sfx.err ] || {
    cat /tmp/fb-package-smoke/sfx.err
    fail "sfx binary wrote stderr"
}

echo "==> TEST PASSED"
TEST_RUNNER_EOF

##############################################################################
# Package discovery
##############################################################################

discover_targets() {
    local dir
    local rel
    local distro
    local codename
    local arch

    [ -d "$ROOT/out/linux" ] || return 0

    while IFS= read -r dir; do
        rel="${dir#"$ROOT/out/linux/"}"
        IFS="/" read -r distro codename arch <<EOF
$rel
EOF
        [ -n "$distro" ] && [ -n "$codename" ] && [ -n "$arch" ] || continue
        target_matches_filters "$distro" "$arch" || continue
        echo "${distro}|${codename}|${arch}|${dir}"
    done < <(find "$ROOT/out/linux" -mindepth 3 -maxdepth 3 -type d | sort)
}

TARGETS=()
while IFS= read -r target; do
    TARGETS+=("$target")
done < <(discover_targets)

if [ "$LIST_ONLY" -eq 1 ]; then
    for target in "${TARGETS[@]}"; do
        IFS="|" read -r distro codename arch dir <<EOF
$target
EOF
        shopt -s nullglob
        debs=("$dir"/*.deb)
        shopt -u nullglob
        echo "${distro}|${codename}|${arch}|${#debs[@]} deb(s)|${dir}"
    done
    exit 0
fi

[ "${#TARGETS[@]}" -gt 0 ] || die "no target directories found under out/linux"

##############################################################################
# Build prep
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
    local codename
    local arch
    local dir
    local tag
    local platform
    local image
    local log

    IFS="|" read -r distro codename arch dir <<EOF
$target
EOF

    if ! tag="$(image_tag_for_target "$distro" "$codename")"; then
        echo "SKIPPED: ${distro}/${codename} (${arch}) is not in the known Docker target map"
        return 0
    fi

    shopt -s nullglob
    debs=("$dir"/*.deb)
    shopt -u nullglob

    if [ "${#debs[@]}" -eq 0 ]; then
        echo "SKIPPED: ${distro}/${codename} (${arch}) has no .deb packages in $dir"
        return 0
    fi

    platform="$(docker_platform_for_arch "$arch")"
    image="${distro}:${tag}"
    log="$dir/docker_test.log"

    echo
    echo "============================================================"
    echo "Testing ${distro}/${codename} (${arch})"
    echo "Docker image: ${image}"
    echo "Docker platform: ${platform}"
    echo "Packages: ${#debs[@]}"
    echo "Log: ${log}"
    echo "============================================================"

    if ! {
        run_root docker pull --platform "$platform" "$image" &&
        run_root docker run --rm \
            --platform "$platform" \
            -e DEBIAN_FRONTEND=noninteractive \
            -v "$dir:/packages:ro" \
            -v "$TEST_RUNNER:/test-freebasic-packages.sh:ro" \
            "$image" \
            bash /test-freebasic-packages.sh
    } &> "$log"; then
        if log_has_missing_manifest "$log"; then
            echo "SKIPPED: ${distro}/${codename} (${arch}) has no Docker image for ${platform}"
            echo "Log: $log"
            return 0
        fi

        echo "TEST FAILED: ${distro}/${codename} (${arch})"
        echo "Log: $log"
        return 1
    fi

    echo "TEST PASSED: ${distro}/${codename} (${arch})"
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
    echo "LINUX PACKAGE TESTS FINISHED WITH FAILURES: $failures"
    echo "============================================================"
    exit 1
fi

echo
echo "============================================================"
echo "ALL LINUX PACKAGE TESTS FINISHED"
echo "============================================================"
