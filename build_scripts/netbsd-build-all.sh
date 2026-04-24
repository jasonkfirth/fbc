#!/bin/sh
set -eu
umask 022

##############################################################################
# Validate invocation location
##############################################################################

if [ "$(basename "$PWD")" = "build_scripts" ]; then
    echo
    echo "ERROR: do not run this script from the build_scripts directory."
    echo "Run it from the project root:"
    echo "  ./build_scripts/netbsd-build-all.sh"
    exit 1
fi

if [ ! -d "build_scripts" ]; then
    echo
    echo "ERROR: run this script from the project root."
    echo "Expected to find ./build_scripts directory."
    exit 1
fi

if [ ! -d "mk" ] || \
   { [ ! -f "GNUmakefile" ] && [ ! -f "Makefile" ] && [ ! -f "makefile" ]; }; then
    echo
    echo "ERROR: not a FreeBASIC source root."
    exit 1
fi

if [ ! -f "build_scripts/netbsd-build-freebasic.sh" ]; then
    echo
    echo "ERROR: missing inner build script:"
    echo "  build_scripts/netbsd-build-freebasic.sh"
    exit 1
fi

##############################################################################
# Helpers
##############################################################################

log() { echo "==> $*" >&2; }
die() { echo "ERROR: $*" >&2; exit 1; }

ensure_cmd() {
    command -v "$1" >/dev/null 2>&1 || die "required command not found: $1"
}

cleanup_mounts() {
    if [ -n "${CURRENT_JAILDIR:-}" ] && [ -d "${CURRENT_JAILDIR}/proc" ]; then
        if mount | grep -F "${CURRENT_JAILDIR}/proc" >/dev/null 2>&1; then
            umount "${CURRENT_JAILDIR}/proc" >/dev/null 2>&1 || true
        fi
    fi
}

sort_versions() {
    sort -t. -k1,1n -k2,2n -u
}

intersect_versions() {
    left="$1"
    right="$2"

    for l in $left; do
        for r in $right; do
            [ "$l" = "$r" ] && printf '%s\n' "$l"
        done
    done | sort_versions
}

fetch_index() {
    url="$1"
    curl -4 -L --fail --show-error \
        --retry 3 --retry-delay 2 --retry-all-errors \
        --connect-timeout 20 \
        -A "$USER_AGENT" \
        "$url"
}

trap cleanup_mounts EXIT HUP INT TERM

##############################################################################
# Early validation
##############################################################################

[ "$(uname -s)" = "NetBSD" ] || die "must run on NetBSD"
[ "$(id -u)" -eq 0 ] || die "must run as root"

##############################################################################
# Configuration
##############################################################################

SETS="${SETS:-base comp etc xbase xcomp}"

ROOT="$(pwd)"
JAIL_BASE="${JAIL_BASE:-/usr/jails/netbsd}"
SETS_BASE="${SETS_BASE:-$JAIL_BASE/sets}"
ARTIFACT_BASE="${ARTIFACT_BASE:-$ROOT/NetBSD}"

USER_AGENT="${USER_AGENT:-Mozilla/5.0 (NetBSD build bootstrap)}"

##############################################################################
# Architecture mapping
##############################################################################

HOST_ARCH="$(uname -m)"

case "$HOST_ARCH" in
    amd64|x86_64)
        NETBSD_PORT="amd64"
        PKG_ARCH="x86_64"
        FAKE_UNAME_M="amd64"
        ;;
    *)
        die "unsupported architecture: $HOST_ARCH"
        ;;
esac

##############################################################################
# Host release detection
##############################################################################

HOST_REL_RAW="$(uname -r)"
HOST_REL="${HOST_REL_RAW%%_*}"
HOST_MAJOR="${HOST_REL%%.*}"

PKG_INDEX_URL="https://cdn.netbsd.org/pub/pkgsrc/packages/NetBSD/$PKG_ARCH/"
RELEASE_INDEX_URL="https://cdn.netbsd.org/pub/NetBSD/"

##############################################################################
# Host bootstrap
##############################################################################

export PATH="/usr/pkg/sbin:/usr/pkg/bin:/usr/sbin:/usr/bin:/sbin:/bin"

HOST_PKG_REPO="${HOST_PKG_REPO:-https://cdn.netbsd.org/pub/pkgsrc/packages/NetBSD/$PKG_ARCH/$HOST_REL/All}"
export PKG_PATH="$HOST_PKG_REPO"

ensure_cmd pkg_add
ensure_cmd tar
ensure_cmd chroot
ensure_cmd pwd_mkdb
ensure_cmd file
ensure_cmd mount
ensure_cmd umount
ensure_cmd stat
ensure_cmd curl
ensure_cmd grep
ensure_cmd sed
ensure_cmd sort
ensure_cmd awk
ensure_cmd dirname

##############################################################################
# Dynamic release discovery
##############################################################################

list_pkg_versions() {
    fetch_index "$PKG_INDEX_URL" |
        sed -n 's@.*href="\([0-9][0-9]*\.[0-9][0-9]*\)/".*@\1@p' |
        awk -F. -v major="$HOST_MAJOR" '$1 == major { print $0 }' |
        sort_versions
}

