#!/usr/bin/env bash

set -euo pipefail

trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

run() { echo "==> $*"; "$@"; }
die() { echo "ERROR: $*" >&2; exit 1; }
have() { command -v "$1" >/dev/null 2>&1; }
msg() { echo ""; echo "==> $1"; }

run_root() {
	if [ "$(id -u)" -eq 0 ]; then
		run "$@"
	elif have sudo; then
		run sudo "$@"
	else
		die "this step requires administrator privileges; rerun as root or install sudo"
	fi
}

usage() {
	cat <<'EOF'
Usage: ./build_scripts/msdos-test-freebasic.sh [options]

Build a DOS test hard disk image from the packaged out/msdos distribution,
compile test programs inside DOS using the packaged compiler, and verify:
  - hello-world console compile/run
  - SCREEN 13 gfxlib compile/run
  - Sound Blaster sound/MIDI test
  - no-BLASTER PC speaker fallback test

Options:
  --skip-deps      Skip host dependency installation
  --keep-workdir   Reuse the existing work directory instead of deleting it first
  --help           Show this help

Environment:
  OUT               DOS package output root (default: <repo>/out/msdos)
  PKGDIR            DOS package directory to test (default: latest FreeBASIC-*-dos in OUT)
  WORKDIR           Working directory for test artifacts (default: /tmp/fbdos-test)
  DOSBOX_BIN        DOSBox-X executable to use (default: dosbox-x from PATH)
  DOSBOX_TIMEOUT    DOSBox-X timeout in seconds (default: 120)
  DOSBOX_CAPTURE_DIR Directory for captured WAVs (default: <workdir>/capture)
EOF
}

##############################################################################
# Locate project root
##############################################################################

START_DIR="$(pwd)"
SEARCH_DIR="$START_DIR"
ROOT=""

while :; do
	if [ -d "$SEARCH_DIR/mk" ] && [ -f "$SEARCH_DIR/GNUmakefile" ]; then
		ROOT="$SEARCH_DIR"
		break
	fi
	[ "$SEARCH_DIR" = "/" ] && break
	SEARCH_DIR="$(dirname "$SEARCH_DIR")"
done

[ -n "$ROOT" ] || die "could not locate FreeBASIC root"

cd "$ROOT"

##############################################################################
# Configuration
##############################################################################

DO_DEPS=1
KEEP_WORKDIR=0

for arg in "$@"; do
	case "$arg" in
		--skip-deps) DO_DEPS=0 ;;
		--keep-workdir) KEEP_WORKDIR=1 ;;
		--help)
			usage
			exit 0
			;;
		*)
			die "unknown option: $arg"
			;;
	esac
done

HOST_UNAME="$(uname -s)"
case "$HOST_UNAME" in
	Linux) ;;
	*)
		die "this script currently supports Linux hosts only"
		;;
esac

OUT="${OUT:-$ROOT/out/msdos}"
WORKDIR="${WORKDIR:-/tmp/fbdos-test}"
DOSBOX_TIMEOUT="${DOSBOX_TIMEOUT:-120}"
DOSBOX_CAPTURE_DIR="${DOSBOX_CAPTURE_DIR:-$WORKDIR/capture}"

TEST_ROOT="$WORKDIR/root"
CONF_DIR="$WORKDIR/conf"
DOSBOX_CONF="$WORKDIR/dosbox-x.conf"
IMAGE_FILE="$WORKDIR/freebasic-test.img"
RUN_BAT="$TEST_ROOT/RUNTESTS.BAT"
TRACE_LOG="$TEST_ROOT/TRACE.LOG"
HELLO_LOG="$TEST_ROOT/HELLO.LOG"
HELLO_TXT="$TEST_ROOT/HELLO.TXT"
GFX_LOG="$TEST_ROOT/GFX13.LOG"
GFX_TXT="$TEST_ROOT/GFX13.TXT"
SOUND_LOG="$TEST_ROOT/SOUND.LOG"
SOUND_TXT="$TEST_ROOT/SOUND.TXT"
NOBLAST_LOG="$TEST_ROOT/NOBLAST.LOG"
NOBLAST_TXT="$TEST_ROOT/NOBLAST.TXT"
IMGMAKE_LOG="$WORKDIR/imgmake.log"
DOSBOX_RUN_LOG="$WORKDIR/dosbox-run.log"

##############################################################################
# Helpers
##############################################################################

find_latest_pkg() {
	find "$OUT" -maxdepth 1 -mindepth 1 -type d -name 'FreeBASIC-*-dos' | sort | tail -n1
}

