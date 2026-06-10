#!/usr/bin/env bash
#
# Project: FreeBASIC Linux Package Factory
# ----------------------------------------
#
# File: linux-cross-build-freebasic-matrix.sh
#
# Purpose:
#
#     Define the top-level Linux package build graph for the fast package
#     factory. This script is intended to become the single command used to
#     build all Linux distribution packages into the out/ tree.
#
#     Each distro/release row names the Docker image that owns that packaging
#     environment. The build model is native packaging tools per container,
#     with cross toolchains installed inside that container for the target CPU
#     architectures.
#
# Responsibilities:
#
#     * list distro/release/package-family targets
#     * name the Docker image that should run each distro release build
#     * compile the documentation maintenance tools before package builds
#     * define the intended output directory layout
#     * provide one stable entry point above distro-specific builders
#
# This file intentionally does NOT contain:
#
#     * the per-distro package build implementation
#     * target package validation
#     * qemu/binfmt native-emulation build logic
#

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
CLEANUP_SUCCESS=0
CLEANUP_DIRS=(
    "$ROOT/.build-docs"
    "$ROOT/.build-debianubuntu-cross"
    "$ROOT/.build-debianubuntu-xbox"
)

cleanup_build_roots() {
    local path

    [ "$CLEANUP_SUCCESS" -eq 1 ] || return 0

    for path in "${CLEANUP_DIRS[@]}"; do
        [ -n "$path" ] || continue
        rm -rf "$path" 2>/dev/null || true
    done
}

trap cleanup_build_roots EXIT

##############################################################################
# Helpers
##############################################################################

die() { echo "ERROR: $*" >&2; exit 1; }

usage() {
    cat <<EOF
Usage: ./build_scripts/linux-cross-build-freebasic-matrix.sh [options]

Options:
  --distro NAME     Limit to one distro name
  --release NAME    Limit to one release/codename
  --arch ARCH       Limit to one package architecture
  --family NAME     Limit to one package family (deb, apk, rpm, archlinux, slackware, vm)
  --jobs N          Parallel make jobs for package builds
  --keep-going      Continue after package-family failures
  --skip-host-deps  Skip host dependency installation in family builders
  --skip-bootstrap  Reuse existing bootstrap source tarballs where supported
  --skip-docs       Skip the offline documentation tool build preflight
  --execute         Attempt package builds
  --list            Show the planned package factory matrix
  --help            Show this help text

This is the intended top-level Linux package factory entry point.

The implementation dispatches each package family to the builder that owns
that packaging format: Debian/Ubuntu/Raspbian, APK, RPM, Arch Linux, and
Slackware.

Each row is a Docker packaging environment plus one target CPU architecture.
EOF
}

##############################################################################
# Options
##############################################################################

DISTRO_FILTER=""
RELEASE_FILTER=""
ARCH_FILTER=""
FAMILY_FILTER=""
EXECUTE=0
LIST_ONLY=0
KEEP_GOING=0
SKIP_HOST_DEPS=0
SKIP_BOOTSTRAP=0
SKIP_DOCS=0

if command -v nproc >/dev/null 2>&1; then
    MAKE_JOBS="$(nproc)"
else
    MAKE_JOBS=1
fi

while [ $# -gt 0 ]; do
    case "$1" in
        --distro) DISTRO_FILTER="$2"; shift 2 ;;
        --release) RELEASE_FILTER="$2"; shift 2 ;;
        --arch) ARCH_FILTER="$2"; shift 2 ;;
        --family) FAMILY_FILTER="$2"; shift 2 ;;
        --jobs) MAKE_JOBS="$2"; shift 2 ;;
        --keep-going) KEEP_GOING=1; shift ;;
        --skip-host-deps) SKIP_HOST_DEPS=1; shift ;;
        --skip-bootstrap) SKIP_BOOTSTRAP=1; shift ;;
        --skip-docs) SKIP_DOCS=1; shift ;;
        --execute) EXECUTE=1; shift ;;
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

case "$MAKE_JOBS" in
    ''|*[!0-9]*|0) die "--jobs must be a positive integer" ;;
esac

##############################################################################
# Architecture sets
##############################################################################

DEB_ARCHES=(
    amd64
    arm64
    armhf
    ppc64el
    s390x
    riscv64
)

