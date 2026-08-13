#!/usr/bin/env bash
#
# Project: FreeBASIC Debian Ports Package Tests
# ---------------------------------------------
#
# File: debianports-test-freebasic.sh
#
# Purpose:
#
#     Test FreeBASIC Debian packages for Debian Ports architectures, plus
#     Debian architectures such as armel that are no longer present in sid.
#
# Responsibilities:
#
#     * create a Debian ports/main chroot for powerpc, ppc64, loong64, or armel
#     * run the chroot through the matching qemu user emulator
#     * install the packaged FreeBASIC .deb files
#     * compile and run console, gfxlib, and sfxlib smoke programs
#     * optionally run the full tests/ fbctests suite
#
# This file intentionally does NOT contain:
#
#     * Debian package building
#     * non-Debian package validation
#     * full-system virtual machine boot logic
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR
CLEANUP_SUCCESS=0

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

root_cmd() {
    if [ "$(id -u)" -eq 0 ]; then
        "$@"
    elif command -v sudo >/dev/null 2>&1; then
        sudo "$@"
    else
        die "this step requires root privileges; rerun as root or install sudo"
    fi
}

run_root() {
    echo "==> $*"
    root_cmd "$@"
}

usage() {
    cat <<EOF
Usage: ./build_scripts/debianports-test-freebasic.sh [options]

Options:
  --arch ARCH       Debian arch to test: armel, powerpc, ppc64, or loong64
  --suite SUITE     Debian suite to test (default: trixie for armel, sid otherwise)
  --packages DIR    Directory containing FreeBASIC .deb packages
  --buildroot DIR   Work directory for the chroot
  --reuse-root      Reuse an existing chroot if present
  --keep-root       Keep the chroot after testing
  --fbctests        Run the full tests/ suite with the packaged compiler
  --fbctests-jobs N Parallel jobs for --fbctests
  --fbctests-unit-args ARGS
                    Extra arguments passed to the fbc-tests binary
  --exampleageddon  Compile examples/ and run self-contained examples
  --exampleageddon-jobs N
                    Parallel jobs for --exampleageddon
  --exampleageddon-compile-timeout N
                    Per-example compile timeout in seconds
  --exampleageddon-run-timeout N
                    Per-example runtime timeout in seconds
  --skip-host-deps  Skip host dependency installation
  --help            Show this help text

The default package directory is out/linux/debian/<suite>/<arch>.
EOF
}

##############################################################################
# Options
##############################################################################

ARCH=""
SUITE="${DEBIAN_SUITE:-}"
PACKAGES_DIR=""
BUILDROOT="${BUILDROOT:-$ROOT/.build-debianports-test}"
REUSE_ROOT=0
KEEP_ROOT=0
RUN_FBCTESTS=0
FBCTESTS_JOBS="$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"
FBCTESTS_UNIT_ARGS=""
RUN_EXAMPLEAGEDDON=0
EXAMPLEAGEDDON_JOBS="$FBCTESTS_JOBS"
EXAMPLEAGEDDON_COMPILE_TIMEOUT=180
EXAMPLEAGEDDON_RUN_TIMEOUT=10
SKIP_HOST_DEPS=0
PORTS_MIRROR="${DEBIAN_PORTS_MIRROR:-http://ftp.ports.debian.org/debian-ports}"
DEBIAN_MAIN_MIRROR="${DEBIAN_MAIN_MIRROR:-http://deb.debian.org/debian}"

while [ $# -gt 0 ]; do
    case "$1" in
        --arch) ARCH="$2"; shift 2 ;;
        --suite) SUITE="$2"; shift 2 ;;
        --packages) PACKAGES_DIR="$2"; shift 2 ;;
        --buildroot) BUILDROOT="$2"; shift 2 ;;
        --reuse-root) REUSE_ROOT=1; shift ;;
        --keep-root) KEEP_ROOT=1; shift ;;
        --fbctests) RUN_FBCTESTS=1; shift ;;
        --fbctests-jobs) FBCTESTS_JOBS="$2"; shift 2 ;;
        --fbctests-unit-args) FBCTESTS_UNIT_ARGS="$2"; shift 2 ;;
        --exampleageddon) RUN_EXAMPLEAGEDDON=1; shift ;;
        --exampleageddon-jobs) EXAMPLEAGEDDON_JOBS="$2"; shift 2 ;;
        --exampleageddon-compile-timeout) EXAMPLEAGEDDON_COMPILE_TIMEOUT="$2"; shift 2 ;;
        --exampleageddon-run-timeout) EXAMPLEAGEDDON_RUN_TIMEOUT="$2"; shift 2 ;;
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

