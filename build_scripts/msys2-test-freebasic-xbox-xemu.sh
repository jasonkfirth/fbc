#!/usr/bin/env bash

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# msys2-test-freebasic-xbox-xemu.sh
#
# Build small FreeBASIC Xbox smoke programs, pack them as Xbox XISO disc
# images, and optionally launch them through xemu.
#
# Responsibilities:
#   - locate a packaged fbc-xbox distribution
#   - compile console, gfxlib, sfxlib, and file I/O smoke XBEs
#   - pack each XBE as default.xbe inside an XISO image
#   - launch configured xemu instances with the generated XISO images
#   - optionally capture and analyze xemu WAV output from the sfx smoke
#
# This file intentionally does NOT contain:
#   - the fbc-xbox package build itself
#   - copyrighted Xbox boot ROM, BIOS, dashboard, or game assets
#   - full fbcunit execution
##############################################################################

SELF_DIR="$(CDPATH= cd -- "$(dirname "$0")" && pwd)"
ROOT="$(CDPATH= cd -- "$SELF_DIR/.." && pwd)"

cd "$ROOT"

if [ ! -d "$ROOT/build_scripts" ] || [ ! -f "$ROOT/GNUmakefile" ]; then
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
# Options
##############################################################################

DIST_DIR=""
XEMU_EXE="${XEMU:-}"
XEMU_CONFIG="${XEMU_CONFIG:-}"
XEMU_MCPX="${XEMU_MCPX:-}"
XEMU_BIOS="${XEMU_BIOS:-}"
XEMU_HDD="${XEMU_HDD:-}"
XEMU_EEPROM="${XEMU_EEPROM:-}"
WORKROOT="${WORKROOT:-$ROOT/.build-msys2/freebasic-xbox-xemu-test}"
KEEP_WORKROOT=0
SKIP_LAUNCH=0
REQUIRE_XEMU=0
LAUNCH_TIMEOUT="${LAUNCH_TIMEOUT:-20}"
LAUNCH_PROGRAMS="${LAUNCH_PROGRAMS:-console}"
VALIDATE_AUDIO=0
AUDIO_TIMEOUT="${AUDIO_TIMEOUT:-8}"

usage() {
	cat <<EOF
Usage: ./build_scripts/msys2-test-freebasic-xbox-xemu.sh [options]

Options:
  --dist-dir DIR       FreeBASIC fbc-xbox distribution directory
  --xemu EXE           xemu executable path
  --xemu-config FILE   xemu.toml path to pass with -config_path
  --mcpx FILE          MCPX boot ROM path for a temporary xemu.toml
  --bios FILE          Flash ROM / BIOS path for a temporary xemu.toml
  --hdd FILE           Xbox HDD image path for a temporary xemu.toml
                       (defaults to ./xbox_emulator_xemu/xbox_hdd.qcow2)
  --eeprom FILE        Optional EEPROM path for a temporary xemu.toml
  --workroot DIR       Test work directory
  --keep-workroot      Keep the generated smoke test directory
  --skip-launch        Only compile XBEs and XISOs, do not start xemu
  --require-xemu       Fail if xemu cannot be found
  --launch-timeout N   Seconds to leave each XISO running (default: 20)
  --launch-programs L  Comma-separated XISO names to launch (default: console)
  --validate-audio     Capture sfx.iso through xemu's WAV backend and verify
                       the expected 440 Hz and 660 Hz tones without gaps
  --audio-timeout N    Seconds to leave sfx.iso running for audio capture
                       (default: 8)
  --help               Show this help text

The script always compiles and packs console, gfx-screenres, gfx-screen13, sfx,
and fileio smoke programs. xemu is a full-system Xbox emulator, so it must be
configured separately with legally acquired Xbox boot assets. This script never
downloads or supplies those files.
EOF
}

while [ $# -gt 0 ]; do
	case "$1" in
		--dist-dir)
			DIST_DIR="$2"
			shift 2
			;;
		--xemu)
			XEMU_EXE="$2"
			shift 2
			;;
		--xemu-config)
			XEMU_CONFIG="$2"
			shift 2
			;;
		--mcpx)
			XEMU_MCPX="$2"
			shift 2
			;;
		--bios)
			XEMU_BIOS="$2"
			shift 2
			;;
		--hdd)
			XEMU_HDD="$2"
			shift 2
			;;
		--eeprom)
			XEMU_EEPROM="$2"
			shift 2
			;;
		--workroot)
			WORKROOT="$2"
			shift 2
			;;
		--keep-workroot)
			KEEP_WORKROOT=1
			shift
			;;
		--skip-launch)
			SKIP_LAUNCH=1
			shift
			;;
		--require-xemu)
			REQUIRE_XEMU=1
			shift
			;;
		--launch-timeout)
			LAUNCH_TIMEOUT="$2"
			shift 2
			;;
		--launch-programs)
			LAUNCH_PROGRAMS="$2"
			shift 2
			;;
		--validate-audio)
			VALIDATE_AUDIO=1
			shift
			;;
		--audio-timeout)
			AUDIO_TIMEOUT="$2"
			shift 2
			;;
		-h|--help)
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

