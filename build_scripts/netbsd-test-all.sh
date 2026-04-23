#!/usr/bin/env bash

set -euo pipefail
trap 'echo "ERROR at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Validate invocation location
##############################################################################

if [[ "$(basename "$PWD")" == "build_scripts" ]]; then
    echo
    echo "ERROR: do not run this script from the build_scripts directory."
    echo "Run it from the project root:"
    echo "  ./build_scripts/netbsd-test-all.sh"
    exit 1
fi

if [[ ! -d "build_scripts" ]]; then
    echo
    echo "ERROR: run this script from the project root."
    exit 1
fi

if [[ ! -d "out/NetBSD" ]]; then
    echo
    echo "ERROR: missing out/NetBSD"
    exit 1
fi

##############################################################################
# Configuration
##############################################################################

ROOT="$(pwd)"
JAIL_BASE="/usr/jails/netbsd-test"
SETS_BASE="/usr/jails/netbsd/sets"
SETS=( base comp etc xbase xcomp )

HOST_REL="$(uname -r)"
HOST_REL="${HOST_REL%%_*}"
HOST_MAJOR="${HOST_REL%%.*}"

ARCH_RAW="$(uname -m)"

case "$ARCH_RAW" in
    amd64|x86_64)
        NETBSD_PORT="amd64"
        OUT_ARCH="amd64"
        FAKE_UNAME_M="amd64"
        ;;
    i386|i486|i586|i686)
        NETBSD_PORT="i386"
        OUT_ARCH="i386"
        FAKE_UNAME_M="i386"
        ;;
    aarch64|earm64)
        NETBSD_PORT="aarch64"
        OUT_ARCH="aarch64"
        FAKE_UNAME_M="aarch64"
        ;;
    *)
        echo "ERROR: unsupported architecture: $ARCH_RAW"
        exit 1
        ;;
esac

echo "==> NetBSD test matrix"
echo "==> host: $HOST_REL"
echo "==> arch: $OUT_ARCH"

##############################################################################
# Discover releases (same-major only)
##############################################################################

mapfile -t RELEASES < <(
    find "$ROOT/out/NetBSD" -mindepth 1 -maxdepth 1 -type d \
        -exec basename {} \; \
        | grep "^${HOST_MAJOR}\." \
        | awk -F. '{ printf "%d %d %s\n", $1, $2, $0 }' \
        | sort -n -k1,1 -k2,2 \
        | awk '{ print $3 }'
)

