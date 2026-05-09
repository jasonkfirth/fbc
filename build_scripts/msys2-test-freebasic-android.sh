#!/usr/bin/env bash

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# msys2-test-freebasic-android.sh
#
# Build the fbcunit test suite as an Android NativeActivity APK, install it on
# an Android emulator, and report the fbcunit pass/fail result from logcat.
#
# This runner intentionally uses the packaged fbc-android distribution.  It
# verifies the same path that an installed user will exercise, including the
# package-local Android SDK setup script when the SDK/NDK has not been installed
# in the distribution yet.
##############################################################################

SELF_DIR="$(CDPATH= cd -- "$(dirname "$0")" && pwd)"
ROOT="$(CDPATH= cd -- "$SELF_DIR/.." && pwd)"

cd "$ROOT"

if [ ! -d "$ROOT/tests" ] || [ ! -f "$ROOT/GNUmakefile" ]; then
	echo ""
	echo "ERROR: could not locate the FreeBASIC project root."
	exit 1
fi

case "$(uname -s)" in
	MINGW*|MSYS*) ;;
	*)
		echo ""
		echo "ERROR: this script must be run inside an MSYS2 environment."
		exit 1
		;;
esac

##############################################################################
# Helpers
##############################################################################

msg() {
	echo ""
	echo "==> $1"
}

fail() {
	echo ""
	echo "ERROR: $1" >&2
	exit 1
}

have() {
	command -v "$1" >/dev/null 2>&1
}

run() {
	echo "==> $*"
	"$@"
}

extract_var() {
	local name="$1"
	awk -F ':=' -v name="$name" '
		$1 ~ "^[[:space:]]*" name "[[:space:]]*$" {
			gsub(/^[[:space:]]+|[[:space:]]+$/, "", $2)
			print $2
			exit
		}
	' "$ROOT/mk/version.mk"
}

max_jobs() {
	local n
	n="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)"
	case "$n" in
		''|*[!0-9]*) n=2 ;;
	esac
	echo "$n"
}

