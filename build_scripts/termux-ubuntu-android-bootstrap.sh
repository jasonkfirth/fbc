#!/data/data/com.termux/files/usr/bin/bash

# Project: FreeBASIC Android packaging
# -----------------------------------
#
# File: termux-ubuntu-android-bootstrap.sh
#
# Purpose:
#
#     Prepare a Termux phone for building FreeBASIC Android APKs with a
#     native ARM64 Ubuntu PRoot and Termux's native Android build tools.
#
# Responsibilities:
#
#     * install the native Termux Clang, NDK sysroot, Java, and APK tools
#     * create an ARM64 Ubuntu build container on an ARM64 Android phone
#     * install a Noble-compatible ARM64 FreeBASIC host compiler
#     * extract the architecture-independent Android runtimes and templates
#     * build and inspect a signed arm64-v8a APK entirely on the phone
#
# This file intentionally does NOT contain:
#
#     * QEMU or x86_64 host-tool setup
#     * Android emulator setup
#     * production APK signing-key management
#
# Google publishes Linux NDK host binaries for x86_64 only.  Termux supplies
# native ARM64 Clang and the NDK sysroot, plus native AAPT2 and Java-hosted D8
# and APK signer tools.  The Ubuntu PRoot therefore remains ARM64 throughout;
# only the generated application is cross-linked against Android's ABI.

set -euo pipefail

SCRIPT_NAME="${0##*/}"
TERMUX_ROOT="${HOME}/.cache/freebasic-termux-android"
CONTAINER_NAME="freebasic-android-arm64"

ANDROID_PACKAGE_URL="${FBANDROID_DEB_URL:-https://deb.fbxl.net/linux/ubuntu/noble/amd64/freebasic-android_1.20.3-1_amd64.deb}"
ANDROID_PACKAGE_SHA256="${FBANDROID_DEB_SHA256:-b313f059cc292bd832bf8594de2f1576da25bdd3c132150bd6f47edd79b24647}"
HOST_PACKAGE_URL="${FBANDROID_HOST_DEB_URL:-https://deb.fbxl.net/install/assets/freebasic-host-noble-arm64_1.20.3-1_arm64.deb}"
HOST_PACKAGE_SHA256="${FBANDROID_HOST_DEB_SHA256:-f58f32f564eefc2cb223c5e011861e256a3012a96b0bdc18ed6b3361d359fc6d}"
PLATFORM_JAR_URL="${FBANDROID_PLATFORM_JAR_URL:-https://deb.fbxl.net/install/assets/android-platform-35.jar}"
PLATFORM_JAR_SHA256="${FBANDROID_PLATFORM_JAR_SHA256:-4566663c3876e022b4fa4ced8c8697c4ab1688267f090114fd92d027b32e619b}"
SETUP_ONLY=0

die() {
	echo "${SCRIPT_NAME}: $*" >&2
	exit 1
}

usage() {
	cat <<EOF
Usage: ${SCRIPT_NAME} [options]

Options:
  --package-url URL       Android runtime/template package URL
  --host-package-url URL  ARM64 FreeBASIC host package URL
  --container NAME        PRoot-Distro container name
  --setup-only            Install tools but do not build the APK smoke test
  --help                  Show this help text

The bootstrap uses an ARM64 Ubuntu PRoot and native Termux Android tools.
No QEMU or x86_64 NDK host binaries are installed.
EOF
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--package-url)
			[ "$#" -ge 2 ] || die "--package-url requires a URL"
			ANDROID_PACKAGE_URL="$2"
			shift 2
			;;
		--host-package-url)
			[ "$#" -ge 2 ] || die "--host-package-url requires a URL"
			HOST_PACKAGE_URL="$2"
			shift 2
			;;
		--container)
			[ "$#" -ge 2 ] || die "--container requires a name"
			CONTAINER_NAME="$2"
			shift 2
			;;
		--setup-only)
			SETUP_ONLY=1
			shift
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			die "unknown option: $1"
			;;
	esac
done

case "${PREFIX:-}" in
	*/com.termux/files/usr)
		;;
	*)
		die "run this script from the Termux application"
		;;
