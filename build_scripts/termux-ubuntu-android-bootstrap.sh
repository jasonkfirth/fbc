#!/usr/bin/env bash

# Project: FreeBASIC Android packaging
# -----------------------------------
#
# File: termux-ubuntu-android-bootstrap.sh
#
# Purpose:
#
#     Prepare a Termux phone for building FreeBASIC Android APKs and compile
#     a small arm64-v8a APK as a setup verification.
#
# Responsibilities:
#
#     * install PRoot-Distro and its x86_64 QEMU user-mode helper
#     * create an x86_64 Ubuntu build container on an ARM Android phone
#     * install Android command-line tools, platform 35, and NDK r27d
#     * install an amd64 freebasic-android package from deb.fbxl.net
#     * build and inspect an arm64-v8a APK smoke program
#
# This file intentionally does NOT contain:
#
#     * Android emulator setup
#     * APK signing-key management beyond the debug key used by fbc-android
#     * installation of the resulting APK on the phone
#
# Android NDK host tools are published for Linux x86_64, not Linux ARM64.
# The Ubuntu container is therefore deliberately x86_64 and uses Termux's
# QEMU user-mode support. The generated APK itself targets the phone's native
# arm64-v8a ABI.

set -euo pipefail

SCRIPT_NAME="${0##*/}"
TERMUX_ROOT="${HOME}/.cache/freebasic-termux-android"
CONTAINER_NAME="freebasic-android-amd64"
PACKAGE_URL="${FBANDROID_DEB_URL:-https://deb.fbxl.net/linux/ubuntu/noble/amd64/freebasic-android_1.20.3-1_amd64.deb}"
SDK_BUNDLE_URL="${FBANDROID_SDK_BUNDLE_URL:-https://deb.fbxl.net/install/assets/freebasic-android-sdk-api35-ndk27d-linux-x86_64.tar.zst}"
SDK_BUNDLE_SHA256="${FBANDROID_SDK_BUNDLE_SHA256:-a8fe9ac1f99d70aae27023376a61beb1d8487497ded1df00151656aaa5d793ef}"
SETUP_ONLY=0

die() {
	echo "${SCRIPT_NAME}: $*" >&2
	exit 1
}

usage() {
	cat <<EOF
Usage: ${SCRIPT_NAME} [options]

Options:
  --package-url URL  URL of the amd64 freebasic-android .deb package
  --container NAME   PRoot-Distro container name
  --setup-only       Install tools but do not build the APK smoke test
  --help             Show this help text

The default package URL is:
  ${PACKAGE_URL}

Run directly in Termux.  The script needs roughly 5 GiB of free phone
storage because the x86_64 Ubuntu image, Android SDK, and NDK are retained
for later APK builds.
EOF
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--package-url)
			[ "$#" -ge 2 ] || die "--package-url requires a URL"
			PACKAGE_URL="$2"
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

echo "==> Installing Termux container prerequisites"
export DEBIAN_FRONTEND=noninteractive

pkg update -y

# Termux is a rolling distribution. A partial upgrade can install a current
# PRoot-Distro package beside an older Python ABI and leave its launcher
# unusable, so bring the existing base environment forward as one unit.
#
# Keep local package-manager configuration when the phone already has a
# selected mirror. The explicit dpkg pass also repairs an upgrade that was
# interrupted at a conffile prompt.
dpkg --force-confold --configure -a
pkg upgrade -y -o Dpkg::Options::=--force-confold

pkg install -y -o Dpkg::Options::=--force-confold \
	curl proot-distro qemu-user-x86-64 unzip zstd

mkdir -p "$TERMUX_ROOT/out"

# Download and unpack the large Android SDK with Termux's native ARM tools.
# Running curl and decompression through QEMU makes an initial phone setup
# needlessly slow. The bundle is a cached copy of Google's Linux x86_64 API 35
# tools and NDK r27d, which are still executed inside the Ubuntu container.
TERMUX_SDK_ROOT="$TERMUX_ROOT/android-sdk"
SDK_BUNDLE_PATH="$TERMUX_ROOT/freebasic-android-sdk.tar.zst"

if [ ! -x "$TERMUX_SDK_ROOT/ndk/27.3.13750724/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang" ]; then
	echo "==> Downloading Android API 35 and NDK r27d bundle"
	curl -fL --retry 3 --output "$SDK_BUNDLE_PATH.part" "$SDK_BUNDLE_URL"
	echo "${SDK_BUNDLE_SHA256}  ${SDK_BUNDLE_PATH}.part" | sha256sum -c -
	mv "$SDK_BUNDLE_PATH.part" "$SDK_BUNDLE_PATH"

	case "$TERMUX_SDK_ROOT" in
		"$TERMUX_ROOT"/android-sdk)
			rm -rf -- "$TERMUX_SDK_ROOT"
			;;
		*)
			die "refusing to replace unexpected SDK path: $TERMUX_SDK_ROOT"
			;;
	esac

	mkdir -p "$TERMUX_SDK_ROOT"
	tar --use-compress-program=unzstd -xf "$SDK_BUNDLE_PATH" -C "$TERMUX_SDK_ROOT"
fi

# Older Android kernels can deliver PRoot's seccomp-assisted syscall events
# out of the order expected by its tracer when QEMU is also translating the
# guest CPU. The ordinary ptrace path is slower but works across these phones.
export PROOT_NO_SECCOMP="${PROOT_NO_SECCOMP:-1}"