[ -n "$ARCH" ] || die "--arch is required"

case "$FBCTESTS_JOBS" in
    ''|*[!0-9]*|0) die "--fbctests-jobs must be a positive integer" ;;
esac

case "$EXAMPLEAGEDDON_JOBS" in
    ''|*[!0-9]*|0) die "--exampleageddon-jobs must be a positive integer" ;;
esac

case "$EXAMPLEAGEDDON_COMPILE_TIMEOUT" in
    ''|*[!0-9]*|0) die "--exampleageddon-compile-timeout must be a positive integer" ;;
esac

case "$EXAMPLEAGEDDON_RUN_TIMEOUT" in
    ''|*[!0-9]*|0) die "--exampleageddon-run-timeout must be a positive integer" ;;
esac

case "$ARCH" in
    armel)
        QEMU_BIN="qemu-arm"
        DEFAULT_SUITE="trixie"
        APT_SOURCE_KIND="debian"
        ;;
    powerpc)
        QEMU_BIN="qemu-ppc"
        DEFAULT_SUITE="sid"
        APT_SOURCE_KIND="ports"
        ;;
    ppc64)
        QEMU_BIN="qemu-ppc64"
        DEFAULT_SUITE="sid"
        APT_SOURCE_KIND="ports"
        ;;
    loong64)
        QEMU_BIN="qemu-loongarch64"
        DEFAULT_SUITE="sid"
        APT_SOURCE_KIND="debian"
        ;;
    *)
        die "unsupported Debian arch: $ARCH"
        ;;
esac

[ -n "$SUITE" ] || SUITE="$DEFAULT_SUITE"
[ -n "$PACKAGES_DIR" ] || PACKAGES_DIR="$ROOT/out/linux/debian/$SUITE/$ARCH"
PACKAGES_DIR="$(cd "$PACKAGES_DIR" && pwd -P)"
ROOTFS="$BUILDROOT/$ARCH/rootfs"
QEMU_HOST_PATH="$(command -v "$QEMU_BIN" || true)"

##############################################################################
# Host dependencies
##############################################################################

install_host_deps() {
    [ "$SKIP_HOST_DEPS" -eq 0 ] || return 0

    if command -v apt-get >/dev/null 2>&1; then
        msg "installing host dependencies via apt"
        run_root apt-get update -y
        run_root apt-get install -y --no-install-recommends \
            debootstrap \
            qemu-user \
            qemu-user-binfmt \
            binfmt-support \
            debian-archive-keyring \
            debian-ports-archive-keyring \
            ca-certificates \
            rsync
        return 0
    fi

    if command -v dnf >/dev/null 2>&1; then
        msg "installing host dependencies via dnf"
        run_root dnf install -y debootstrap qemu-user-static ca-certificates rsync
        return 0
    fi

    if command -v pacman >/dev/null 2>&1; then
        msg "installing host dependencies via pacman"
        run_root pacman -Sy --noconfirm debootstrap qemu-user-static ca-certificates rsync
        return 0
    fi

    die "unsupported host package manager; install debootstrap, qemu-user, and rsync manually"
}

##############################################################################
# Chroot management
##############################################################################

MOUNTS=()

cleanup_mounts() {
    local i
    local target

    for ((i=${#MOUNTS[@]} - 1; i >= 0; i--)); do
        target="${MOUNTS[$i]}"
        if mountpoint -q "$target"; then
            root_cmd umount -R "$target" || true
        fi
    done
}

cleanup_root() {
    cleanup_mounts

    [ "$CLEANUP_SUCCESS" -eq 1 ] || return 0

    if [ "$KEEP_ROOT" -eq 0 ] && [ "$REUSE_ROOT" -eq 0 ]; then
        root_cmd rm -rf "$BUILDROOT" || true
    fi
}

trap cleanup_root EXIT

mount_one() {
    local source="$1"
    local target="$2"
    local mode="$3"
    local fstype
    local mounted=0

    run_root mkdir -p "$target"
    if mountpoint -q "$target"; then
        mounted=1
    fi

    case "$mode:$mounted" in
        proc:1|devpts:1)
            fstype="$(findmnt -rn --mountpoint "$target" -o FSTYPE 2>/dev/null | head -n 1 || true)"
            if [ "$fstype" != "$mode" ]; then
                root_cmd umount -R "$target" 2>/dev/null || true
                mounted=0
            fi
            ;;
    esac

    if [ "$mounted" -eq 0 ]; then
        case "$mode" in
            proc)
                run_root mount -t proc proc "$target"
                ;;
            devpts)
                run_root mount -t devpts devpts "$target"
                ;;
            rbind)
                run_root mount --rbind "$source" "$target"
                run_root mount --make-rslave "$target"
                ;;
            bind)
                run_root mount --bind "$source" "$target"
                ;;
            *)
                die "internal error: unsupported mount mode: $mode"
                ;;
        esac

        MOUNTS+=("$target")
    fi

    if [ "$mode" = "proc" ] && [ ! -r "$target/self/status" ]; then
        root_cmd umount -R "$target" 2>/dev/null || true
        run_root mount -t proc proc "$target"
        MOUNTS+=("$target")
    fi

    if [ "$mode" = "devpts" ] && [ ! -e "$target/ptmx" ]; then
        root_cmd umount -R "$target" 2>/dev/null || true
        run_root mount -t devpts devpts "$target"
        MOUNTS+=("$target")
    fi
}