read_fb_dirlist() {
	awk '
		$1 == "DIRLIST_FB" {
			in_list = 1
			sub(/^DIRLIST_FB[[:space:]]*:=[[:space:]]*/, "")
		}
		in_list {
			line = $0
			sub(/#.*/, "", line)
			gsub(/\\/, "", line)
			gsub(/^[[:space:]]+|[[:space:]]+$/, "", line)
			if (line != "")
				print line
			if ($0 !~ /\\[[:space:]]*$/)
				in_list = 0
		}
	' "$ROOT/tests/dirlist.mk"
}

find_android_tool() {
	local name="$1"
	local found

	if have "$name"; then
		command -v "$name"
		return 0
	fi

	shopt -s nullglob
	for found in \
		"$SDKROOT"/build-tools/*/"$name" \
		"$SDKROOT"/build-tools/*/"$name.exe" \
		"$SDKROOT"/build-tools/*/"$name.bat" \
		"$SDKROOT"/cmdline-tools/latest/bin/"$name" \
		"$SDKROOT"/cmdline-tools/latest/bin/"$name.bat" \
		"$SDKROOT"/emulator/"$name" \
		"$SDKROOT"/emulator/"$name.exe" \
		"$SDKROOT"/platform-tools/"$name" \
		"$SDKROOT"/platform-tools/"$name.exe"
	do
		[ -f "$found" ] || continue
		echo "$found"
		shopt -u nullglob
		return 0
	done
	shopt -u nullglob

	return 1
}

find_ndk_root() {
	local candidate

	shopt -s nullglob
	for candidate in "$SDKROOT"/ndk/* "$SDKROOT"/ndk-bundle; do
		[ -d "$candidate/toolchains/llvm/prebuilt" ] || continue
		echo "$candidate"
		shopt -u nullglob
		return 0
	done
	shopt -u nullglob
	return 1
}

find_ndk_prebuilt() {
	local ndk="$1"
	local candidate

	shopt -s nullglob
	for candidate in "$ndk"/toolchains/llvm/prebuilt/windows-x86_64 "$ndk"/toolchains/llvm/prebuilt/*; do
		[ -d "$candidate/bin" ] || continue
		echo "$candidate"
		shopt -u nullglob
		return 0
	done
	shopt -u nullglob
	return 1
}

find_platform_jar() {
	local selected=""
	local platform

	shopt -s nullglob
	for platform in "$SDKROOT"/platforms/android-*; do
		[ -f "$platform/android.jar" ] || continue
		selected="$platform/android.jar"
	done
	shopt -u nullglob

	[ -n "$selected" ] || return 1
	echo "$selected"
}

ensure_packaged_sdk() {
	local setup_cmd
	local setup_win

	if [ -f "$SDKROOT/cmdline-tools/latest/bin/sdkmanager.bat" ] &&
		{ [ -d "$SDKROOT/ndk" ] || [ -d "$SDKROOT/ndk-bundle" ]; }; then
		return 0
	fi

	setup_cmd="$DISTROOT/setup-android-sdk.cmd"
	[ -f "$setup_cmd" ] || fail "packaged Android SDK not found and setup script is missing: $setup_cmd"
	have cygpath || fail "cygpath not found"

	msg "Installing Android SDK/NDK into the packaged distribution"
	setup_win="$(cygpath -aw "$setup_cmd")"
	if [ "${ACCEPT_GOOGLE_ANDROID_SDK_TERMS:-0}" = "1" ]; then
		run cmd.exe //C "\"$setup_win\" --accept-google-android-sdk-terms"
	else
		run cmd.exe //C "\"$setup_win\""
	fi
}

ensure_emulator() {
	local avdmanager
	local emulator
	local adb
	local sdkmanager
	local booted
	local tries

	adb="$(find_android_tool adb)" || fail "adb was not found in the packaged Android SDK"

	if "$adb" get-state >/dev/null 2>&1; then
		echo "$adb"
		return 0
	fi

	emulator="$(find_android_tool emulator || true)"
	avdmanager="$(find_android_tool avdmanager || true)"
	if [ -z "$emulator" ] || [ -z "$avdmanager" ]; then
		sdkmanager="$(find_android_tool sdkmanager)" ||
			fail "sdkmanager was not found; cannot install Android emulator tools"
		msg "Installing Android emulator tools into the packaged SDK" >&2
		printf 'y\n%.0s' {1..1000} | "$sdkmanager" --sdk_root="$SDKROOT" --licenses >/dev/null || true
		run "$sdkmanager" --sdk_root="$SDKROOT" emulator "$ANDROID_SYSTEM_IMAGE_PACKAGE" >&2
		emulator="$(find_android_tool emulator)" || fail "Android emulator was not installed"
		avdmanager="$(find_android_tool avdmanager)" || fail "avdmanager was not installed"
	fi

	if ! "$emulator" -list-avds | grep -qx "$AVD_NAME"; then
		msg "Creating Android emulator AVD: $AVD_NAME" >&2
		printf 'no\n' | "$avdmanager" create avd \
			-n "$AVD_NAME" \
			-k "$ANDROID_SYSTEM_IMAGE_PACKAGE" \
			--device "pixel_5" >/dev/null
	fi

	msg "Starting Android emulator: $AVD_NAME" >&2
	"$emulator" -avd "$AVD_NAME" -no-snapshot -no-window -no-boot-anim >/tmp/fbc-android-emulator.log 2>&1 &

	tries=180
	while [ "$tries" -gt 0 ]; do
		if "$adb" wait-for-device >/dev/null 2>&1; then
			booted="$("$adb" shell getprop sys.boot_completed 2>/dev/null | tr -d '\r' || true)"
			if [ "$booted" = "1" ]; then
				echo "$adb"
				return 0
			fi
		fi
		tries=$((tries - 1))
		sleep 2
	done

	fail "Android emulator did not finish booting"
}

copy_asset_dir() {
	local src="$1"
	local dst="$2"

	mkdir -p "$dst"
	if have rsync; then
		rsync -a --delete \
			--exclude '*.o' \
			--exclude '*.obj' \
			--exclude '*.exe' \
			--exclude '*.log' \
			--exclude '*.tmp' \
			"$src/" "$dst/"
	else
		cp -a "$src"/. "$dst/"
	fi
}

logcat_has_passing_fbcunit_summary() {
	awk '
		$0 ~ /[[:space:]]Total[[:space:]]+[0-9]+[[:space:]]*$/ {
			for (i = 1; i <= NF; i++) {
				if ($i == "Total" && i >= 4 && $(i - 1) == "0") {
					found = 1
				}
			}
		}
		END {
			exit(found ? 0 : 1)
		}
	' "$LOGCAT"
}

compile_fbcunit() {
	local src
	local obj
	local objs=()

	msg "Building fbcunit for Android"
	mkdir -p "$FBCU_OBJROOT"
	for src in \
		"$ROOT/tests/fbcunit/src/fbcunit.bas" \
		"$ROOT/tests/fbcunit/src/fbcunit_qb.bas" \
		"$ROOT/tests/fbcunit/src/fbcunit_console.bas" \
		"$ROOT/tests/fbcunit/src/fbcunit_report.bas"
	do
		obj="$FBCU_OBJROOT/$(basename "${src%.bas}").o"
		run bash "$FBC_ANDROID" --emit-obj --no-entry -mt \
			-i "$FBCU_INC" \
			-m fbc-tests \
			-x "$obj" \
			"$src"
		objs+=("$obj")
	done

	run "$AR" rcs "$FBCU_LIB" "${objs[@]}"
	run "$RANLIB" "$FBCU_LIB"
}

write_android_main() {
	cat > "$ANDROID_MAIN" <<'EOF'
#include once "fbcunit.bi"

function fb_android_program_main cdecl alias "fb_android_program_main" _
	( _
		byval argc as integer, _
		byval argv as zstring ptr ptr _
	) as integer

	dim passed as boolean = false

	print "FREEBASIC_ANDROID_TESTS_START"

	if fbcu.check_internal_state() = false then
		print "FREEBASIC_ANDROID_TESTS_FAIL:fbcu.check_internal_state"
		return 1
	end if

	fbcu.setBriefSummary( true )
	fbcu.setHideCases( false )
	fbcu.setShowConsole( false )

	passed = fbcu.run_tests( true, false )

	if passed then
		print "FREEBASIC_ANDROID_TESTS_PASS"
		return 0
	else
		print "FREEBASIC_ANDROID_TESTS_FAIL"
		return 1
	end if

end function
EOF
}

generate_source_list() {
	msg "Generating Android fbcunit source list"
	if [ "${#CUSTOM_SRCS[@]}" -gt 0 ]; then
		printf '%s\n' "${CUSTOM_SRCS[@]}" | sed -e 's#^tests/##' -e 's#\.bmk$#.bas#' | sort -u > "$SOURCE_LIST"
	else
		(
			cd "$ROOT/tests"
			find "${TEST_DIRS[@]}" -type f \( -name '*.bas' -o -name '*.bmk' \) -print0 \
				| xargs -0 grep -l -i -E \
					'(^[[:space:]]*#[[:space:]]*include[[:space:]]+(once[[:space:]]+)?["]fbcu(nit)?\.bi["])|([[:space:]]*.[[:space:]]*TEST_MODE[[:space:]]*:[[:space:]]*FBCUNIT_COMPATIBLE)' \
				| sed -e 's#^\./##' -e 's#\.bmk$#.bas#' \
				| sort -u
		) > "$SOURCE_LIST"
	fi

	[ -s "$SOURCE_LIST" ] || fail "no fbcunit tests were found"
	wc -l "$SOURCE_LIST" | awk '{ print "fbcunit sources: " $1 }'
}

write_object_map() {
	local src
	local obj

	: > "$OBJECT_MAP"
	while IFS= read -r src; do
		obj="$TEST_OBJROOT/${src//\//__}"
		obj="${obj%.bas}.o"
		printf '%s\t%s\n' "$src" "$obj" >> "$OBJECT_MAP"
	done < "$SOURCE_LIST"
}

compile_test_objects() {
	msg "Compiling fbcunit tests for Android"
	mkdir -p "$TEST_OBJROOT" "$COMPILE_LOGROOT"

	export FBC_ANDROID FBCU_INC ROOT COMPILE_LOGROOT
	xargs -P "$JOBS" -n 2 bash -c '
		set -euo pipefail
		src="$1"
		obj="$2"
		log="$COMPILE_LOGROOT/${src//\//__}.log"
		mkdir -p "$(dirname "$obj")"
		bash "$FBC_ANDROID" --emit-obj --no-entry -mt \
			-i "$FBCU_INC" \
			-m fbc-tests \
			-w 3 \
			-Wc -Wno-tautological-compare \
			-x "$obj" \
			"$ROOT/tests/$src" >"$log" 2>&1
	' _ < "$OBJECT_MAP"

	cut -f2 "$OBJECT_MAP" > "$OBJECT_LIST"
}

compile_main_object() {
	msg "Compiling Android test-suite main"
	write_android_main
	run bash "$FBC_ANDROID" --emit-obj --no-entry -mt \
		-i "$FBCU_INC" \
		-m fbc-tests \
		-x "$MAIN_OBJ" \
		"$ANDROID_MAIN"
}

copy_test_assets() {
	local dir

	msg "Staging test data as Android APK assets"
	rm -rf "$ASSETROOT"
	mkdir -p "$ASSETROOT"
	for dir in "${TEST_DIRS[@]}"; do
		[ -d "$ROOT/tests/$dir" ] || fail "test directory not found: tests/$dir"
		copy_asset_dir "$ROOT/tests/$dir" "$ASSETROOT/$dir"
	done

	(
		cd "$ASSETROOT"
		find . -type f -print \
			| sed -e 's#^\./##' \
			| grep -v '^_freebasic_asset_manifest\.txt$' \
			| sort > "_freebasic_asset_manifest.txt"
	)
}

link_shared_library() {
	local runtime=()
	local appobj="$BUILDROOT_TEST/fb_android_app.o"
	local rsp="$BUILDROOT_TEST/android-test-objects.rsp"
	local required
	local optional

	msg "Linking Android test-suite shared library"
	run "$CC" -fPIC -DANDROID -I"$INCDIR" -c "$ROOT/src/tools/android/fb_android_app.c" -o "$appobj"

	for required in fbrt0pic.o libfbmtpic.a; do
		[ -f "$LIBDIR/$required" ] || fail "required Android runtime file is missing: $LIBDIR/$required"
		runtime+=("$LIBDIR/$required")
	done
	for optional in libfbrtmtpic.a libfbgfxmtpic.a libsfxmtpic.a; do
		if [ -f "$LIBDIR/$optional" ]; then
			runtime+=("$LIBDIR/$optional")
		fi
	done

	{
		while IFS= read -r required; do
			printf '"%s"\n' "$(cygpath -am "$required")"
		done < "$OBJECT_LIST"
		printf '"%s"\n' "$(cygpath -am "$MAIN_OBJ")"
	} > "$rsp"

	run "$CC" -shared -Wl,-soname,libfreebasicapp.so -o "$SOFILE" \
		"$appobj" @"$rsp" \
		-Wl,--start-group "$FBCU_LIB" "${runtime[@]}" -Wl,--end-group \
		-Wl,--wrap=exit \
		-llog -landroid -lOpenSLES -ldl -lm
}

package_apk() {
	local platform_jar
	local target_sdk
	local unsigned="$BUILDROOT_TEST/freebasic-tests-unsigned.apk"
	local unaligned="$BUILDROOT_TEST/freebasic-tests-unaligned.apk"
	local aligned="$BUILDROOT_TEST/freebasic-tests-aligned.apk"
	local manifest="$BUILDROOT_TEST/AndroidManifest.xml"
	local resroot="$BUILDROOT_TEST/res"
	local apkroot="$BUILDROOT_TEST/apk"

	msg "Packaging Android test-suite APK"
	platform_jar="$(find_platform_jar)" || fail "no Android platform android.jar found under $SDKROOT/platforms"
	target_sdk="${platform_jar%/android.jar}"
	target_sdk="${target_sdk##*/android-}"

	rm -rf "$resroot" "$apkroot"
	mkdir -p "$resroot/values" "$apkroot/lib/$ABI"
	cp "$ROOT/src/tools/android/strings.xml" "$resroot/values/strings.xml"
	cp "$SOFILE" "$apkroot/lib/$ABI/libfreebasicapp.so"
	sed \
		-e "s|@PACKAGE@|$PACKAGE_NAME|g" \
		-e "s|@MIN_SDK@|$MIN_API|g" \
		-e "s|@TARGET_SDK@|$target_sdk|g" \
		-e "s|@LABEL@|$APP_LABEL|g" \
		"$ROOT/src/tools/android/AndroidManifest.xml.in" > "$manifest"

	run "$AAPT" package -f -M "$manifest" -S "$resroot" -A "$ASSETROOT" -I "$platform_jar" -F "$unsigned"
	run "$JAR" uf "$unsigned" -C "$apkroot" lib
	cp "$unsigned" "$unaligned"

	if [ -n "$ZIPALIGN" ]; then
		run "$ZIPALIGN" -f 4 "$unaligned" "$aligned"
	else
		cp "$unaligned" "$aligned"
	fi

	mkdir -p "${KEYSTORE%/*}"
	if [ ! -f "$KEYSTORE" ]; then
		run "$KEYTOOL" -genkeypair \
			-keystore "$KEYSTORE" \
			-storepass android \
			-keypass android \
			-alias androiddebugkey \
			-keyalg RSA \
			-keysize 2048 \
			-validity 10000 \
			-dname "CN=Android Debug,O=Android,C=US"
	fi

	run "$APKSIGNER" sign \
		--ks "$KEYSTORE" \
		--ks-pass pass:android \
		--key-pass pass:android \
		--out "$APK" \
		"$aligned"
}

