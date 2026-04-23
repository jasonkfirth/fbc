#!/usr/bin/env bash

set -euo pipefail
trap 'echo "ERROR at line $LINENO: $BASH_COMMAND" >&2' ERR

RELEASES=(
  "15.0-RELEASE"
  "14.4-RELEASE"
  "14.3-RELEASE"
  "13.5-RELEASE"
)

JAIL_BASE="/usr/jails"
ROOT="$(pwd)"

if [ ! -d "$ROOT/bootstrap" ] || [ ! -d "$ROOT/inc" ]; then
    echo "ERROR: not a FreeBASIC source root"
    exit 1
fi

if [ ! -f "$ROOT/build_scripts/freebsd-build-freebasic.sh" ]; then
    echo "ERROR: missing build script"
    exit 1
fi

echo "==> installing host dependencies"
pkg update -f
pkg install -y gmake gcc git rsync bash

ARCH="$(uname -m)"
echo "==> detected architecture: $ARCH"

for REL in "${RELEASES[@]}"; do
    echo
    echo "=================================================="
    echo "==> building for $REL"
    echo "=================================================="

    JAILDIR="$JAIL_BASE/$REL"

    if [ -d "$JAILDIR" ]; then
        echo "==> clearing flags: $JAILDIR"
        chflags -R noschg,nouchg "$JAILDIR" 2>/dev/null || true

        if mount | grep -q "on $JAILDIR/dev "; then
            echo "==> unmounting devfs"
            umount "$JAILDIR/dev" || true
        fi

        echo "==> removing jail"
        rm -rf "$JAILDIR"
    fi

    mkdir -p "$JAILDIR"

    echo "==> fetching $REL ($ARCH)"
    fetch -o "$JAILDIR/base.txz" \
        "https://download.freebsd.org/releases/$ARCH/$REL/base.txz"

    echo "==> extracting base"
    tar -xpf "$JAILDIR/base.txz" -C "$JAILDIR"

    echo "==> setting up jail environment"
    cp /etc/resolv.conf "$JAILDIR/etc/resolv.conf"
    mkdir -p "$JAILDIR/dev"
    mount -t devfs devfs "$JAILDIR/dev"

    echo "==> syncing source tree"
    mkdir -p "$JAILDIR/root/src"
    rsync -a --delete --delete-excluded --prune-empty-dirs \
        --exclude-from "$ROOT/mk/source-copy-excludes.rsync" \
        "$ROOT/" "$JAILDIR/root/src/"

    if [ ! -f "$JAILDIR/root/src/build_scripts/freebsd-build-freebasic.sh" ]; then
        echo "ERROR: build script missing in jail"
        exit 1
    fi

    echo "==> entering jail"

    chroot "$JAILDIR" /bin/sh << 'CHROOT_EOF'
set -e

echo "==> bootstrapping pkg"
env ASSUME_ALWAYS_YES=yes pkg bootstrap

echo "==> installing build deps"
pkg update
pkg install -y gmake gcc git bash

cd /root/src

echo "==> verifying build script"
[ -f build_scripts/freebsd-build-freebasic.sh ] || {
    echo "ERROR: build script missing"
    exit 1
}

echo "==> running build"
exec /usr/local/bin/bash build_scripts/freebsd-build-freebasic.sh
CHROOT_EOF

    echo "==> cleaning up devfs"
    umount "$JAILDIR/dev"

    OUTDIR="$ROOT/out/$REL"
    mkdir -p "$OUTDIR"

    echo "==> verifying package output"
    ls "$JAILDIR/root/src/out/"*.pkg >/dev/null 2>&1 || {
        echo "ERROR: no package produced for $REL"
        exit 1
    }

    echo "==> copying package to $OUTDIR"
    cp -v "$JAILDIR/root/src/out/"*.pkg "$OUTDIR/" || {
        echo "ERROR: package copy failed"
        exit 1
    }

done

echo
echo "==> ALL BUILDS COMPLETE"