mount_chroot_filesystems() {
    mount_one proc "$ROOTFS/proc" proc
    mount_one /sys "$ROOTFS/sys" rbind
    mount_one /dev "$ROOTFS/dev" rbind
    mount_one devpts "$ROOTFS/dev/pts" devpts

    [ -r "$ROOTFS/proc/self/status" ] || die "procfs was not mounted in $ROOTFS/proc"
    [ -e "$ROOTFS/dev/pts/ptmx" ] || die "devpts was not mounted in $ROOTFS/dev/pts"
}

chroot_run() {
    run_root chroot "$ROOTFS" "/usr/bin/$QEMU_BIN" "$@"
}

verify_chroot_filesystems() {
    chroot_run /bin/sh -c \
        'test -r /proc/self/status && test -e /dev/pts/ptmx && test -d /sys/kernel'
}

chroot_shell() {
    local script="$1"

    chroot_run /bin/bash -lc "$script"
}

debootstrap_keyring_args() {
    local keyring
    local label

    case "$APT_SOURCE_KIND" in
        debian)
            keyring="/usr/share/keyrings/debian-archive-keyring.gpg"
            label="Debian archive"
            ;;
        ports|ports-with-main)
            keyring="/usr/share/keyrings/debian-ports-archive-keyring.gpg"
            label="Debian Ports"
            ;;
        *)
            die "internal error: unsupported apt source kind: $APT_SOURCE_KIND"
            ;;
    esac

    if [ -f "$keyring" ]; then
        printf '%s\n' "--keyring=$keyring"
        return 0
    fi

    echo "WARNING: $label keyring not found; debootstrap will skip Release signature verification" >&2
    printf '%s\n' "--no-check-gpg"
}

debootstrap_mirror() {
    case "$APT_SOURCE_KIND" in
        debian)
            printf '%s\n' "$DEBIAN_MAIN_MIRROR"
            ;;
        ports|ports-with-main)
            printf '%s\n' "$PORTS_MIRROR"
            ;;
        *)
            die "internal error: unsupported apt source kind: $APT_SOURCE_KIND"
            ;;
    esac
}

write_apt_sources() {
    local sources="$ROOTFS/etc/apt/sources.list"

    root_cmd mkdir -p "$(dirname "$sources")"
    if [ "$APT_SOURCE_KIND" = "debian" ]; then
        root_cmd tee "$sources" >/dev/null <<EOF
deb [signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] ${DEBIAN_MAIN_MIRROR} ${SUITE} main
EOF
    elif [ "$APT_SOURCE_KIND" = "ports-with-main" ]; then
        root_cmd tee "$sources" >/dev/null <<EOF
deb [signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] ${DEBIAN_MAIN_MIRROR} ${SUITE} main
deb [signed-by=/usr/share/keyrings/debian-ports-archive-keyring.gpg] ${PORTS_MIRROR} ${SUITE} main
deb [signed-by=/usr/share/keyrings/debian-ports-archive-keyring.gpg] ${PORTS_MIRROR} unreleased main
EOF
    else
        root_cmd tee "$sources" >/dev/null <<EOF
deb [signed-by=/usr/share/keyrings/debian-ports-archive-keyring.gpg] ${PORTS_MIRROR} ${SUITE} main
deb [signed-by=/usr/share/keyrings/debian-ports-archive-keyring.gpg] ${PORTS_MIRROR} unreleased main
EOF
    fi
}