run_apk() {
	local adb="$1"
	local logpid
	local status=1
	local deadline
	local exit_deadline=0
	local exit_line

	msg "Installing and running Android test-suite APK"
	run "$adb" install -r "$APK"
	"$adb" logcat -c || true
	"$adb" shell am force-stop "$PACKAGE_NAME" >/dev/null 2>&1 || true
	"$adb" logcat -v time FreeBASIC:I '*:S' > "$LOGCAT" 2>&1 &
	logpid=$!
	trap 'kill "$logpid" >/dev/null 2>&1 || true' EXIT

	run "$adb" shell am start -n "$PACKAGE_NAME/android.app.NativeActivity"

	deadline=$((SECONDS + RUN_TIMEOUT))
	while [ "$SECONDS" -lt "$deadline" ]; do
		if grep -q 'FREEBASIC_ANDROID_TESTS_PASS' "$LOGCAT"; then
			status=0
			break
		fi
		if grep -q 'FREEBASIC_ANDROID_TESTS_FAIL' "$LOGCAT"; then
			status=1
			break
		fi
		exit_line="$(grep 'FREEBASIC_ANDROID_EXIT:' "$LOGCAT" | tail -n 1 || true)"
		if [ -n "$exit_line" ]; then
			if grep -q 'FREEBASIC_ANDROID_EXIT:0' <<<"$exit_line"; then
				if logcat_has_passing_fbcunit_summary; then
					status=0
					break
				fi
			else
				if [ "$exit_deadline" -eq 0 ]; then
					exit_deadline=$((SECONDS + 10))
				elif [ "$SECONDS" -ge "$exit_deadline" ]; then
					status=1
					break
				fi
			fi
			if [ "$exit_deadline" -eq 0 ]; then
				exit_deadline=$((SECONDS + 10))
			elif [ "$SECONDS" -ge "$exit_deadline" ]; then
				status=1
				break
			fi
		fi
		sleep 2
	done

	kill "$logpid" >/dev/null 2>&1 || true
	trap - EXIT

	if [ "$status" -eq 0 ]; then
		msg "Android fbcunit suite passed"
		return 0
	fi

	echo ""
	echo "Android fbcunit suite did not pass. Logcat tail:"
	tail -n 80 "$LOGCAT" || true
	return 1
}

