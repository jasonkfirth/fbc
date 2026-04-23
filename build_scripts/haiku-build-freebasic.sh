#!/bin/sh

set -e

##############################################################################
# Validate invocation location
##############################################################################

if [ ! -d "build_scripts" ]; then
	echo ""
	echo "ERROR: run this script from the project root."
	echo "Expected to find ./build_scripts directory."
	exit 1
fi

if [ "$(basename "$PWD")" = "build_scripts" ]; then
	echo ""
	echo "ERROR: do not run this script from the build_scripts directory."
	echo "Run it from the project root:"
	echo "  ./build_scripts/haiku-build-freebasic.sh"
	exit 1
fi

##############################################################################
# Options
##############################################################################

NOBUILD=0
NOPACKAGE=0

for arg in "$@"; do
	case "$arg" in
		--nobuild) NOBUILD=1 ;;
		--nopackage) NOPACKAGE=1 ;;
		*) echo "Unknown option: $arg"; exit 1 ;;
	esac
done

msg(){ echo ""; echo "==> $1"; }
fail(){ echo ""; echo "ERROR: $1"; exit 1; }

##############################################################################
# Version extraction
##############################################################################

VERSION=$(sed -n "s/^FBVERSION[[:space:]]*:=[[:space:]]*//p" mk/version.mk | head -n1)
REV=$(sed -n "s/^REV[[:space:]]*:=[[:space:]]*//p" mk/version.mk | head -n1)

[ -z "$VERSION" ] && fail "FBVERSION missing"
[ -z "$REV" ] && fail "REV missing"

FULLVERSION="${VERSION}-${REV}"
ARCH=$(uname -m)
HPKG="freebasic-${FULLVERSION}-${ARCH}.hpkg"

##############################################################################
# Build phase
##############################################################################

if [ "$NOBUILD" -eq 0 ]; then

	msg "Cleaning packaging artifacts"
	rm -rf package-root
	rm -f ./*.hpkg
	rm -f ./*.install_manifest package-root.install_manifest

	msg "Ensuring build tools"
	pkgman install -y haiku_devel make gcc binutils rsync libffi_devel ||true
	pkgman install -y  ncurses6 ncurses6_devel || true

	CPU_COUNT=$(sysinfo -cpu 2>/dev/null | grep -c "^CPU #" || true)
	[ -z "$CPU_COUNT" ] && CPU_COUNT=1
	[ "$CPU_COUNT" -lt 1 ] && CPU_COUNT=1
	JOBS=$((CPU_COUNT + 1))

	msg "Building FreeBASIC ($JOBS threads)"
	make -j"$JOBS"

fi

##############################################################################
# Packaging phase
##############################################################################

if [ "$NOPACKAGE" -eq 0 ]; then

	STAGE="$PWD/package-root"

	msg "Preparing staging directory"
	rm -rf "$STAGE"

	make install DESTDIR="$STAGE"

##############################################################################
# Package metadata
##############################################################################

	msg "Generating PackageInfo"

	cat > "$STAGE/.PackageInfo" <<META
name "freebasic"
version "$FULLVERSION"
architecture "$ARCH"
summary "FreeBASIC compiler"
description "FreeBASIC compiler for Haiku"
vendor "FreeBASIC"
packager "local build"

licenses {
	"GNU GPL v2"
	"GNU LGPL v2.1"
}

copyrights {
	"2004-2024 FreeBASIC Team"
}

provides {
	freebasic = $FULLVERSION
	cmd:fbc = $FULLVERSION
}

requires {
	haiku
	lib:libstdc++
	lib:libgcc_s
	lib:libncursesw
	ncurses6
}
META

##############################################################################
# Create package
##############################################################################

	msg "Creating Haiku package"
	(
		cd "$STAGE"
		package create "../$HPKG"
	)

##############################################################################
# Remove existing installation
##############################################################################

	msg "Removing previous FreeBASIC installation"
	pkgman uninstall -y freebasic

##############################################################################
# Install package
##############################################################################

	msg "Installing package"
	pkgman install -y "./$HPKG"

##############################################################################
# Sanity check
##############################################################################

	msg "Running compiler sanity check"
	if command -v fbc >/dev/null 2>&1; then
		fbc -version
	else
		fail "Installed compiler not found in PATH"
	fi

##############################################################################
# Cleanup
##############################################################################

	msg "Cleaning staging files"
	rm -rf "$STAGE"
	rm -f ./*.install_manifest package-root.install_manifest

	msg "Build complete"

	echo "Package created: $HPKG"
	echo "Compiler installed at: /boot/system/bin/fbc"

fi
