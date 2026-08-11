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
    grep -Eq 'no matching manifest|manifest unknown|not found: manifest|no match for platform' "$log"
}

run_root() {
    if [ "$(id -u)" -eq 0 ]; then
        run "$@"
    elif [ "${1:-}" = "docker" ] && docker ps >/dev/null 2>&1; then
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
  --distro NAME     Limit tests to one distro family
  --release NAME    Limit tests to one release/codename
  --arch ARCH       Limit tests to one Debian-style CPU arch
  --keep-going      Continue after per-entry failures
  --fbctests        Also run the full tests/ compiler test suite
  --fbctests-jobs N Parallel jobs for --fbctests (default: host CPU count)
  --fbctests-unit-args ARGS
                    Extra arguments passed to the fbc-tests binary
  --exampleageddon  Compile examples/ and run self-contained examples
  --exampleageddon-jobs N
                    Parallel jobs for --exampleageddon (default: host CPU count)
  --exampleageddon-compile-timeout N
                    Per-example compile timeout in seconds (default: 180)
  --exampleageddon-run-timeout N
                    Per-example runtime timeout in seconds (default: 10)
  --skip-host-deps  Skip host dependency installation
  --list            Show package directories that would be tested
  --help            Show this help text

The script discovers package artifacts under out/linux/<distro>/<codename>/<arch>,
starts a fresh Docker container for each target, installs that target's .deb
packages, then compiles/runs console, gfxlib, and sfxlib smoke programs.
With --fbctests, it also copies the repository tests/ tree into the container
and runs the FreeBASIC unit and log tests using the packaged compiler.
With --exampleageddon, it writes the example sweep report under each target's
out/linux/<distro>/<codename>/<arch>/exampleageddon directory.
EOF
}

##############################################################################
# Options
##############################################################################

DISTRO_FILTER=""
CODENAME_FILTER=""
ARCH_FILTER=""
KEEP_GOING=0
RUN_FBCTESTS=0
FBCTESTS_JOBS="$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"
FBCTESTS_UNIT_ARGS=""
RUN_EXAMPLEAGEDDON=0
EXAMPLEAGEDDON_JOBS="$FBCTESTS_JOBS"
EXAMPLEAGEDDON_COMPILE_TIMEOUT=180
EXAMPLEAGEDDON_RUN_TIMEOUT=10
SKIP_HOST_DEPS=0
LIST_ONLY=0

while [ $# -gt 0 ]; do
    case "$1" in
        --distro) DISTRO_FILTER="$2"; shift 2 ;;
        --release) CODENAME_FILTER="$2"; shift 2 ;;
        --arch) ARCH_FILTER="$2"; shift 2 ;;
        --keep-going) KEEP_GOING=1; shift ;;
        --fbctests) RUN_FBCTESTS=1; shift ;;
        --fbctests-jobs) FBCTESTS_JOBS="$2"; shift 2 ;;
        --fbctests-unit-args) FBCTESTS_UNIT_ARGS="$2"; shift 2 ;;
        --exampleageddon) RUN_EXAMPLEAGEDDON=1; shift ;;
        --exampleageddon-jobs) EXAMPLEAGEDDON_JOBS="$2"; shift 2 ;;
        --exampleageddon-compile-timeout) EXAMPLEAGEDDON_COMPILE_TIMEOUT="$2"; shift 2 ;;
        --exampleageddon-run-timeout) EXAMPLEAGEDDON_RUN_TIMEOUT="$2"; shift 2 ;;
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
    "ubuntu|ubuntu:26.04|resolute"
    "debian|debian:13|trixie"
    "debian|debian:sid|sid"
    "raspbian|badaix/raspios-lite:trixie|trixie"
)

docker_platform_for_target() {
    local distro="$1"
    local codename="$2"
    local arch="$3"

    case "$distro/$codename/$arch" in
        raspbian/*/armhf)
            echo "linux/arm/v7"
            return 0
            ;;
        raspbian/*/arm64)
            echo "linux/arm64"
            return 0
            ;;
    esac

    case "$arch" in
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
            die "unsupported Docker platform arch: $arch"
            ;;
    esac
}