##############################################################################
# Options
##############################################################################

FBVERSION="$(extract_var FBVERSION)"
[ -n "$FBVERSION" ] || fail "could not determine FBVERSION"

BUILDROOT="${BUILDROOT:-/tmp/freebasic-android-build}"
DISTROOT="${DISTROOT:-$BUILDROOT/dist/FreeBASIC-${FBVERSION}-fbc-android}"
BUILDROOT_TEST="${ANDROID_TEST_BUILDROOT:-$BUILDROOT/android-tests}"
OUT="${OUT:-$ROOT/out/mingw32-android}"
PACKAGE_NAME="${PACKAGE_NAME:-org.freebasic.testsuite}"
APP_LABEL="${APP_LABEL:-FreeBASIC Tests}"
AVD_NAME="${AVD_NAME:-fbandroid35}"
ANDROID_SYSTEM_IMAGE_PACKAGE="${ANDROID_SYSTEM_IMAGE_PACKAGE:-system-images;android-35;google_apis;x86_64}"
ANDROID_API="${ANDROID_API:-26}"
MIN_API="${ANDROID_MIN_API:-21}"
JOBS="${JOBS:-$(max_jobs)}"
RUN_TIMEOUT="${RUN_TIMEOUT:-900}"
NO_RUN=0
KEEP_BUILDROOT=0
CUSTOM_DIRS=()
CUSTOM_SRCS=()

