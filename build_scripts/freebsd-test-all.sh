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
ARCH="$(uname -m)"

echo "==> starting clean package tests"
echo "==> arch: $ARCH"

for REL in "${RELEASES[@]}"; do
    echo
    echo "=================================================="
    echo "==> testing package on $REL"
    echo "=================================================="

    JAILDIR="$JAIL_BASE/test-$REL"
    PKGDIR="$ROOT/out/$REL"

    if [ ! -d "$PKGDIR" ]; then
        echo "ERROR: missing package dir: $PKGDIR"
        exit 1
    fi

    PKGFILE="$(ls "$PKGDIR"/freebasic-*.pkg | head -n1)"
    [ -f "$PKGFILE" ] || {
        echo "ERROR: package not found for $REL"
        exit 1
    }

    echo "==> using package: $PKGFILE"

    ##########################################################################
    # Clean jail
    ##########################################################################

    if [ -d "$JAILDIR" ]; then
        echo "==> removing existing test jail"
        chflags -R noschg,nouchg "$JAILDIR" 2>/dev/null || true

        if mount | grep -q "on $JAILDIR/dev "; then
            umount "$JAILDIR/dev" || true
        fi

        rm -rf "$JAILDIR"
    fi

    mkdir -p "$JAILDIR"

    ##########################################################################
    # Fetch + extract base
    ##########################################################################

    echo "==> fetching $REL"
    fetch -o "$JAILDIR/base.txz" \
        "https://download.freebsd.org/releases/$ARCH/$REL/base.txz"

    echo "==> extracting base"
    tar -xpf "$JAILDIR/base.txz" -C "$JAILDIR"

    ##########################################################################
    # Jail setup
    ##########################################################################

    echo "==> setting up jail"
    cp /etc/resolv.conf "$JAILDIR/etc/resolv.conf"
    mkdir -p "$JAILDIR/dev"
    mount -t devfs devfs "$JAILDIR/dev"

    mkdir -p "$JAILDIR/root/pkgtest"
    cp "$PKGFILE" "$JAILDIR/root/pkgtest/"

    ##########################################################################
    # Run test inside jail
    ##########################################################################

    echo "==> entering jail"

    chroot "$JAILDIR" /bin/sh << 'CHROOT_EOF'
set -e

echo "==> bootstrapping pkg"
env ASSUME_ALWAYS_YES=yes pkg bootstrap

echo "==> updating repo"
pkg update

echo "==> installing package (clean environment)"
env ASSUME_ALWAYS_YES=yes pkg install /root/pkgtest/freebasic-*.pkg

echo "==> verifying installation"
[ -x /usr/local/bin/fbc ] || {
    echo "ERROR: fbc not installed"
    exit 1
}

echo "==> compiling console test"
cat > /tmp/test.bas <<'FBEOF'
print "OK"
FBEOF

/usr/local/bin/fbc /tmp/test.bas

OUTPUT="$(/tmp/test)"
echo "==> output: $OUTPUT"

[ "$OUTPUT" = "OK" ] || {
    echo "ERROR: bad output"
    exit 1
}

echo "==> compiling gfx test (no run)"
cat > /tmp/gfx.bas <<'FBEOF'
screen 13
end
FBEOF

/usr/local/bin/fbc /tmp/gfx.bas

[ -x /tmp/gfx ] || {
    echo "ERROR: gfx compile failed"
    exit 1
}

echo "==> checking runtime deps"
pkg check -d freebasic || {
    echo "ERROR: dependency check failed"
    exit 1
}

echo "==> TEST PASSED"
CHROOT_EOF

    ##########################################################################
    # Cleanup
    ##########################################################################

    echo "==> cleaning up jail"
    umount "$JAILDIR/dev"

done

echo
echo "=================================================="
echo "==> ALL TESTS PASSED"
echo "=================================================="