if [ "$SKIP_LAUNCH" -eq 1 ] && [ "$VALIDATE_AUDIO" -eq 1 ]; then
	fail "--validate-audio cannot be combined with --skip-launch"
fi

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

run() {
	echo "==> $*"
	"$@"
}

have() {
	command -v "$1" >/dev/null 2>&1
}

first_existing_dir() {
	local candidate

	for candidate in "$@"; do
		[ -n "$candidate" ] || continue
		if [ -d "$candidate" ]; then
			echo "$candidate"
			return 0
		fi
	done

	return 1
}

first_existing_file() {
	local candidate

	for candidate in "$@"; do
		[ -n "$candidate" ] || continue
		if [ -f "$candidate" ]; then
			echo "$candidate"
			return 0
		fi
	done

	return 1
}

cygpath_abs_win() {
	cygpath -aw "$1"
}

cygpath_abs_msys() {
	cygpath -au "$1"
}

toml_literal_path() {
	local path="$1"

	case "$path" in
		*"'"*)
			fail "xemu config paths may not contain single quotes: $path"
			;;
	esac

	cygpath_abs_win "$path"
}

detect_command_file() {
	local name="$1"
	local found

	found="$(command -v "$name" 2>/dev/null || true)"
	if [ -n "$found" ]; then
		cygpath_abs_msys "$found"
		return 0
	fi

	return 1
}

detect_xemu() {
	local found

	if [ -n "$XEMU_EXE" ]; then
		cygpath_abs_msys "$XEMU_EXE"
		return 0
	fi

	found="$(detect_command_file xemu.exe || true)"
	if [ -z "$found" ]; then
		found="$(detect_command_file xemu || true)"
	fi
	if [ -n "$found" ]; then
		echo "$found"
		return 0
	fi

	first_existing_file \
		"$ROOT/xemu/xemu.exe" \
		"$ROOT/xbox_emulator_xemu/xemu.exe" \
		"$ROOT/xbox_emulator/xemu.exe"
}

find_extract_xiso() {
	first_existing_file \
		"$DIST_DIR/nxdk/tools/extract-xiso/build/extract-xiso.exe" \
		"$DIST_DIR/nxdk/tools/extract-xiso/build/extract-xiso" \
		"/tmp/freebasic-xbox-build/nxdk/tools/extract-xiso/build/extract-xiso.exe" \
		"/tmp/freebasic-xbox-build/nxdk/tools/extract-xiso/build/extract-xiso"
}

detect_default_xemu_assets() {
	if [ -z "$XEMU_MCPX" ]; then
		XEMU_MCPX="$(first_existing_file \
			"$ROOT/xbox_emulator_xemu/Boot ROM image/mcpx_1.0.bin" \
			"$ROOT/xbox_emulator_xemu/mcpx_1.0.bin" \
			"$ROOT/xbox_emulator_xemu/mcpx.bin" \
			|| true)"
	fi

	if [ -z "$XEMU_BIOS" ]; then
		XEMU_BIOS="$(first_existing_file \
			"$ROOT/xbox_emulator_xemu/BIOS/Complex_4627.bin" \
			"$ROOT/xbox_emulator_xemu/BIOS/Complex_4627v1.03.bin" \
			"$ROOT/xbox_emulator_xemu/bios.bin" \
			"$ROOT/xbox_emulator_xemu/flash.bin" \
			|| true)"
	fi
}