is_debian_ports_test_arch() {
    case "$1" in
        armel|loong64|powerpc|ppc64)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

image_for_target() {
    local distro="$1"
    local codename="$2"
    local entry
    local entry_distro
    local image
    local entry_codename

    for entry in "${DISTRO_TARGETS[@]}"; do
        IFS="|" read -r entry_distro image entry_codename <<EOF
$entry
EOF
        if [ "$entry_distro" = "$distro" ] && [ "$entry_codename" = "$codename" ]; then
            echo "$image"
            return 0
        fi
    done

    return 1
}

package_family_for_target() {
    case "$1" in
        debian|ubuntu|raspbian) echo "deb" ;;
        alpine|postmarketos) echo "apk" ;;
        fedora|rocky|almalinux|opensuse) echo "rpm" ;;
        archlinux) echo "arch" ;;
        slackware) echo "slackware" ;;
        *) return 1 ;;
    esac
}

archlinux_image_for_arch() {
    case "$1" in
        x86_64)
            echo "archlinux/archlinux:base"
            ;;
        aarch64|armv7h|armv7l|riscv64)
            echo "menci/archlinuxarm:base"
            ;;
        *)
            return 1
            ;;
    esac
}

image_for_nondeb_target() {
    local distro="$1"
    local codename="$2"
    local arch="${3:-}"

    case "$distro/$codename" in
        alpine/3.24) echo "alpine:3.24" ;;
        alpine/edge) echo "alpine:edge" ;;
        postmarketos/edge) echo "adamthiede/postmarketos:edge" ;;
        fedora/44) echo "fedora:44" ;;
        fedora/rawhide) echo "fedora:rawhide" ;;
        rocky/10) echo "rockylinux/rockylinux:10" ;;
        almalinux/10) echo "almalinux:10" ;;
        opensuse/leap-16.0) echo "opensuse/leap:16.0" ;;
        opensuse/tumbleweed) echo "opensuse/tumbleweed" ;;
        archlinux/current) archlinux_image_for_arch "$arch" ;;
        slackware/15.0) echo "vbatts/slackware:15.0" ;;
        slackware/current) echo "vbatts/slackware:current" ;;
        *) return 1 ;;
    esac
}

docker_platform_for_nondeb_target() {
    local arch="$1"

    case "$arch" in
        x86_64) echo "linux/amd64" ;;
        i586|x86) echo "linux/386" ;;
        aarch64|arm64) echo "linux/arm64" ;;
        armv7|armhf) echo "linux/arm/v7" ;;
        ppc64le|ppc64el) echo "linux/ppc64le" ;;
        s390x) echo "linux/s390x" ;;
        riscv64) echo "linux/riscv64" ;;
        armv7|armv7h) echo "linux/arm/v7" ;;
        *)
            die "unsupported Docker platform arch: $arch"
            ;;
    esac
}

package_glob_for_family() {
    case "$1" in
        deb) echo "*.deb" ;;
        apk) echo "*.apk" ;;
        rpm) echo "*.rpm" ;;
        arch) echo "*.pkg.tar.*" ;;
        slackware) echo "*.txz" ;;
        *) return 1 ;;
    esac
}

target_matches_filters() {
    local distro="$1"
    local codename="$2"
    local arch="$3"

    if [ -n "$DISTRO_FILTER" ] && [ "$DISTRO_FILTER" != "$distro" ]; then
        return 1
    fi

    if [ -n "$CODENAME_FILTER" ] && [ "$CODENAME_FILTER" != "$codename" ]; then
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

run_gfx_smoke() {
    local out="$1"
    local err="$2"
    local combined="/tmp/fb-package-smoke/gfx.combined"

    shift 2

    if timeout 20s "$@" > "$out" 2> "$err"; then
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

fbctests_jobs() {
    case "${FBCTESTS_JOBS:-}" in
        ''|*[!0-9]*)
            getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1
            ;;
        0)
            echo 1
            ;;
        *)
            echo "$FBCTESTS_JOBS"
            ;;
    esac
}

