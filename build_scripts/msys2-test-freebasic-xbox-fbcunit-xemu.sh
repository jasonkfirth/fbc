#!/usr/bin/env bash
#
# Project: FreeBASIC Xbox package tests
# ------------------------------------
#
# File: msys2-test-freebasic-xbox-fbcunit-xemu.sh
#
# Purpose:
#
#   Run the aggregate fbcunit test suite as an Xbox XBE under xemu.
#
# Responsibilities:
#
#   * reuse the Xbox test objects produced by msys2-test-freebasic-xbox-tests.sh
#   * build a small Xbox-only fbcunit runner without changing the real tests
#   * package the runner as an XISO for xemu
#   * launch xemu through QMP and capture the final framebuffer
#   * classify the run from the final green/red pass/fail screen
#
# This file intentionally does NOT contain:
#
#   * FreeBASIC compiler or runtime fixes
#   * test source modifications
#   * generic xemu installation logic
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

BUILD_ROOT="${BUILD_ROOT:-/tmp/freebasic-xbox-build}"
DIST_DIR="${DIST_DIR:-${BUILD_ROOT}/dist/FreeBASIC-1.20.1-fbc-xbox}"
TEST_WORKDIR="${TEST_WORKDIR:-${ROOT_DIR}/.build-msys2/freebasic-xbox-tests/tests}"
WORKROOT="${WORKROOT:-${ROOT_DIR}/.build-msys2/freebasic-xbox-fbcunit-xemu}"
XEMU_DIR="${XEMU_DIR:-${ROOT_DIR}/xbox_emulator_xemu}"
XEMU_EXE="${XEMU_EXE:-${XEMU_DIR}/xemu.exe}"
XEMU_CONFIG="${XEMU_CONFIG:-${ROOT_DIR}/.build-msys2/freebasic-xbox-xemu-test/xemu.toml}"
TIMEOUT_SECONDS="${TIMEOUT_SECONDS:-240}"
KEEP_WORK="${KEEP_WORK:-0}"
SUITE_DIRS="${SUITE_DIRS:-}"
OBJECTS="${OBJECTS:-}"
SNAPSHOT="${SNAPSHOT:-1}"
CAPTURE_SCREEN="${CAPTURE_SCREEN:-1}"

RUNNER_MODULE="xbox-fbcunit-runtime"
RUNNER_BAS="${WORKROOT}/${RUNNER_MODULE}.bas"
RUNNER_OBJ="${WORKROOT}/${RUNNER_MODULE}.o"
RUNNER_XBE="${WORKROOT}/${RUNNER_MODULE}.xbe"
XISO_STAGE="${WORKROOT}/xiso-stage"
XISO_DIR="${WORKROOT}/xiso"
XISO_PATH="${XISO_DIR}/${RUNNER_MODULE}.iso"
SCREENSHOT_PATH="${WORKROOT}/logs/fbcunit-screen.png"
XEMU_STDOUT="${WORKROOT}/logs/xemu.stdout.log"
XEMU_STDERR="${WORKROOT}/logs/xemu.stderr.log"
QMP_SCRIPT="${WORKROOT}/launch-fbcunit-xemu.ps1"
XISO_CMD="${WORKROOT}/pack-fbcunit-xiso.cmd"
XISO_LOG="${WORKROOT}/logs/pack-fbcunit-xiso.log"

usage()
{
	cat <<EOF
Usage: $(basename "$0") [options]

Options:
  --dist-dir PATH        packaged freebasic-xbox directory
  --test-workdir PATH    build directory from msys2-test-freebasic-xbox-tests.sh
  --xemu PATH            xemu.exe path
  --config PATH          xemu config path
  --timeout SECONDS      seconds to let the guest run before capture
  --suite-dirs LIST      comma-separated test directories to link
  --objects LIST         comma-separated object paths or basenames to link
  --keep-work            leave generated runner and XISO in place
  --no-snapshot          run against the writable HDD from the xemu config
  --no-capture           launch for the timeout without framebuffer capture
  -h, --help             show this help

Environment overrides use the same uppercase names as the option labels.
EOF
}