DEB_EXTENDED_ARCHES=(
    "${DEB_ARCHES[@]}"
    armel
    loong64
)

DEB_SID_ARCHES=(
    "${DEB_ARCHES[@]}"
    loong64
    powerpc
    ppc64
)

APK_ARCHES=(
    x86_64
    aarch64
    armv7
    ppc64le
    s390x
    riscv64
)

RPM_COMMON_ARCHES=(
    x86_64
    aarch64
    ppc64le
    s390x
)

RPM_RISCV_ARCHES=(
    "${RPM_COMMON_ARCHES[@]}"
    riscv64
)

SLACKWARE_ARCHES=(
    x86_64
    i586
    aarch64
)

VM_ARCHES=(
    x86_64
)

ARCHLINUX_ARCHES=(
    x86_64
    aarch64
    armv7h
    riscv64
)

##############################################################################
# Distro/release matrix
##############################################################################

TARGETS=(
    "deb|ubuntu|ubuntu:22.04|jammy|debianubuntu-cross-build-freebasic-matrix.sh"
    "deb|ubuntu|ubuntu:24.04|noble|debianubuntu-cross-build-freebasic-matrix.sh"
    "deb|ubuntu|ubuntu:24.10|oracular|debianubuntu-cross-build-freebasic-matrix.sh"
    "deb|ubuntu|ubuntu:25.04|plucky|debianubuntu-cross-build-freebasic-matrix.sh"
    "deb|ubuntu|ubuntu:25.10|questing|debianubuntu-cross-build-freebasic-matrix.sh"
    "deb|ubuntu|ubuntu:26.04|resolute|debianubuntu-cross-build-freebasic-matrix.sh"
    "deb|debian|debian:12|bookworm|debianubuntu-cross-build-freebasic-matrix.sh"
    "deb|debian|debian:13|trixie|debianubuntu-cross-build-freebasic-matrix.sh"
    "deb|debian|debian:sid|sid|debianubuntu-cross-build-freebasic-matrix.sh"
    "deb|raspbian|badaix/raspios-lite:trixie|trixie|debianubuntu-cross-build-freebasic-matrix.sh"
    "deb|raspbian|badaix/raspios-lite:bookworm|bookworm|debianubuntu-cross-build-freebasic-matrix.sh"
    "deb|raspbian|badaix/raspios-buster-armhf-lite:latest|buster|debianubuntu-cross-build-freebasic-matrix.sh"
    "apk|alpine|alpine:3.23|3.23|apk-cross-build-freebasic-matrix.sh"
    "apk|alpine|alpine:3.22|3.22|apk-cross-build-freebasic-matrix.sh"
    "apk|alpine|alpine:3.21|3.21|apk-cross-build-freebasic-matrix.sh"
    "apk|alpine|alpine:edge|edge|apk-cross-build-freebasic-matrix.sh"
    "apk|postmarketos|adamthiede/postmarketos:edge|edge|apk-cross-build-freebasic-matrix.sh"
    "rpm|fedora|fedora:44|44|rpm-cross-build-freebasic-matrix.sh"
    "rpm|fedora|fedora:rawhide|rawhide|rpm-cross-build-freebasic-matrix.sh"
    "rpm|rocky|rockylinux:10|10|rpm-cross-build-freebasic-matrix.sh"
    "rpm|rocky|rockylinux:9|9|rpm-cross-build-freebasic-matrix.sh"
    "rpm|almalinux|almalinux:10|10|rpm-cross-build-freebasic-matrix.sh"
    "rpm|almalinux|almalinux:9|9|rpm-cross-build-freebasic-matrix.sh"
    "rpm|opensuse|opensuse/tumbleweed|tumbleweed|rpm-cross-build-freebasic-matrix.sh"
    "slackware|slackware|vbatts/slackware:15.0|15.0|slackware-cross-build-freebasic-matrix.sh"
    "slackware|slackware|vbatts/slackware:current|current|slackware-cross-build-freebasic-matrix.sh"
    "archlinux|archlinux|archlinux/archlinux:base|current|archlinux-build-freebasic.sh"
    "vm|haiku||x86-64|haiku-vm-build-freebasic.sh"
    "vm|haiku||i386|haiku-vm-build-freebasic.sh"
    "vm|netbsd||x86-64|netbsd-vm-build-freebasic.sh"
    "vm|openbsd||x86-64|openbsd-vm-build-freebasic.sh"
    "vm|freebsd||x86-64|freebsd-vm-build-freebasic.sh"
    "vm|dragonfly||x86-64|dragonfly-vm-build-freebasic.sh"
)