prepare_generated_xemu_config() {
	local bootrom_win
	local flashrom_win
	local hdd_win
	local eeprom_win

	if [ -n "$XEMU_CONFIG" ]; then
		return 0
	fi

	if [ -z "$XEMU_MCPX" ] && [ -z "$XEMU_BIOS" ] && [ -z "$XEMU_HDD" ] && [ -z "$XEMU_EEPROM" ]; then
		return 0
	fi

	[ -n "$XEMU_MCPX" ] || fail "--mcpx is required when generating a temporary xemu config"
	[ -n "$XEMU_BIOS" ] || fail "--bios is required when generating a temporary xemu config"
	if [ -z "$XEMU_HDD" ]; then
		XEMU_HDD="$(first_existing_file \
			"$ROOT/xbox_emulator_xemu/Pre-built Xbox HDD image/xbox_hdd.qcow2" \
			"$ROOT/xbox_emulator_xemu/xbox_hdd.qcow2" \
			|| true)"
	fi
	[ -n "$XEMU_HDD" ] || fail "--hdd is required when generating a temporary xemu config"

	XEMU_MCPX="$(cygpath_abs_msys "$XEMU_MCPX")"
	XEMU_BIOS="$(cygpath_abs_msys "$XEMU_BIOS")"
	XEMU_HDD="$(cygpath_abs_msys "$XEMU_HDD")"

	[ -f "$XEMU_MCPX" ] || fail "MCPX boot ROM not found: $XEMU_MCPX"
	[ -f "$XEMU_BIOS" ] || fail "flash ROM / BIOS not found: $XEMU_BIOS"
	[ -f "$XEMU_HDD" ] || fail "Xbox HDD image not found: $XEMU_HDD"

	bootrom_win="$(toml_literal_path "$XEMU_MCPX")"
	flashrom_win="$(toml_literal_path "$XEMU_BIOS")"
	hdd_win="$(toml_literal_path "$XEMU_HDD")"

	XEMU_CONFIG="$WORKROOT/xemu.toml"
	cat > "$XEMU_CONFIG" <<EOF
[general]
show_welcome = false
skip_boot_anim = true

[general.updates]
check = false

[sys.files]
bootrom_path = '$bootrom_win'
flashrom_path = '$flashrom_win'
hdd_path = '$hdd_win'
EOF

	if [ -n "$XEMU_EEPROM" ]; then
		XEMU_EEPROM="$(cygpath_abs_msys "$XEMU_EEPROM")"
		[ -f "$XEMU_EEPROM" ] || fail "EEPROM image not found: $XEMU_EEPROM"
		eeprom_win="$(toml_literal_path "$XEMU_EEPROM")"
		cat >> "$XEMU_CONFIG" <<EOF
eeprom_path = '$eeprom_win'
EOF
	fi

	echo "==> generated temporary xemu config: $XEMU_CONFIG"
}

write_sources() {
	rm -rf "$SRC_DIR"
	mkdir -p "$SRC_DIR"

	cat > "$SRC_DIR/console.bas" <<'EOF'
print "FREEBASIC_XBOX_CONSOLE_SMOKE"
sleep 10000
EOF

	cat > "$SRC_DIR/gfx-screenres.bas" <<'EOF'
screenres 160, 120, 32, 2
screenset 1, 0

line (0, 0)-(159, 119), rgb(0, 128, 255), bf
line (16, 16)-(143, 103), rgb(255, 255, 255), b
circle (80, 60), 24, rgb(255, 0, 0), , , 1.0, f

screenset 1, 1
open err for output as #1
print #1, "FREEBASIC_XBOX_GFX_SCREENRES_SMOKE"
close #1
sleep 10000
EOF

	cat > "$SRC_DIR/gfx-screen13.bas" <<'EOF'
screen 13

line (0, 0)-(319, 199), 1, bf
line (32, 32)-(287, 167), 15, b
circle (160, 100), 40, 4, , , 1.0, f

open err for output as #1
print #1, "FREEBASIC_XBOX_GFX_SCREEN13_SMOKE"
close #1
sleep 10000
EOF

	cat > "$SRC_DIR/sfx.bas" <<'EOF'
declare sub fb_sfxUpdate cdecl alias "fb_sfxUpdate" ( byval frames as integer )

sound 0, 440, 0.25, 0.70
fb_sfxUpdate( 12000 )

sound 0, 660, 0.25, 0.70
fb_sfxUpdate( 12000 )

open err for output as #1
print #1, "FREEBASIC_XBOX_SFX_SMOKE"
close #1
sleep 10000
EOF

	cat > "$SRC_DIR/fileio.bas" <<'EOF'
open "fbxbox.tmp" for output as #1
print #1, "FREEBASIC_XBOX_FILEIO_SMOKE"
close #1

dim text as string
open "fbxbox.tmp" for input as #1
line input #1, text
close #1

print text
sleep 10000
EOF
}

