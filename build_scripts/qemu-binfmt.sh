#!/usr/bin/env bash
#
# Project: FreeBASIC XL build infrastructure
# ------------------------------------------
#
# File: qemu-binfmt.sh
#
# Purpose:
#
#     Install the QEMU user-mode interpreters used by Docker package and test
#     matrices when a target architecture differs from the build host.
#
# Responsibilities:
#
#     * pin the binfmt installer and QEMU user-mode version
#     * replace older distro-provided qemu-* registrations
#     * register the complete architecture set used by the matrices
#
# This file intentionally does NOT contain:
#
#     * Docker installation or daemon startup
#     * distribution or package-matrix policy
#     * target-specific build commands

FB_QEMU_BINFMT_IMAGE_DEFAULT="tonistiigi/binfmt:qemu-v10.2.3@sha256:400a4873b838d1b89194d982c45e5fb3cda4593fbfd7e08a02e76b03b21166f0"

fb_install_qemu_binfmt() {
    local image
    local reset_existing

    image="${FBC_QEMU_BINFMT_IMAGE:-$FB_QEMU_BINFMT_IMAGE_DEFAULT}"
    reset_existing="${FBC_QEMU_BINFMT_RESET:-auto}"

    msg "installing pinned QEMU binfmt interpreters"
    run_root docker pull "$image"

    #
    # qemu-user-static packages may register an older interpreter before this
    # helper runs.  The binfmt installer preserves an existing qemu-* entry,
    # so merely asking it to install all architectures does not replace that
    # interpreter.  Ubuntu 26.04 tar uses openat2() while extracting archives;
    # QEMU 8.2 returns ENOSYS for that call and breaks dpkg-source.  Remove the
    # existing entries first so the pinned QEMU version is actually selected.
    #
    if [ "$reset_existing" = "auto" ]; then
        if command -v pgrep >/dev/null 2>&1 && \
            pgrep -f '[q]emu-(aarch64|arm|loongarch64|mips64|mips64el|ppc64le|riscv64|s390x)' >/dev/null 2>&1; then
            reset_existing=0
            msg "preserving QEMU registrations used by active emulated processes"
        else
            reset_existing=1
        fi
    fi

    if [ "$reset_existing" = "1" ]; then
        run_root docker run --rm --privileged "$image" --uninstall 'qemu-*'
    elif [ "$reset_existing" = "0" ]; then
        msg "preserving existing QEMU registrations"
    else
        echo "ERROR: FBC_QEMU_BINFMT_RESET must be auto, 0, or 1" >&2
        return 1
    fi
    run_root docker run --rm --privileged "$image" --install all
}

# end of qemu-binfmt.sh