fail()
{
	echo "error: $*" >&2
	exit 1
}

to_windows_path()
{
	cygpath -am "$1"
}

to_cmd_path()
{
	cygpath -aw "$1"
}

stage_test_data_files()
{
	local data_dir
	local data_file
	local dst
	local object_file
	local rel_object

	while IFS= read -r data_file; do
		data_file="${data_file#./}"
		dst="${XISO_STAGE}/${RUNNER_MODULE}/${data_file}"
		mkdir -p "$(dirname "$dst")"
		cp -p "${TEST_WORKDIR}/${data_file}" "$dst"
	done < <(
		{
			if [ -d "${TEST_WORKDIR}/data" ]; then
				printf '%s\n' "data"
			fi

			while IFS= read -r object_file; do
				rel_object="${object_file#./}"
				case "$rel_object" in
					*/*)
						printf '%s\n' "${rel_object%%/*}"
						;;
				esac
			done < "$OBJECT_LIST"
		} | sort -u | while IFS= read -r data_dir; do
			[ -d "${TEST_WORKDIR}/${data_dir}" ] || continue
			(
				cd "$TEST_WORKDIR"
				find "$data_dir" -type f \( \
					-name '*.bin' -o \
					-name '*.bas' -o \
					-name '*.bmp' -o \
					-name '*.csv' -o \
					-name '*.dat' -o \
					-name '*.json' -o \
					-name '*.png' -o \
					-name '*.raw' -o \
					-name '*.txt' -o \
					-name '*.wav' \
				\) -print
			)
		done | sort -u
	)
}

while [ $# -gt 0 ]; do
	case "$1" in
		--dist-dir)
			DIST_DIR="$2"
			shift 2
			;;
		--test-workdir)
			TEST_WORKDIR="$2"
			shift 2
			;;
		--xemu)
			XEMU_EXE="$2"
			shift 2
			;;
		--config)
			XEMU_CONFIG="$2"
			shift 2
			;;
		--timeout)
			TIMEOUT_SECONDS="$2"
			shift 2
			;;
		--suite-dirs)
			SUITE_DIRS="$2"
			shift 2
			;;
		--objects)
			OBJECTS="$2"
			shift 2
			;;
		--keep-work)
			KEEP_WORK=1
			shift
			;;
		--no-snapshot)
			SNAPSHOT=0
			shift
			;;
		--no-capture)
			CAPTURE_SCREEN=0
			shift
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			fail "unknown option: $1"
			;;
	esac
done

FBC_XBOX="${DIST_DIR}/fbc-xbox-package.sh"
EXTRACT_XISO="${DIST_DIR}/nxdk/tools/extract-xiso/build/extract-xiso.exe"
UNIT_OBJECT_LIST="${TEST_WORKDIR}/unit-tests-obj.lst"
SUBSET_OBJECT_LIST="${WORKROOT}/xbox-fbcunit-runtime-objects.lst"
FBCUNIT_INC="${TEST_WORKDIR}/fbcunit/inc"
FBCUNIT_LIB="${TEST_WORKDIR}/fbcunit/lib/libfbcunit.a"

[ -x "$FBC_XBOX" ] || fail "missing fbc-xbox wrapper: $FBC_XBOX"
[ -x "$EXTRACT_XISO" ] || fail "missing extract-xiso: $EXTRACT_XISO"
[ -f "$UNIT_OBJECT_LIST" ] || fail "missing unit object list: $UNIT_OBJECT_LIST"
[ -d "$FBCUNIT_INC" ] || fail "missing fbcunit includes: $FBCUNIT_INC"
[ -f "$FBCUNIT_LIB" ] || fail "missing fbcunit library: $FBCUNIT_LIB"
[ -f "$XEMU_EXE" ] || fail "missing xemu executable: $XEMU_EXE"
[ -f "$XEMU_CONFIG" ] || fail "missing xemu config: $XEMU_CONFIG"

case "$TIMEOUT_SECONDS" in
	''|*[!0-9]*)
		fail "--timeout must be a positive integer"
		;;
esac

if [ "$TIMEOUT_SECONDS" -lt 15 ]; then
	fail "--timeout must be at least 15 seconds"
fi

if [ "$KEEP_WORK" = "0" ]; then
	rm -rf "$WORKROOT"
fi

mkdir -p "$WORKROOT" "$XISO_DIR" "${WORKROOT}/logs"

OBJECT_LIST="$UNIT_OBJECT_LIST"

if [ -n "$SUITE_DIRS" ] && [ -n "$OBJECTS" ]; then
	fail "--suite-dirs and --objects are mutually exclusive"
fi

if [ -n "$OBJECTS" ]; then
	IFS=',' read -r -a requested_objects <<< "$OBJECTS"
	: > "$SUBSET_OBJECT_LIST"

	for requested_object in "${requested_objects[@]}"; do
		requested_object="${requested_object#"${requested_object%%[![:space:]]*}"}"
		requested_object="${requested_object%"${requested_object##*[![:space:]]}"}"

		case "$requested_object" in
			''|*[^A-Za-z0-9._/-]*)
				fail "invalid object name: $requested_object"
				;;
		esac

		found=0

		while IFS= read -r object_file; do
			case "$requested_object" in
				*/*)
					match_object="./${requested_object#./}"
					;;
				*)
					match_object="*/${requested_object%.o}.o"
					;;
			esac

			case "$object_file" in
				$match_object)
					echo "$object_file" >> "$SUBSET_OBJECT_LIST"
					found=1
					;;
			esac
		done < "$UNIT_OBJECT_LIST"

		[ "$found" -eq 1 ] || fail "object was not found in Xbox test objects: $requested_object"
	done

	OBJECT_LIST="$SUBSET_OBJECT_LIST"
	echo "Using Xbox fbcunit objects: $OBJECTS"