compile_xbe() {
	local name="$1"
	local src="$SRC_DIR/$name.bas"
	local xbe="$XBE_DIR/$name.xbe"
	local cmd="$CMD_DIR/build-$name.cmd"
	local src_win
	local xbe_win
	local log_win
	local dist_win

	src_win="$(cygpath_abs_win "$src")"
	xbe_win="$(cygpath_abs_win "$xbe")"
	log_win="$(cygpath_abs_win "$LOG_DIR/$name.build.log")"
	dist_win="$(cygpath_abs_win "$DIST_DIR")"

	cat > "$cmd" <<EOF
@echo off
setlocal
call "$dist_win\\fbc-xbox.cmd" "$src_win" -x "$xbe_win" -v > "$log_win" 2>&1
set "FBC_XBOX_STATUS=%ERRORLEVEL%"
if not "%FBC_XBOX_STATUS%"=="0" exit /b %FBC_XBOX_STATUS%
if not exist "$xbe_win" exit /b 2
exit /b 0
EOF

	msg "compiling $name.xbe"
	if ! cmd.exe //C "$(cygpath_abs_win "$cmd")"; then
		cat "$LOG_DIR/$name.build.log" >&2 || true
		fail "fbc-xbox failed while compiling $name.bas"
	fi
}

pack_xiso() {
	local name="$1"
	local stage="$ISO_STAGE_DIR/$name"
	local xbe="$XBE_DIR/$name.xbe"
	local iso="$ISO_DIR/$name.iso"
	local cmd="$CMD_DIR/xiso-$name.cmd"
	local tool_win
	local stage_win
	local iso_win
	local log_win

	[ -f "$xbe" ] || fail "missing XBE: $xbe"

	rm -rf "$stage"
	mkdir -p "$stage"
	cp -a "$xbe" "$stage/default.xbe"

	tool_win="$(cygpath_abs_win "$EXTRACT_XISO")"
	stage_win="$(cygpath_abs_win "$stage")"
	iso_win="$(cygpath_abs_win "$iso")"
	log_win="$(cygpath_abs_win "$LOG_DIR/$name.xiso.log")"

	cat > "$cmd" <<EOF
@echo off
setlocal
"$tool_win" -q -c "$stage_win" "$iso_win" > "$log_win" 2>&1
set "XISO_STATUS=%ERRORLEVEL%"
if not "%XISO_STATUS%"=="0" exit /b %XISO_STATUS%
if not exist "$iso_win" exit /b 2
exit /b 0
EOF

	msg "packing $name.iso"
	if ! cmd.exe //C "$(cygpath_abs_win "$cmd")"; then
		cat "$LOG_DIR/$name.xiso.log" >&2 || true
		fail "extract-xiso failed while packing $name.iso"
	fi
}

