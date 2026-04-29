#!/usr/bin/env bash

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

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

[ -n "$ROOT" ] || { echo "ERROR: could not locate FreeBASIC root"; exit 1; }
cd "$ROOT"

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }
msg() { echo ""; echo "==> $1"; }

run_root() {
    if [ "$(id -u)" -eq 0 ]; then
        run "$@"
    elif command -v sudo >/dev/null 2>&1; then
        run sudo "$@"
    else
        die "this step requires root privileges; rerun as root or install sudo"
    fi
}

usage() {
    cat <<EOF
Usage: ./build_scripts/debianubuntu-test-freebasic-android.sh [options]

Options:
  --package-dir DIR   Directory containing Debian package artifacts
  --image IMAGE       Docker image to use (default: ubuntu:questing)
  --docker-cmd CMD    Docker command to use (default: docker)
  --system-image PKG  Android SDK system image package for emulator tests
                      (default: system-images;android-35;google_apis;x86_64)
  --skip-host-deps    Skip Docker host dependency installation
  --help              Show this help text

The test first starts a fresh Debian/Ubuntu-style container, installs the local
freebasic-android .deb package with only its declared dependencies, and builds
console/gfxlib/sfxlib APKs with fbc-android. It then starts a separate emulator
harness container to install Android test tooling and validate those APKs.
EOF
}

PACKAGE_DIR=""
IMAGE="${IMAGE:-ubuntu:questing}"
DOCKER_CMD="${DOCKER_CMD:-docker}"
SYSTEM_IMAGE_PACKAGE="${ANDROID_SYSTEM_IMAGE_PACKAGE:-system-images;android-35;google_apis;x86_64}"
SKIP_HOST_DEPS=0

while [ $# -gt 0 ]; do
    case "$1" in
        --package-dir) PACKAGE_DIR="$2"; shift 2 ;;
        --image) IMAGE="$2"; shift 2 ;;
        --docker-cmd) DOCKER_CMD="$2"; shift 2 ;;
        --system-image) SYSTEM_IMAGE_PACKAGE="$2"; shift 2 ;;
        --skip-host-deps) SKIP_HOST_DEPS=1; shift ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            die "unknown option: $1"
            ;;
    esac
done

if [ -z "$PACKAGE_DIR" ]; then
    ARCH="$(dpkg --print-architecture 2>/dev/null || true)"
    DISTRO_ID="unknown"
    CODENAME="unknown"
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        DISTRO_ID="${ID:-unknown}"
        CODENAME="${VERSION_CODENAME:-unknown}"
    fi
    PACKAGE_DIR="$ROOT/out/linux/$DISTRO_ID/$CODENAME/$ARCH"
fi

[ -d "$PACKAGE_DIR" ] || die "package directory not found: $PACKAGE_DIR"
PACKAGE_DIR="$(cd "$PACKAGE_DIR" && pwd -P)"
ls "$PACKAGE_DIR"/freebasic-android_*.deb >/dev/null 2>&1 || die "missing freebasic-android .deb in $PACKAGE_DIR"

if [ ! -e /dev/kvm ]; then
    die "Android emulator smoke test requires host /dev/kvm. Enable KVM virtualization and rerun."
fi

install_host_deps() {
    [ "$SKIP_HOST_DEPS" -eq 0 ] || return 0

    if command -v "${DOCKER_CMD%% *}" >/dev/null 2>&1; then
        return 0
    fi

    if command -v apt-get >/dev/null 2>&1; then
        msg "installing Docker host dependency via apt"
        run_root apt-get update -y
        run_root apt-get install -y --no-install-recommends docker.io ca-certificates
        return 0
    fi

    die "Docker is required; install it or rerun with --skip-host-deps after installing Docker"
}

BUILD_RUNNER="$(mktemp -t fb-android-deb-package-build.XXXXXX.sh)"
EMULATOR_RUNNER="$(mktemp -t fb-android-deb-package-emulator.XXXXXX.sh)"
APK_OUTDIR="$(mktemp -d -t fb-android-apks.XXXXXX)"
cleanup() {
    rm -f "$BUILD_RUNNER" "$EMULATOR_RUNNER"
    rm -rf "$APK_OUTDIR"
}
trap cleanup EXIT

chmod 755 "$BUILD_RUNNER" "$EMULATOR_RUNNER"
cat > "$BUILD_RUNNER" <<'BUILD_RUNNER_EOF'
#!/usr/bin/env bash

set -euo pipefail

run() {
    echo "==> $*"
    "$@"
}

fail() {
    echo "ERROR: $*" >&2
    exit 1
}

export DEBIAN_FRONTEND=noninteractive
export ANDROID_HOME=/usr/lib/android-sdk
export ANDROID_SDK_ROOT=/usr/lib/android-sdk
export HOME=/root

run apt-get update -y
run apt-get install -y --no-install-recommends /packages/freebasic-android_*.deb

