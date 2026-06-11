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
NOINSTALL=0
CLEANUP_SUCCESS=0

for arg in "$@"; do
	case "$arg" in
		--nobuild) NOBUILD=1 ;;
		--nopackage) NOPACKAGE=1 ;;
		--noinstall) NOINSTALL=1 ;;
		*) echo "Unknown option: $arg"; exit 1 ;;
	esac
done

msg(){ echo ""; echo "==> $1"; }
fail(){ echo ""; echo "ERROR: $1"; exit 1; }

##############################################################################
# Haiku package helpers
##############################################################################

run_limited() {
	limit="$1"
	shift

	"$@" &
	pid=$!
	elapsed=0

	while kill -0 "$pid" >/dev/null 2>&1; do
		if [ "$elapsed" -ge "$limit" ]; then
			kill "$pid" >/dev/null 2>&1 || true
			wait "$pid" >/dev/null 2>&1 || true
			return 124
		fi

		sleep 1
		elapsed=$((elapsed + 1))
	done

	wait "$pid"
}

install_image_package() {
	name="$1"
	package_file="$(find /boot/_packages_ -maxdepth 1 -type f -name "$name-*.hpkg" 2>/dev/null | sort | tail -n 1)"

	if [ -z "$package_file" ]; then
		return 1
	fi

	if [ -e "/boot/system/packages/$(basename "$package_file")" ]; then
		return 0
	fi

	echo "==> installing image package $package_file"
	pkgman install -y "$package_file"
}

install_image_packages() {
	for package_name in "$@"; do
		install_image_package "$package_name" || true
	done
}

install_optional_network_packages() {
	[ "${HAIKU_SKIP_NET_DEPS:-0}" = "1" ] && return 0

	run_limited 300 pkgman install -y "$@" || {
		echo "WARNING: optional pkgman install failed or timed out: $*" >&2
		return 0
	}
}

cleanup_package_states() {
	find /boot/system/packages/administrative -maxdepth 1 -type d -name 'state_*' -exec rm -rf {} + 2>/dev/null || true
}

cleanup_build_artifacts() {
	[ "$CLEANUP_SUCCESS" -eq 1 ] || return 0

	rm -rf package-root package-root.install_manifest
	for artifact in ./*.hpkg ./*.install_manifest; do
		[ -e "$artifact" ] || continue
		case "$artifact" in
			*.hpkg)
				[ "${HAIKU_PRESERVE_HPKG:-0}" = "1" ] && continue
				;;
		esac
		rm -f "$artifact" || true
	done
}

trap cleanup_build_artifacts EXIT

##############################################################################
# Version extraction
##############################################################################

VERSION=$(sed -n "s/^FBVERSION[[:space:]]*:=[[:space:]]*//p" mk/version.mk | head -n1)
REV=$(sed -n "s/^REV[[:space:]]*:=[[:space:]]*//p" mk/version.mk | head -n1)

[ -z "$VERSION" ] && fail "FBVERSION missing"
[ -z "$REV" ] && fail "REV missing"

FULLVERSION="${VERSION}-${REV}"
detect_package_arch() {
	if command -v getarch >/dev/null 2>&1; then
		getarch
		return 0
	fi

	case "$(uname -m)" in
		x86_64|amd64)
			echo "x86_64"
			;;
		BePC|i386|i486|i586|i686|x86)
			echo "x86_gcc2"
			;;
		aarch64|arm64)
			echo "arm64"
			;;
		*)
			uname -m
			;;
	esac
}

ARCH=$(detect_package_arch)
PACKAGE_NAME="freebasic"
PACKAGE_FBCFLAGS=""
USE_X86_SECONDARY=0

if [ "$ARCH" = "x86_gcc2" ]; then
	PACKAGE_NAME="freebasic_x86"
	PACKAGE_FBCFLAGS='-d ENABLE_PREFIX=\"/boot/system\"'
	USE_X86_SECONDARY=1
fi

HPKG="${PACKAGE_NAME}-${FULLVERSION}-${ARCH}.hpkg"

run_build_arch() {
	if [ "$USE_X86_SECONDARY" -eq 1 ]; then
		setarch x86 "$@"
	else
		"$@"
	fi
}

##############################################################################
# Build phase
##############################################################################

