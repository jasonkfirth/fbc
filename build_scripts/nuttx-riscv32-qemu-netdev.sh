#!/usr/bin/env bash
#
# Project: FreeBASIC NuttX/RISC-V development harness
# ----------------------------------------------------
#
# File: nuttx-riscv32-qemu-netdev.sh
#
# Purpose:
#
#     Start the NuttX RISC-V QEMU harness inside the project Docker image with
#     telnet and FTP ports published from Docker to the host.
#
# Responsibilities:
#
#     - locate the FreeBASIC source tree on the Docker host
#     - publish the telnet and FTP ports used by QEMU host forwarding
#     - mount the FreeBASIC tree and the NuttX work tree into the container
#     - start the heavier network-shell mode of nuttx-riscv32-qemu-smoke.sh
#
# This file intentionally does NOT contain:
#
#     - NuttX board configuration
#     - FreeBASIC compiler changes
#     - QEMU guest validation logic
#     - release packaging
#

set -euo pipefail

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

[ -n "$ROOT" ] || { echo "ERROR: could not locate FreeBASIC root" >&2; exit 1; }

##############################################################################
# Options
##############################################################################

NUTTX_WORKDIR="${NUTTX_WORKDIR:-$HOME/fbxl-nuttx-rv32}"
FBXL_NUTTX_DOCKER_IMAGE="${FBXL_NUTTX_DOCKER_IMAGE:-fbxl/nuttx-rv32:ubuntu24}"
HOST_TELNET_PORT="${HOST_TELNET_PORT:-2323}"
HOST_FTP_PORT="${HOST_FTP_PORT:-2121}"
QEMU_MEMORY="${QEMU_MEMORY:-4G}"
QEMU_TIMEOUT="${QEMU_TIMEOUT:-0}"
DOCKER_TTY_ARGS="${DOCKER_TTY_ARGS:--it}"

[ -d "$NUTTX_WORKDIR/nuttx" ] ||
    { echo "ERROR: missing NuttX tree: $NUTTX_WORKDIR/nuttx" >&2; exit 1; }

[ -d "$NUTTX_WORKDIR/apps" ] ||
    { echo "ERROR: missing NuttX apps tree: $NUTTX_WORKDIR/apps" >&2; exit 1; }

##############################################################################
# Start Docker/QEMU
##############################################################################

echo "Docker image: $FBXL_NUTTX_DOCKER_IMAGE"
echo "NuttX tree:   $NUTTX_WORKDIR"
echo "Telnet:       telnet <docker-host> $HOST_TELNET_PORT"
echo "FTP:          ftp -p <docker-host> $HOST_FTP_PORT"

# DOCKER_TTY_ARGS is intentionally word-split so callers can set it to an empty
# string in non-interactive automation, or leave the default -it for a local
# terminal session.
# shellcheck disable=SC2086
docker run --rm $DOCKER_TTY_ARGS \
    -p "${HOST_TELNET_PORT}:${HOST_TELNET_PORT}" \
    -p "${HOST_FTP_PORT}:${HOST_FTP_PORT}" \
    -e "HOST_TELNET_PORT=$HOST_TELNET_PORT" \
    -e "HOST_FTP_PORT=$HOST_FTP_PORT" \
    -e "QEMU_MEMORY=$QEMU_MEMORY" \
    -e "QEMU_TIMEOUT=$QEMU_TIMEOUT" \
    -v "$NUTTX_WORKDIR:/work" \
    -v "$ROOT:/fbxl" \
    -w /fbxl \
    "$FBXL_NUTTX_DOCKER_IMAGE" \
    bash build_scripts/nuttx-riscv32-qemu-smoke.sh \
        --nuttx-workdir /work \
        --network-shell \
        "$@"

# end of nuttx-riscv32-qemu-netdev.sh