esac

command -v pkg >/dev/null 2>&1 || die "Termux pkg command is unavailable"

download_verified() {
	local url="$1"
	local expected_sha256="$2"
	local output="$3"
	local label="$4"

	mkdir -p "${output%/*}"

	if [ -f "$output" ] && \
		printf '%s  %s\n' "$expected_sha256" "$output" | sha256sum -c - >/dev/null 2>&1; then
		echo "==> Reusing ${label}"
		return 0
	fi

	echo "==> Downloading ${label}"
	curl -fL --retry 3 --output "${output}.part" "$url"
	printf '%s  %s\n' "$expected_sha256" "${output}.part" | sha256sum -c -
	mv "${output}.part" "$output"
}

echo "==> Installing native Termux Android build tools"
export DEBIAN_FRONTEND=noninteractive

pkg update -y

# Keep the package database coherent before adding the large Java and NDK
# packages.  An interrupted rolling upgrade can otherwise leave PRoot-Distro
# beside an incompatible Python or C library.
dpkg --force-confold --configure -a
pkg upgrade -y -o Dpkg::Options::=--force-confold

pkg install -y -o Dpkg::Options::=--force-confold \
	aapt2 apksigner clang coreutils curl d8 ndk-multilib ndk-sysroot \
	openjdk-21 proot-distro tar unzip zip

mkdir -p "$TERMUX_ROOT/out"

# Older Android system certificate stores may not trust the current public
# web PKI.  Termux maintains its own current bundle and should use it for all
# bootstrap downloads.
TERMUX_CA_BUNDLE="${PREFIX}/etc/tls/cert.pem"

if [ -z "${CURL_CA_BUNDLE:-}" ] && [ -s "$TERMUX_CA_BUNDLE" ]; then
	export CURL_CA_BUNDLE="$TERMUX_CA_BUNDLE"
fi

HOST_PACKAGE_PATH="$TERMUX_ROOT/freebasic-host-arm64.deb"
ANDROID_PACKAGE_PATH="$TERMUX_ROOT/freebasic-android.deb"
NATIVE_SDK_ROOT="$TERMUX_ROOT/android-sdk-native"
PLATFORM_JAR_PATH="$NATIVE_SDK_ROOT/platforms/android-35/android.jar"

download_verified \
	"$HOST_PACKAGE_URL" \
	"$HOST_PACKAGE_SHA256" \
	"$HOST_PACKAGE_PATH" \
	"FreeBASIC ARM64 host package"

download_verified \
	"$ANDROID_PACKAGE_URL" \
	"$ANDROID_PACKAGE_SHA256" \
	"$ANDROID_PACKAGE_PATH" \
	"FreeBASIC Android runtime package"

download_verified \
	"$PLATFORM_JAR_URL" \
	"$PLATFORM_JAR_SHA256" \
	"$PLATFORM_JAR_PATH" \
	"Android API 35 platform jar"

# ---------------------------------------------------------------------------
# Native Android SDK facade
# ---------------------------------------------------------------------------

# fbc-android expects the directory layout used by Google's NDK.  These small
# launchers preserve that stable interface while selecting Termux's native
# ARM64 tools.  ndk-multilib supplies the non-native ABI libraries when a
# developer requests ARMv7 or x86-64 output.
NATIVE_BUILD_TOOLS="$NATIVE_SDK_ROOT/build-tools/termux-native"
NATIVE_NDK_BIN="$NATIVE_SDK_ROOT/ndk/termux-native/toolchains/llvm/prebuilt/linux-aarch64/bin"
NATIVE_CLANG_WRAPPER="$NATIVE_NDK_BIN/android-clang"

mkdir -p "$NATIVE_BUILD_TOOLS" "$NATIVE_NDK_BIN"

cat > "$NATIVE_BUILD_TOOLS/aapt" <<EOF
#!${PREFIX}/bin/bash

if [ "\${1:-}" = version ]; then
	echo "Android Asset Packaging Tool, v0.2-debian"
	exit 0
fi

exec "${PREFIX}/bin/aapt2" "\$@"
EOF