elif [ -n "$SUITE_DIRS" ]; then
	IFS=',' read -r -a requested_dirs <<< "$SUITE_DIRS"
	: > "$SUBSET_OBJECT_LIST"

	for suite_dir in "${requested_dirs[@]}"; do
		suite_dir="${suite_dir#"${suite_dir%%[![:space:]]*}"}"
		suite_dir="${suite_dir%"${suite_dir##*[![:space:]]}"}"

		case "$suite_dir" in
			''|*[^A-Za-z0-9._-]*)
				fail "invalid suite directory name: $suite_dir"
				;;
		esac

		found=0

		while IFS= read -r object_file; do
			case "$object_file" in
				"./${suite_dir}/"*)
					echo "$object_file" >> "$SUBSET_OBJECT_LIST"
					found=1
					;;
			esac
		done < "$UNIT_OBJECT_LIST"

		[ "$found" -eq 1 ] || fail "suite directory has no Xbox test objects: $suite_dir"

		case "$suite_dir" in
			interactive)
				if grep -qx './console/common.o' "$UNIT_OBJECT_LIST"; then
					echo './console/common.o' >> "$SUBSET_OBJECT_LIST"
				fi
				;;
		esac
	done

	sort -u "$SUBSET_OBJECT_LIST" -o "$SUBSET_OBJECT_LIST"
	OBJECT_LIST="$SUBSET_OBJECT_LIST"
	echo "Using Xbox fbcunit subset: $SUITE_DIRS"
fi

