#!/bin/sh
set -eu

##############################################################################
# Validation
##############################################################################

if [ "$(basename "$PWD")" = "build_scripts" ]; then
    echo "ERROR: run from project root"
    exit 1
fi

[ -d "build_scripts" ] || {
    echo "ERROR: missing build_scripts directory"
    exit 1
}

[ -f "build_scripts/dragonfly-build-freebasic.sh" ] || {
    echo "ERROR: missing inner build script"
    exit 1
}

##############################################################################
# Config
##############################################################################

ROOT="$(pwd)"
JAIL_BASE="${JAIL_BASE:-/usr/jails/dragonfly}"
CACHE_BASE="${CACHE_BASE:-$JAIL_BASE/cache}"
HOST_REL="$(uname -r)"
HOST_ARCH_RAW="$(uname -m)"

case "$HOST_ARCH_RAW" in
    x86_64|amd64) OUT_ARCH="amd64" ;;
    *) OUT_ARCH="$HOST_ARCH_RAW" ;;
esac

OUTDIR="$ROOT/out/Dragonfly/$HOST_REL/$OUT_ARCH"
JAILDIR="$JAIL_BASE/$HOST_REL-$OUT_ARCH"
CURRENT_JAILDIR=""

BUILD_PKGS="pkg gmake gcc git bash ncurses libffi mesa-libs libglvnd xorgproto libX11 libXext libXrandr libXrender libXpm libXcursor libXi libXinerama libXxf86vm libxcb libXau libXdmcp"

PKG_FETCH_DIR="$CACHE_BASE/pkgrepo-$HOST_REL-$OUT_ARCH"

##############################################################################
# Helpers
##############################################################################

log() { echo "==> $*"; }
die() { echo "ERROR: $*" >&2; exit 1; }

cleanup_mounts() {
    if [ -n "${CURRENT_JAILDIR:-}" ]; then
        mount | grep -q "on $CURRENT_JAILDIR/dev " && \
            umount "$CURRENT_JAILDIR/dev" || true
        mount | grep -q "on $CURRENT_JAILDIR/proc " && \
            umount "$CURRENT_JAILDIR/proc" || true
    fi
}

trap cleanup_mounts EXIT HUP INT TERM

##############################################################################
# Host dependencies
##############################################################################

log "installing host dependencies"
pkg update -f
pkg install -y gmake gcc git rsync bash

##############################################################################
# Fetch packages
##############################################################################

log "prefetching build packages"
mkdir -p "$PKG_FETCH_DIR"

pkg fetch -y -d -o "$PKG_FETCH_DIR" $BUILD_PKGS

PKG_FILES="$(find "$PKG_FETCH_DIR" -type f -name '*.pkg')"
[ -n "$PKG_FILES" ] || die "no packages fetched into $PKG_FETCH_DIR"

PKG_BOOTSTRAP_FILE="$(find "$PKG_FETCH_DIR" -type f -name 'pkg-[0-9]*.pkg' | sort -V | tail -n1)"
[ -n "$PKG_BOOTSTRAP_FILE" ] || die "failed to locate pkg bootstrap package"

##############################################################################
# Recreate jail (snapshot-style)
##############################################################################

CURRENT_JAILDIR="$JAILDIR"

log "recreating jail: $JAILDIR"

if [ -d "$JAILDIR" ]; then
    chflags -R noschg,nouchg "$JAILDIR" 2>/dev/null || true
    cleanup_mounts
    rm -rf "$JAILDIR"
fi

mkdir -p "$JAILDIR"

##############################################################################
# Minimal base (copy host root)
##############################################################################

log "creating minimal base system"

mkdir -p "$JAILDIR"
cp -a /bin /lib /libexec /sbin /usr "$JAILDIR"

##############################################################################
# Configure jail
##############################################################################

log "configuring jail"

[ -f /etc/resolv.conf ] && cp /etc/resolv.conf "$JAILDIR/etc/resolv.conf"
[ -f /etc/hosts ] && cp /etc/hosts "$JAILDIR/etc/hosts"

mkdir -p "$JAILDIR/dev"
mount -t devfs devfs "$JAILDIR/dev"

mkdir -p "$JAILDIR/proc"
mount -t procfs proc "$JAILDIR/proc"

mkdir -p "$JAILDIR/tmp"
chmod 1777 "$JAILDIR/tmp"

##############################################################################
# Copy source + packages
##############################################################################

log "copying source tree"
mkdir -p "$JAILDIR/root/src"

rsync -a --delete --delete-excluded --prune-empty-dirs \
    --exclude-from "$ROOT/mk/source-copy-excludes.rsync" \
    "$ROOT/" "$JAILDIR/root/src/"

log "copying packages"
mkdir -p "$JAILDIR/root/pkgrepo"

find "$PKG_FETCH_DIR" -type f -name '*.pkg' \
    -exec cp {} "$JAILDIR/root/pkgrepo/" \;

##############################################################################
# Enter jail
##############################################################################

log "entering jail"

chroot "$JAILDIR" /bin/sh <<EOF
set -eu

export PATH="/usr/sbin:/usr/bin:/sbin:/bin:/usr/local/sbin:/usr/local/bin"

echo "==> installing pkg"
pkg add "$PKG_BOOTSTRAP_FILE"

mkdir -p /usr/local/etc/pkg/repos

cat > /usr/local/etc/pkg/repos/Local.conf <<'REPO'
Local: {
  url: "file:///root/pkgrepo",
  enabled: yes
}
REPO

echo "==> installing dependencies"
pkg add /root/pkgrepo/*.pkg

cd /root/src
exec /bin/sh build_scripts/dragonfly-build-freebasic.sh
EOF

##############################################################################
# Collect output
##############################################################################

log "collecting package"
mkdir -p "$OUTDIR"

set -- "$JAILDIR/root/src/out/"*.pkg
[ -e "$1" ] || die "no package produced"

cp "$JAILDIR/root/src/out/"*.pkg "$OUTDIR/"

echo "==> done"