write_launch_script() {
	local ps1="$1"

	cat > "$ps1" <<'EOF'
param(
	[string] $XemuPath,
	[string] $ConfigPath,
	[string] $BiosPath,
	[string] $IsoPath,
	[string] $StdoutPath,
	[string] $StderrPath,
	[int] $TimeoutSeconds
)

$ErrorActionPreference = "Stop"

if (!(Test-Path -LiteralPath $XemuPath)) {
	throw "xemu executable was not found: $XemuPath"
}
if (!(Test-Path -LiteralPath $IsoPath)) {
	throw "XISO image was not found: $IsoPath"
}

$arguments = @("-snapshot", "-dvd_path", $IsoPath)
if ($ConfigPath.Length -gt 0) {
	if (!(Test-Path -LiteralPath $ConfigPath)) {
		throw "xemu config was not found: $ConfigPath"
	}
	$arguments = @("-config_path", $ConfigPath) + $arguments
}
if ($BiosPath.Length -gt 0) {
	if (!(Test-Path -LiteralPath $BiosPath)) {
		throw "xemu BIOS was not found: $BiosPath"
	}
	$arguments += @("-bios", $BiosPath)
}

$process = Start-Process `
	-FilePath $XemuPath `
	-ArgumentList $arguments `
	-RedirectStandardOutput $StdoutPath `
	-RedirectStandardError $StderrPath `
	-PassThru

Start-Sleep -Seconds $TimeoutSeconds

if ($process.HasExited) {
	if ($process.ExitCode -ne 0) {
		throw "xemu exited with status $($process.ExitCode)"
	}
	Write-Output "xemu exited cleanly after loading $IsoPath"
} else {
	Stop-Process -Id $process.Id -Force
	Write-Output "xemu accepted $IsoPath and stayed open for $TimeoutSeconds seconds"
}

$combined = ""
if (Test-Path -LiteralPath $StdoutPath) {
	$combined += Get-Content -LiteralPath $StdoutPath -Raw
}
if (Test-Path -LiteralPath $StderrPath) {
	$combined += Get-Content -LiteralPath $StderrPath -Raw
}

if ($combined -match "The guest has not initialized the display") {
	throw "xemu reported missing, mismatched, or corrupt MCPX/BIOS images"
}
if ($combined -match "(?i)failed to load (bios|mcpx|boot rom|flash rom|hard disk|hdd)") {
	throw "xemu reported missing or invalid configured Xbox boot assets"
}
if ($combined -match "Failed to load DVD image|failed to load DVD image") {
	throw "xemu could not load the generated XISO"
}
EOF
}

launch_xiso() {
	local name="$1"
	local ps1="$CMD_DIR/launch-xemu.ps1"
	local iso="$ISO_DIR/$name.iso"
	local log="$LOG_DIR/$name.xemu.log"
	local stdout_log="$LOG_DIR/$name.xemu.stdout.log"
	local stderr_log="$LOG_DIR/$name.xemu.stderr.log"
	local config_win=""
	local bios_win=""

	[ -f "$iso" ] || fail "missing XISO: $iso"

	if [ -n "$XEMU_CONFIG" ]; then
		config_win="$(cygpath_abs_win "$XEMU_CONFIG")"
	fi
	if [ -n "$XEMU_BIOS" ]; then
		bios_win="$(cygpath_abs_win "$XEMU_BIOS")"
	fi

	write_launch_script "$ps1"

	msg "launching $name.iso in xemu"
	if ! powershell.exe -NoProfile -ExecutionPolicy Bypass \
		-File "$(cygpath_abs_win "$ps1")" \
		-XemuPath "$(cygpath_abs_win "$XEMU_EXE")" \
		-ConfigPath "$config_win" \
		-BiosPath "$bios_win" \
		-IsoPath "$(cygpath_abs_win "$iso")" \
		-StdoutPath "$(cygpath_abs_win "$stdout_log")" \
		-StderrPath "$(cygpath_abs_win "$stderr_log")" \
		-TimeoutSeconds "$LAUNCH_TIMEOUT" > "$log" 2>&1; then
		cat "$log" >&2 || true
		cat "$stdout_log" >&2 || true
		cat "$stderr_log" >&2 || true
		fail "xemu launch failed for $name.iso"
	fi

	cat "$log"
}

write_audio_validation_script() {
	local ps1="$1"

	cat > "$ps1" <<'EOF'
param(
	[string] $XemuPath,
	[string] $ConfigPath,
	[string] $BiosPath,
	[string] $IsoPath,
	[string] $WavPath,
	[string] $StdoutPath,
	[string] $StderrPath,
	[int] $TimeoutSeconds
)

$ErrorActionPreference = "Stop"

function Read-UInt16LE {
	param([byte[]] $Bytes, [int] $Offset)
	return [BitConverter]::ToUInt16($Bytes, $Offset)
}

function Read-UInt32LE {
	param([byte[]] $Bytes, [int] $Offset)
	return [BitConverter]::ToUInt32($Bytes, $Offset)
}

function Find-DataChunk {
	param([byte[]] $Bytes)

	for ($i = 12; $i -le ($Bytes.Length - 8); $i++) {
		if (($Bytes[$i] -eq 100) -and ($Bytes[$i + 1] -eq 97) -and
		    ($Bytes[$i + 2] -eq 116) -and ($Bytes[$i + 3] -eq 97)) {
			return $i
		}
	}

	throw "WAV data chunk not found"
}

function Get-Rms {
	param([double[]] $Samples, [int] $Start, [int] $Frames)

	$sum = 0.0
	for ($i = 0; $i -lt $Frames; $i++) {
		$s = $Samples[$Start + $i]
		$sum += $s * $s
	}

	return [Math]::Sqrt($sum / [double]$Frames)
}

function Get-BandPower {
	param([double[]] $Samples, [int] $Start, [int] $Frames, [int] $Rate, [double] $Frequency)

	$re = 0.0
	$im = 0.0

	for ($i = 0; $i -lt $Frames; $i++) {
		$angle = 2.0 * [Math]::PI * $Frequency * [double]$i / [double]$Rate
		$sample = $Samples[$Start + $i]

		$re += $sample * [Math]::Cos($angle)
		$im -= $sample * [Math]::Sin($angle)
	}

	return [Math]::Sqrt(($re * $re) + ($im * $im)) / [double]$Frames
}

if (!(Test-Path -LiteralPath $XemuPath)) {
	throw "xemu executable was not found: $XemuPath"
}
if (!(Test-Path -LiteralPath $IsoPath)) {
	throw "XISO image was not found: $IsoPath"
}

if (Test-Path -LiteralPath $WavPath) {
	Remove-Item -LiteralPath $WavPath -Force
}

$arguments = @("-snapshot", "-audio", "wav,path=$WavPath", "-dvd_path", $IsoPath)
if ($ConfigPath.Length -gt 0) {
	if (!(Test-Path -LiteralPath $ConfigPath)) {
		throw "xemu config was not found: $ConfigPath"
	}
	$arguments = @("-config_path", $ConfigPath) + $arguments
}
if ($BiosPath.Length -gt 0) {
	if (!(Test-Path -LiteralPath $BiosPath)) {
		throw "xemu BIOS was not found: $BiosPath"
	}
	$arguments += @("-bios", $BiosPath)
}

$process = Start-Process `
	-FilePath $XemuPath `
	-ArgumentList $arguments `
	-RedirectStandardOutput $StdoutPath `
	-RedirectStandardError $StderrPath `
	-PassThru

Start-Sleep -Seconds $TimeoutSeconds

if ($process.HasExited) {
	if ($process.ExitCode -ne 0) {
		throw "xemu exited with status $($process.ExitCode)"
	}
} else {
	Stop-Process -Id $process.Id -Force
}

if (!$process.WaitForExit(5000)) {
	throw "xemu did not exit after audio capture stop request"
}
$process.Dispose()
Start-Sleep -Milliseconds 500

$combined = ""
if (Test-Path -LiteralPath $StdoutPath) {
	$combined += Get-Content -LiteralPath $StdoutPath -Raw
}
if (Test-Path -LiteralPath $StderrPath) {
	$combined += Get-Content -LiteralPath $StderrPath -Raw
}

if ($combined -match "Failed to load DVD image|failed to load DVD image") {
	throw "xemu could not load the generated XISO"
}
if ($combined -match "(?i)failed to load (bios|mcpx|boot rom|flash rom|hard disk|hdd)") {
	throw "xemu reported missing or invalid configured Xbox boot assets"
}

if (!(Test-Path -LiteralPath $WavPath)) {
	throw "xemu did not create the WAV capture"
}

$bytes = $null
$lastReadError = $null

for ($attempt = 0; $attempt -lt 20; $attempt++) {
	try {
		$bytes = [IO.File]::ReadAllBytes($WavPath)
		break
	} catch [System.IO.IOException] {
		$lastReadError = $_.Exception.Message
		Start-Sleep -Milliseconds 250
	}
}

if ($null -eq $bytes) {
	throw "could not read xemu WAV capture after waiting for it to close: $lastReadError"
}

if ($bytes.Length -lt 48) {
	throw "WAV capture is too small"
}

$channels = Read-UInt16LE $bytes 22
$rate = Read-UInt32LE $bytes 24
$bits = Read-UInt16LE $bytes 34
$dataChunk = Find-DataChunk $bytes
$dataStart = $dataChunk + 8
$sampleBytes = $bytes.Length - $dataStart

if (($channels -lt 1) -or ($bits -ne 16) -or ($rate -lt 8000)) {
	throw "unexpected WAV format: rate=$rate channels=$channels bits=$bits"
}
if ($sampleBytes -lt ($channels * 2 * 4096)) {
	throw "WAV capture did not contain enough sample data"
}

$frames = [Math]::Floor($sampleBytes / (2 * $channels))
$mono = New-Object 'double[]' $frames

for ($frame = 0; $frame -lt $frames; $frame++) {
	$sum = 0.0
	for ($channel = 0; $channel -lt $channels; $channel++) {
		$offset = $dataStart + (($frame * $channels + $channel) * 2)
		$value = [BitConverter]::ToInt16($bytes, $offset)
		$sum += [double]$value
	}
	$mono[$frame] = $sum / [double]$channels
}

$window = 2048
$activeThreshold = 200.0
$lowThreshold = 500.0
$activeStartWindow = -1
$activeEndWindow = -1
$activeWindows = 0
$windowCount = [Math]::Floor($frames / $window)

for ($w = 0; $w -lt $windowCount; $w++) {
	$rms = Get-Rms $mono ($w * $window) $window
	if ($rms -gt $activeThreshold) {
		if ($activeStartWindow -lt 0) {
			$activeStartWindow = $w
		}
		$activeEndWindow = $w
		$activeWindows++
	}
}

if ($activeStartWindow -lt 0) {
	throw "WAV capture is silent"
}

$activeStart = $activeStartWindow * $window
$activeEnd = ($activeEndWindow + 1) * $window
$activeFrames = $activeEnd - $activeStart
$activeSeconds = [double]$activeFrames / [double]$rate

if ($activeSeconds -lt 0.40) {
	throw ("active audio was too short: {0:N3}s" -f $activeSeconds)
}

$lowWindows = 0
$interiorWindows = 0
for ($start = $activeStart + $window; $start + 1024 -le $activeEnd - $window; $start += 1024) {
	$interiorWindows++
	if ((Get-Rms $mono $start 1024) -lt $lowThreshold) {
		$lowWindows++
	}
}

if ($lowWindows -gt 0) {
	throw "active audio contained $lowWindows low-energy interior window(s)"
}

$toneFrames = [Math]::Floor([Math]::Min($rate * 0.16, $activeFrames / 3))
if ($toneFrames -lt 2048) {
	throw "active audio is too short for tone analysis"
}

$firstCenter = $activeStart + [Math]::Floor($activeFrames * 0.25)
$secondCenter = $activeStart + [Math]::Floor($activeFrames * 0.75)
$firstStart = [Math]::Max($activeStart, $firstCenter - [Math]::Floor($toneFrames / 2))
$secondStart = [Math]::Min($activeEnd - $toneFrames, $secondCenter - [Math]::Floor($toneFrames / 2))

$first440 = Get-BandPower $mono $firstStart $toneFrames $rate 440.0
$first660 = Get-BandPower $mono $firstStart $toneFrames $rate 660.0
$second440 = Get-BandPower $mono $secondStart $toneFrames $rate 440.0
$second660 = Get-BandPower $mono $secondStart $toneFrames $rate 660.0

if (($first440 -lt 500.0) -or ($first440 -lt ($first660 * 4.0))) {
	throw ("first tone did not look like 440 Hz: 440={0:N1} 660={1:N1}" -f $first440, $first660)
}
if (($second660 -lt 500.0) -or ($second660 -lt ($second440 * 4.0))) {
	throw ("second tone did not look like 660 Hz: 440={0:N1} 660={1:N1}" -f $second440, $second660)
}

Write-Output ("xemu audio capture passed: rate={0}Hz channels={1} active={2:N3}s windows={3}" -f $rate, $channels, $activeSeconds, $activeWindows)
Write-Output ("first tone bands: 440={0:N1} 660={1:N1}" -f $first440, $first660)
Write-Output ("second tone bands: 440={0:N1} 660={1:N1}" -f $second440, $second660)
Write-Output ("continuity: {0} low-energy interior windows out of {1}" -f $lowWindows, $interiorWindows)
Write-Output ("wav: {0}" -f $WavPath)
EOF
}

