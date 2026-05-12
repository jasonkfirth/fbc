#!/usr/bin/env bash

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# msys2-test-freebasic-xbox-cxbx.sh
#
# Build small FreeBASIC Xbox XBE smoke programs and launch them through the
# local Cxbx-Reloaded checkout.
#
# Responsibilities:
#   - locate a packaged fbc-xbox distribution
#   - compile console, gfxlib, sfxlib, and file I/O smoke XBEs
#   - launch Cxbx-Reloaded through cxbxr-ldr with the XBE path form it accepts
#   - keep build and launch logs in a repeatable work directory
#
# This file intentionally does NOT contain:
#   - the fbc-xbox package build itself
#   - full fbcunit execution
#   - pixel or audio capture analysis
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
EMULATOR_DIR="${EMULATOR_DIR:-$ROOT/xbox_emulator}"
WORKROOT="${WORKROOT:-$ROOT/.build-msys2/freebasic-xbox-cxbx-test}"
KEEP_WORKROOT=0
SKIP_LAUNCH=0
LAUNCH_TIMEOUT="${LAUNCH_TIMEOUT:-12}"
LAUNCH_PROGRAMS="${LAUNCH_PROGRAMS:-console}"

usage() {
	cat <<EOF
Usage: ./build_scripts/msys2-test-freebasic-xbox-cxbx.sh [options]

Options:
  --dist-dir DIR       FreeBASIC fbc-xbox distribution directory
  --emulator-dir DIR   Cxbx-Reloaded directory (default: ./xbox_emulator)
  --workroot DIR       Test work directory
  --keep-workroot      Keep the generated smoke test directory
  --skip-launch        Only compile XBEs, do not start Cxbx-Reloaded
  --launch-timeout N   Seconds to leave each XBE running (default: 12)
  --launch-programs L  Comma-separated XBE names to launch (default: console)
  --help              Show this help text

The script always compiles console, gfx-screenres, gfx-screen13, sfx, and fileio
smoke XBEs. Cxbx-Reloaded currently expects /load to name a file in its process
working directory, so the launch step copies the selected XBE into the emulator
directory and runs:

    cxbxr-ldr.exe /load freebasic-cxbx-smoke-<name>.xbe
EOF
}

while [ $# -gt 0 ]; do
	case "$1" in
		--dist-dir)
			DIST_DIR="$2"
			shift 2
			;;
		--emulator-dir)
			EMULATOR_DIR="$2"
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
		--launch-timeout)
			LAUNCH_TIMEOUT="$2"
			shift 2
			;;
		--launch-programs)
			LAUNCH_PROGRAMS="$2"
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

cygpath_abs_win() {
	cygpath -aw "$1"
}