if [ "$NOBUILD" -eq 0 ]; then

	msg "Cleaning packaging artifacts"
	rm -rf package-root
	rm -f ./*.hpkg
	rm -f ./*.install_manifest package-root.install_manifest

	if [ "${HAIKU_SKIP_DEPS:-0}" != "1" ]; then
		msg "Ensuring build tools"
		install_image_packages \
			haiku_devel \
			make \
			binutils \
			mpc \
			mpfr \
			gcc \
			pkgconfig \
			zstd_devel

		if [ "$USE_X86_SECONDARY" -eq 1 ]; then
			install_image_packages \
				haiku_x86_devel \
				binutils_x86 \
				mpc_x86 \
				mpfr_x86 \
				gcc_x86 \
				gcc_x86_syslibs \
				zstd_x86_devel
		fi

		cleanup_package_states
		if [ "$USE_X86_SECONDARY" -eq 1 ]; then
			install_optional_network_packages libffi_x86_devel ncurses6_x86_devel
		else
			install_optional_network_packages libffi_devel ncurses6_devel
		fi
		cleanup_package_states
	fi

	CPU_COUNT=$(sysinfo -cpu 2>/dev/null | grep -c "^CPU #" || true)
	[ -z "$CPU_COUNT" ] && CPU_COUNT=1
	[ "$CPU_COUNT" -lt 1 ] && CPU_COUNT=1
	JOBS=$((CPU_COUNT + 1))

	if [ -x bin/fbc ] && ! run_build_arch bin/fbc -version >/dev/null 2>&1; then
		msg "Removing unusable in-tree compiler"
		rm -f bin/fbc bin/fbc-js
	fi

	if [ ! -x bin/fbc ]; then
		msg "Building bootstrap compiler ($JOBS threads)"
		run_build_arch make -j"$JOBS" HAVE_PREREQS_MK= FBCFLAGS="$PACKAGE_FBCFLAGS" bootstrap-minimal
	fi

	msg "Building FreeBASIC ($JOBS threads)"
	run_build_arch make -j"$JOBS" HAVE_PREREQS_MK= FBCFLAGS="$PACKAGE_FBCFLAGS"

fi
##############################################################################
# Packaging phase
##############################################################################

if [ "$NOPACKAGE" -eq 0 ]; then

	STAGE="$PWD/package-root"

	msg "Preparing staging directory"
	rm -rf "$STAGE"

	run_build_arch make HAVE_PREREQS_MK= FBCFLAGS="$PACKAGE_FBCFLAGS" install DESTDIR="$STAGE"

	msg "Staging examples"
	mkdir -p "$STAGE/data/freebasic"
	cp -R examples "$STAGE/data/freebasic/"
	find "$STAGE/data/freebasic/examples" -type f \
		\( -name '*.o' -o -name '*.obj' -o -name '*.exe' \
		   -o -name '*.dll' -o -name '*.so' \) \
		-exec rm -f {} +
	rm -f "$STAGE/data/freebasic/examples/sfxlib/showcase"

##############################################################################
# Package metadata
##############################################################################

	msg "Generating PackageInfo"

	cat > "$STAGE/.PackageInfo" <<META
name "$PACKAGE_NAME"
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
	$PACKAGE_NAME = $FULLVERSION
	cmd:fbc = $FULLVERSION
}

META

	if [ "$USE_X86_SECONDARY" -eq 1 ]; then
		cat >> "$STAGE/.PackageInfo" <<META
requires {
	haiku_x86
	lib:libstdc++_x86
	lib:libgcc_s_x86
	lib:libncursesw_x86
	ncurses6_x86
}
META
	else
		cat >> "$STAGE/.PackageInfo" <<META
requires {
	haiku
	lib:libstdc++
	lib:libgcc_s
	lib:libncursesw
	ncurses6
}
META
	fi

##############################################################################
# Create package
##############################################################################

	msg "Creating Haiku package"
	(
		cd "$STAGE"
		package create "../$HPKG"
	)

	if [ "$NOINSTALL" -eq 0 ]; then

##############################################################################
# Remove existing installation
##############################################################################

	msg "Removing previous FreeBASIC installation"
	pkgman uninstall -y "$PACKAGE_NAME"

##############################################################################
# Install package
##############################################################################

	msg "Installing package"
	pkgman install -y "./$HPKG"

##############################################################################
# Sanity check
##############################################################################

	msg "Running compiler sanity check"
	run_build_arch fbc -version

	fi

##############################################################################
# Cleanup
##############################################################################

	msg "Cleaning staging files"
	rm -rf "$STAGE"
	rm -f ./*.install_manifest package-root.install_manifest

	msg "Build complete"

	CLEANUP_SUCCESS=1

	echo "Package created: $HPKG"
	echo "Compiler installed at: /boot/system/bin/fbc"

fi