validate_sfx_audio() {
	local ps1="$CMD_DIR/validate-xemu-audio.ps1"
	local iso="$ISO_DIR/sfx.iso"
	local wav="$LOG_DIR/sfx.xemu.wav"
	local log="$LOG_DIR/sfx.audio.log"
	local stdout_log="$LOG_DIR/sfx.audio.stdout.log"
	local stderr_log="$LOG_DIR/sfx.audio.stderr.log"
	local config_win=""
	local bios_win=""

	[ -f "$iso" ] || fail "missing XISO: $iso"

	if [ -n "$XEMU_CONFIG" ]; then
		config_win="$(cygpath_abs_win "$XEMU_CONFIG")"
	fi
	if [ -n "$XEMU_BIOS" ]; then
		bios_win="$(cygpath_abs_win "$XEMU_BIOS")"
	fi

	write_audio_validation_script "$ps1"

	msg "capturing and validating sfx.iso audio in xemu"
	if ! powershell.exe -NoProfile -ExecutionPolicy Bypass \
		-File "$(cygpath_abs_win "$ps1")" \
		-XemuPath "$(cygpath_abs_win "$XEMU_EXE")" \
		-ConfigPath "$config_win" \
		-BiosPath "$bios_win" \
		-IsoPath "$(cygpath_abs_win "$iso")" \
		-WavPath "$(cygpath_abs_win "$wav")" \
		-StdoutPath "$(cygpath_abs_win "$stdout_log")" \
		-StderrPath "$(cygpath_abs_win "$stderr_log")" \
		-TimeoutSeconds "$AUDIO_TIMEOUT" > "$log" 2>&1; then
		cat "$log" >&2 || true
		cat "$stdout_log" >&2 || true
		cat "$stderr_log" >&2 || true
		fail "xemu audio validation failed for sfx.iso"
	fi

	cat "$log"
}