list_stable_release_versions() {
    fetch_index "$RELEASE_INDEX_URL" |
        sed -n 's@.*href="NetBSD-\([0-9][0-9]*\.[0-9][0-9]*\)/".*@\1@p' |
        awk -F. -v major="$HOST_MAJOR" '$1 == major { print $0 }' |
        sort_versions
}

detect_releases() {
    pkg_versions="$(list_pkg_versions)"
    [ -n "$pkg_versions" ] || die "no pkgsrc package repos found for NetBSD $HOST_MAJOR.x at $PKG_INDEX_URL"

    stable_versions="$(list_stable_release_versions)"
    [ -n "$stable_versions" ] || die "no stable NetBSD $HOST_MAJOR.x release trees found at $RELEASE_INDEX_URL"

    releases="$(intersect_versions "$pkg_versions" "$stable_versions")"
    [ -n "$releases" ] || die "no matching NetBSD $HOST_MAJOR.x versions found in both package and release indexes"

    printf '%s\n' "$releases"
}

if [ -n "${RELEASES:-}" ]; then
    log "using explicit RELEASES override: $RELEASES"
else
    RELEASES="$(detect_releases)"
    log "auto-detected NetBSD $HOST_MAJOR.x releases: $(echo $RELEASES)"
fi

##############################################################################
# Release helpers
##############################################################################

resolve_pkg_repo() {
    rel="$1"
    printf '%s\n' "https://cdn.netbsd.org/pub/pkgsrc/packages/NetBSD/$PKG_ARCH/$rel/All"
}

resolve_pkg_repo_bootstrap() {
    rel="$1"
    printf '%s\n' "http://cdn.netbsd.org/pub/pkgsrc/packages/NetBSD/$PKG_ARCH/$rel/All"
}

release_base_urls() {
    rel="$1"
    printf '%s\n' "https://cdn.netbsd.org/pub/NetBSD/NetBSD-$rel"
    printf '%s\n' "https://ftp.iij.ad.jp/pub/NetBSD/NetBSD-$rel"
    printf '%s\n' "https://ftp.jaist.ac.jp/pub/NetBSD/NetBSD-$rel"
}

##############################################################################
# Download + validation
##############################################################################

validate_archive() {
    archive="$1"
    [ -f "$archive" ] || return 1
    file "$archive" | grep -q 'XZ compressed data' || return 1
    tar -tJf "$archive" >/dev/null 2>&1 || return 1
}

fetch_release_file() {
    rel="$1"
    relpath="$2"
    dest="$3"
    tmp="${dest}.part.$$"

    mkdir -p "$(dirname "$dest")"

    if [ -f "$dest" ] && validate_archive "$dest"; then
        log "using cached $dest"
        return 0
    fi

    rm -f "$dest" "$tmp"

    for base in $(release_base_urls "$rel"); do
        url="$base/$relpath"
        log "fetching $url"

        rm -f "$tmp"

        if curl -4 -L --fail --show-error \
            --retry 3 --retry-delay 2 --retry-all-errors \
            --connect-timeout 20 \
            -A "$USER_AGENT" \
            -o "$tmp" \
            "$url"; then

            if [ ! -s "$tmp" ]; then
                log "empty download from $url"
                rm -f "$tmp"
                continue
            fi

            log "download size: $(stat -f %z "$tmp" 2>/dev/null || echo 0)"

            if validate_archive "$tmp"; then
                mv -f "$tmp" "$dest" || die "failed to rename $tmp to $dest"
                log "success: $url"
                return 0
            fi

            log "invalid archive from $url"
            rm -f "$tmp"
        else
            log "curl failed for $url"
            rm -f "$tmp"
        fi
    done

    rm -f "$tmp"
    die "failed to fetch valid archive: $dest"
}

##############################################################################
# Source copy helper
##############################################################################

copy_source_tree() {
    src="$1"
    dst="$2"

    mkdir -p "$dst"

    (
        cd "$src" || exit 1
        tar \
            --exclude './.git' \
            --exclude './.*' \
            --exclude './dist' \
            --exclude './out' \
            --exclude './packages' \
            --exclude './package-root' \
            --exclude './package-root*' \
            --exclude './.build-netbsd' \
            --exclude './pkgroot' \
            --exclude './pkgroot*' \
            --exclude './stage' \
            --exclude './NetBSD' \
            --exclude './FreeBSD' \
            --exclude './OpenBSD' \
            --exclude './Haiku' \
            -cf - .
    ) | (
        cd "$dst" || exit 1
        tar xpf -
    )
}

##############################################################################
# Build matrix
##############################################################################

echo
echo "=================================================="
echo "==> NetBSD build matrix"
echo "==> host release: $HOST_REL_RAW"
echo "==> target releases: $(echo $RELEASES)"
echo "=================================================="