cat > "$RUNNER_BAS" <<'EOF'
'
' Project: FreeBASIC Xbox package tests
' ------------------------------------
'
' File: xbox-fbcunit-runtime.bas
'
' Purpose:
'
'   Provide an emulator-visible entry point for the aggregate fbcunit
'   object set produced by the Xbox package test build.
'
' Responsibilities:
'
'   * run the registered fbcunit tests
'   * draw a simple pass/fail color marker for framebuffer capture
'   * keep the guest alive long enough for xemu/QMP screenshots
'
' This file intentionally does NOT contain:
'
'   * any individual test cases
'   * platform-specific test exclusions
'   * compiler or runtime diagnostics
'

const FB_XBOX_TEST_SCREEN_W = 640
const FB_XBOX_TEST_SCREEN_H = 480
const FB_XBOX_RESULT_FILE = "E:\FBCUNIT.TXT"
const FB_XBOX_LOG_FILE = "E:\FBCUNIT.LOG"

dim shared passed as boolean

sub reset_text_file( byref filename as const string )
	dim h as integer

	h = freefile
	if open( filename for output as #h ) <> 0 then
		exit sub
	end if

	close #h
end sub

sub append_text_file( byref filename as const string, byref text as const string )
	dim h as integer

	h = freefile
	if open( filename for append as #h ) <> 0 then
		exit sub
	end if

	print #h, text;
	close #h
end sub

sub write_result_file( byref text as const string )
	reset_text_file( FB_XBOX_RESULT_FILE )
	append_text_file( FB_XBOX_RESULT_FILE, text & chr(10) )
end sub

sub crt_print_output( byref s as const string )
	append_text_file( FB_XBOX_LOG_FILE, s )
end sub

#include once "fbcunit.bi"

reset_text_file( FB_XBOX_LOG_FILE )
write_result_file( "FREEBASIC_XBOX_FBCUNIT_START" )

if fbcu.check_internal_state() then
	fbcu.setBriefSummary( true )
	fbcu.setHideCases( false )
	fbcu.setShowConsole( false )
	passed = fbcu.run_tests( true, true )
else
	passed = false
end if

if passed then
	write_result_file( "FREEBASIC_XBOX_FBCUNIT_PASS" )
else
	write_result_file( "FREEBASIC_XBOX_FBCUNIT_FAIL" )
end if

screenres FB_XBOX_TEST_SCREEN_W, FB_XBOX_TEST_SCREEN_H, 32

if passed then
	line (0, 0)-(FB_XBOX_TEST_SCREEN_W - 1, FB_XBOX_TEST_SCREEN_H - 1), rgb(0, 180, 0), bf
	locate 2, 2
	print "FREEBASIC_XBOX_FBCUNIT_PASS"
else
	line (0, 0)-(FB_XBOX_TEST_SCREEN_W - 1, FB_XBOX_TEST_SCREEN_H - 1), rgb(180, 0, 0), bf
	locate 2, 2
	print "FREEBASIC_XBOX_FBCUNIT_FAIL"
end if

' xemu screenshots are taken from the host after a fixed timeout.
' Keep the guest alive so fast successful runs do not exit before capture.
sleep 600000

if passed then
	end 0
else
	end 1
end if

' end of xbox-fbcunit-runtime.bas
EOF

echo "Building Xbox fbcunit runtime runner..."
(
	cd "$TEST_WORKDIR"
	"$FBC_XBOX" -target xbox -mt -w 3 -Wc -Wno-tautological-compare \
		-i "$FBCUNIT_INC" \
		-m "$RUNNER_MODULE" \
		-c "$RUNNER_BAS" \
		-o "$RUNNER_OBJ"

	"$FBC_XBOX" -target xbox -fbgfx \
		-x "$RUNNER_XBE" \
		@"$OBJECT_LIST" \
		"$RUNNER_OBJ" \
		"$FBCUNIT_LIB"
)

rm -rf "$XISO_STAGE"
mkdir -p "${XISO_STAGE}/${RUNNER_MODULE}"
cp "$RUNNER_XBE" "${XISO_STAGE}/${RUNNER_MODULE}/default.xbe"
stage_test_data_files

echo "Packing XISO..."
cat > "$XISO_CMD" <<EOF
@echo off
setlocal
"$(to_cmd_path "$EXTRACT_XISO")" -q -c "$(to_cmd_path "${XISO_STAGE}/${RUNNER_MODULE}")" "$(to_cmd_path "$XISO_PATH")" > "$(to_cmd_path "$XISO_LOG")" 2>&1
set "XISO_STATUS=%ERRORLEVEL%"
if not "%XISO_STATUS%"=="0" exit /b %XISO_STATUS%
if not exist "$(to_cmd_path "$XISO_PATH")" exit /b 2
exit /b 0
EOF

if ! cmd.exe //C "$(to_cmd_path "$XISO_CMD")"; then
	cat "$XISO_LOG" >&2 || true
	fail "extract-xiso failed while packing fbcunit runtime XISO"
fi

cat > "$QMP_SCRIPT" <<'EOF'
param(
	[string]$XemuPath,
	[string]$ConfigPath,
	[string]$IsoPath,
	[string]$ScreenshotPath,
	[string]$StdoutPath,
	[string]$StderrPath,
	[int]$TimeoutSeconds,
	[int]$UseSnapshot,
	[int]$CaptureScreen
)

$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.Drawing

Add-Type @"
using System;
using System.Runtime.InteropServices;

public struct RECT
{
	public int Left;
	public int Top;
	public int Right;
	public int Bottom;
}

public static class FreeBasicXboxWin32
{
	[DllImport("user32.dll")]
	public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);

	[DllImport("user32.dll")]
	public static extern bool SetForegroundWindow(IntPtr hWnd);

	[DllImport("user32.dll")]
	public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
}
"@

function Get-XemuWindowHandle {
	param(
		[System.Diagnostics.Process]$Process
	)

	$deadline = (Get-Date).AddSeconds(30)

	do {
		$Process.Refresh()

		if ($Process.MainWindowHandle -ne [IntPtr]::Zero) {
			return $Process.MainWindowHandle
		}

		if ($Process.HasExited) {
			throw "xemu exited before opening a window"
		}

		Start-Sleep -Milliseconds 250
	} while ((Get-Date) -lt $deadline)

	throw "xemu did not open a window"
}

function Save-WindowCapture {
	param(
		[System.IntPtr]$Handle,
		[string]$Path
	)

	$rect = New-Object RECT

	if (-not [FreeBasicXboxWin32]::GetWindowRect($Handle, [ref]$rect)) {
		throw "could not read xemu window bounds"
	}

	$width = $rect.Right - $rect.Left
	$height = $rect.Bottom - $rect.Top

	if (($width -lt 64) -or ($height -lt 64)) {
		throw "xemu window is too small to capture: ${width}x${height}"
	}

	$null = [FreeBasicXboxWin32]::ShowWindow($Handle, 9)
	$null = [FreeBasicXboxWin32]::SetForegroundWindow($Handle)
	Start-Sleep -Milliseconds 750

	$bitmap = [System.Drawing.Bitmap]::new($width, $height)
	$graphics = [System.Drawing.Graphics]::FromImage($bitmap)

	try {
		$graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $bitmap.Size)
		$bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
	} finally {
		$graphics.Dispose()
		$bitmap.Dispose()
	}
}