prepare_dos_runtime_layout() {
	local root="$1"
	local compat_libdir="$root/lib/dos"
	local legacy_libdir="$root/lib/freebas/dos"
	local host_libdir="$root/lib/freebasic/dos"
	local djgpp_ldscript="$root/djgpp/lib/ldscripts/i386go32.x"

	[ -d "$compat_libdir" ] || return 0

	mkdir -p "$(dirname "$legacy_libdir")"
	mkdir -p "$(dirname "$host_libdir")"
	rm -rf "$legacy_libdir"
	rm -rf "$host_libdir"
	cp -a "$compat_libdir" "$legacy_libdir"
	cp -a "$compat_libdir" "$host_libdir"

	if [ -f "$djgpp_ldscript" ]; then
		cp -f "$djgpp_ldscript" "$legacy_libdir/i386go32.x"
		cp -f "$djgpp_ldscript" "$compat_libdir/i386go32.x"
		cp -f "$djgpp_ldscript" "$host_libdir/i386go32.x"
	fi
}

require_tool() {
	have "$1" || die "required tool not found: $1"
}

require_log_line() {
	local file="$1"
	local pattern="$2"
	local description="$3"

	grep -Eq "$pattern" "$file" || die "$description not found in $file"
}

find_captured_wavs() {
	find "$DOSBOX_CAPTURE_DIR" -maxdepth 1 -type f -name '*.wav' -printf '%T@ %p\n' | \
		sort -n | \
		sed 's/^[^ ]* //'
}

validate_wav_audio() {
	local wav="$1"
	local first_nonzero

	first_nonzero="$(
		od -v -An -t d2 -j 44 "$wav" |
			awk '{
				for (i = 1; i <= NF; ++i) {
					if ($i ~ /^-?[0-9]+$/ && $i != 0) {
						print $i
						exit
					}
				}
			}'
	)"

	[ -n "$first_nonzero" ] || die "captured WAV appears silent: $wav"
	echo "$first_nonzero"
}

install_linux_dependencies() {
	msg "updating APT package database"
	run_root apt-get update

	msg "installing Linux DOS test dependencies"
	run_root env DEBIAN_FRONTEND=noninteractive apt-get install -y \
		dosbox-x \
		file \
		mtools \
		rsync \
		util-linux
}

##############################################################################
# Resolve inputs
##############################################################################

if [ "$DO_DEPS" = "1" ]; then
	install_linux_dependencies
fi

PKGDIR="${PKGDIR:-$(find_latest_pkg)}"
[ -n "${PKGDIR:-}" ] || die "no DOS package found in $OUT; run ./build_scripts/msdos-build-freebasic.sh first"
[ -d "$PKGDIR" ] || die "missing DOS package directory: $PKGDIR"

DOSBOX_BIN="${DOSBOX_BIN:-$(command -v dosbox-x || true)}"
[ -n "$DOSBOX_BIN" ] || die "dosbox-x not found"

require_tool mcopy
require_tool sfdisk
require_tool rsync
require_tool timeout
require_tool od
require_tool file
require_tool awk

##############################################################################
# Prepare work area
##############################################################################

if [ "$KEEP_WORKDIR" != "1" ]; then
	rm -rf "$WORKDIR"
fi

mkdir -p "$TEST_ROOT" "$CONF_DIR" "$DOSBOX_CAPTURE_DIR"
rm -f "$IMAGE_FILE" "$TRACE_LOG" "$HELLO_LOG" "$HELLO_TXT" "$GFX_LOG" "$GFX_TXT" \
	"$SOUND_LOG" "$SOUND_TXT" "$NOBLAST_LOG" "$NOBLAST_TXT" "$IMGMAKE_LOG" "$DOSBOX_RUN_LOG"
