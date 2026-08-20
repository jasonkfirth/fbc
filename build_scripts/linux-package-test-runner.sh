#!/usr/bin/env bash
#
# Project: FreeBASIC Linux Package Factory
# ----------------------------------------
#
# File: linux-package-test-runner.sh
#
# Purpose:
#
#     Run package-install validation from inside a target Linux container.
#
# Responsibilities:
#
#     * install FreeBASIC packages for the package family selected by the host
#     * compile and run console, gfxlib, and sfxlib smoke programs
#     * optionally run fbctests from a mounted tests/ tree
#     * optionally run exampleageddon from a mounted examples/ tree
#     * write durable smoke and exampleageddon artifacts to the mounted out/
#       target directory
#
# This file intentionally does NOT contain:
#
#     * package build logic
#     * Docker image selection
#     * host binfmt setup
#     * Debian Ports chroot setup
#

set -euo pipefail

##############################################################################
# Helpers
##############################################################################

run() { echo "==> $*"; "$@"; }
fail() { echo "ERROR: $*" >&2; exit 1; }

disable_pacman_download_sandbox() {
    [ -f /etc/pacman.conf ] || return 0

    if grep -Eq '^[[:space:]]*DisableSandbox([[:space:]]|$)' /etc/pacman.conf; then
        return 0
    fi

    sed -i '/^\[options\]/a DisableSandbox' /etc/pacman.conf
}

pkg_family="${PACKAGE_FAMILY:-}"
packages_dir="${PACKAGE_DIR:-/packages}"
results_dir="${RESULTS_DIR:-/results}"

[ -n "$pkg_family" ] || fail "PACKAGE_FAMILY was not set"
[ -d "$packages_dir" ] || fail "package directory was not mounted: $packages_dir"
[ -d "$results_dir" ] || fail "results directory was not mounted: $results_dir"

export TERM="${TERM:-dumb}"
export FB_GFX_DRIVER="${FB_GFX_DRIVER:-null}"
export SFXLIB_DRIVER="${SFXLIB_DRIVER:-null}"

mkdir -p "$results_dir/smoke"

##############################################################################
# Package-manager setup
##############################################################################

install_apk_packages() {
    local packages

    run apk update
    run apk add --no-cache \
        bash \
        build-base \
        g++ \
        make \
        python3 \
        tar \
        xz \
        libffi-dev \
        ncurses-dev \
        gpm-dev \
        alsa-lib-dev \
        pulseaudio-dev \
        libx11-dev \
        libxext-dev \
        libxpm-dev \
        libxrandr-dev \
        libxrender-dev \
        mesa-dev \
        glu-dev

    mapfile -t packages < <(find "$packages_dir" -maxdepth 1 -type f -name '*.apk' | sort)
    [ "${#packages[@]}" -gt 0 ] || fail "no .apk packages found in $packages_dir"

    echo "==> installing FreeBASIC APK package(s)"
    run apk add --allow-untrusted "${packages[@]}"
}