##############################################################################
# Distribution and tool detection
##############################################################################

if [ -z "$DIST_DIR" ]; then
	DIST_DIR="$(first_existing_dir \
		"/tmp/freebasic-xbox-build/dist/FreeBASIC-1.20.1-fbc-xbox" \
		"$ROOT/out/mingw32-xbox/FreeBASIC-1.20.1-fbc-xbox" \
		"/c/freebasic-xbox" \
		"/c/FreeBASIC-xbox" \
		|| true)"
fi

[ -n "$DIST_DIR" ] || fail "could not locate fbc-xbox distribution; pass --dist-dir"
DIST_DIR="$(CDPATH= cd -- "$DIST_DIR" && pwd)"

[ -f "$DIST_DIR/fbc-xbox.cmd" ] || fail "missing fbc-xbox.cmd in $DIST_DIR"
[ -f "$DIST_DIR/bin/fbc.exe" ] || fail "missing bin/fbc.exe in $DIST_DIR"

EXTRACT_XISO="$(find_extract_xiso || true)"
[ -n "$EXTRACT_XISO" ] || fail "could not locate extract-xiso; rebuild the fbc-xbox package"
EXTRACT_XISO="$(cygpath_abs_msys "$EXTRACT_XISO")"

if [ -n "$XEMU_CONFIG" ]; then
	XEMU_CONFIG="$(cygpath_abs_msys "$XEMU_CONFIG")"
	[ -f "$XEMU_CONFIG" ] || fail "xemu config not found: $XEMU_CONFIG"