cygpath_abs_msys() {
	cygpath -au "$1"
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
screenset 0, 1

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

write_launch_script() {
	local ps1="$1"

	cat > "$ps1" <<'EOF'
param(
	[string] $EmulatorDir,
	[string] $XbePath,
	[string] $XbeName,
	[string] $StdoutPath,
	[string] $StderrPath,
	[int] $TimeoutSeconds
)

$ErrorActionPreference = "Stop"

$cxbx = Join-Path $EmulatorDir "cxbxr-ldr.exe"
if (!(Test-Path -LiteralPath $cxbx)) {
	throw "cxbxr-ldr.exe was not found in $EmulatorDir"
}

$stagedXbe = Join-Path $EmulatorDir $XbeName
Copy-Item -LiteralPath $XbePath -Destination $stagedXbe -Force

try {
	$process = Start-Process `
		-FilePath $cxbx `
		-ArgumentList @("/load", $XbeName) `
		-WorkingDirectory $EmulatorDir `
		-RedirectStandardOutput $StdoutPath `
		-RedirectStandardError $StderrPath `
		-PassThru

	Start-Sleep -Seconds $TimeoutSeconds

	if ($process.HasExited) {
		if ($process.ExitCode -ne 0) {
			throw "Cxbx-Reloaded exited with status $($process.ExitCode)"
		}
		Write-Output "Cxbx-Reloaded exited cleanly after loading $XbeName"
	} else {
		Stop-Process -Id $process.Id -Force
		Write-Output "Cxbx-Reloaded accepted $XbeName and stayed open for $TimeoutSeconds seconds"
	}

	$stdout = ""
	if (Test-Path -LiteralPath $StdoutPath) {
		$stdout = Get-Content -LiteralPath $StdoutPath -Raw
	}

	if ($stdout -match "No /load in command line") {
		throw "Cxbx-Reloaded did not receive /load"
	}
	if ($stdout -match "Could not open the Xbe file") {
		throw "Cxbx-Reloaded could not open $XbeName"
	}
	if ($stdout -match "Emulation must be launched from cxbxr-ldr.exe") {
		throw "Cxbx-Reloaded was launched through the wrong executable"
	}
	if ($stdout -notmatch "Xbe::Xbe: Opening Xbe file\.\.\.OK") {
		throw "Cxbx-Reloaded did not report opening $XbeName"
	}
} finally {
	Remove-Item -LiteralPath $stagedXbe -Force -ErrorAction SilentlyContinue
}
EOF
}

launch_xbe() {
	local name="$1"
	local ps1="$CMD_DIR/launch-cxbx.ps1"
	local xbe="$XBE_DIR/$name.xbe"
	local staged_name="freebasic-cxbx-smoke-$name.xbe"
	local log="$LOG_DIR/$name.cxbx.log"
	local stdout_log="$LOG_DIR/$name.cxbx.stdout.log"
	local stderr_log="$LOG_DIR/$name.cxbx.stderr.log"

	[ -f "$xbe" ] || fail "missing XBE: $xbe"

	write_launch_script "$ps1"

	msg "launching $name.xbe in Cxbx-Reloaded"
	if ! powershell.exe -NoProfile -ExecutionPolicy Bypass \
		-File "$(cygpath_abs_win "$ps1")" \
		-EmulatorDir "$(cygpath_abs_win "$EMULATOR_DIR")" \
		-XbePath "$(cygpath_abs_win "$xbe")" \
		-XbeName "$staged_name" \
		-StdoutPath "$(cygpath_abs_win "$stdout_log")" \
		-StderrPath "$(cygpath_abs_win "$stderr_log")" \
		-TimeoutSeconds "$LAUNCH_TIMEOUT" > "$log" 2>&1; then
		cat "$log" >&2 || true
		cat "$stdout_log" >&2 || true
		cat "$stderr_log" >&2 || true
		fail "Cxbx-Reloaded launch failed for $name.xbe"
	fi

	cat "$log"
}

##############################################################################
# Distribution and emulator detection
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

EMULATOR_DIR="$(cygpath_abs_msys "$EMULATOR_DIR")"
[ -d "$EMULATOR_DIR" ] || fail "Cxbx-Reloaded directory not found: $EMULATOR_DIR"
[ -f "$EMULATOR_DIR/cxbxr-ldr.exe" ] || fail "missing cxbxr-ldr.exe in $EMULATOR_DIR"

have cmd.exe || fail "cmd.exe was not found"
have powershell.exe || fail "powershell.exe was not found"
have cygpath || fail "cygpath was not found"

##############################################################################
# Build and launch
##############################################################################

SRC_DIR="$WORKROOT/src"
XBE_DIR="$WORKROOT/xbe"
CMD_DIR="$WORKROOT/cmd"
LOG_DIR="$WORKROOT/logs"

rm -rf "$WORKROOT"
mkdir -p "$SRC_DIR" "$XBE_DIR" "$CMD_DIR" "$LOG_DIR"

write_sources

for name in console gfx-screenres gfx-screen13 sfx fileio; do
	compile_xbe "$name"
done

echo ""
echo "==> XBE artifacts:"
find "$XBE_DIR" -maxdepth 1 -type f -name '*.xbe' -print | sort

if [ "$SKIP_LAUNCH" -eq 1 ]; then
	echo "==> Cxbx launch phase skipped by request"
	if [ "$KEEP_WORKROOT" -eq 0 ]; then
		echo "==> workroot kept for compiled XBEs: $WORKROOT"
	fi
	exit 0
fi

IFS=',' read -r -a LAUNCH_LIST <<< "$LAUNCH_PROGRAMS"
for name in "${LAUNCH_LIST[@]}"; do
	[ -n "$name" ] || continue
	launch_xbe "$name"
done

if [ "$KEEP_WORKROOT" -eq 0 ]; then
	echo "==> workroot kept for launch logs and XBEs: $WORKROOT"
else
	echo "==> workroot kept: $WORKROOT"
fi

echo "freebasic-xbox Cxbx smoke test completed"

# end of msys2-test-freebasic-xbox-cxbx.sh