if [[ ${#RELEASES[@]} -eq 0 ]]; then
    echo "ERROR: no matching releases for NetBSD ${HOST_MAJOR}.x"
    exit 1
fi

echo "==> releases: ${RELEASES[*]}"

##############################################################################
# Helpers
##############################################################################

cleanup_mounts() {
    if [[ -n "${CURRENT_JAILDIR:-}" && -d "${CURRENT_JAILDIR}/proc" ]]; then
        if mount | grep -q "on ${CURRENT_JAILDIR}/proc "; then
            umount "${CURRENT_JAILDIR}/proc" || true
        fi
    fi
}

trap cleanup_mounts EXIT

validate_archive() {
    local archive="$1"
    [[ -f "$archive" ]] || return 1
    file "$archive" | grep -q 'XZ compressed data' || return 1
    tar -tJf "$archive" >/dev/null 2>&1 || return 1
}

fetch_set() {
    local rel="$1"
    local set="$2"
    local dest="$3"
    local tmp="${dest}.part.$$"
    local base
    local url

    mkdir -p "$(dirname "$dest")"

    if [[ -f "$dest" ]] && validate_archive "$dest"; then
        echo "==> using cached $dest"
        return 0
    fi

    rm -f "$dest" "$tmp"

    for base in \
        "https://cdn.netbsd.org/pub/NetBSD/NetBSD-$rel" \
        "https://ftp.iij.ad.jp/pub/NetBSD/NetBSD-$rel" \
        "https://ftp.jaist.ac.jp/pub/NetBSD/NetBSD-$rel"
    do
        url="$base/$NETBSD_PORT/binary/sets/$set.tar.xz"
        echo "==> fetching $url"

        rm -f "$tmp"

        if curl -4 -L --fail --show-error \
            --retry 3 --retry-delay 2 --retry-all-errors \
            --connect-timeout 20 \
            -o "$tmp" \
            "$url"; then

            if [[ ! -s "$tmp" ]]; then
                echo "==> empty download from $url"
                rm -f "$tmp"
                continue
            fi

            if validate_archive "$tmp"; then
                mv -f "$tmp" "$dest"
                return 0
            fi

            echo "==> invalid archive from $url"
            rm -f "$tmp"
        else
            echo "==> curl failed for $url"
            rm -f "$tmp"
        fi
    done

    echo "ERROR: failed to fetch $set for $rel"
    exit 1
}

##############################################################################
# Main loop
##############################################################################

for REL in "${RELEASES[@]}"; do
    echo
    echo "=================================================="
    echo "==> testing NetBSD $REL"
    echo "=================================================="

    PKGDIR="$ROOT/out/NetBSD/$REL/$OUT_ARCH"

    if [[ ! -d "$PKGDIR" ]]; then
        echo "ERROR: missing package dir: $PKGDIR"
        exit 1
    fi

    PKGFILE="$(find "$PKGDIR" -maxdepth 1 -type f -name 'freebasic-*.tgz' | sort | head -n1)"

    if [[ -z "$PKGFILE" || ! -f "$PKGFILE" ]]; then
        echo "ERROR: missing package for $REL in $PKGDIR"
        exit 1
    fi

    echo "==> using package: $PKGFILE"

    JAILDIR="$JAIL_BASE/$REL-$NETBSD_PORT"
    CURRENT_JAILDIR="$JAILDIR"
    SETSDIR="$SETS_BASE/$REL-$NETBSD_PORT"

    mkdir -p "$SETSDIR"

    ##########################################################################
    # Fetch sets
    ##########################################################################

    for set in "${SETS[@]}"; do
        fetch_set "$REL" "$set" "$SETSDIR/$set.tar.xz"
    done

    ##########################################################################
    # Clean jail
    ##########################################################################

    echo "==> recreating test jail"
    cleanup_mounts
    rm -rf "$JAILDIR"
    mkdir -p "$JAILDIR"

    ##########################################################################
    # Extract sets
    ##########################################################################

    echo "==> extracting sets"
    for set in "${SETS[@]}"; do
        tar -xpf "$SETSDIR/$set.tar.xz" -C "$JAILDIR"
    done

    ##########################################################################
    # Jail setup
    ##########################################################################

    echo "==> configuring jail"

    mkdir -p "$JAILDIR/tmp"
    chmod 1777 "$JAILDIR/tmp"

    if [[ -f /etc/resolv.conf ]]; then
        cp /etc/resolv.conf "$JAILDIR/etc/resolv.conf"
    fi

    mkdir -p "$JAILDIR/dev"
    (
        cd "$JAILDIR/dev"
        sh ./MAKEDEV all >/dev/null
    )

    mkdir -p "$JAILDIR/proc"
    mount -t procfs proc "$JAILDIR/proc"

    if [[ -d /etc/openssl/certs ]]; then
        mkdir -p "$JAILDIR/etc/openssl"
        cp -a /etc/openssl/certs "$JAILDIR/etc/openssl/"
    fi

    if [[ -f /etc/openssl/openssl.cnf ]]; then
        mkdir -p "$JAILDIR/etc/openssl"
        cp -a /etc/openssl/openssl.cnf "$JAILDIR/etc/openssl/"
    fi

    mkdir -p "$JAILDIR/root/pkgtest"
    cp "$PKGFILE" "$JAILDIR/root/pkgtest/"

    if [[ -x "$JAILDIR/usr/sbin/pkg_add" && ! -e "$JAILDIR/usr/sbin/pkg_add.real" ]]; then
        mv "$JAILDIR/usr/sbin/pkg_add" "$JAILDIR/usr/sbin/pkg_add.real"

        cat > "$JAILDIR/usr/sbin/pkg_add" <<'PKGADD_EOF'
#!/bin/sh
exec /usr/sbin/pkg_add.real -f "$@"
PKGADD_EOF

        chmod 755 "$JAILDIR/usr/sbin/pkg_add"
    fi

    mkdir -p "$JAILDIR/root/fakebin"

    cat > "$JAILDIR/root/fakebin/uname" <<EOF
#!/bin/sh
case "\${1-}" in
    -s) echo "NetBSD" ;;
    -r) echo "$REL" ;;
    -m) echo "$FAKE_UNAME_M" ;;
    *) exec /usr/bin/uname "\$@" ;;