for REL in $RELEASES; do
    echo
    echo "=================================================="
    echo "==> building for NetBSD $REL"
    echo "=================================================="

    JAILDIR="$JAIL_BASE/$REL-$NETBSD_PORT"
    CURRENT_JAILDIR="$JAILDIR"

    SETSDIR="$SETS_BASE/$REL-$NETBSD_PORT"
    OUTDIR="$ARTIFACT_BASE/$REL"

    PKG_REPO="$(resolve_pkg_repo "$REL")"
    PKG_REPO_BOOTSTRAP="$(resolve_pkg_repo_bootstrap "$REL")"

    log "pkg repo: $PKG_REPO"
    log "bootstrap repo: $PKG_REPO_BOOTSTRAP"
    log "set cache: $SETSDIR"
    log "artifact dir: $OUTDIR"

    mkdir -p "$SETSDIR" "$OUTDIR"

    for set in $SETS; do
        fetch_release_file \
            "$REL" \
            "$NETBSD_PORT/binary/sets/$set.tar.xz" \
            "$SETSDIR/$set.tar.xz"
    done

    log "verifying downloaded sets"
    for set in $SETS; do
        [ -f "$SETSDIR/$set.tar.xz" ] || die "missing set archive: $SETSDIR/$set.tar.xz"
        validate_archive "$SETSDIR/$set.tar.xz" || die "invalid set archive: $SETSDIR/$set.tar.xz"
    done

    log "recreating jail"
    cleanup_mounts
    rm -rf "$JAILDIR"
    mkdir -p "$JAILDIR"

    log "extracting sets"
    for set in $SETS; do
        tar -xpf "$SETSDIR/$set.tar.xz" -C "$JAILDIR"
    done

    log "configuring jail"
    mkdir -p "$JAILDIR/tmp"
    chmod 1777 "$JAILDIR/tmp"

    if [ -f /etc/resolv.conf ]; then
        cp /etc/resolv.conf "$JAILDIR/etc/resolv.conf"
    fi

    mkdir -p "$JAILDIR/dev"
    (
        cd "$JAILDIR/dev" || exit 1
        sh ./MAKEDEV all >/dev/null
    )

    log "mounting /proc"
    mkdir -p "$JAILDIR/proc"
    mount -t procfs proc "$JAILDIR/proc"

    log "seeding SSL certificates from host"
    if [ -d /etc/openssl/certs ]; then
        mkdir -p "$JAILDIR/etc/openssl"
        cp -a /etc/openssl/certs "$JAILDIR/etc/openssl/"
    fi

    if [ -f /etc/openssl/openssl.cnf ]; then
        mkdir -p "$JAILDIR/etc/openssl"
        cp -a /etc/openssl/openssl.cnf "$JAILDIR/etc/openssl/"
    fi

    log "copying source"
    copy_source_tree "$ROOT" "$JAILDIR/root/src"

    [ -f "$JAILDIR/root/src/build_scripts/netbsd-build-freebasic.sh" ] || \
        die "inner build script missing in jail"

    log "patching pkg_add"
    if [ -x "$JAILDIR/usr/sbin/pkg_add" ] && \
       [ ! -e "$JAILDIR/usr/sbin/pkg_add.real" ]; then
        mv "$JAILDIR/usr/sbin/pkg_add" "$JAILDIR/usr/sbin/pkg_add.real"

        cat > "$JAILDIR/usr/sbin/pkg_add" <<'EOF'
#!/bin/sh
exec /usr/sbin/pkg_add.real -f "$@"
EOF
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

    log "entering jail"

    chroot "$JAILDIR" /bin/sh <<EOF
set -eu

pwd_mkdb -p /etc/master.passwd

export PATH="/root/fakebin:/usr/pkg/sbin:/usr/pkg/bin:/usr/sbin:/usr/bin:/sbin:/bin"

export SSL_CERT_DIR=/etc/openssl/certs
export SSL_CERT_FILE=/etc/openssl/certs/ca-certificates.crt

export PKG_PATH="$PKG_REPO_BOOTSTRAP"

if ! command -v pkgin >/dev/null 2>&1; then
    pkg_add pkgin
fi

mkdir -p /usr/pkg/etc/pkgin
echo "$PKG_REPO_BOOTSTRAP" > /usr/pkg/etc/pkgin/repositories.conf

pkgin -y update
pkgin -y install mozilla-rootcerts-openssl

if command -v mozilla-rootcerts >/dev/null 2>&1; then
    mozilla-rootcerts install || true
fi

export SSL_CERT_DIR=/etc/openssl/certs
export SSL_CERT_FILE=/etc/openssl/certs/ca-certificates.crt

echo "$PKG_REPO" > /usr/pkg/etc/pkgin/repositories.conf
export PKG_PATH="$PKG_REPO"

pkgin -y update

cd /root/src
exec /bin/sh build_scripts/netbsd-build-freebasic.sh
EOF

    log "collecting package"

    set -- "$JAILDIR/root/src/out/"*.tgz
    [ -e "$1" ] || die "no package produced for $REL"

    mkdir -p "$OUTDIR"
    cp "$JAILDIR/root/src/out/"*.tgz "$OUTDIR/"

    cleanup_mounts
    CURRENT_JAILDIR=""
done

echo
echo "==> ALL BUILDS COMPLETE"