command -v fbc-android >/dev/null 2>&1 || fail "fbc-android was not installed"
command -v aapt >/dev/null 2>&1 || fail "aapt dependency was not installed"
command -v apksigner >/dev/null 2>&1 || fail "apksigner dependency was not installed"

mkdir -p /tmp/fb-android-smoke
cat > /tmp/fb-android-smoke/console.bas <<'EOF'
print "FREEBASIC_ANDROID_CONSOLE_SMOKE"
sleep 1000
EOF

cat > /tmp/fb-android-smoke/gfx.bas <<'EOF'
screenres 160, 120, 32
line (0, 0)-(159, 119), rgb(0, 128, 255), bf
line (10, 10)-(149, 109), rgb(255, 255, 255), b
print "FREEBASIC_ANDROID_GFX_SMOKE"
sleep 1000
EOF

cat > /tmp/fb-android-smoke/sfx.bas <<'EOF'
sound 440, 0.25
print "FREEBASIC_ANDROID_SFX_SMOKE"
sleep 1000
EOF

cd /tmp/fb-android-smoke
run fbc-android --target-api 24 --package org.freebasic.smoke.console --label FBConsole console.bas
run fbc-android --target-api 24 --package org.freebasic.smoke.gfx --label FBGfx gfx.bas
run fbc-android --target-api 24 --package org.freebasic.smoke.sfx --label FBSfx sfx.bas

[ -f console.apk ] || fail "console.apk was not produced"
[ -f gfx.apk ] || fail "gfx.apk was not produced"
[ -f sfx.apk ] || fail "sfx.apk was not produced"

cp -av console.apk gfx.apk sfx.apk /apk-out/

echo "freebasic-android package APK build test passed"
BUILD_RUNNER_EOF

cat > "$EMULATOR_RUNNER" <<'EMULATOR_RUNNER_EOF'
#!/usr/bin/env bash

set -euo pipefail

run() {
    echo "==> $*"
    "$@"
}

fail() {
    echo "ERROR: $*" >&2
    exit 1
}

find_tool() {
    local name="$1"
    local found

    if command -v "$name" >/dev/null 2>&1; then
        command -v "$name"
        return 0
    fi

    for found in /usr/lib/android-sdk/emulator/"$name" /usr/lib/android-sdk/platform-tools/"$name" /usr/lib/android-sdk/cmdline-tools/latest/bin/"$name" /usr/lib/android-sdk/cmdline-tools/*/bin/"$name" /usr/lib/android-sdk/tools/bin/"$name"; do
        [ -x "$found" ] || continue
        echo "$found"
        return 0
    done

    return 1
}

find_system_image_package() {
    local image
    image="$(find /usr/lib/android-sdk "$HOME/Android/Sdk" -path '*/system-images/android-*/*/*/system.img' -print 2>/dev/null | head -n1 || true)"
    [ -n "$image" ] || return 1

    image="${image%/system.img}"
    local abi="${image##*/}"
    image="${image%/*}"
    local flavor="${image##*/}"
    image="${image%/*}"
    local api="${image##*/}"

    echo "system-images;$api;$flavor;$abi"
}

ensure_system_image() {
    local system_image_package="$1"
    local sdkmanager

    find_system_image_package >/dev/null 2>&1 && return 0

    sdkmanager="$(find_tool sdkmanager || true)"
    [ -n "$sdkmanager" ] || fail "sdkmanager is missing from test harness dependencies"

    printf 'y\n%.0s' {1..1000} | "$sdkmanager" --licenses >/dev/null || true
    echo "==> $sdkmanager $system_image_package"
    printf 'y\n%.0s' {1..1000} | "$sdkmanager" "$system_image_package"
}

wait_for_boot() {
    local adb="$1"
    local emulator_pid="$2"
    local deadline
    local booted
    local state
    local next_notice
    deadline=$((SECONDS + 180))
    next_notice=$SECONDS

    while [ "$SECONDS" -lt "$deadline" ]; do
        if ! kill -0 "$emulator_pid" >/dev/null 2>&1; then
            echo "ERROR: Android emulator process exited before ADB became ready" >&2
            return 1
        fi

        state="$("$adb" get-state 2>/dev/null || true)"
        if [ "$state" != "device" ]; then
            if [ "$SECONDS" -ge "$next_notice" ]; then
                echo "==> waiting for Android emulator ADB device..."
                "$adb" devices || true
                next_notice=$((SECONDS + 15))
            fi
            sleep 2
            continue
        fi

        booted="$("$adb" shell getprop sys.boot_completed 2>/dev/null | tr -d '\r' || true)"
        if [ "$booted" = "1" ]; then
            return 0
        fi
        if [ "$SECONDS" -ge "$next_notice" ]; then
            echo "==> waiting for Android emulator boot_completed..."
            next_notice=$((SECONDS + 15))
        fi
        sleep 2
    done

    return 1
}