exampleageddon_jobs() {
    case "${EXAMPLEAGEDDON_JOBS:-}" in
        ''|*[!0-9]*)
            getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1
            ;;
        0)
            echo 1
            ;;
        *)
            echo "$EXAMPLEAGEDDON_JOBS"
            ;;
    esac
}

run_fbctests() {
    local jobs
    local failed_log

    [ -d /source-tests ] || fail "tests/ tree was not mounted at /source-tests"
    [ -d /source-inc ] || fail "inc/ tree was not mounted at /source-inc"

    jobs="$(fbctests_jobs)"

    echo "==> installing fbctests dependencies"
    run apt-get install -y --no-install-recommends make g++

    echo "==> copying fbctests source"
    rm -rf /tmp/fbctests-source
    mkdir -p /tmp/fbctests-source/tests /tmp/fbctests-source/inc
    (
        cd /source-tests
        tar \
            --exclude='*.o' \
            --exclude='*.a' \
            --exclude='fbc-tests' \
            --exclude='unit-tests.inc' \
            --exclude='unit-tests-obj.lst' \
            --exclude='log-tests-*.inc' \
            --exclude='failed-log-tests-*.inc' \
            --exclude='log-tests-*.lst' \
            --exclude='log-tests-results-*.log' \
            -cf - .
    ) | (
        cd /tmp/fbctests-source/tests
        tar xf -
    )
    (
        cd /source-inc
        tar cf - .
    ) | (
        cd /tmp/fbctests-source/inc
        tar xf -
    )

    cd /tmp/fbctests-source/tests

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

run_exampleageddon() {
    local jobs
    local compile_timeout
    local run_timeout

    [ -d /source-root/examples ] || fail "examples/ tree was not mounted at /source-root/examples"
    [ -f /exampleageddon-freebasic.py ] || fail "exampleageddon runner was not mounted"
    [ -d /results ] || fail "target result directory was not mounted at /results"

    jobs="$(exampleageddon_jobs)"
    compile_timeout="${EXAMPLEAGEDDON_COMPILE_TIMEOUT:-180}"
    run_timeout="${EXAMPLEAGEDDON_RUN_TIMEOUT:-10}"

    case "$compile_timeout" in ''|*[!0-9]*|0) compile_timeout=180 ;; esac
    case "$run_timeout" in ''|*[!0-9]*|0) run_timeout=10 ;; esac

    echo "==> installing exampleageddon dependencies"
    run apt-get install -y --no-install-recommends python3

    echo "==> running exampleageddon with ${jobs} job(s)"
    rm -rf /results/exampleageddon
    mkdir -p /results/exampleageddon
    run python3 /exampleageddon-freebasic.py \
        --root /source-root \
        --outdir /results/exampleageddon \
        --prefix /usr \
        --include-dir /usr/include/freebasic \
        --fbc fbc \
        --jobs "$jobs" \
        --compile-timeout "$compile_timeout" \
        --run-timeout "$run_timeout" \
        --fail-on-self-contained
}

export DEBIAN_FRONTEND=noninteractive
export TERM=dumb
export FB_GFX_DRIVER="${FB_GFX_DRIVER:-null}"
export SFXLIB_DRIVER="${SFXLIB_DRIVER:-null}"

if [ "${FB_SKIP_APT_UPDATE:-0}" = "1" ] && compgen -G "/var/lib/apt/lists/*Packages*" >/dev/null; then
    echo "==> using package lists already present in the test image"
else
    run apt-get update -y
fi