usage() {
	cat <<EOF
Usage: ./build_scripts/msys2-test-freebasic-android.sh [options]

Options:
  --distroot DIR      Packaged fbc-android distribution root
  --buildroot DIR     Test build directory
  --dir NAME          Limit the run to one tests/ subdirectory. Repeatable.
  --source FILE       Limit the run to one tests/ source file. Repeatable.
  --jobs N            Parallel compile jobs (default: detected CPU count)
  --timeout SECONDS   Emulator test timeout (default: 900)
  --package NAME      Android package name (default: org.freebasic.testsuite)
  --avd NAME          Android virtual device name (default: fbandroid35)
  --no-run            Build the APK but do not install/run it
  --keep-buildroot    Keep intermediate test build files
  --help              Show this help text

Environment:
  BUILDROOT                 Android package build root
  DISTROOT                  Existing fbc-android distribution root
  ANDROID_TEST_BUILDROOT    Intermediate test build root
  OUT                       Output directory for the test APK
  PACKAGE_NAME              Android package name
  AVD_NAME                  Emulator AVD name
  ANDROID_SYSTEM_IMAGE_PACKAGE
                            SDK system image used when creating the AVD
  ACCEPT_GOOGLE_ANDROID_SDK_TERMS=1
                            Allow noninteractive SDK setup through the
                            package-local setup-android-sdk.cmd script
EOF
}