function Read-BitmapAverage {
	param(
		[string]$Path
	)

	$bitmap = [System.Drawing.Bitmap]::FromFile($Path)

	try {
		$width = $bitmap.Width
		$height = $bitmap.Height
	$boxWidth = [Math]::Min(96, [Math]::Max(8, [int]($width / 5)))
	$boxHeight = [Math]::Min(96, [Math]::Max(8, [int]($height / 5)))
	$x0 = [Math]::Max(0, [int](($width - $boxWidth) / 2))
	$y0 = [Math]::Max(0, [int](($height - $boxHeight) / 2))
	$x1 = [Math]::Min($width, $x0 + $boxWidth)
	$y1 = [Math]::Min($height, $y0 + $boxHeight)
	[double]$sumR = 0
	[double]$sumG = 0
	[double]$sumB = 0
	[int]$count = 0

	for ($y = $y0; $y -lt $y1; $y++) {
		for ($x = $x0; $x -lt $x1; $x++) {
			$pixel = $bitmap.GetPixel($x, $y)
			$sumR += $pixel.R
			$sumG += $pixel.G
			$sumB += $pixel.B
			$count++
		}
	}

	return [pscustomobject]@{
		Width = $width
		Height = $height
		R = $sumR / $count
		G = $sumG / $count
		B = $sumB / $count
	}
	} finally {
		$bitmap.Dispose()
	}
}