prepare_chroot() {
    local keyring_arg

    need_cmd debootstrap

    QEMU_HOST_PATH="$(command -v "$QEMU_BIN" || true)"
    [ -n "$QEMU_HOST_PATH" ] || die "missing emulator: $QEMU_BIN"

    if [ "$REUSE_ROOT" -eq 0 ] || [ ! -x "$ROOTFS/bin/sh" ]; then
        msg "creating Debian $SUITE chroot for $ARCH"
        run_root rm -rf "$ROOTFS"
        run_root mkdir -p "$ROOTFS"

        keyring_arg="$(debootstrap_keyring_args)"
        run_root debootstrap \
            --foreign \
            --arch="$ARCH" \
            --variant=minbase \
            --include=debian-archive-keyring,debian-ports-archive-keyring,ca-certificates \
            "$keyring_arg" \
            "$SUITE" \
            "$ROOTFS" \
            "$(debootstrap_mirror)"

        run_root install -m 755 "$QEMU_HOST_PATH" "$ROOTFS/usr/bin/$QEMU_BIN"
        write_apt_sources
        [ ! -f /etc/resolv.conf ] || run_root cp -L /etc/resolv.conf "$ROOTFS/etc/resolv.conf"

        mount_chroot_filesystems
        verify_chroot_filesystems
        chroot_run /bin/sh /debootstrap/debootstrap --second-stage

        # The foreign second stage may unmount procfs while completing the
        # base system.  Package post-install scripts such as OpenJDK require
        # it, so restore and verify every chroot filesystem before testing.
        mount_chroot_filesystems
        verify_chroot_filesystems
    else
        msg "reusing Debian $SUITE chroot for $ARCH"
        run_root install -m 755 "$QEMU_HOST_PATH" "$ROOTFS/usr/bin/$QEMU_BIN"
        write_apt_sources
        [ ! -f /etc/resolv.conf ] || run_root cp -L /etc/resolv.conf "$ROOTFS/etc/resolv.conf"
        mount_chroot_filesystems
        verify_chroot_filesystems
    fi
}

##############################################################################
# Test staging
##############################################################################