shopt -s nullglob
debs=(/packages/*.deb)
[ "${#debs[@]}" -gt 0 ] || fail "no .deb packages mounted at /packages"

echo "==> installing FreeBASIC packages"
if ! run apt-get install -y --no-install-recommends "${debs[@]}"; then
    echo "==> refreshing package lists and retrying FreeBASIC package install"
    run apt-get update -y
    run apt-get install -y --no-install-recommends "${debs[@]}"
fi

echo "==> installing package smoke-test dependencies"
if ! run apt-get install -y --no-install-recommends \
        libasound2-dev \
        libpulse-dev \
        libx11-dev \
        libxext-dev \
        libxpm-dev \
        libxrandr-dev \
        libxrender-dev; then
    echo "==> refreshing package lists and retrying smoke-test dependencies"
    run apt-get update -y
    run apt-get install -y --no-install-recommends \
        libasound2-dev \
        libpulse-dev \
        libx11-dev \
        libxext-dev \
        libxpm-dev \
        libxrandr-dev \
        libxrender-dev
fi

if [ -n "${FB_RUNTIME_QEMU_CPU:-}" ]; then
    echo "==> using QEMU_CPU=${FB_RUNTIME_QEMU_CPU} for compiler/runtime checks"
    export QEMU_CPU="$FB_RUNTIME_QEMU_CPU"
fi

echo "==> verifying fbc"
command -v fbc
fbc -version
dpkg --print-architecture
uname -m

if [ "$(dpkg --print-architecture)" = "armhf" ]; then
    echo "==> verifying ARM CPU compatibility"
    sed -n '1,16p' /proc/cpuinfo || true
    if [ "${QEMU_CPU:-}" = "arm1176" ] && [ "$(uname -m)" != "armv6l" ]; then
        fail "QEMU_CPU=arm1176 did not expose an armv6l runtime"
    fi
fi

mkdir -p /tmp/fb-package-smoke

cat > /tmp/fb-package-smoke/console.bas <<'FBEOF'
print "Hello world"
FBEOF

cat > /tmp/fb-package-smoke/gfx.bas <<'FBEOF'
screenres 160, 100, 32
screenset 0, 0
color rgb(255, 255, 255), rgb(0, 0, 0)
cls
draw string (8, 8), "Hello world"
line (8, 28)-(120, 70), rgb(0, 200, 255), bf
print "Hello world"
sleep 50
screen 0
FBEOF

cat > /tmp/fb-package-smoke/gfx-screen13.bas <<'FBEOF'
screen 13
screenset 0, 0
pset (8, 8), 12
print "gfx screen 13"
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
run fbc /tmp/fb-package-smoke/console.bas -x /tmp/fb-package-smoke/console
[ -x /tmp/fb-package-smoke/console ] || fail "console binary was not created"

echo "==> running console smoke"
console_output="$(/tmp/fb-package-smoke/console)"
echo "$console_output"
[ "$console_output" = "Hello world" ] || fail "unexpected console output: $console_output"

if [ "$(dpkg --print-architecture)" = "armhf" ] && command -v readelf >/dev/null 2>&1; then
    echo "==> checking ARM ELF attributes"
    readelf -A /tmp/fb-package-smoke/console | tee /tmp/fb-package-smoke/console.elf-attrs
    grep -Eq 'Tag_CPU_arch:.*v6' /tmp/fb-package-smoke/console.elf-attrs ||
        fail "armhf smoke binary is not tagged as ARMv6-compatible"
    grep -Eq 'Tag_ABI_VFP_args:.*VFP registers' /tmp/fb-package-smoke/console.elf-attrs ||
        fail "armhf smoke binary is not tagged for hard-float VFP calling convention"
fi

echo "==> compiling gfxlib smoke"
run fbc /tmp/fb-package-smoke/gfx.bas -x /tmp/fb-package-smoke/gfx
[ -x /tmp/fb-package-smoke/gfx ] || fail "gfx binary was not created"

echo "==> compiling gfxlib SCREEN 13 smoke"
run fbc /tmp/fb-package-smoke/gfx-screen13.bas -x /tmp/fb-package-smoke/gfx-screen13
[ -x /tmp/fb-package-smoke/gfx-screen13 ] || fail "gfx SCREEN 13 binary was not created"

echo "==> running gfxlib smoke"
if command -v xvfb-run >/dev/null 2>&1; then
    run_gfx_smoke /tmp/fb-package-smoke/gfx.out /tmp/fb-package-smoke/gfx.err xvfb-run -a /tmp/fb-package-smoke/gfx
else
    run_gfx_smoke /tmp/fb-package-smoke/gfx.out /tmp/fb-package-smoke/gfx.err /tmp/fb-package-smoke/gfx
fi

echo "==> compiling sfxlib smoke"
run fbc /tmp/fb-package-smoke/sfx.bas -x /tmp/fb-package-smoke/sfx
[ -x /tmp/fb-package-smoke/sfx ] || fail "sfx binary was not created"

echo "==> compiling sfxlib showcase"
[ -f /usr/share/freebasic/examples/sfxlib/showcase.bas ] || fail "sfxlib showcase example is not installed"
(
    cd /usr/share/freebasic/examples/sfxlib
    run fbc showcase.bas -x /tmp/fb-package-smoke/sfx-showcase
)
[ -x /tmp/fb-package-smoke/sfx-showcase ] || fail "sfxlib showcase binary was not created"

echo "==> running sfxlib smoke"
SFXLIB_DRIVER=null timeout 20s /tmp/fb-package-smoke/sfx > /tmp/fb-package-smoke/sfx.out 2> /tmp/fb-package-smoke/sfx.err || {
    cat /tmp/fb-package-smoke/sfx.out || true
    cat /tmp/fb-package-smoke/sfx.err || true
    fail "sfx binary failed"
}
cat /tmp/fb-package-smoke/sfx.out || true
grep -qx 'sfx-start' /tmp/fb-package-smoke/sfx.out || fail "sfx binary did not print sfx-start"
grep -qx 'sfx-end' /tmp/fb-package-smoke/sfx.out || fail "sfx binary did not print sfx-end"
if [ -s /tmp/fb-package-smoke/sfx.err ]; then
    cat /tmp/fb-package-smoke/sfx.err
    fail "sfx binary wrote stderr"
fi

if [ "${RUN_FBCTESTS:-0}" = "1" ]; then
    run_fbctests
fi

if [ "${RUN_EXAMPLEAGEDDON:-0}" = "1" ]; then
    run_exampleageddon
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
    local codename
    local arch

    [ -d "$ROOT/out/linux" ] || return 0

    while IFS= read -r dir; do
        rel="${dir#"$ROOT/out/linux/"}"
        IFS="/" read -r distro codename arch <<EOF
$rel
EOF
        [ -n "$distro" ] && [ -n "$codename" ] && [ -n "$arch" ] || continue
        target_matches_filters "$distro" "$codename" "$arch" || continue
        echo "${distro}|${codename}|${arch}|${dir}"
    done < <(find "$ROOT/out/linux" -mindepth 3 -maxdepth 3 -type d | sort)
}

TARGETS=()
while IFS= read -r target; do
    TARGETS+=("$target")
done < <(discover_targets)

if [ "$LIST_ONLY" -eq 1 ]; then
    for target in "${TARGETS[@]}"; do
        family=""
        glob=""
        IFS="|" read -r distro codename arch dir <<EOF
$target
EOF
        if ! family="$(package_family_for_target "$distro")"; then
            family="unknown"
        fi
        if ! glob="$(package_glob_for_family "$family")"; then
            glob="*"
        fi
        shopt -s nullglob
        packages=("$dir"/$glob)
        shopt -u nullglob
        echo "${distro}|${codename}|${arch}|${family}|${#packages[@]} package(s)|${dir}"
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
    local platform
    local image
    local log
    local family
    local package_glob
    local ports_args=()
    local docker_env_args=()
    local docker_platform_args=()
    local docker_cmd=()

    IFS="|" read -r distro codename arch dir <<EOF
$target
EOF

    family="$(package_family_for_target "$distro" || true)"
    [ -n "$family" ] || {
        echo "SKIPPED: ${distro}/${codename} (${arch}) is not in the known package-family map"
        return 0
    }

    if [ "$distro" = "debian" ] && is_debian_ports_test_arch "$arch"; then
        log="$dir/debian_ports_test.log"

        echo
        echo "============================================================"
        echo "Testing ${distro}/${codename} (${arch})"
        echo "Runner: build_scripts/debianports-test-freebasic.sh"
        echo "Packages: ${dir}"
        echo "Log: ${log}"
        echo "============================================================"

        ports_args=(--arch "$arch" --suite "$codename" --packages "$dir")
        if [ "$RUN_FBCTESTS" = "1" ]; then
            ports_args+=(--fbctests --fbctests-jobs "$FBCTESTS_JOBS")
            if [ -n "$FBCTESTS_UNIT_ARGS" ]; then
                ports_args+=(--fbctests-unit-args "$FBCTESTS_UNIT_ARGS")
            fi
        fi
        if [ "$RUN_EXAMPLEAGEDDON" = "1" ]; then
            ports_args+=(
                --exampleageddon
                --exampleageddon-jobs "$EXAMPLEAGEDDON_JOBS"
                --exampleageddon-compile-timeout "$EXAMPLEAGEDDON_COMPILE_TIMEOUT"
                --exampleageddon-run-timeout "$EXAMPLEAGEDDON_RUN_TIMEOUT"
            )
        fi
        if [ "$SKIP_HOST_DEPS" -eq 1 ]; then
            ports_args+=(--skip-host-deps)
        fi

        if ! "$ROOT/build_scripts/debianports-test-freebasic.sh" "${ports_args[@]}" &> "$log"; then
            echo "TEST FAILED: ${distro}/${codename} (${arch})"
            echo "Log: $log"
            return 1
        fi

        echo "TEST PASSED: ${distro}/${codename} (${arch})"
        return 0
    fi

    if [ "$family" != "deb" ]; then
        if ! image="$(image_for_nondeb_target "$distro" "$codename" "$arch")"; then
            echo "SKIPPED: ${distro}/${codename} (${arch}) is not in the known Docker target map"
            return 0
        fi

        if ! package_glob="$(package_glob_for_family "$family")"; then
            echo "SKIPPED: ${distro}/${codename} (${arch}) has no package glob for family $family"
            return 0
        fi

        shopt -s nullglob
        packages=("$dir"/$package_glob)
        shopt -u nullglob

        if [ "${#packages[@]}" -eq 0 ]; then
            echo "SKIPPED: ${distro}/${codename} (${arch}) has no ${package_glob} packages in $dir"
            return 0
        fi

        platform="$(docker_platform_for_nondeb_target "$arch")"
        log="$dir/docker_test.log"

        if [ -n "$platform" ]; then
            docker_platform_args=(--platform "$platform")
        fi

        case "$family" in
            apk)
                docker_cmd=(sh -lc "apk add --no-cache bash && bash /linux-package-test-runner.sh")
                ;;
            *)
                docker_cmd=(bash /linux-package-test-runner.sh)
                ;;
        esac

        echo
        echo "============================================================"
        echo "Testing ${distro}/${codename} (${arch})"
        echo "Package family: ${family}"
        echo "Docker image: ${image}"
        [ -z "$platform" ] || echo "Docker platform: ${platform}"
        echo "Packages: ${#packages[@]}"
        echo "Log: ${log}"
        echo "============================================================"

        if ! {
            run_root docker pull "${docker_platform_args[@]}" "$image" &&
            run_root docker run --rm \
                "${docker_platform_args[@]}" \
                -e PACKAGE_FAMILY="$family" \
                -e PACKAGE_DIR=/target \
                -e RESULTS_DIR=/target \
                -e RUN_FBCTESTS="$RUN_FBCTESTS" \
                -e FBCTESTS_JOBS="$FBCTESTS_JOBS" \
                -e FBCTESTS_UNIT_ARGS="$FBCTESTS_UNIT_ARGS" \
                -e RUN_EXAMPLEAGEDDON="$RUN_EXAMPLEAGEDDON" \
                -e EXAMPLEAGEDDON_JOBS="$EXAMPLEAGEDDON_JOBS" \
                -e EXAMPLEAGEDDON_COMPILE_TIMEOUT="$EXAMPLEAGEDDON_COMPILE_TIMEOUT" \
                -e EXAMPLEAGEDDON_RUN_TIMEOUT="$EXAMPLEAGEDDON_RUN_TIMEOUT" \
                -e FBC_PACKAGE_CODENAME="$codename" \
                -v "$dir:/target" \
                -v "$ROOT/tests:/source-tests:ro" \
                -v "$ROOT/inc:/source-inc:ro" \
                -v "$ROOT/examples:/source-root/examples:ro" \
                -v "$ROOT/build_scripts/exampleageddon-freebasic.py:/exampleageddon-freebasic.py:ro" \
                -v "$ROOT/build_scripts/linux-package-test-runner.sh:/linux-package-test-runner.sh:ro" \
                "$image" \
                "${docker_cmd[@]}"
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
        return 0
    fi

    if ! image="$(image_for_target "$distro" "$codename")"; then
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

    platform="$(docker_platform_for_target "$distro" "$codename" "$arch")"
    log="$dir/docker_test.log"

    if [ -n "$platform" ]; then
        docker_platform_args=(--platform "$platform")
    fi

    if [ "$distro" = "raspbian" ] && [ "$arch" = "armhf" ]; then
        docker_env_args+=(-e FB_SKIP_APT_UPDATE=1)
        docker_env_args+=(-e FB_RUNTIME_QEMU_CPU=arm1176)
    elif [ "$distro" = "raspbian" ]; then
        docker_env_args+=(-e FB_SKIP_APT_UPDATE=1)
    fi

    echo
    echo "============================================================"
    echo "Testing ${distro}/${codename} (${arch})"
    echo "Docker image: ${image}"
    [ -z "$platform" ] || echo "Docker platform: ${platform}"
    echo "Packages: ${#debs[@]}"
    echo "Log: ${log}"
    echo "============================================================"

    if ! {
        run_root docker pull "${docker_platform_args[@]}" "$image" &&
        run_root docker run --rm \
            "${docker_platform_args[@]}" \
            -e DEBIAN_FRONTEND=noninteractive \
            -e RUN_FBCTESTS="$RUN_FBCTESTS" \
            -e FBCTESTS_JOBS="$FBCTESTS_JOBS" \
            -e FBCTESTS_UNIT_ARGS="$FBCTESTS_UNIT_ARGS" \
            -e RUN_EXAMPLEAGEDDON="$RUN_EXAMPLEAGEDDON" \
            -e EXAMPLEAGEDDON_JOBS="$EXAMPLEAGEDDON_JOBS" \
            -e EXAMPLEAGEDDON_COMPILE_TIMEOUT="$EXAMPLEAGEDDON_COMPILE_TIMEOUT" \
            -e EXAMPLEAGEDDON_RUN_TIMEOUT="$EXAMPLEAGEDDON_RUN_TIMEOUT" \
            "${docker_env_args[@]}" \
            -v "$dir:/packages:ro" \
            -v "$dir:/results" \
            -v "$ROOT/examples:/source-root/examples:ro" \
            -v "$ROOT/tests:/source-tests:ro" \
            -v "$ROOT/inc:/source-inc:ro" \
            -v "$ROOT/build_scripts/exampleageddon-freebasic.py:/exampleageddon-freebasic.py:ro" \
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