rm -f "$DOSBOX_CAPTURE_DIR"/*.wav

msg "staging DOS package"
run rsync -a "$PKGDIR"/ "$TEST_ROOT"/
prepare_dos_runtime_layout "$TEST_ROOT"

cat > "$DOSBOX_CONF" <<EOF
[dosbox]
captures = $DOSBOX_CAPTURE_DIR

[midi]
mididevice = default

[sblaster]
sbtype = sb16
sbbase = 220
irq = 7
dma = 1
hdma = 5
oplmode = auto
oplemu = default

[speaker]
pcspeaker = true
EOF

cat > "$TEST_ROOT/HELLO.BAS" <<'EOF'
print "hello-start"
print "Hello from DOS FreeBASIC"
open "C:\HELLO.TXT" for output as #1
print #1, "hello-done"
close #1
print "hello-end"
EOF

cat > "$TEST_ROOT/GFX13.BAS" <<'EOF'
screen 13
color 15, 1
cls
line (20, 20)-(120, 80), 12, bf
circle (160, 100), 30, 14
pset (10, 10), 15
sleep 200
screen 0
print "gfx13-start"
open "C:\GFX13.TXT" for output as #1
print #1, "gfx13-done"
close #1
print "gfx13-end"
EOF

cat > "$TEST_ROOT/SOUND.BAS" <<'EOF'
dim as long result

print "sound-start"
DEVICE LIST
DEVICE INFO

result = DEVICE SELECT(0)
print "device-select="; result

print "play-begin"
PLAY "T120 O4 L8 CDEFGAB>C"
print "play-end"

print "sound-begin"
SOUND 440, 0.35
sleep 400
print "sound-end"

result = MIDI OPEN(0)
print "midi-open="; result
if result = 0 then
	result = MIDI SEND(&H90, 60, 100)
	print "midi-send-on="; result
	sleep 300
	result = MIDI SEND(&H80, 60, 0)
	print "midi-send-off="; result
	MIDI CLOSE
end if

open "C:\SOUND.TXT" for output as #1
print #1, "sound-done"
close #1

print "sound-end-marker"
EOF

cat > "$TEST_ROOT/NOBLAST.BAS" <<'EOF'
print "noblast-start"
DEVICE LIST
DEVICE INFO

print "play-begin"
PLAY "T120 O4 L8 CEG>C"
print "play-end"

print "sound-begin"
SOUND 523, 0.35
sleep 400
print "sound-end"

open "C:\NOBLAST.TXT" for output as #1
print #1, "noblast-done"
close #1

print "noblast-end"
EOF

cat > "$RUN_BAT" <<'EOF'
@echo off
echo begin>D:\TRACE.LOG
set DJGPP=C:\DJGPP\DJGPP.ENV
echo djgpp=%DJGPP%>>D:\TRACE.LOG
set PATH=C:\FB;C:\DJGPP\BIN;%PATH%
echo path=%PATH%>>D:\TRACE.LOG
echo blaster=%BLASTER%>>D:\TRACE.LOG
if not exist C:\FB\FBC.EXE echo missing-fbc>>D:\TRACE.LOG
if not exist C:\DJGPP\BIN\REDIR.EXE echo missing-redir>>D:\TRACE.LOG
C:\DJGPP\BIN\CWSDPMI.EXE -p >>D:\TRACE.LOG
echo cwsdpmi-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG
set SFXLIB_DEBUG=1

C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD-HELLO.LOG C:\FB\FBC.EXE C:\HELLO.BAS -x C:\HELLO.EXE
echo build-hello-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG
if exist C:\HELLO.EXE C:\DJGPP\BIN\REDIR.EXE -eo -o D:\HELLO.LOG C:\HELLO.EXE
echo hello-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG
if exist C:\HELLO.TXT copy C:\HELLO.TXT D:\HELLO.TXT >NUL

C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD-GFX13.LOG C:\FB\FBC.EXE C:\GFX13.BAS -x C:\GFX13.EXE
echo build-gfx13-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG
if exist C:\GFX13.EXE C:\DJGPP\BIN\REDIR.EXE -eo -o D:\GFX13.LOG C:\GFX13.EXE
echo gfx13-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG
if exist C:\GFX13.TXT copy C:\GFX13.TXT D:\GFX13.TXT >NUL

C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD-SOUND.LOG C:\FB\FBC.EXE C:\SOUND.BAS -x C:\SOUND.EXE
echo build-sound-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG
DX-CAPTURE /A C:\DJGPP\BIN\REDIR.EXE -eo -o D:\SOUND.LOG C:\SOUND.EXE
echo sound-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG
if exist C:\SOUND.TXT copy C:\SOUND.TXT D:\SOUND.TXT >NUL

C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD-NOBLAST.LOG C:\FB\FBC.EXE C:\NOBLAST.BAS -x C:\NOBLAST.EXE
echo build-noblast-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG
set BLASTER=
echo cleared-blaster=%BLASTER%>>D:\TRACE.LOG
DX-CAPTURE /A C:\DJGPP\BIN\REDIR.EXE -eo -o D:\NOBLAST.LOG C:\NOBLAST.EXE
echo noblast-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG
if exist C:\NOBLAST.TXT copy C:\NOBLAST.TXT D:\NOBLAST.TXT >NUL
EOF

##############################################################################
# Build image and run DOSBox-X
##############################################################################

msg "creating DOS test image"
run "$DOSBOX_BIN" \
	-conf "$DOSBOX_CONF" \
	-fastlaunch \
	-nogui \
	-nomenu \
	-exit \
	-set "cpu cputype=ppro_slow" \
	-c "imgmake \"$IMAGE_FILE\" -t hd -size 256 -fat 16" \
	-c "exit" \
	>"$IMGMAKE_LOG" 2>&1

PARTITION_START="$(sfdisk -d "$IMAGE_FILE" | sed -n 's/.*start= *\([0-9][0-9]*\).*/\1/p' | head -n1)"
[ -n "$PARTITION_START" ] || die "could not determine image partition start"
PARTITION_OFFSET="$((PARTITION_START * 512))"