stage_packages() {
    local debs=()

    shopt -s nullglob
    debs=("$PACKAGES_DIR"/*.deb)
    shopt -u nullglob

    [ "${#debs[@]}" -gt 0 ] || die "no .deb packages found in $PACKAGES_DIR"

    msg "staging FreeBASIC packages"
    run_root rm -rf "$ROOTFS/packages"
    run_root mkdir -p "$ROOTFS/packages"
    run_root cp "${debs[@]}" "$ROOTFS/packages/"
}

stage_sources_for_fbctests() {
    [ "$RUN_FBCTESTS" -eq 1 ] || return 0

    msg "staging fbctests source"
    run_root rm -rf \
        "$ROOTFS/source-tests" \
        "$ROOTFS/source-inc" \
        "$ROOTFS/source-sfxlib"
    run_root mkdir -p \
        "$ROOTFS/source-tests" \
        "$ROOTFS/source-inc" \
        "$ROOTFS/source-sfxlib"

    root_cmd rsync -a --delete \
        --exclude='*.o' \
        --exclude='*.a' \
        --exclude='fbc-tests' \
        --exclude='unit-tests.inc' \
        --exclude='unit-tests-obj.lst' \
        --exclude='log-tests-*.inc' \
        --exclude='failed-log-tests-*.inc' \
        --exclude='log-tests-*.lst' \
        --exclude='log-tests-results-*.log' \
        "$ROOT/tests/" "$ROOTFS/source-tests/"

    root_cmd rsync -a --delete "$ROOT/inc/" "$ROOTFS/source-inc/"
    root_cmd rsync -a --delete --exclude='obj' \
        "$ROOT/src/sfxlib/" "$ROOTFS/source-sfxlib/"
}

stage_sources_for_exampleageddon() {
    [ "$RUN_EXAMPLEAGEDDON" -eq 1 ] || return 0

    msg "staging exampleageddon source"
    run_root rm -rf "$ROOTFS/source-root" "$ROOTFS/exampleageddon-freebasic.py"
    run_root mkdir -p "$ROOTFS/source-root/examples"

    root_cmd rsync -a --delete "$ROOT/examples/" "$ROOTFS/source-root/examples/"
    root_cmd install -m 644 \
        "$ROOT/build_scripts/exampleageddon-freebasic.py" \
        "$ROOTFS/exampleageddon-freebasic.py"

    run_root mkdir -p "$ROOTFS/results"
    mount_one "$PACKAGES_DIR" "$ROOTFS/results" bind
}

write_test_runner() {
    local runner="$ROOTFS/test-freebasic-packages.sh"

    root_cmd tee "$runner" >/dev/null <<'TEST_RUNNER_EOF'
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
            if grep -Eiq 'error loading shared library|relocation error|undefined symbol|cannot execute|exec format error|no such file or directory|ld-linux|ld.so|ld64|ld-musl' "$combined"; then
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

    if grep -Eiq 'error loading shared library|relocation error|undefined symbol|cannot execute|exec format error|no such file or directory|ld-linux|ld.so|ld64|ld-musl' "$combined"; then
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

    [ -d /source-tests ] || fail "tests/ tree was not staged at /source-tests"
    [ -d /source-inc ] || fail "inc/ tree was not staged at /source-inc"
    [ -d /source-sfxlib ] || fail "src/sfxlib/ was not staged at /source-sfxlib"

    jobs="$(fbctests_jobs)"

    echo "==> installing fbctests dependencies"
    run apt-get install -y --no-install-recommends make g++

    echo "==> copying fbctests source"
    rm -rf /tmp/fbctests-source
    mkdir -p \
        /tmp/fbctests-source/tests \
        /tmp/fbctests-source/inc \
        /tmp/fbctests-source/src/sfxlib
    (
        cd /source-tests
        tar -cf - .
    ) | (
        cd /tmp/fbctests-source/tests
        tar xf -
    )
    (
        cd /source-inc
        tar -cf - .
    ) | (
        cd /tmp/fbctests-source/inc
        tar xf -
    )
    (
        cd /source-sfxlib
        tar --exclude='obj' -cf - .
    ) | (
        cd /tmp/fbctests-source/src/sfxlib
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

    [ -d /source-root/examples ] || fail "examples/ tree was not staged at /source-root/examples"
    [ -f /exampleageddon-freebasic.py ] || fail "exampleageddon runner was not staged"
    [ -d /results ] || fail "results directory was not mounted at /results"

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
export FBGFX="${FBGFX:-null}"
export FB_GFX_DRIVER="${FB_GFX_DRIVER:-null}"
export SFXLIB_DRIVER="${SFXLIB_DRIVER:-null}"

run apt-get update -y

echo "==> installing FreeBASIC packages"
run apt-get install -y --install-recommends --reinstall /packages/*.deb

echo "==> installing FreeBASIC full package recommendations"
run apt-get install -y --install-recommends freebasic-full

echo "==> installing smoke-test development dependencies"
run apt-get install -y --no-install-recommends \
    libasound2-dev \
    libpulse-dev

echo "==> verifying fbc"
command -v fbc
fbc -version

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

echo "==> compiling gfxlib smoke"
run fbc /tmp/fb-package-smoke/gfx.bas -x /tmp/fb-package-smoke/gfx
[ -x /tmp/fb-package-smoke/gfx ] || fail "gfx binary was not created"

echo "==> compiling gfxlib SCREEN 13 smoke"
run fbc /tmp/fb-package-smoke/gfx-screen13.bas -x /tmp/fb-package-smoke/gfx-screen13
[ -x /tmp/fb-package-smoke/gfx-screen13 ] || fail "gfx SCREEN 13 binary was not created"

echo "==> running gfxlib smoke"
run_gfx_smoke /tmp/fb-package-smoke/gfx.out /tmp/fb-package-smoke/gfx.err /tmp/fb-package-smoke/gfx

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

    run_root chmod 755 "$runner"
}

##############################################################################
# Main
##############################################################################

install_host_deps
prepare_chroot
stage_packages
stage_sources_for_fbctests
stage_sources_for_exampleageddon
write_test_runner

msg "running Debian package tests for $ARCH on $SUITE"
chroot_run /usr/bin/env \
    RUN_FBCTESTS="$RUN_FBCTESTS" \
    FBCTESTS_JOBS="$FBCTESTS_JOBS" \
    FBCTESTS_UNIT_ARGS="$FBCTESTS_UNIT_ARGS" \
    RUN_EXAMPLEAGEDDON="$RUN_EXAMPLEAGEDDON" \
    EXAMPLEAGEDDON_JOBS="$EXAMPLEAGEDDON_JOBS" \
    EXAMPLEAGEDDON_COMPILE_TIMEOUT="$EXAMPLEAGEDDON_COMPILE_TIMEOUT" \
    EXAMPLEAGEDDON_RUN_TIMEOUT="$EXAMPLEAGEDDON_RUN_TIMEOUT" \
    bash /test-freebasic-packages.sh

msg "Debian package tests passed for $ARCH on $SUITE"
CLEANUP_SUCCESS=1

##############################################################################
# end of debianports-test-freebasic.sh
##############################################################################