esac
EOF
    chmod 755 "$JAILDIR/root/fakebin/uname"

    cat > "$JAILDIR/root/fakebin/sysctl" <<EOF
#!/bin/sh
case "\$*" in
    "-n kern.ostype") echo "NetBSD" ;;
    "-n kern.osrelease") echo "$REL" ;;
    "-n hw.machine") echo "$FAKE_UNAME_M" ;;
    *) exec /sbin/sysctl "\$@" ;;
esac
EOF
    chmod 755 "$JAILDIR/root/fakebin/sysctl"

    ##########################################################################
    # Run tests inside jail
    ##########################################################################

    echo "==> entering jail"

    chroot "$JAILDIR" /bin/sh <<EOF
set -eu

pwd_mkdb -p /etc/master.passwd

export PATH="/root/fakebin:/usr/pkg/sbin:/usr/pkg/bin:/usr/sbin:/usr/bin:/sbin:/bin"
export SSL_CERT_DIR=/etc/openssl/certs
export SSL_CERT_FILE=/etc/openssl/certs/ca-certificates.crt

PKG_REPO_BOOTSTRAP="http://cdn.netbsd.org/pub/pkgsrc/packages/NetBSD/$OUT_ARCH/$REL/All"
PKG_REPO="https://cdn.netbsd.org/pub/pkgsrc/packages/NetBSD/$OUT_ARCH/$REL/All"

echo "==> installing certs"
export PKG_PATH="\$PKG_REPO_BOOTSTRAP"
pkg_add mozilla-rootcerts-openssl || true
if command -v mozilla-rootcerts >/dev/null 2>&1; then
    mozilla-rootcerts install || true
fi

echo "==> installing package"
export PKG_PATH="\$PKG_REPO"
pkg_add /root/pkgtest/freebasic-*.tgz

echo "==> verifying install"
[ -x /usr/pkg/bin/fbc ] || {
    echo "ERROR: fbc missing"
    exit 1
}

pkg_info -e 'freebasic-*' >/dev/null 2>&1 || {
    echo "ERROR: package not registered"
    exit 1
}

echo "==> console test"
cat > /tmp/test.bas <<'FBEOF'
print "OK"
FBEOF

/usr/pkg/bin/fbc /tmp/test.bas -x /tmp/test

OUT="\$(/tmp/test)"
echo "==> output: \$OUT"

[ "\$OUT" = "OK" ] || {
    echo "ERROR: bad output"
    exit 1
}

echo "==> gfx test"
cat > /tmp/gfx.bas <<'FBEOF'
screen 13
end
FBEOF

/usr/pkg/bin/fbc /tmp/gfx.bas -x /tmp/gfx

[ -x /tmp/gfx ] || {
    echo "ERROR: gfx compile failed"
    exit 1
}

echo "==> TEST PASSED"
EOF

    ##########################################################################
    # Cleanup
    ##########################################################################

    echo "==> cleaning up jail"
    cleanup_mounts
    CURRENT_JAILDIR=""
done

echo
echo "=================================================="
echo "==> ALL TESTS PASSED"
echo "=================================================="