msg "copying DOS payload into image"
run env MTOOLS_SKIP_CHECK=1 mcopy -i "${IMAGE_FILE}@@${PARTITION_OFFSET}" -s "$TEST_ROOT"/* ::

msg "running DOSBox-X FreeBASIC test suite"
run timeout "$DOSBOX_TIMEOUT" "$DOSBOX_BIN" \
	-conf "$DOSBOX_CONF" \
	-fastlaunch \
	-nogui \
	-nomenu \
	-exit \
	-set "cpu cputype=ppro_slow" \
	-c "mount d \"$TEST_ROOT\"" \
	-c "imgmount c \"$IMAGE_FILE\"" \
	-c "c:" \
	-c "RUNTESTS.BAT" \
	-c "exit" \
	>"$DOSBOX_RUN_LOG" 2>&1

##############################################################################
# Validate results
##############################################################################

[ -f "$TRACE_LOG" ] || die "missing trace log: $TRACE_LOG"
[ -f "$HELLO_LOG" ] || die "missing hello log: $HELLO_LOG"
[ -f "$HELLO_TXT" ] || die "missing hello result file: $HELLO_TXT"
[ -f "$GFX_LOG" ] || die "missing gfx log: $GFX_LOG"
[ -f "$GFX_TXT" ] || die "missing gfx result file: $GFX_TXT"
[ -f "$SOUND_LOG" ] || die "missing sound log: $SOUND_LOG"
[ -f "$SOUND_TXT" ] || die "missing sound result file: $SOUND_TXT"
[ -f "$NOBLAST_LOG" ] || die "missing no-BLASTER log: $NOBLAST_LOG"
[ -f "$NOBLAST_TXT" ] || die "missing no-BLASTER result file: $NOBLAST_TXT"

require_log_line "$HELLO_LOG" 'Hello from DOS FreeBASIC' "hello-world output"
require_log_line "$HELLO_TXT" 'hello-done' "hello-world completion file"
require_log_line "$GFX_LOG" 'gfx13-end' "gfx SCREEN 13 completion marker"
require_log_line "$GFX_TXT" 'gfx13-done' "gfx SCREEN 13 completion file"
require_log_line "$SOUND_LOG" 'play-end' "Sound Blaster PLAY completion marker"
require_log_line "$SOUND_LOG" 'sound-end' "Sound Blaster SOUND completion marker"
require_log_line "$SOUND_LOG" 'device-select=[[:space:]]*0' "Sound Blaster DEVICE SELECT success"
require_log_line "$SOUND_LOG" 'midi-open=[[:space:]]*0' "Sound Blaster MIDI OPEN success"
require_log_line "$SOUND_LOG" 'midi-send-on=[[:space:]]*0' "Sound Blaster MIDI note-on success"
require_log_line "$SOUND_LOG" 'midi-send-off=[[:space:]]*0' "Sound Blaster MIDI note-off success"
require_log_line "$SOUND_TXT" 'sound-done' "Sound Blaster completion file"
require_log_line "$NOBLAST_LOG" 'play-end' "no-BLASTER PLAY completion marker"
require_log_line "$NOBLAST_LOG" 'sound-end' "no-BLASTER SOUND completion marker"
require_log_line "$NOBLAST_TXT" 'noblast-done' "no-BLASTER completion file"
require_log_line "$TRACE_LOG" 'cleared-blaster=' "BLASTER cleared trace marker"

mapfile -t CAPTURE_WAVS < <(find_captured_wavs)
[ "${#CAPTURE_WAVS[@]}" -ge 2 ] || die "expected at least two captured WAVs in $DOSBOX_CAPTURE_DIR"
SB_CAPTURE_WAV="${CAPTURE_WAVS[0]}"
PCSPK_CAPTURE_WAV="${CAPTURE_WAVS[1]}"
SB_FIRST_NONZERO_SAMPLE="$(validate_wav_audio "$SB_CAPTURE_WAV")"
PCSPK_FIRST_NONZERO_SAMPLE="$(validate_wav_audio "$PCSPK_CAPTURE_WAV")"

msg "FreeBASIC DOS test suite passed"
echo "Package: $PKGDIR"
echo "Trace log: $TRACE_LOG"
echo "Hello log: $HELLO_LOG"
echo "GFX log: $GFX_LOG"
echo "Sound log: $SOUND_LOG"
echo "No-BLASTER log: $NOBLAST_LOG"
echo "Sound Blaster capture: $SB_CAPTURE_WAV"
echo "Sound Blaster first non-zero sample: $SB_FIRST_NONZERO_SAMPLE"
echo "PC speaker capture: $PCSPK_CAPTURE_WAV"
echo "PC speaker first non-zero sample: $PCSPK_FIRST_NONZERO_SAMPLE"
run file "$SB_CAPTURE_WAV"
run file "$PCSPK_CAPTURE_WAV"