cat > "$NATIVE_CLANG_WRAPPER" <<EOF
#!${PREFIX}/bin/bash

set -euo pipefail

tool_name="\${0##*/}"
target="\${tool_name%-clang}"

exec "${PREFIX}/bin/clang" "--target=\${target}" "\$@"
EOF

chmod 700 "$NATIVE_BUILD_TOOLS/aapt" "$NATIVE_CLANG_WRAPPER"
ln -sfn "${PREFIX}/bin/aapt2" "$NATIVE_BUILD_TOOLS/aapt2"
ln -sfn "${PREFIX}/bin/apksigner" "$NATIVE_BUILD_TOOLS/apksigner"
ln -sfn "${PREFIX}/bin/d8" "$NATIVE_BUILD_TOOLS/d8"
ln -sfn "${PREFIX}/bin/llvm-ar" "$NATIVE_NDK_BIN/llvm-ar"
ln -sfn "${PREFIX}/bin/llvm-ranlib" "$NATIVE_NDK_BIN/llvm-ranlib"

# FreeBASIC's Android driver supports API 21 and newer.  Generate the familiar
# target-prefixed NDK names for every API through the packaged API 35 platform.
for api in $(seq 21 35); do
	for triple in \
		aarch64-linux-android \
		armv7a-linux-androideabi \
		x86_64-linux-android
	do
		ln -sfn android-clang "$NATIVE_NDK_BIN/${triple}${api}-clang"
	done
done

export PROOT_NO_SECCOMP="${PROOT_NO_SECCOMP:-1}"

if ! proot-distro login "$CONTAINER_NAME" -- /bin/true >/dev/null 2>&1; then
	echo "==> Installing native ARM64 Ubuntu container: $CONTAINER_NAME"
	proot-distro install ubuntu:24.04 --name "$CONTAINER_NAME" --architecture aarch64
fi

PROVISION_SCRIPT="$TERMUX_ROOT/provision-ubuntu-arm64.sh"
cat > "$PROVISION_SCRIPT" <<'PROVISION_EOF'
#!/usr/bin/env bash
#
# FreeBASIC native ARM64 Android provisioner
# -------------------------------------------
#
# Install the ARM64 host compiler, expose the Android package data, and build
# the on-phone APK smoke test with Termux's native Android tools.

set -euo pipefail

SETUP_ONLY="$1"
TERMUX_PREFIX="$2"
WORKDIR=/mnt/freebasic-termux
SDK_ROOT=/opt/android-sdk-native
ANDROID_PACKAGE_ROOT="$WORKDIR/android-package"
HOST_PACKAGE="$WORKDIR/freebasic-host-arm64.deb"
ANDROID_PACKAGE="$WORKDIR/freebasic-android.deb"

die() {
	echo "Termux Ubuntu ARM64 provisioner: $*" >&2
	exit 1
}

export DEBIAN_FRONTEND=noninteractive

if [ ! -s /etc/ssl/certs/ca-certificates.crt ] || \
	! command -v curl >/dev/null 2>&1 || \
	! command -v gcc >/dev/null 2>&1 || \
	! command -v unzip >/dev/null 2>&1 || \
	! command -v zip >/dev/null 2>&1
then
	apt-get update -y
	apt-get install -y --no-install-recommends ca-certificates curl gcc unzip zip
fi

# apt installs the compiler's normal libc, terminal, assembler, and header
# dependencies.  The package was built on Ubuntu Noble so it remains usable
# in this Noble ARM64 PRoot without a newer glibc.
apt-get install -y "$HOST_PACKAGE"

fbc -version | grep -q 'Version 1.20.3-1' || \
	die "the native ARM64 FreeBASIC 1.20.3 host compiler is unavailable"

case "$ANDROID_PACKAGE_ROOT" in
	/mnt/freebasic-termux/android-package)
		rm -rf -- "$ANDROID_PACKAGE_ROOT"
		;;
	*)
		die "refusing to replace unexpected Android package root: $ANDROID_PACKAGE_ROOT"
		;;
esac

mkdir -p "$ANDROID_PACKAGE_ROOT"
dpkg-deb --extract "$ANDROID_PACKAGE" "$ANDROID_PACKAGE_ROOT"