arches_for_target() {
    local family="$1"
    local distro="$2"
    local release="$3"

    case "$family/$distro/$release" in
        deb/raspbian/bookworm)
            printf '%s\n' armhf arm64
            ;;
        deb/debian/trixie)
            printf '%s\n' "${DEB_EXTENDED_ARCHES[@]}"
            ;;
        deb/debian/sid)
            printf '%s\n' "${DEB_SID_ARCHES[@]}"
            ;;
        deb/raspbian/*)
            printf '%s\n' armhf
            ;;
        deb/*)
            printf '%s\n' "${DEB_ARCHES[@]}"
            ;;
        apk/*)
            printf '%s\n' "${APK_ARCHES[@]}"
            ;;
        rpm/rocky/10)
            printf '%s\n' "${RPM_RISCV_ARCHES[@]}"
            ;;
        rpm/*)
            printf '%s\n' "${RPM_COMMON_ARCHES[@]}"
            ;;
        slackware/*)
            printf '%s\n' "${SLACKWARE_ARCHES[@]}"
            ;;
        archlinux/*)
            printf '%s\n' "${ARCHLINUX_ARCHES[@]}"
            ;;
        vm/haiku/x86-64)
            printf '%s\n' x86_64
            ;;
        vm/haiku/i386)
            printf '%s\n' i386
            ;;
        vm/*)
            printf '%s\n' x86_64
            ;;
        *)
            die "unsupported package family: $family"
            ;;
    esac
}

outdir_for_target() {
    local family="$1"
    local distro="$2"
    local release="$3"
    local arch="$4"

    case "$family" in
        vm)
            echo "$ROOT/out/$distro/$release"
            ;;
        *)
            echo "$ROOT/out/linux/${distro}/${release}/${arch}"
            ;;
    esac
}

target_matches_filters() {
    local family="$1"
    local distro="$2"
    local release="$3"
    local arch="$4"

    if [ -n "$FAMILY_FILTER" ] && [ "$FAMILY_FILTER" != "$family" ]; then
        return 1
    fi

    if [ -n "$DISTRO_FILTER" ] && [ "$DISTRO_FILTER" != "$distro" ]; then
        return 1
    fi

    if [ -n "$RELEASE_FILTER" ] && [ "$RELEASE_FILTER" != "$release" ]; then
        return 1
    fi

    if [ -n "$ARCH_FILTER" ] && [ "$ARCH_FILTER" != "$arch" ]; then
        return 1
    fi

    return 0
}

##############################################################################
# Documentation preflight
##############################################################################

doc_fbc() {
    if [ -x "$ROOT/bin/fbc" ]; then
        echo "$ROOT/bin/fbc -i $ROOT/inc -p $ROOT/.build-docs/lib"
        return 0
    fi

    command -v fbc >/dev/null 2>&1 || die "cannot build documentation tools without fbc"
    command -v fbc
}

find_runtime_libcurl() {
    local candidate

    for candidate in \
        /lib/x86_64-linux-gnu/libcurl.so.4 \
        /usr/lib/x86_64-linux-gnu/libcurl.so.4 \
        /lib/aarch64-linux-gnu/libcurl.so.4 \
        /usr/lib/aarch64-linux-gnu/libcurl.so.4 \
        /usr/lib64/libcurl.so.4 \
        /usr/lib/libcurl.so.4 \
        /lib/libcurl.so.4
    do
        if [ -e "$candidate" ]; then
            echo "$candidate"
            return 0
        fi
    done

    if command -v ldconfig >/dev/null 2>&1; then
        ldconfig -p 2>/dev/null | awk '/libcurl\.so\.4[[:space:]]/ { print $NF; exit }'
    fi
}

prepare_doc_link_libs() {
    local libcurl

    mkdir -p "$ROOT/.build-docs/lib"

    if [ ! -e "$ROOT/.build-docs/lib/libcurl.so" ]; then
        libcurl="$(find_runtime_libcurl || true)"
        if [ -n "$libcurl" ]; then
            ln -sfn "$libcurl" "$ROOT/.build-docs/lib/libcurl.so"
        fi
    fi
}

build_doc_tools() {
    local fbc

    [ "$SKIP_DOCS" -eq 0 ] || return 0

    prepare_doc_link_libs
    fbc="$(doc_fbc)"

    echo
    echo "============================================================"
    echo "Building documentation tools"
    echo "============================================================"
    echo "Command: make -C doc FBC=$fbc"
    echo "Note: this compiles the offline doc tools only; it does not refresh doc/manual/cache."

    MAKEFLAGS= make -C "$ROOT/doc" "FBC=$fbc"
}

##############################################################################
# Planned graph
##############################################################################

list_plan() {
    local target
    local family
    local distro
    local image
    local release
    local script
    local arch
    local outdir

    for target in "${TARGETS[@]}"; do
        IFS="|" read -r family distro image release script <<EOF
$target
EOF

        while IFS= read -r arch; do
            target_matches_filters "$family" "$distro" "$release" "$arch" || continue
            outdir="$(outdir_for_target "$family" "$distro" "$release" "$arch")"
            printf '%s|%s|%s|%s|%s|%s|%s\n' \
                "$family" "$distro" "$release" "$arch" "$image" "$script" "$outdir"
        done < <(arches_for_target "$family" "$distro" "$release")
    done
}

##############################################################################
# Execution
##############################################################################

execute_plan() {
    local failures
    local plan
    local ran

    family_enabled() {
        local family="$1"

        [ -z "$FAMILY_FILTER" ] || [ "$FAMILY_FILTER" = "$family" ]
    }

    distro_allowed_for_family() {
        local family="$1"

        [ -n "$DISTRO_FILTER" ] || return 0

        case "$family" in
            deb)
                case "$DISTRO_FILTER" in
                    ubuntu|debian|raspbian) return 0 ;;
                esac
                ;;
            apk)
                case "$DISTRO_FILTER" in
                    alpine|postmarketos) return 0 ;;
                esac
                ;;
            rpm)
                case "$DISTRO_FILTER" in
                    fedora|rocky|almalinux|opensuse) return 0 ;;
                esac
                ;;
            archlinux)
                [ "$DISTRO_FILTER" = "archlinux" ] && return 0
                ;;
            slackware)
                [ "$DISTRO_FILTER" = "slackware" ] && return 0
                ;;
            vm)
                case "$DISTRO_FILTER" in
                    haiku|netbsd|openbsd|freebsd|dragonfly) return 0 ;;
                esac
                ;;
        esac

        return 1
    }

    family_has_selected_targets() {
        local wanted_family="$1"
        local family

        while IFS="|" read -r family _; do
            [ "$family" = "$wanted_family" ] && return 0
        done < <(list_plan)

        return 1
    }

    run_vm_families() {
        local seen_scripts=()
        local plan_family
        local plan_distro
        local plan_release
        local plan_arch
        local plan_image
        local plan_script
        local plan_outdir
        local script
        local seen

        family_enabled vm || return 0
        distro_allowed_for_family vm || return 0
        family_has_selected_targets vm || return 0

        while IFS="|" read -r plan_family plan_distro plan_release plan_arch plan_image plan_script plan_outdir; do
            [ "$plan_family" = vm ] || continue

            seen=0
            for script in "${seen_scripts[@]}"; do
                if [ "$script" = "$plan_script" ]; then
                    seen=1
                    break
                fi
            done

            [ "$seen" -eq 0 ] || continue
            seen_scripts+=("$plan_script")

            run_family vm "$plan_script" || return 1
        done < <(list_plan)

        return 0
    }

    run_family() {
        local family="$1"
        local script="$2"
        local args=()

        family_enabled "$family" || return 0
        distro_allowed_for_family "$family" || return 0
        family_has_selected_targets "$family" || return 0

        args+=(--execute)

        if [ "$family" = vm ]; then
            local plan_family plan_distro plan_release plan_arch plan_image plan_script plan_outdir

            while IFS="|" read -r plan_family plan_distro plan_release plan_arch plan_image plan_script plan_outdir; do
                [ "$plan_family" = "$family" ] || continue
                [ "$plan_script" = "$script" ] || continue
                target_matches_filters "$plan_family" "$plan_distro" "$plan_release" "$plan_arch" || continue

                args=(--execute)
                args+=(--arch "$plan_arch")
                args+=(--archive-dir "$plan_outdir")
                args+=(--workroot "$ROOT/out/${plan_distro}-vm/$plan_arch")
                args+=(--jobs "$MAKE_JOBS")

                echo
                echo "============================================================"
                echo "Dispatching package family: $family"
                echo "Script: build_scripts/$script"
                echo "Target: $plan_distro / $plan_release / $plan_arch"
                echo "Archive: $plan_outdir"
                echo "Workroot: $ROOT/out/${plan_distro}-vm/$plan_arch"
                echo "============================================================"

                ran=$((ran + 1))
                if ! JOBS="$MAKE_JOBS" "$ROOT/build_scripts/$script" "${args[@]}"; then
                    failures=$((failures + 1))
                    if [ "$KEEP_GOING" -eq 0 ]; then
                        return 1
                    fi
                fi
            done < <(list_plan)

            return 0
        fi

        if [ -n "$DISTRO_FILTER" ]; then
            args+=(--distro "$DISTRO_FILTER")
        fi

        if [ -n "$RELEASE_FILTER" ]; then
            args+=(--release "$RELEASE_FILTER")
        fi

        if [ -n "$ARCH_FILTER" ]; then
            args+=(--arch "$ARCH_FILTER")
        fi

        case "$family" in
            deb|apk|rpm|archlinux|slackware)
                args+=(--jobs "$MAKE_JOBS")
                [ "$KEEP_GOING" -eq 0 ] || args+=(--keep-going)
                [ "$SKIP_HOST_DEPS" -eq 0 ] || args+=(--skip-host-deps)
                [ "$SKIP_BOOTSTRAP" -eq 0 ] || args+=(--skip-bootstrap)
                ;;
        esac

        echo
        echo "============================================================"
        echo "Dispatching package family: $family"
        echo "Script: build_scripts/$script"
        echo "============================================================"

        ran=$((ran + 1))
        if ! JOBS="$MAKE_JOBS" "$ROOT/build_scripts/$script" "${args[@]}"; then
            failures=$((failures + 1))
            if [ "$KEEP_GOING" -eq 0 ]; then
                return 1
            fi
        fi

        return 0
    }

    failures=0
    ran=0

    plan="$(list_plan)"
    [ -n "$plan" ] || die "no package target matched the selected filters"

    build_doc_tools

    run_family deb debianubuntu-cross-build-freebasic-matrix.sh || true
    if [ "$KEEP_GOING" -eq 0 ] && [ "$failures" -ne 0 ]; then
        exit 1
    fi

    run_family apk apk-cross-build-freebasic-matrix.sh || true
    if [ "$KEEP_GOING" -eq 0 ] && [ "$failures" -ne 0 ]; then
        exit 1
    fi

    run_family rpm rpm-cross-build-freebasic-matrix.sh || true
    if [ "$KEEP_GOING" -eq 0 ] && [ "$failures" -ne 0 ]; then
        exit 1
    fi

    run_family archlinux archlinux-freebasic-matrix-build.sh || true
    if [ "$KEEP_GOING" -eq 0 ] && [ "$failures" -ne 0 ]; then
        exit 1
    fi

    run_family slackware slackware-cross-build-freebasic-matrix.sh || true
    if [ "$KEEP_GOING" -eq 0 ] && [ "$failures" -ne 0 ]; then
        exit 1
    fi

    run_vm_families || true

    [ "$ran" -gt 0 ] || die "no package family matched the selected filters"

    [ "$failures" -eq 0 ] || exit 1
    CLEANUP_SUCCESS=1
}

##############################################################################
# Main
##############################################################################

if [ "$EXECUTE" -eq 0 ] || [ "$LIST_ONLY" -eq 1 ]; then
    list_plan
    if [ "$LIST_ONLY" -eq 0 ]; then
        echo
        echo "==> plan only"
        echo "==> pass --execute to build implemented package families"
    fi
    exit 0
fi

execute_plan

##############################################################################
# end of linux-cross-build-freebasic-matrix.sh
##############################################################################