fi

if [ "$SKIP_LAUNCH" -eq 0 ] || [ "$VALIDATE_AUDIO" -eq 1 ]; then
	XEMU_EXE="$(detect_xemu || true)"
	if [ -z "$XEMU_EXE" ]; then
		if [ "$REQUIRE_XEMU" -eq 1 ] || [ "$VALIDATE_AUDIO" -eq 1 ]; then
			fail "xemu was not found; pass --xemu or set XEMU"
		fi
		echo ""
		echo "==> xemu was not found; XBE/XISO build will run and launch will be skipped"
		SKIP_LAUNCH=1
	else
		XEMU_EXE="$(cygpath_abs_msys "$XEMU_EXE")"
	fi
fi

have cmd.exe || fail "cmd.exe was not found"
have powershell.exe || fail "powershell.exe was not found"
have cygpath || fail "cygpath was not found"

##############################################################################
# Build, pack, and launch
##############################################################################

SRC_DIR="$WORKROOT/src"
XBE_DIR="$WORKROOT/xbe"
ISO_STAGE_DIR="$WORKROOT/xiso-stage"
ISO_DIR="$WORKROOT/xiso"
CMD_DIR="$WORKROOT/cmd"
LOG_DIR="$WORKROOT/logs"

rm -rf "$WORKROOT"
mkdir -p "$SRC_DIR" "$XBE_DIR" "$ISO_STAGE_DIR" "$ISO_DIR" "$CMD_DIR" "$LOG_DIR"

detect_default_xemu_assets
prepare_generated_xemu_config
write_sources

for name in console gfx-screenres gfx-screen13 sfx fileio; do
	compile_xbe "$name"
	pack_xiso "$name"
done

echo ""
echo "==> XBE artifacts:"
find "$XBE_DIR" -maxdepth 1 -type f -name '*.xbe' -print | sort

echo ""
echo "==> XISO artifacts:"
find "$ISO_DIR" -maxdepth 1 -type f -name '*.iso' -print | sort

if [ "$SKIP_LAUNCH" -eq 1 ]; then
	echo "==> xemu launch phase skipped"
	echo "==> workroot kept for compiled XBEs and XISOs: $WORKROOT"
	exit 0
fi

IFS=',' read -r -a LAUNCH_LIST <<< "$LAUNCH_PROGRAMS"
for name in "${LAUNCH_LIST[@]}"; do
	[ -n "$name" ] || continue
	launch_xiso "$name"
done

if [ "$VALIDATE_AUDIO" -eq 1 ]; then
	validate_sfx_audio
fi

if [ "$KEEP_WORKROOT" -eq 0 ]; then
	echo "==> workroot kept for launch logs, XBEs, and XISOs: $WORKROOT"
else
	echo "==> workroot kept: $WORKROOT"
fi

echo "freebasic-xbox xemu smoke test completed"

# end of msys2-test-freebasic-xbox-xemu.sh