ANDROID_PREFIX="$ANDROID_PACKAGE_ROOT/usr"
FBC_ANDROID="$ANDROID_PREFIX/bin/fbc-android"

[ -x "$FBC_ANDROID" ] || die "the Android package did not contain fbc-android"
[ -f "$SDK_ROOT/platforms/android-35/android.jar" ] || \
	die "the Android API 35 platform jar is missing"
[ -x "$SDK_ROOT/ndk/termux-native/toolchains/llvm/prebuilt/linux-aarch64/bin/aarch64-linux-android26-clang" ] || \
	die "the native Termux Android Clang facade is missing"

export ANDROID_HOME="$SDK_ROOT"
export ANDROID_SDK_ROOT="$SDK_ROOT"
export ANDROID_NDK_HOME="$SDK_ROOT/ndk/termux-native"
export FBANDROID_PREFIX="$ANDROID_PREFIX"
export FBANDROID_COMPILER=/usr/bin/fbc
export FBANDROID_LIBROOT="$ANDROID_PREFIX/lib/freebasic-android"
export FBANDROID_INCDIR="$ANDROID_PREFIX/include/freebasic-android"
export FBANDROID_SHARE="$ANDROID_PREFIX/share/freebasic-android"
export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:${TERMUX_PREFIX}/bin"

if [ "$SETUP_ONLY" = 1 ]; then
	echo "TERMUX_FREEBASIC_ANDROID_NATIVE_SETUP_PASS"
	exit 0
fi

cat > "$WORKDIR/termux-android-smoke.bas" <<'BASIC_EOF'
print "TERMUX_FREEBASIC_ANDROID_NATIVE_SMOKE"
sleep 10000, 1
BASIC_EOF

echo "==> Building arm64-v8a APK with native ARM64 host tools"
cd "$WORKDIR/out"
"$FBC_ANDROID" \
	--target android-aarch64 \
	--target-api 35 \
	--package org.freebasic.termux.smoke \
	--label FreeBASIC-Termux-Smoke \
	"$WORKDIR/termux-android-smoke.bas" \
	-x termux-freebasic-smoke.apk

[ -f termux-freebasic-smoke.apk ] || die "fbc-android did not produce an APK"
unzip -l termux-freebasic-smoke.apk | grep -q 'lib/arm64-v8a/libfreebasicapp.so' || \
	die "APK is missing its arm64-v8a native library"

echo "TERMUX_FREEBASIC_ANDROID_NATIVE_BUILD_PASS"

# end of provision-ubuntu-arm64.sh
PROVISION_EOF
chmod 700 "$PROVISION_SCRIPT"

PROOT_LOGIN_ARGS=(
	--bind "$TERMUX_ROOT:/mnt/freebasic-termux"
	--bind "$NATIVE_SDK_ROOT:/opt/android-sdk-native"
)

for PROXY_VARIABLE in \
	http_proxy https_proxy all_proxy \
	HTTP_PROXY HTTPS_PROXY ALL_PROXY
do
	PROXY_VALUE="${!PROXY_VARIABLE:-}"

	if [ -n "$PROXY_VALUE" ]; then
		PROOT_LOGIN_ARGS+=(--env "${PROXY_VARIABLE}=${PROXY_VALUE}")
	fi
done

echo "==> Preparing native ARM64 Ubuntu Android build environment"
proot-distro login \
	"${PROOT_LOGIN_ARGS[@]}" \
	"$CONTAINER_NAME" \
	-- /bin/bash /mnt/freebasic-termux/provision-ubuntu-arm64.sh "$SETUP_ONLY" "$PREFIX"

if [ "$SETUP_ONLY" -eq 0 ]; then
	APK_PATH="$TERMUX_ROOT/out/termux-freebasic-smoke.apk"
	[ -f "$APK_PATH" ] || die "Ubuntu reported success but the APK is not visible in Termux"
	echo "APK: $APK_PATH"
	echo "TERMUX_FREEBASIC_ANDROID_NATIVE_BUILD_PASS"
fi

# end of termux-ubuntu-android-bootstrap.sh