while [ $# -gt 0 ]; do
	case "$1" in
		--distroot) DISTROOT="$2"; shift 2 ;;
		--buildroot) BUILDROOT_TEST="$2"; shift 2 ;;
		--dir) CUSTOM_DIRS+=("$2"); shift 2 ;;
		--source) CUSTOM_SRCS+=("$2"); shift 2 ;;
		--jobs) JOBS="$2"; shift 2 ;;
		--timeout) RUN_TIMEOUT="$2"; shift 2 ;;
		--package) PACKAGE_NAME="$2"; shift 2 ;;
		--avd) AVD_NAME="$2"; shift 2 ;;
		--no-run) NO_RUN=1; shift ;;
		--keep-buildroot) KEEP_BUILDROOT=1; shift ;;
		--help)
			usage
			exit 0
			;;
		*)
			echo "ERROR: unknown option: $1" >&2
			usage >&2
			exit 1
			;;
	esac
done

if [ "${#CUSTOM_SRCS[@]}" -gt 0 ]; then
	TEST_DIRS=()
	for src in "${CUSTOM_SRCS[@]}"; do
		src="${src#tests/}"
		TEST_DIRS+=("${src%%/*}")
	done
elif [ "${#CUSTOM_DIRS[@]}" -gt 0 ]; then
	TEST_DIRS=("${CUSTOM_DIRS[@]}")
else
	mapfile -t TEST_DIRS < <(read_fb_dirlist)
fi

##############################################################################
# Configuration
##############################################################################