run_apk_smoke() {
    local adb="$1"
    local apk="$2"
    local package_name="$3"
    local marker="$4"
    local log_file="$5"
    local deadline
    local exit_marker="FREEBASIC_ANDROID_EXIT:0"

    run "$adb" install -r "$apk"
    run "$adb" shell am force-stop "$package_name" >/dev/null 2>&1 || true
    run "$adb" logcat -c
    if ! run "$adb" shell am start -W \
        -a android.intent.action.MAIN \
        -c android.intent.category.LAUNCHER \
        -n "$package_name/android.app.NativeActivity"; then
        echo "==> explicit activity launch failed; falling back to monkey"
        run "$adb" shell monkey -p "$package_name" -c android.intent.category.LAUNCHER 1
    fi

    deadline=$((SECONDS + 60))
    while [ "$SECONDS" -lt "$deadline" ]; do
        "$adb" logcat -d > "$log_file"
        if grep -q "$marker" "$log_file" && grep -q "$exit_marker" "$log_file"; then
            return 0
        fi
        sleep 2
    done

    "$adb" logcat -d > "$log_file"
    echo "ERROR: app smoke logcat tail for $package_name:" >&2
    tail -n 200 "$log_file" >&2 || true
    echo "ERROR: app activity state for $package_name:" >&2
    "$adb" shell pidof "$package_name" >&2 || true
    "$adb" shell dumpsys activity activities | grep -F "$package_name" >&2 || true
    grep -q "$marker" "$log_file" || fail "missing app marker '$marker' in logcat for $package_name"
    grep -q "$exit_marker" "$log_file" || fail "missing clean exit marker in logcat for $package_name"
}

export DEBIAN_FRONTEND=noninteractive
export ANDROID_HOME=/usr/lib/android-sdk
export ANDROID_SDK_ROOT=/usr/lib/android-sdk
export ANDROID_SYSTEM_IMAGE_PACKAGE="${ANDROID_SYSTEM_IMAGE_PACKAGE:-system-images;android-35;google_apis;x86_64}"
export HOME=/root

for apk in /apk/console.apk /apk/gfx.apk /apk/sfx.apk; do
    [ -f "$apk" ] || fail "missing APK from build phase: $apk"
done

run apt-get update -y
run apt-get install -y --no-install-recommends \
    adb \
    google-android-emulator-installer \
    google-android-cmdline-tools-19.0-installer \
    libx11-6 \
    libxcb1

ensure_system_image "$ANDROID_SYSTEM_IMAGE_PACKAGE"

emulator="$(find_tool emulator)" || fail "Android emulator binary is missing from test harness dependencies"
adb="$(find_tool adb)" || fail "adb binary is missing from test harness dependencies"
avdmanager="$(find_tool avdmanager || true)"
[ -n "$avdmanager" ] || fail "avdmanager is missing from test harness dependencies"

system_image="$(find_system_image_package || true)"
[ -n "$system_image" ] || fail "no Android emulator system image is installed under /usr/lib/android-sdk; install a system image package or preseed one with sdkmanager"

echo no | "$avdmanager" create avd --force -n fbsmoke -k "$system_image"

"$emulator" -avd fbsmoke -no-window -no-audio -no-snapshot -wipe-data -no-boot-anim -gpu swiftshader_indirect > emulator.log 2>&1 &
emulator_pid=$!
trap 'kill "$emulator_pid" >/dev/null 2>&1 || true' EXIT

wait_for_boot "$adb" "$emulator_pid" || { cat emulator.log >&2; fail "Android emulator did not boot within 180 seconds"; }

run_apk_smoke "$adb" /apk/console.apk org.freebasic.smoke.console FREEBASIC_ANDROID_CONSOLE_SMOKE console.log
run_apk_smoke "$adb" /apk/gfx.apk org.freebasic.smoke.gfx FREEBASIC_ANDROID_GFX_SMOKE gfx.log
run_apk_smoke "$adb" /apk/sfx.apk org.freebasic.smoke.sfx FREEBASIC_ANDROID_SFX_SMOKE sfx.log

echo "freebasic-android package smoke test passed"
EMULATOR_RUNNER_EOF

install_host_deps

msg "building Android smoke APKs from freebasic-android package in $IMAGE"
run ${DOCKER_CMD} run --rm \
    -v "$PACKAGE_DIR:/packages:ro" \
    -v "$APK_OUTDIR:/apk-out" \
    -v "$BUILD_RUNNER:/tmp/build-freebasic-android-apks.sh:ro" \
    "$IMAGE" \
    /bin/bash /tmp/build-freebasic-android-apks.sh

msg "testing Android smoke APKs in emulator harness container"
run ${DOCKER_CMD} run --rm \
    --device /dev/kvm \
    -e ANDROID_SYSTEM_IMAGE_PACKAGE="$SYSTEM_IMAGE_PACKAGE" \
    -v "$APK_OUTDIR:/apk:ro" \
    -v "$EMULATOR_RUNNER:/tmp/test-freebasic-android-apks.sh:ro" \
    "$IMAGE" \
    /bin/bash /tmp/test-freebasic-android-apks.sh