install_rpm_packages() {
    local packages
    local package_path

    packages=()

    for package_path in "$packages_dir"/*.rpm; do
        [ -f "$package_path" ] || continue

        case "$package_path" in
            *src.rpm) continue ;;
        esac

        packages+=("$package_path")
    done

    [ "${#packages[@]}" -gt 0 ] || fail "no .rpm packages found in $packages_dir"

    if command -v dnf >/dev/null 2>&1; then
        run dnf clean all || true
        run dnf --refresh --setopt=install_weak_deps=False install -y "${packages[@]}"
        run dnf --setopt=install_weak_deps=False install -y \
            gcc gcc-c++ make python3 tar xz findutils diffutils which
        run dnf --setopt=install_weak_deps=False install -y \
            ncurses-devel gpm-devel libffi-devel \
            alsa-lib-devel pulseaudio-libs-devel \
            libX11-devel libXext-devel libXpm-devel libXrandr-devel \
            libXrender-devel mesa-libGL-devel mesa-libGLU-devel || true
        return 0
    fi

    if command -v yum >/dev/null 2>&1; then
        run yum install -y "${packages[@]}"
        run yum install -y \
            gcc gcc-c++ make python3 tar xz findutils diffutils which
        run yum install -y \
            ncurses-devel gpm-devel libffi-devel \
            alsa-lib-devel pulseaudio-libs-devel \
            libX11-devel libXext-devel libXpm-devel libXrandr-devel \
            libXrender-devel mesa-libGL-devel mesa-libGLU-devel || true
        return 0
    fi

    if command -v zypper >/dev/null 2>&1; then
        run zypper --non-interactive install -y --allow-unsigned-rpm "${packages[@]}"
        run zypper --non-interactive install -y \
            gcc gcc-c++ make python3 tar xz findutils diffutils which
        run zypper --non-interactive install -y \
            ncurses-devel gpm-devel libffi-devel \
            alsa-devel libpulse-devel \
            libX11-devel libXext-devel libXpm-devel libXrandr-devel \
            libXrender-devel Mesa-libGL-devel Mesa-libGLU-devel || true
        return 0
    fi

    fail "no supported RPM package manager found"
}

install_arch_packages() {
    local packages
    local package_path

    mapfile -t packages < <(find "$packages_dir" -maxdepth 1 -type f -name '*.pkg.tar.*' | sort)
    [ "${#packages[@]}" -gt 0 ] || fail "no .pkg.tar.* packages found in $packages_dir"

    disable_pacman_download_sandbox
    run pacman -Syu --noconfirm
    run pacman -S --noconfirm --needed \
        base \
        base-devel \
        gcc \
        make \
        python3 \
        tar \
        xz \
        findutils \
        diffutils \
        which \
        ncurses \
        gpm \
        libffi \
        alsa-lib \
        pulseaudio \
        libx11 \
        libxext \
        libxpm \
        libxrandr \
        libxrender \
        mesa \
        glu

    for package_path in "${packages[@]}"; do
        run pacman -U --noconfirm --needed "$package_path"
    done
}

configure_slackpkg_mirror() {
    local release="${FBC_PACKAGE_CODENAME:-}"
    local machine
    local mirror

    [ -f /etc/slackpkg/mirrors ] || return 0
    grep -Eq '^[[:space:]]*(http|https|ftp)://' /etc/slackpkg/mirrors && return 0

    machine="$(uname -m)"

    case "$machine/$release" in
        x86_64/current) mirror="https://mirrors.slackware.com/slackware/slackware64-current/" ;;
        x86_64/15.0) mirror="https://mirrors.slackware.com/slackware/slackware64-15.0/" ;;
        i?86/current) mirror="https://mirrors.slackware.com/slackware/slackware-current/" ;;
        i?86/15.0) mirror="https://mirrors.slackware.com/slackware/slackware-15.0/" ;;
        *) return 0 ;;
    esac

    echo "$mirror" >> /etc/slackpkg/mirrors
}

install_slackware_test_deps() {
    command -v slackpkg >/dev/null 2>&1 || return 0

    configure_slackpkg_mirror

    slackpkg -batch=on -default_answer=y update gpg || true
    slackpkg -batch=on -default_answer=y update || return 0
    slackpkg -batch=on -default_answer=y install \
        gcc gcc-g++ make python3 tar xz findutils diffutils which \
        ncurses gpm libffi alsa-lib pulseaudio libX11 libXext libXpm \
        libXrandr libXrender mesa libglvnd glu || true
}

install_slackware_packages() {
    local packages

    mapfile -t packages < <(find "$packages_dir" -maxdepth 1 -type f -name '*.txz' | sort)
    [ "${#packages[@]}" -gt 0 ] || fail "no .txz packages found in $packages_dir"

    install_slackware_test_deps

    echo "==> installing FreeBASIC Slackware package(s)"
    run installpkg "${packages[@]}"
}

install_packages() {
    case "$pkg_family" in
        apk) install_apk_packages ;;
        rpm) install_rpm_packages ;;
        arch) install_arch_packages ;;
        slackware) install_slackware_packages ;;
        *) fail "unsupported PACKAGE_FAMILY: $pkg_family" ;;
    esac
}

##############################################################################
# Smoke tests
##############################################################################

run_gfx_smoke() {
    local out="$1"
    local err="$2"
    local combined="$results_dir/smoke/gfx.combined"

    shift 2

    if timeout 20 "$@" > "$out" 2> "$err"; then
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

run_smoke_tests() {
    local smoke_dir="/tmp/fb-package-smoke"
    local console_output

    rm -rf "$smoke_dir"
    mkdir -p "$smoke_dir"

    cat > "$smoke_dir/console.bas" <<'FBEOF'
print "Hello world"
FBEOF

    cat > "$smoke_dir/gfx.bas" <<'FBEOF'
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

    cat > "$smoke_dir/gfx-screen13.bas" <<'FBEOF'
screen 13
screenset 0, 0
pset (8, 8), 12
print "gfx screen 13"
screen 0
FBEOF

    cat > "$smoke_dir/gfx3-screen13.bas" <<'FBEOF'
screen 13
screenset 0, 0
pset (8, 8), 12
print "gfxlib3 screen 13"
screen 0
FBEOF

    cat > "$smoke_dir/sfx.bas" <<'FBEOF'
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

    echo "==> verifying fbc"
    command -v fbc
    fbc -version
    uname -m

    echo "==> compiling console smoke"
    run fbc "$smoke_dir/console.bas" -x "$smoke_dir/console"

    echo "==> running console smoke"
    console_output="$("$smoke_dir/console")"
    echo "$console_output" | tee "$results_dir/smoke/console.out"
    [ "$console_output" = "Hello world" ] || fail "unexpected console output: $console_output"

    echo "==> compiling gfxlib smoke"
    run fbc "$smoke_dir/gfx.bas" -x "$smoke_dir/gfx"

    echo "==> compiling gfxlib SCREEN 13 smoke"
    run fbc "$smoke_dir/gfx-screen13.bas" -x "$smoke_dir/gfx-screen13"

    echo "==> compiling gfxlib3 SCREEN 13 smoke"
    run fbc -gfx3 "$smoke_dir/gfx3-screen13.bas" -x "$smoke_dir/gfx3-screen13"

    echo "==> running gfxlib smoke"
    if command -v xvfb-run >/dev/null 2>&1; then
        run_gfx_smoke "$results_dir/smoke/gfx.out" "$results_dir/smoke/gfx.err" xvfb-run -a "$smoke_dir/gfx"
    else
        run_gfx_smoke "$results_dir/smoke/gfx.out" "$results_dir/smoke/gfx.err" "$smoke_dir/gfx"
    fi

    echo "==> compiling sfxlib smoke"
    run fbc "$smoke_dir/sfx.bas" -x "$smoke_dir/sfx"

    echo "==> compiling sfxlib showcase"
    [ -f /usr/share/freebasic/examples/sfxlib/showcase.bas ] ||
        fail "sfxlib showcase example is not installed"
    (
        cd /usr/share/freebasic/examples/sfxlib
        run fbc showcase.bas -x "$smoke_dir/sfx-showcase"
    )

    echo "==> running sfxlib smoke"
    SFXLIB_DRIVER=null timeout 20 "$smoke_dir/sfx" > "$results_dir/smoke/sfx.out" 2> "$results_dir/smoke/sfx.err" || {
        cat "$results_dir/smoke/sfx.out" || true
        cat "$results_dir/smoke/sfx.err" || true
        fail "sfx binary failed"
    }
    cat "$results_dir/smoke/sfx.out" || true
    grep -qx 'sfx-start' "$results_dir/smoke/sfx.out" || fail "sfx binary did not print sfx-start"
    grep -qx 'sfx-end' "$results_dir/smoke/sfx.out" || fail "sfx binary did not print sfx-end"
    if [ -s "$results_dir/smoke/sfx.err" ]; then
        cat "$results_dir/smoke/sfx.err"
        fail "sfx binary wrote stderr"
    fi
}

##############################################################################
# fbctests
##############################################################################

job_count() {
    local value="$1"

    case "$value" in
        ''|*[!0-9]*)
            getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1
            ;;
        0)
            echo 1
            ;;
        *)
            echo "$value"
            ;;
    esac
}

run_fbctests() {
    local jobs
    local failed_log
    local detail_log

    [ -d /source-tests ] || fail "tests/ tree was not mounted at /source-tests"
    [ -d /source-inc ] || fail "inc/ tree was not mounted at /source-inc"
    [ -d /source-sfxlib ] || fail "src/sfxlib/ was not mounted at /source-sfxlib"

    jobs="$(job_count "${FBCTESTS_JOBS:-}")"

    echo "==> copying fbctests source"
    rm -rf /tmp/fbctests-source
    mkdir -p /tmp/fbctests-source/tests /tmp/fbctests-source/inc /tmp/fbctests-source/src/sfxlib
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
    (
        cd /source-sfxlib
        tar cf - .
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
            while IFS=: read -r detail_log _; do
                [ -n "$detail_log" ] || continue
                [ -f "$detail_log" ] || continue
                echo
                echo "==> $detail_log"
                cat "$detail_log"
            done < "$failed_log"
            fail "log-tests reported failures in $failed_log"
        fi
    done

    echo "==> fbctests passed"
}

##############################################################################
# exampleageddon
##############################################################################

run_exampleageddon() {
    local jobs
    local compile_timeout
    local run_timeout

    [ -d /source-root/examples ] || fail "examples/ tree was not mounted at /source-root/examples"
    [ -f /exampleageddon-freebasic.py ] || fail "exampleageddon runner was not mounted"

    jobs="$(job_count "${EXAMPLEAGEDDON_JOBS:-}")"
    compile_timeout="${EXAMPLEAGEDDON_COMPILE_TIMEOUT:-180}"
    run_timeout="${EXAMPLEAGEDDON_RUN_TIMEOUT:-10}"

    case "$compile_timeout" in ''|*[!0-9]*|0) compile_timeout=180 ;; esac
    case "$run_timeout" in ''|*[!0-9]*|0) run_timeout=10 ;; esac

    echo "==> running exampleageddon with ${jobs} job(s)"
    rm -rf "$results_dir/exampleageddon"
    mkdir -p "$results_dir/exampleageddon"
    run python3 /exampleageddon-freebasic.py \
        --root /source-root \
        --outdir "$results_dir/exampleageddon" \
        --prefix /usr \
        --include-dir /usr/include/freebasic \
        --fbc fbc \
        --jobs "$jobs" \
        --compile-timeout "$compile_timeout" \
        --run-timeout "$run_timeout" \
        --fail-on-self-contained
}

##############################################################################
# Main
##############################################################################

install_packages
run_smoke_tests

if [ "${RUN_FBCTESTS:-0}" = "1" ]; then
    run_fbctests
fi

if [ "${RUN_EXAMPLEAGEDDON:-0}" = "1" ]; then
    run_exampleageddon
fi

echo "==> TEST PASSED"

##############################################################################
# end of linux-package-test-runner.sh
##############################################################################