SDKROOT="$DISTROOT/toolchain/android-sdk"
FBC_ANDROID="${FBC_ANDROID:-$ROOT/src/tools/android/fbc-android}"
INCDIR="$DISTROOT/include/freebasic-android"
LIBDIR="$DISTROOT/lib/freebasic-android/android-aarch64"
FBCU_INC="$ROOT/tests/fbcunit/inc"
FBCU_OBJROOT="$BUILDROOT_TEST/fbcunit"
FBCU_LIB="$BUILDROOT_TEST/libfbcunit.a"
TEST_OBJROOT="$BUILDROOT_TEST/objects"
COMPILE_LOGROOT="$BUILDROOT_TEST/compile-logs"
SOURCE_LIST="$BUILDROOT_TEST/android-unit-tests-src.lst"
OBJECT_MAP="$BUILDROOT_TEST/android-unit-tests-objects.tsv"
OBJECT_LIST="$BUILDROOT_TEST/android-unit-tests-objects.lst"
ANDROID_MAIN="$BUILDROOT_TEST/fbc-tests-android.bas"
MAIN_OBJ="$BUILDROOT_TEST/fbc-tests-android.o"
ASSETROOT="$BUILDROOT_TEST/assets"
SOFILE="$BUILDROOT_TEST/libfreebasicapp.so"
APK="$OUT/FreeBASIC-${FBVERSION}-android-fbcunit-tests.apk"
LOGCAT="$BUILDROOT_TEST/android-fbcunit.log"
ABI="arm64-v8a"
KEYSTORE="${ANDROID_DEBUG_KEYSTORE:-$HOME/.android/debug.keystore}"

[ -f "$FBC_ANDROID" ] || fail "fbc-android driver not found: $FBC_ANDROID"
ensure_packaged_sdk
[ -d "$SDKROOT" ] || fail "packaged Android SDK not found: $SDKROOT"
[ -d "$INCDIR" ] || fail "Android include directory not found: $INCDIR"
[ -d "$LIBDIR" ] || fail "Android runtime directory not found: $LIBDIR"

NDKROOT="$(find_ndk_root)" || fail "Android NDK not found under $SDKROOT"
PREBUILT="$(find_ndk_prebuilt "$NDKROOT")" || fail "Android NDK LLVM prebuilt toolchain not found"
CC="$PREBUILT/bin/aarch64-linux-android${ANDROID_API}-clang"
AR="$PREBUILT/bin/llvm-ar"
RANLIB="$PREBUILT/bin/llvm-ranlib"
[ -x "$CC" ] || CC="$CC.exe"
[ -x "$AR" ] || AR="$AR.exe"
[ -x "$RANLIB" ] || RANLIB="$RANLIB.exe"
[ -x "$CC" ] || fail "Android clang not found: $CC"
[ -x "$AR" ] || fail "Android llvm-ar not found: $AR"
[ -x "$RANLIB" ] || fail "Android llvm-ranlib not found: $RANLIB"

AAPT="$(find_android_tool aapt)" || fail "aapt was not found in the packaged Android SDK"
APKSIGNER="$(find_android_tool apksigner)" || fail "apksigner was not found in the packaged Android SDK"
ZIPALIGN="$(find_android_tool zipalign || true)"
JAR="$DISTROOT/toolchain/java/bin/jar.exe"
KEYTOOL="$DISTROOT/toolchain/java/bin/keytool.exe"
[ -x "$JAR" ] || JAR="$(command -v jar || true)"
[ -x "$KEYTOOL" ] || KEYTOOL="$(command -v keytool || true)"
[ -n "$JAR" ] || fail "jar was not found"
[ -n "$KEYTOOL" ] || fail "keytool was not found"

PATH="$DISTROOT/toolchain/msys2/usr/bin:$DISTROOT/toolchain/java/bin:$SDKROOT/platform-tools:$PATH"
JAVA_HOME="$DISTROOT/toolchain/java"
ANDROID_HOME="$SDKROOT"
ANDROID_SDK_ROOT="$SDKROOT"
FBANDROID_PREFIX="$DISTROOT"
export PATH JAVA_HOME ANDROID_HOME ANDROID_SDK_ROOT FBANDROID_PREFIX

if [ "$KEEP_BUILDROOT" -eq 0 ]; then
	rm -rf "$BUILDROOT_TEST"
fi
mkdir -p "$BUILDROOT_TEST" "$OUT"

##############################################################################
# Main
##############################################################################

generate_source_list
write_object_map
compile_fbcunit
compile_test_objects
compile_main_object
copy_test_assets
link_shared_library
package_apk

echo ""
echo "Android fbcunit APK: $APK"
echo "Android fbcunit log: $LOGCAT"

if [ "$NO_RUN" -eq 0 ]; then
	ADB="$(ensure_emulator)"
	run_apk "$ADB"
fi

##############################################################################
# End of msys2-test-freebasic-android.sh
##############################################################################