$xemuArgs = @(
	"-config_path", $ConfigPath
)

if ($UseSnapshot -ne 0) {
	$xemuArgs += "-snapshot"
}

$xemuArgs += @(
	"-dvd_path", $IsoPath
)

$process = Start-Process `
	-FilePath $XemuPath `
	-ArgumentList $xemuArgs `
	-RedirectStandardOutput $StdoutPath `
	-RedirectStandardError $StderrPath `
	-PassThru

try {
	$handle = Get-XemuWindowHandle $process
	Start-Sleep -Seconds $TimeoutSeconds

	if ($CaptureScreen -eq 0) {
		Write-Output "xemu fbcunit runtime result: GUEST-DISK"
		exit 0
	}

	Save-WindowCapture $handle $ScreenshotPath

	if ((-not (Test-Path -LiteralPath $ScreenshotPath)) -or ((Get-Item -LiteralPath $ScreenshotPath).Length -eq 0)) {
		throw "xemu window capture was not created"
	}

	$average = Read-BitmapAverage $ScreenshotPath
	"{0}x{1} center average: R={2:N1} G={3:N1} B={4:N1}" -f $average.Width, $average.Height, $average.R, $average.G, $average.B

	if (($average.G -gt 90) -and ($average.G -gt ($average.R * 1.45)) -and ($average.G -gt ($average.B * 1.45))) {
		Write-Output "xemu fbcunit runtime result: PASS"
		exit 0
	}

	if (($average.R -gt 90) -and ($average.R -gt ($average.G * 1.45)) -and ($average.R -gt ($average.B * 1.45))) {
		Write-Output "xemu fbcunit runtime result: FAIL"
		exit 2
	}

	Write-Output "xemu fbcunit runtime result: INCONCLUSIVE"
	exit 3
} finally {
	if (($process -ne $null) -and (-not $process.HasExited)) {
		Stop-Process -Id $process.Id -Force
	}
}
EOF

echo "Launching xemu fbcunit runtime for ${TIMEOUT_SECONDS}s..."
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$(to_windows_path "$QMP_SCRIPT")" \
	-XemuPath "$(to_windows_path "$XEMU_EXE")" \
	-ConfigPath "$(to_windows_path "$XEMU_CONFIG")" \
	-IsoPath "$(to_windows_path "$XISO_PATH")" \
	-ScreenshotPath "$(to_windows_path "$SCREENSHOT_PATH")" \
	-StdoutPath "$(to_windows_path "$XEMU_STDOUT")" \
	-StderrPath "$(to_windows_path "$XEMU_STDERR")" \
	-TimeoutSeconds "$TIMEOUT_SECONDS" \
	-UseSnapshot "$SNAPSHOT" \
	-CaptureScreen "$CAPTURE_SCREEN"

if [ "$CAPTURE_SCREEN" -ne 0 ]; then
	echo "screenshot: $(to_windows_path "$SCREENSHOT_PATH")"
fi

# end of msys2-test-freebasic-xbox-fbcunit-xemu.sh