if ! proot-distro login "$CONTAINER_NAME" -- /bin/true >/dev/null 2>&1; then
	echo "==> Installing x86_64 Ubuntu container: $CONTAINER_NAME"
	proot-distro install ubuntu:24.04 --name "$CONTAINER_NAME" --architecture x86_64
fi

PROVISION_SCRIPT="$TERMUX_ROOT/provision-ubuntu.sh"
cat > "$PROVISION_SCRIPT" <<'PROVISION_EOF'
#!/usr/bin/env bash

set -euo pipefail

PACKAGE_URL="$1"
SETUP_ONLY="$2"
WORKDIR=/mnt/freebasic-termux
SDK_ROOT=/opt/android-sdk
NDK_VERSION=27.3.13750724

die() {
	echo "termux Ubuntu provisioner: $*" >&2
	exit 1
}

export DEBIAN_FRONTEND=noninteractive

if [ ! -s /etc/ssl/certs/ca-certificates.crt ] || \
	! command -v curl >/dev/null 2>&1 || \
	! command -v javac >/dev/null 2>&1 || \
	! command -v unzip >/dev/null 2>&1 || \
	! command -v zip >/dev/null 2>&1
then
	apt-get update -y
	apt-get install -y --no-install-recommends \
		ca-certificates curl openjdk-17-jdk-headless unzip zip
fi

mkdir -p "$WORKDIR/out"

export ANDROID_HOME="$SDK_ROOT"
export ANDROID_SDK_ROOT="$SDK_ROOT"
export PATH="$SDK_ROOT/cmdline-tools/latest/bin:$SDK_ROOT/platform-tools:$SDK_ROOT/build-tools/35.0.0:$PATH"

[ -f "$SDK_ROOT/platforms/android-35/android.jar" ] || die "Android API 35 platform is missing"
[ -x "$SDK_ROOT/build-tools/35.0.0/aapt2" ] || die "Android API 35 build tools are missing"

compiler_works() {
	command -v fbc-android >/dev/null 2>&1 &&
		[ -x /usr/lib/freebasic-android/bin/fbc-android-compiler ] &&
		/usr/lib/freebasic-android/bin/fbc-android-compiler -version >/dev/null 2>&1
}

if ! compiler_works; then
	echo "==> Downloading freebasic-android package"
	curl -fL --retry 3 --output "$WORKDIR/freebasic-android.deb" "$PACKAGE_URL"

	# The package's Ubuntu dependency names describe desktop SDK packages.
	# The SDK and NDK above are deliberately installed from Google's official
	# archives, so force only those metadata dependencies while retaining the
	# package's files and normal executable dependencies.
	dpkg --force-depends -i "$WORKDIR/freebasic-android.deb"
fi

compiler_works || die "fbc-android was not installed or cannot run on this Ubuntu release"
[ -x "$SDK_ROOT/ndk/${NDK_VERSION}/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang" ] || \
	die "Android NDK arm64 compiler is missing"

if [ "$SETUP_ONLY" = 1 ]; then
	echo "TERMUX_FREEBASIC_ANDROID_SETUP_PASS"
	exit 0
fi

cat > "$WORKDIR/termux-android-smoke.bas" <<'BASIC_EOF'
print "TERMUX_FREEBASIC_ANDROID_SMOKE"
sleep 1000
BASIC_EOF

echo "==> Building arm64-v8a APK smoke program"
cd "$WORKDIR/out"
fbc-android \
	--target android-aarch64 \
	--target-api 35 \
	--package org.freebasic.termux.smoke \
	--label FreeBASIC-Termux-Smoke \
	"$WORKDIR/termux-android-smoke.bas" \
	-x termux-freebasic-smoke.apk

[ -f termux-freebasic-smoke.apk ] || die "fbc-android did not produce an APK"
unzip -l termux-freebasic-smoke.apk | grep -q 'lib/arm64-v8a/libfreebasicapp.so' || \
	die "APK is missing its arm64-v8a native library"

echo "TERMUX_FREEBASIC_ANDROID_BUILD_PASS"
PROVISION_EOF
chmod 700 "$PROVISION_SCRIPT"

PROOT_LOGIN_ARGS=(
	--bind "$TERMUX_ROOT:/mnt/freebasic-termux"
	--bind "$TERMUX_SDK_ROOT:/opt/android-sdk"
)

# PRoot-Distro constructs a controlled guest environment. Forward an
# explicitly configured download proxy so the same script also works on
# USB-attached devices that reach the network through an ADB reverse tunnel.
for PROXY_VARIABLE in \
	http_proxy https_proxy all_proxy \
	HTTP_PROXY HTTPS_PROXY ALL_PROXY
do
	PROXY_VALUE="${!PROXY_VARIABLE:-}"

	if [ -n "$PROXY_VALUE" ]; then
		PROOT_LOGIN_ARGS+=(--env "${PROXY_VARIABLE}=${PROXY_VALUE}")
	fi
done

echo "==> Preparing Ubuntu Android build environment"
proot-distro login \
	"${PROOT_LOGIN_ARGS[@]}" \
	"$CONTAINER_NAME" \
	-- /bin/bash /mnt/freebasic-termux/provision-ubuntu.sh "$PACKAGE_URL" "$SETUP_ONLY"

if [ "$SETUP_ONLY" -eq 0 ]; then
	APK_PATH="$TERMUX_ROOT/out/termux-freebasic-smoke.apk"
	[ -f "$APK_PATH" ] || die "Ubuntu reported success but the APK is not visible in Termux"
	echo "APK: $APK_PATH"
	echo "TERMUX_FREEBASIC_ANDROID_BUILD_PASS"
fi

# end of termux-ubuntu-android-bootstrap.sh
