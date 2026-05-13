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
  - FFT, continuity, and spectral-purity checks for captured Sound Blaster and PC speaker tones
  - DOS sfxlib command and file playback coverage for BEEP, SOUND, NOTE, PLAY, TONE, NOISE,
    SFX, MUSIC, AUDIO, STREAM, WAV playback, and MP3 playback

Options:
  --skip-deps      Skip host dependency installation
  --keep-workdir   Reuse the existing work directory instead of deleting it first
  --help           Show this help

Environment:
  OUT               DOS package output root (default: <repo>/out/msdos)
  PKGDIR            DOS package directory to test (default: latest FreeBASIC-*-dos in OUT)
  WORKDIR           Working directory for test artifacts (default: /tmp/fbdos-test)
  DOSBOX_BIN        DOSBox-X executable to use (default: dosbox-x from PATH)
  DOSBOX_X_RELEASE_TAG DOSBox-X release tag for the portable Windows build
  DOSBOX_X_ASSET    DOSBox-X portable asset name
  DOSBOX_X_URL      DOSBox-X portable asset URL
  DOSBOX_X_ROOT     DOSBox-X download/cache directory
  DOSBOX_TIMEOUT    DOSBox-X timeout in seconds (default: 120)
  DOSBOX_CAPTURE_DIR Directory for captured WAVs (default: <workdir>/capture)
  FFMPEG_BIN        ffmpeg executable used to generate deterministic MP3 test media
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
HOST_KIND=""
case "$HOST_UNAME" in
	Linux)
		HOST_KIND="linux"
		;;
	MINGW*|MSYS*)
		HOST_KIND="msys2"
		;;
	*)
		die "unsupported host environment: $HOST_UNAME; use Linux or MSYS2/Windows"
		;;
esac

OUT="${OUT:-$ROOT/out/msdos}"
WORKDIR="${WORKDIR:-/tmp/fbdos-test}"
DOSBOX_TIMEOUT="${DOSBOX_TIMEOUT:-120}"
DOSBOX_CAPTURE_DIR="${DOSBOX_CAPTURE_DIR:-$WORKDIR/capture}"
DOSBOX_X_RELEASE_TAG="${DOSBOX_X_RELEASE_TAG:-dosbox-x-v2026.05.02-osfree}"
DOSBOX_X_ASSET="${DOSBOX_X_ASSET:-dosbox-x-mingw64-${DOSBOX_X_RELEASE_TAG}-portable.zip}"
DOSBOX_X_URL="${DOSBOX_X_URL:-https://github.com/joncampbell123/dosbox-x/releases/download/${DOSBOX_X_RELEASE_TAG}/${DOSBOX_X_ASSET}}"
DOSBOX_X_ROOT="${DOSBOX_X_ROOT:-$ROOT/.build-msdos/dosbox-x}"

if [ -z "${CURL_BIN+x}" ]; then
	if [ "$HOST_KIND" = "msys2" ] && [ -x /usr/bin/curl ]; then
		CURL_BIN="/usr/bin/curl"
	else
		CURL_BIN="curl"
	fi
fi

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
SBFFT_LOG="$TEST_ROOT/SBFFT.LOG"
SBFFT_TXT="$TEST_ROOT/SBFFT.TXT"
SBDUAL_LOG="$TEST_ROOT/SBDUAL.LOG"
SBDUAL_TXT="$TEST_ROOT/SBDUAL.TXT"
PCSPKFFT_LOG="$TEST_ROOT/PCSPKFFT.LOG"
PCSPKFFT_TXT="$TEST_ROOT/PCSPKFFT.TXT"
PCSPKDUAL_LOG="$TEST_ROOT/PCDUAL.LOG"
PCSPKDUAL_TXT="$TEST_ROOT/PCDUAL.TXT"
IMGMAKE_LOG="$WORKDIR/imgmake.log"
DOSBOX_RUN_LOG="$WORKDIR/dosbox-run.log"
SBFFT_REPORT="$WORKDIR/sbfft-audio.txt"
SBDUAL_REPORT="$WORKDIR/sbdual-audio.txt"
PCSPKFFT_REPORT="$WORKDIR/pcspkfft-audio.txt"
PCSPKDUAL_REPORT="$WORKDIR/pcspkdual-audio.txt"

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

configure_msys2_path() {
	if [ -d /mingw64/bin ] && [[ ":$PATH:" != *":/mingw64/bin:"* ]]; then
		export PATH="/mingw64/bin:$PATH"
	fi

	if [ -d /usr/bin ] && [[ ":$PATH:" != *":/usr/bin:"* ]]; then
		export PATH="/usr/bin:$PATH"
	fi
}

normalize_program_path() {
	local path="$1"

	if [ "$HOST_KIND" = "msys2" ] && have cygpath; then
		case "$path" in
			[A-Za-z]:[\\/]*)
				cygpath -u "$path"
				return 0
				;;
		esac
	fi

	printf '%s\n' "$path"
}

download_file() {
	local dst="$1"
	local url="$2"
	local tmp="${dst}.tmp"

	rm -f "$tmp"
	run "$CURL_BIN" -L --retry 3 --fail -o "$tmp" "$url"
	mv -f "$tmp" "$dst"
}

find_portable_dosbox_x() {
	local root="$1"
	local candidate

	for candidate in \
		"$root/portable/mingw-build/mingw/dosbox-x.exe" \
		"$root/portable/mingw-build/mingw-sdl2/dosbox-x.exe" \
		"$root/portable/dosbox-x.exe"
	do
		if [ -x "$candidate" ]; then
			printf '%s\n' "$candidate"
			return 0
		fi
	done

	return 1
}

prepare_portable_dosbox_x() {
	local archive
	local exe

	[ "$HOST_KIND" = "msys2" ] || return 1

	if exe="$(find_portable_dosbox_x "$DOSBOX_X_ROOT")"; then
		printf '%s\n' "$exe"
		return 0
	fi

	have unzip || die "unzip is required to unpack portable DOSBox-X"
	have "$CURL_BIN" || die "curl is required to download portable DOSBox-X"

	archive="$DOSBOX_X_ROOT/$DOSBOX_X_ASSET"
	mkdir -p "$DOSBOX_X_ROOT"

	if [ ! -f "$archive" ]; then
		msg "downloading portable DOSBox-X" >&2
		download_file "$archive" "$DOSBOX_X_URL" >&2
	fi

	rm -rf "$DOSBOX_X_ROOT/portable"
	mkdir -p "$DOSBOX_X_ROOT/portable"
	msg "unpacking portable DOSBox-X" >&2
	run unzip -q -o "$archive" -d "$DOSBOX_X_ROOT/portable" >&2

	exe="$(find_portable_dosbox_x "$DOSBOX_X_ROOT")" || die "portable DOSBox-X archive did not contain dosbox-x.exe"
	printf '%s\n' "$exe"
}

dosbox_path() {
	local path="$1"

	if [ "$HOST_KIND" = "msys2" ] && have cygpath; then
		cygpath -m "$path"
	else
		printf '%s\n' "$path"
	fi
}

find_dosbox_x() {
	local candidate
	local local_appdata

	if [ -n "${DOSBOX_BIN:-}" ]; then
		normalize_program_path "$DOSBOX_BIN"
		return 0
	fi

	if candidate="$(prepare_portable_dosbox_x)"; then
		printf '%s\n' "$candidate"
		return 0
	fi

	for candidate in dosbox-x dosbox-x.exe; do
		if have "$candidate"; then
			command -v "$candidate"
			return 0
		fi
	done

	if [ "$HOST_KIND" = "msys2" ] && have cygpath; then
		for candidate in \
			"/c/DOSBox-X/dosbox-x.exe" \
			"/c/DOSBox-X/DOSBox-X.exe" \
			"/c/Program Files/DOSBox-X/dosbox-x.exe" \
			"/c/Program Files/DOSBox-X/DOSBox-X.exe" \
			"/c/Program Files (x86)/DOSBox-X/dosbox-x.exe" \
			"/c/Program Files (x86)/DOSBox-X/DOSBox-X.exe"
		do
			if [ -x "$candidate" ]; then
				printf '%s\n' "$candidate"
				return 0
			fi
		done

		local_appdata="$(printenv LOCALAPPDATA || true)"
		if [ -n "$local_appdata" ]; then
			for candidate in \
				"$(cygpath -u "$local_appdata")/Programs/DOSBox-X/dosbox-x.exe" \
				"$(cygpath -u "$local_appdata")/Programs/DOSBox-X/DOSBox-X.exe"
			do
				if [ -x "$candidate" ]; then
					printf '%s\n' "$candidate"
					return 0
				fi
			done
		fi
	fi

	return 1
}

find_ffmpeg() {
	local candidate

	if [ -n "${FFMPEG_BIN:-}" ]; then
		normalize_program_path "$FFMPEG_BIN"
		return 0
	fi

	for candidate in \
		ffmpeg \
		ffmpeg.exe \
		/ucrt64/bin/ffmpeg.exe \
		/mingw64/bin/ffmpeg.exe \
		/mingw32/bin/ffmpeg.exe \
		/c/msys64/ucrt64/bin/ffmpeg.exe \
		/c/msys64/mingw64/bin/ffmpeg.exe \
		/c/msys64/mingw32/bin/ffmpeg.exe
	do
		if have "$candidate"; then
			command -v "$candidate"
			return 0
		fi

		if [ -x "$candidate" ]; then
			printf '%s\n' "$candidate"
			return 0
		fi
	done

	return 1
}

generate_sine_wav() {
	local wav="$1"
	local hz="$2"
	local seconds="$3"

	perl -e '
		use strict;
		use warnings;

		my ($path, $hz, $seconds) = @ARGV;
		my $rate = 44100;
		my $channels = 1;
		my $bits = 16;
		my $frames = int(($seconds * $rate) + 0.5);
		my $data = "";

		for my $i (0 .. $frames - 1) {
			my $t = $i / $rate;
			my $sample = int(sin(2.0 * 3.14159265358979323846 * $hz * $t) * 24000.0);
			$data .= pack("s<", $sample);
		}

		open my $fh, ">:raw", $path or die "unable to write $path: $!\n";
		print $fh "RIFF";
		print $fh pack("V", 36 + length($data));
		print $fh "WAVEfmt ";
		print $fh pack("VvvVVvv", 16, 1, $channels, $rate,
		               $rate * $channels * ($bits / 8),
		               $channels * ($bits / 8), $bits);
		print $fh "data";
		print $fh pack("V", length($data));
		print $fh $data;
		close $fh;
	' "$wav" "$hz" "$seconds"
}

prepare_sfx_media() {
	local ffmpeg_bin

	msg "generating DOS sfxlib media assets"
	generate_sine_wav "$TEST_ROOT/SINE440.WAV" 440 1.35

	MP3_TEST_EXPECTED_HZ=""
	if ffmpeg_bin="$(find_ffmpeg)"; then
		run "$ffmpeg_bin" -y -hide_banner -loglevel error \
			-i "$TEST_ROOT/SINE440.WAV" \
			-codec:a libmp3lame \
			-b:a 96k \
			"$TEST_ROOT/SINE440.MP3"
		MP3_TEST_EXPECTED_HZ="440"
	else
		msg "ffmpeg not found; using checked-in CC0 MP3 for activity-only MP3 playback coverage"
		cp -f "$ROOT/examples/sfxlib/media/clown-laugh.mp3" "$TEST_ROOT/SINE440.MP3"
	fi
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

select_wav_audio() {
	local label="$1"
	local report="$2"
	shift 2

	local expected_args
	local wav
	local candidate_report
	local analyzer_args
	local transient_args

	[ "$#" -gt 0 ] || die "no captured WAV candidates for $label"

	expected_args=()
	while [ "$#" -gt 0 ] && [ "$1" != "--" ]; do
		expected_args+=(--expect "$1")
		shift
	done

	[ "$#" -gt 0 ] || die "missing captured WAV separator for $label"
	shift
	[ "$#" -gt 0 ] || die "no captured WAV candidates for $label"

	rm -f "$report"
	candidate_report="$report.tmp"
	analyzer_args=()
	transient_args=()

	case "$label" in
		"Sound Blaster"*)
			transient_args=(--allow-edge-transient-ms=250)
			;;
	esac

	if [ "${#expected_args[@]}" -gt 0 ]; then
		case "$label" in
			"Sound Blaster dual")
				analyzer_args=(--allowed-harmonics=1 --max-unexpected-ratio=0.13)
				;;
			"Sound Blaster command "*)
				analyzer_args=(--allowed-harmonics=15 --max-unexpected-ratio=0.20)
				;;
			"Sound Blaster media MP3 "*)
				analyzer_args=(--allowed-harmonics=1 --max-unexpected-ratio=0.18)
				;;
			"Sound Blaster media "*)
				analyzer_args=(--allowed-harmonics=1 --max-unexpected-ratio=0.08)
				;;
			"Sound Blaster"*)
				analyzer_args=(--allowed-harmonics=1 --max-unexpected-ratio=0.08)
				;;
			"PC speaker dual")
				analyzer_args=(--allowed-harmonics=15 --max-unexpected-ratio=0.45)
				;;
			"PC speaker"*)
				analyzer_args=(--allowed-harmonics=15 --max-unexpected-ratio=0.18)
				;;
		esac
	fi

	for wav in "$@"; do
		rm -f "$candidate_report"
		if perl "$ROOT/build_scripts/analyze-sfx-wav.pl" \
			--label "$label" \
			"${expected_args[@]}" \
			--min-active-seconds=0.80 \
			--max-gap-ms=0 \
			"${transient_args[@]}" \
			"${analyzer_args[@]}" \
			"$wav" \
			>"$candidate_report" 2>&1
		then
			mv "$candidate_report" "$report"
			printf '%s\n' "$wav"
			return 0
		fi

		{
			echo "candidate: $wav"
			cat "$candidate_report"
			echo ""
		} >>"$report"
	done

	cat "$report" >&2
	die "$label FFT/continuity analysis failed"
}

capture_wav_audio() {
	local label="$1"
	local exe_name="$2"
	local log_name="$3"
	local result_name="$4"
	local report="$5"
	local driver_kind="$6"
	shift 6

	local capture_log
	local exe_stem
	local final_wav
	local selected
	local capture_cmds

	exe_stem="${exe_name%.EXE}"
	capture_log="$WORKDIR/dosbox-capture-${exe_stem}.log"
	final_wav="$WORKDIR/${exe_stem}.wav"

	rm -f "$DOSBOX_CAPTURE_DIR"/*.wav
	rm -f "$final_wav"

	capture_cmds=()
	if [ "$driver_kind" = "pcspk" ]; then
		capture_cmds+=(-c "set BLASTER=")
	fi

	msg "capturing $label audio"
	run timeout "$DOSBOX_TIMEOUT" "$DOSBOX_BIN" \
		-conf "$DOSBOX_CONF_PATH" \
		-fastlaunch \
		-nogui \
		-nomenu \
		-exit \
		-c "mount d \"$DOSBOX_TEST_ROOT_PATH\"" \
		-c "imgmount c \"$DOSBOX_IMAGE_FILE_PATH\"" \
		-c "c:" \
		-c "set DJGPP=C:\\DJGPP\\DJGPP.ENV" \
		-c "set PATH=C:\\FB;C:\\DJGPP\\BIN;%PATH%" \
		-c "set SFXLIB_DEBUG=1" \
		"${capture_cmds[@]}" \
		-c "DX-CAPTURE /A C:\\DJGPP\\BIN\\REDIR.EXE -eo -o D:\\$log_name C:\\$exe_name" \
		-c "if exist C:\\$result_name copy C:\\$result_name D:\\$result_name >NUL" \
		-c "exit" \
		>"$capture_log" 2>&1

	mapfile -t CAPTURE_WAVS < <(find_captured_wavs)
	[ "${#CAPTURE_WAVS[@]}" -gt 0 ] || die "expected a captured WAV for $label in $DOSBOX_CAPTURE_DIR"

	selected="$(select_wav_audio "$label" "$report" "$@" -- "${CAPTURE_WAVS[@]}")"
	cp -f "$selected" "$final_wav"
	SELECTED_CAPTURE_WAV="$final_wav"
}

install_linux_dependencies() {
	msg "updating APT package database"
	run_root apt-get update

	msg "installing Linux DOS test dependencies"
	run_root env DEBIAN_FRONTEND=noninteractive apt-get install -y \
		dosbox-x \
		file \
		mtools \
		perl \
		rsync \
		util-linux
}

install_msys2_dependencies() {
	local packages

	configure_msys2_path

	msg "updating MSYS2 package database"
	run pacman -Sy --noconfirm

	packages=(
		coreutils
		file
		gawk
		mingw-w64-x86_64-mtools
		perl
		rsync
		util-linux
	)

	msg "installing MSYS2 DOS test dependencies"
	run pacman -S --needed --noconfirm "${packages[@]}"

	if ! find_dosbox_x >/dev/null 2>&1; then
		msg "portable DOSBox-X could not be prepared; set DOSBOX_BIN to a DOSBox-X executable"
	fi
}

##############################################################################
# Resolve inputs
##############################################################################

if [ "$HOST_KIND" = "msys2" ]; then
	configure_msys2_path
fi

if [ "$DO_DEPS" = "1" ]; then
	case "$HOST_KIND" in
		linux) install_linux_dependencies ;;
		msys2) install_msys2_dependencies ;;
	esac
fi

PKGDIR="${PKGDIR:-$(find_latest_pkg)}"
[ -n "${PKGDIR:-}" ] || die "no DOS package found in $OUT; run ./build_scripts/msdos-build-freebasic.sh first"
[ -d "$PKGDIR" ] || die "missing DOS package directory: $PKGDIR"

if ! DOSBOX_BIN="$(find_dosbox_x)"; then
	die "dosbox-x not found; set DOSBOX_BIN or allow the portable DOSBox-X download"
fi

require_tool mcopy
require_tool sfdisk
require_tool rsync
require_tool timeout
require_tool od
require_tool file
require_tool awk
require_tool perl

##############################################################################
# Prepare work area
##############################################################################

if [ "$KEEP_WORKDIR" != "1" ]; then
	rm -rf "$WORKDIR"
fi

mkdir -p "$TEST_ROOT" "$CONF_DIR" "$DOSBOX_CAPTURE_DIR"
rm -f "$IMAGE_FILE" "$TRACE_LOG" "$HELLO_LOG" "$HELLO_TXT" "$GFX_LOG" "$GFX_TXT" \
	"$SOUND_LOG" "$SOUND_TXT" "$NOBLAST_LOG" "$NOBLAST_TXT" \
	"$SBFFT_LOG" "$SBFFT_TXT" "$SBDUAL_LOG" "$SBDUAL_TXT" \
	"$PCSPKFFT_LOG" "$PCSPKFFT_TXT" "$PCSPKDUAL_LOG" "$PCSPKDUAL_TXT" \
	"$IMGMAKE_LOG" "$DOSBOX_RUN_LOG" "$SBFFT_REPORT" "$SBDUAL_REPORT" \
	"$PCSPKFFT_REPORT" "$PCSPKDUAL_REPORT"
rm -f "$TEST_ROOT"/SBBEEP.* "$TEST_ROOT"/SBSOUND.* "$TEST_ROOT"/SBSNDCH.* \
	"$TEST_ROOT"/SBNOTE.* "$TEST_ROOT"/SBPLAY.* "$TEST_ROOT"/SBPLY2.* \
	"$TEST_ROOT"/SBNOISE.* "$TEST_ROOT"/SBSFXW.* "$TEST_ROOT"/SBSFXM.* \
	"$TEST_ROOT"/SBMUSW.* "$TEST_ROOT"/SBMUSM.* "$TEST_ROOT"/SBAUDW.* \
	"$TEST_ROOT"/SBAUDM.* "$TEST_ROOT"/SBSTRW.* "$TEST_ROOT"/SBSTRM.* \
	"$TEST_ROOT"/SFXCMD.BI "$TEST_ROOT"/SINE440.WAV "$TEST_ROOT"/SINE440.MP3
rm -f "$WORKDIR/SBFFT.wav" "$WORKDIR/SBDUAL.wav" \
	"$WORKDIR/PCSPKFFT.wav" "$WORKDIR/PCDUAL.wav"
rm -f "$WORKDIR"/SBBEEP.wav "$WORKDIR"/SBSOUND.wav "$WORKDIR"/SBSNDCH.wav \
	"$WORKDIR"/SBNOTE.wav "$WORKDIR"/SBPLAY.wav "$WORKDIR"/SBPLY2.wav \
	"$WORKDIR"/SBNOISE.wav "$WORKDIR"/SBSFXW.wav "$WORKDIR"/SBSFXM.wav \
	"$WORKDIR"/SBMUSW.wav "$WORKDIR"/SBMUSM.wav "$WORKDIR"/SBAUDW.wav \
	"$WORKDIR"/SBAUDM.wav "$WORKDIR"/SBSTRW.wav "$WORKDIR"/SBSTRM.wav \
	"$WORKDIR"/sb*-audio.txt
rm -f "$DOSBOX_CAPTURE_DIR"/*.wav

DOSBOX_CONF_PATH="$(dosbox_path "$DOSBOX_CONF")"
DOSBOX_CAPTURE_DIR_PATH="$(dosbox_path "$DOSBOX_CAPTURE_DIR")"
DOSBOX_IMAGE_FILE_PATH="$(dosbox_path "$IMAGE_FILE")"
DOSBOX_TEST_ROOT_PATH="$(dosbox_path "$TEST_ROOT")"
MTOOLS_IMAGE_FILE_PATH="$DOSBOX_IMAGE_FILE_PATH"

msg "staging DOS package"
run rsync -a "$PKGDIR"/ "$TEST_ROOT"/
prepare_dos_runtime_layout "$TEST_ROOT"
prepare_sfx_media

cat > "$DOSBOX_CONF" <<EOF
[dosbox]
captures = $DOSBOX_CAPTURE_DIR_PATH

[dos]
lfn = true

[cpu]
cputype = ppro_slow
cycles = max

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

cat > "$TEST_ROOT/SBFFT.BAS" <<'EOF'
dim as long result

print "sbfft-start"
DEVICE LIST
DEVICE INFO

result = DEVICE SELECT(0)
print "device-select="; result
if result <> 0 then
	end 1
end if

print "tone-hz=440"
WAVE 1, 0
ENVELOPE 0, 0, 0, 1, 0
INSTRUMENT 1, 1, 0
INSTRUMENT 0, 1
print "tone-begin"
TONE 0, 440, 1.20
SLEEP 1300
print "tone-end"

open "C:\SBFFT.TXT" for output as #1
print #1, "sbfft-done"
close #1

print "sbfft-end"
EOF

cat > "$TEST_ROOT/SBDUAL.BAS" <<'EOF'
dim as long result

print "sbdual-start"
DEVICE LIST
DEVICE INFO

result = DEVICE SELECT(0)
print "device-select="; result
if result <> 0 then
	end 1
end if

print "tone-hz=440"
print "tone-hz=660"
WAVE 1, 0
ENVELOPE 0, 0, 0, 1, 0
INSTRUMENT 1, 1, 0
INSTRUMENT 0, 1
INSTRUMENT 1, 1
print "tone-begin"
TONE 0, 440, 1.20
TONE 1, 660, 1.20
SLEEP 1300
print "tone-end"

open "C:\SBDUAL.TXT" for output as #1
print #1, "sbdual-done"
close #1

print "sbdual-end"
EOF

cat > "$TEST_ROOT/PCSPKFFT.BAS" <<'EOF'
print "pcspkfft-start"
DEVICE LIST
DEVICE INFO

print "tone-hz=523"
WAVE 1, 0
ENVELOPE 0, 0, 0, 1, 0
INSTRUMENT 1, 1, 0
INSTRUMENT 0, 1
print "tone-begin"
TONE 0, 523, 1.20
SLEEP 1300
print "tone-end"

open "C:\PCSPKFFT.TXT" for output as #1
print #1, "pcspkfft-done"
close #1

print "pcspkfft-end"
EOF

cat > "$TEST_ROOT/PCDUAL.BAS" <<'EOF'
print "pcspkdual-start"
DEVICE LIST
DEVICE INFO

print "tone-hz=440"
print "tone-hz=523"
WAVE 1, 0
ENVELOPE 0, 0, 0, 1, 0
INSTRUMENT 1, 1, 0
INSTRUMENT 0, 1
INSTRUMENT 1, 1
print "tone-begin"
TONE 0, 440, 1.20
TONE 1, 523, 1.20
SLEEP 1300
print "tone-end"

open "C:\PCDUAL.TXT" for output as #1
print #1, "pcspkdual-done"
close #1

print "pcspkdual-end"
EOF

cat > "$TEST_ROOT/SFXCMD.BI" <<'EOF'
#macro SFX_SELECT_SB()
	dim as long sfx_device_result

	DEVICE LIST
	DEVICE INFO
	sfx_device_result = DEVICE SELECT(0)
	print "device-select="; sfx_device_result
	if sfx_device_result <> 0 then
		end 1
	end if
#endmacro

#macro SFX_SINE_SETUP()
	WAVE 1, 0
	ENVELOPE 0, 0, 0, 1, 0
	INSTRUMENT 1, 1, 0
	INSTRUMENT 0, 1
	INSTRUMENT 1, 1
#endmacro

#macro SFX_DONE(result_file, marker)
	open result_file for output as #1
	print #1, marker
	close #1
#endmacro
EOF

cat > "$TEST_ROOT/SBBEEP.BAS" <<'EOF'
#include "SFXCMD.BI"

print "sbbeep-start"
SFX_SELECT_SB()
print "tone-hz=440"
print "beep-begin"
BEEP 1.20, 9
print "beep-end"
SFX_DONE("C:\SBBEEP.TXT", "sbbeep-done")
print "sbbeep-end"
EOF

cat > "$TEST_ROOT/SBSOUND.BAS" <<'EOF'
#include "SFXCMD.BI"

print "sbsound-start"
SFX_SELECT_SB()
SFX_SINE_SETUP()
print "tone-hz=440"
print "sound-begin"
SOUND 440, 1.20
SLEEP 1300
print "sound-end"
SFX_DONE("C:\SBSOUND.TXT", "sbsound-done")
print "sbsound-end"
EOF

cat > "$TEST_ROOT/SBSNDCH.BAS" <<'EOF'
#include "SFXCMD.BI"

print "sbsndch-start"
SFX_SELECT_SB()
SFX_SINE_SETUP()
print "tone-hz=440"
print "sound-channel-begin"
SOUND 0, 440, 1.20, 0.8
SLEEP 1300
print "sound-channel-end"
SFX_DONE("C:\SBSNDCH.TXT", "sbsndch-done")
print "sbsndch-end"
EOF

cat > "$TEST_ROOT/SBNOTE.BAS" <<'EOF'
#include "SFXCMD.BI"

print "sbnote-start"
SFX_SELECT_SB()
SFX_SINE_SETUP()
print "tone-hz=440"
print "note-begin"
NOTE "A", 4, 1.20
SLEEP 1300
print "note-end"
SFX_DONE("C:\SBNOTE.TXT", "sbnote-done")
print "sbnote-end"
EOF

cat > "$TEST_ROOT/SBPLAY.BAS" <<'EOF'
#include "SFXCMD.BI"

print "sbplay-start"
SFX_SELECT_SB()
SFX_SINE_SETUP()
print "tone-hz=440"
print "play-begin"
PLAY "T60 O4 L4 A"
print "play-end"
SFX_DONE("C:\SBPLAY.TXT", "sbplay-done")
print "sbplay-end"
EOF

cat > "$TEST_ROOT/SBPLY2.BAS" <<'EOF'
#include "SFXCMD.BI"

print "sbply2-start"
SFX_SELECT_SB()
SFX_SINE_SETUP()
print "tone-hz=440"
print "tone-hz=523"
print "play2-begin"
PLAY "T60 O4 L4 A", "T60 O5 L4 C"
print "play2-end"
SFX_DONE("C:\SBPLY2.TXT", "sbply2-done")
print "sbply2-end"
EOF

cat > "$TEST_ROOT/SBNOISE.BAS" <<'EOF'
#include "SFXCMD.BI"

print "sbnoise-start"
SFX_SELECT_SB()
print "noise-begin"
NOISE 0, 440, 1.20, 0.8
SLEEP 1300
print "noise-end"
SFX_DONE("C:\SBNOISE.TXT", "sbnoise-done")
print "sbnoise-end"
EOF

cat > "$TEST_ROOT/SBSFXW.BAS" <<'EOF'
#include "SFXCMD.BI"

print "sbsfxw-start"
SFX_SELECT_SB()
print "tone-hz=440"
print "sfx-wav-begin"
SFX LOAD 1, "C:\SINE440.WAV"
SFX PLAY 1
SLEEP 1300
SFX STOP
print "sfx-wav-end"
SFX_DONE("C:\SBSFXW.TXT", "sbsfxw-done")
print "sbsfxw-end"
EOF

cat > "$TEST_ROOT/SBSFXM.BAS" <<'EOF'
#include "SFXCMD.BI"

print "sbsfxm-start"
SFX_SELECT_SB()
print "tone-hz=440"
print "sfx-mp3-begin"
SFX LOAD 1, "C:\SINE440.MP3"
SFX PLAY 1
SLEEP 1300
SFX STOP
print "sfx-mp3-end"
SFX_DONE("C:\SBSFXM.TXT", "sbsfxm-done")
print "sbsfxm-end"
EOF

cat > "$TEST_ROOT/SBMUSW.BAS" <<'EOF'
#include "SFXCMD.BI"

dim as long music_id

print "sbmusw-start"
SFX_SELECT_SB()
print "tone-hz=440"
music_id = MUSIC LOAD("C:\SINE440.WAV")
print "music-id="; music_id
if music_id < 0 then
	end 1
end if
print "music-wav-begin"
MUSIC PLAY music_id
SLEEP 1300
MUSIC STOP music_id
print "music-wav-end"
SFX_DONE("C:\SBMUSW.TXT", "sbmusw-done")
print "sbmusw-end"
EOF

cat > "$TEST_ROOT/SBMUSM.BAS" <<'EOF'
#include "SFXCMD.BI"

dim as long music_id

print "sbmusm-start"
SFX_SELECT_SB()
print "tone-hz=440"
music_id = MUSIC PLAY("C:\SINE440.MP3")
print "music-id="; music_id
if music_id < 0 then
	end 1
end if
print "music-mp3-begin"
SLEEP 1300
MUSIC STOP music_id
print "music-mp3-end"
SFX_DONE("C:\SBMUSM.TXT", "sbmusm-done")
print "sbmusm-end"
EOF

cat > "$TEST_ROOT/SBAUDW.BAS" <<'EOF'
#include "SFXCMD.BI"

dim as long result

print "sbaudw-start"
SFX_SELECT_SB()
print "tone-hz=440"
result = AUDIO PLAY("C:\SINE440.WAV")
print "audio-play="; result
if result <> 0 then
	end 1
end if
print "audio-wav-begin"
SLEEP 1300
AUDIO STOP
print "audio-wav-end"
SFX_DONE("C:\SBAUDW.TXT", "sbaudw-done")
print "sbaudw-end"
EOF

cat > "$TEST_ROOT/SBAUDM.BAS" <<'EOF'
#include "SFXCMD.BI"

dim as long result

print "sbaudm-start"
SFX_SELECT_SB()
print "tone-hz=440"
result = AUDIO PLAY("C:\SINE440.MP3")
print "audio-play="; result
if result <> 0 then
	end 1
end if
print "audio-mp3-begin"
SLEEP 1300
AUDIO STOP
print "audio-mp3-end"
SFX_DONE("C:\SBAUDM.TXT", "sbaudm-done")
print "sbaudm-end"
EOF

cat > "$TEST_ROOT/SBSTRW.BAS" <<'EOF'
#include "SFXCMD.BI"

dim as long result

print "sbstrw-start"
SFX_SELECT_SB()
print "tone-hz=440"
result = STREAM OPEN("C:\SINE440.WAV")
print "stream-open="; result
if result <> 0 then
	end 1
end if
result = STREAM PLAY()
print "stream-play="; result
if result <> 0 then
	end 1
end if
print "stream-wav-begin"
SLEEP 1300
STREAM STOP
print "stream-wav-end"
SFX_DONE("C:\SBSTRW.TXT", "sbstrw-done")
print "sbstrw-end"
EOF

cat > "$TEST_ROOT/SBSTRM.BAS" <<'EOF'
#include "SFXCMD.BI"

dim as long result

print "sbstrm-start"
SFX_SELECT_SB()
print "tone-hz=440"
result = STREAM OPEN("C:\SINE440.MP3")
print "stream-open="; result
if result <> 0 then
	end 1
end if
result = STREAM PLAY()
print "stream-play="; result
if result <> 0 then
	end 1
end if
print "stream-mp3-begin"
SLEEP 1300
STREAM STOP
print "stream-mp3-end"
SFX_DONE("C:\SBSTRM.TXT", "sbstrm-done")
print "sbstrm-end"
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
C:\DJGPP\BIN\REDIR.EXE -eo -o D:\SOUND.LOG C:\SOUND.EXE
echo sound-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG
if exist C:\SOUND.TXT copy C:\SOUND.TXT D:\SOUND.TXT >NUL

C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD-SBFFT.LOG C:\FB\FBC.EXE C:\SBFFT.BAS -x C:\SBFFT.EXE
echo build-sbfft-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG

C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD-SBDUAL.LOG C:\FB\FBC.EXE C:\SBDUAL.BAS -x C:\SBDUAL.EXE
echo build-sbdual-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG

C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD-NOBLAST.LOG C:\FB\FBC.EXE C:\NOBLAST.BAS -x C:\NOBLAST.EXE
echo build-noblast-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG
set BLASTER=
echo cleared-blaster=%BLASTER%>>D:\TRACE.LOG
C:\DJGPP\BIN\REDIR.EXE -eo -o D:\NOBLAST.LOG C:\NOBLAST.EXE
echo noblast-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG
if exist C:\NOBLAST.TXT copy C:\NOBLAST.TXT D:\NOBLAST.TXT >NUL

C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD-PCSPKFFT.LOG C:\FB\FBC.EXE C:\PCSPKFFT.BAS -x C:\PCSPKFFT.EXE
echo build-pcspkfft-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG

C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BLDPCD.LOG C:\FB\FBC.EXE C:\PCDUAL.BAS -x C:\PCDUAL.EXE
echo build-pcspkdual-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG

C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD-SBBEEP.LOG C:\FB\FBC.EXE C:\SBBEEP.BAS -x C:\SBBEEP.EXE
echo build-sbbeep-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG

C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD-SBSOUND.LOG C:\FB\FBC.EXE C:\SBSOUND.BAS -x C:\SBSOUND.EXE
echo build-sbsound-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG

C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD-SBSNDCH.LOG C:\FB\FBC.EXE C:\SBSNDCH.BAS -x C:\SBSNDCH.EXE
echo build-sbsndch-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG

C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD-SBNOTE.LOG C:\FB\FBC.EXE C:\SBNOTE.BAS -x C:\SBNOTE.EXE
echo build-sbnote-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG

C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD-SBPLAY.LOG C:\FB\FBC.EXE C:\SBPLAY.BAS -x C:\SBPLAY.EXE
echo build-sbplay-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG

C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD-SBPLY2.LOG C:\FB\FBC.EXE C:\SBPLY2.BAS -x C:\SBPLY2.EXE
echo build-sbply2-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG

C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD-SBNOISE.LOG C:\FB\FBC.EXE C:\SBNOISE.BAS -x C:\SBNOISE.EXE
echo build-sbnoise-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG

C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD-SBSFXW.LOG C:\FB\FBC.EXE C:\SBSFXW.BAS -x C:\SBSFXW.EXE
echo build-sbsfxw-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG

C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD-SBSFXM.LOG C:\FB\FBC.EXE C:\SBSFXM.BAS -x C:\SBSFXM.EXE
echo build-sbsfxm-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG

C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD-SBMUSW.LOG C:\FB\FBC.EXE C:\SBMUSW.BAS -x C:\SBMUSW.EXE
echo build-sbmusw-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG

C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD-SBMUSM.LOG C:\FB\FBC.EXE C:\SBMUSM.BAS -x C:\SBMUSM.EXE
echo build-sbmusm-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG

C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD-SBAUDW.LOG C:\FB\FBC.EXE C:\SBAUDW.BAS -x C:\SBAUDW.EXE
echo build-sbaudw-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG

C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD-SBAUDM.LOG C:\FB\FBC.EXE C:\SBAUDM.BAS -x C:\SBAUDM.EXE
echo build-sbaudm-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG

C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD-SBSTRW.LOG C:\FB\FBC.EXE C:\SBSTRW.BAS -x C:\SBSTRW.EXE
echo build-sbstrw-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG

C:\DJGPP\BIN\REDIR.EXE -eo -o D:\BUILD-SBSTRM.LOG C:\FB\FBC.EXE C:\SBSTRM.BAS -x C:\SBSTRM.EXE
echo build-sbstrm-errorlevel=%ERRORLEVEL%>>D:\TRACE.LOG
exit
EOF

##############################################################################
# Build image and run DOSBox-X
##############################################################################

msg "creating DOS test image"
run "$DOSBOX_BIN" \
	-conf "$DOSBOX_CONF_PATH" \
	-fastlaunch \
	-nogui \
	-nomenu \
	-exit \
	-c "imgmake \"$DOSBOX_IMAGE_FILE_PATH\" -t hd -size 256 -fat 16" \
	-c "exit" \
	>"$IMGMAKE_LOG" 2>&1

PARTITION_START="$(sfdisk -d "$IMAGE_FILE" | sed -n 's/.*start= *\([0-9][0-9]*\).*/\1/p' | head -n1)"
[ -n "$PARTITION_START" ] || die "could not determine image partition start"
PARTITION_OFFSET="$((PARTITION_START * 512))"

msg "copying DOS payload into image"
MTOOLS_SOURCE_PATHS=("$TEST_ROOT"/*)
if [ "$HOST_KIND" = "msys2" ] && have cygpath; then
	MTOOLS_SOURCE_PATHS=()
	for source_path in "$TEST_ROOT"/*; do
		MTOOLS_SOURCE_PATHS+=("$(cygpath -m "$source_path")")
	done
fi
run env MTOOLS_SKIP_CHECK=1 mcopy -i "${MTOOLS_IMAGE_FILE_PATH}@@${PARTITION_OFFSET}" -s "${MTOOLS_SOURCE_PATHS[@]}" ::

msg "running DOSBox-X FreeBASIC test suite"
run timeout "$DOSBOX_TIMEOUT" "$DOSBOX_BIN" \
	-conf "$DOSBOX_CONF_PATH" \
	-fastlaunch \
	-nogui \
	-nomenu \
	-exit \
	-c "mount d \"$DOSBOX_TEST_ROOT_PATH\"" \
	-c "imgmount c \"$DOSBOX_IMAGE_FILE_PATH\"" \
	-c "c:" \
	-c "RUNTESTS.BAT" \
	-c "exit" \
	>"$DOSBOX_RUN_LOG" 2>&1

capture_wav_audio "Sound Blaster" "SBFFT.EXE" "SBFFT.LOG" "SBFFT.TXT" "$SBFFT_REPORT" "sb" 440
SB_CAPTURE_WAV="$SELECTED_CAPTURE_WAV"
capture_wav_audio "Sound Blaster dual" "SBDUAL.EXE" "SBDUAL.LOG" "SBDUAL.TXT" "$SBDUAL_REPORT" "sb" 440 660
SBDUAL_CAPTURE_WAV="$SELECTED_CAPTURE_WAV"
capture_wav_audio "PC speaker" "PCSPKFFT.EXE" "PCSPKFFT.LOG" "PCSPKFFT.TXT" "$PCSPKFFT_REPORT" "pcspk" 523
PCSPK_CAPTURE_WAV="$SELECTED_CAPTURE_WAV"
capture_wav_audio "PC speaker dual" "PCDUAL.EXE" "PCDUAL.LOG" "PCDUAL.TXT" "$PCSPKDUAL_REPORT" "pcspk" 440 523
PCSPKDUAL_CAPTURE_WAV="$SELECTED_CAPTURE_WAV"

SFX_CAPTURE_LABELS=()
SFX_CAPTURE_WAVS=()
SFX_CAPTURE_REPORTS=()
MP3_EXPECTED_ARGS=()

if [ -n "$MP3_TEST_EXPECTED_HZ" ]; then
	MP3_EXPECTED_ARGS=("$MP3_TEST_EXPECTED_HZ")
fi

capture_sfx_command_audio() {
	local label="$1"
	local exe="$2"
	local log="$3"
	local result="$4"
	local report="$5"
	shift 5

	capture_wav_audio "$label" "$exe" "$log" "$result" "$report" "sb" "$@"
	SFX_CAPTURE_LABELS+=("$label")
	SFX_CAPTURE_WAVS+=("$SELECTED_CAPTURE_WAV")
	SFX_CAPTURE_REPORTS+=("$report")
}

capture_sfx_command_audio "Sound Blaster command BEEP" "SBBEEP.EXE" "SBBEEP.LOG" "SBBEEP.TXT" "$WORKDIR/sbbeep-audio.txt" 440
capture_sfx_command_audio "Sound Blaster command SOUND" "SBSOUND.EXE" "SBSOUND.LOG" "SBSOUND.TXT" "$WORKDIR/sbsound-audio.txt" 440
capture_sfx_command_audio "Sound Blaster command SOUND channel" "SBSNDCH.EXE" "SBSNDCH.LOG" "SBSNDCH.TXT" "$WORKDIR/sbsndch-audio.txt" 440
capture_sfx_command_audio "Sound Blaster command NOTE" "SBNOTE.EXE" "SBNOTE.LOG" "SBNOTE.TXT" "$WORKDIR/sbnote-audio.txt" 440
capture_sfx_command_audio "Sound Blaster command PLAY" "SBPLAY.EXE" "SBPLAY.LOG" "SBPLAY.TXT" "$WORKDIR/sbplay-audio.txt" 440
capture_sfx_command_audio "Sound Blaster command PLAY dual" "SBPLY2.EXE" "SBPLY2.LOG" "SBPLY2.TXT" "$WORKDIR/sbply2-audio.txt" 440 523
capture_sfx_command_audio "Sound Blaster command NOISE" "SBNOISE.EXE" "SBNOISE.LOG" "SBNOISE.TXT" "$WORKDIR/sbnoise-audio.txt"
capture_sfx_command_audio "Sound Blaster media WAV SFX" "SBSFXW.EXE" "SBSFXW.LOG" "SBSFXW.TXT" "$WORKDIR/sbsfxw-audio.txt" 440
capture_sfx_command_audio "Sound Blaster media MP3 SFX" "SBSFXM.EXE" "SBSFXM.LOG" "SBSFXM.TXT" "$WORKDIR/sbsfxm-audio.txt" "${MP3_EXPECTED_ARGS[@]}"
capture_sfx_command_audio "Sound Blaster media WAV MUSIC" "SBMUSW.EXE" "SBMUSW.LOG" "SBMUSW.TXT" "$WORKDIR/sbmusw-audio.txt" 440
capture_sfx_command_audio "Sound Blaster media MP3 MUSIC" "SBMUSM.EXE" "SBMUSM.LOG" "SBMUSM.TXT" "$WORKDIR/sbmusm-audio.txt" "${MP3_EXPECTED_ARGS[@]}"
capture_sfx_command_audio "Sound Blaster media WAV AUDIO" "SBAUDW.EXE" "SBAUDW.LOG" "SBAUDW.TXT" "$WORKDIR/sbaudw-audio.txt" 440
capture_sfx_command_audio "Sound Blaster media MP3 AUDIO" "SBAUDM.EXE" "SBAUDM.LOG" "SBAUDM.TXT" "$WORKDIR/sbaudm-audio.txt" "${MP3_EXPECTED_ARGS[@]}"
capture_sfx_command_audio "Sound Blaster media WAV STREAM" "SBSTRW.EXE" "SBSTRW.LOG" "SBSTRW.TXT" "$WORKDIR/sbstrw-audio.txt" 440
capture_sfx_command_audio "Sound Blaster media MP3 STREAM" "SBSTRM.EXE" "SBSTRM.LOG" "SBSTRM.TXT" "$WORKDIR/sbstrm-audio.txt" "${MP3_EXPECTED_ARGS[@]}"

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
[ -f "$SBFFT_LOG" ] || die "missing Sound Blaster FFT log: $SBFFT_LOG"
[ -f "$SBFFT_TXT" ] || die "missing Sound Blaster FFT result file: $SBFFT_TXT"
[ -f "$SBDUAL_LOG" ] || die "missing Sound Blaster dual-tone log: $SBDUAL_LOG"
[ -f "$SBDUAL_TXT" ] || die "missing Sound Blaster dual-tone result file: $SBDUAL_TXT"
[ -f "$PCSPKFFT_LOG" ] || die "missing PC speaker FFT log: $PCSPKFFT_LOG"
[ -f "$PCSPKFFT_TXT" ] || die "missing PC speaker FFT result file: $PCSPKFFT_TXT"
[ -f "$PCSPKDUAL_LOG" ] || die "missing PC speaker dual-tone log: $PCSPKDUAL_LOG"
[ -f "$PCSPKDUAL_TXT" ] || die "missing PC speaker dual-tone result file: $PCSPKDUAL_TXT"

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
require_log_line "$SBFFT_LOG" 'device-select=[[:space:]]*0' "Sound Blaster FFT DEVICE SELECT success"
require_log_line "$SBFFT_LOG" 'tone-hz=440' "Sound Blaster FFT tone marker"
require_log_line "$SBFFT_LOG" 'sbfft-end' "Sound Blaster FFT completion marker"
require_log_line "$SBFFT_TXT" 'sbfft-done' "Sound Blaster FFT completion file"
require_log_line "$SBDUAL_LOG" 'device-select=[[:space:]]*0' "Sound Blaster dual-tone DEVICE SELECT success"
require_log_line "$SBDUAL_LOG" 'tone-hz=440' "Sound Blaster dual-tone first tone marker"
require_log_line "$SBDUAL_LOG" 'tone-hz=660' "Sound Blaster dual-tone second tone marker"
require_log_line "$SBDUAL_LOG" 'sbdual-end' "Sound Blaster dual-tone completion marker"
require_log_line "$SBDUAL_TXT" 'sbdual-done' "Sound Blaster dual-tone completion file"
require_log_line "$PCSPKFFT_LOG" 'tone-hz=523' "PC speaker FFT tone marker"
require_log_line "$PCSPKFFT_LOG" 'pcspkfft-end' "PC speaker FFT completion marker"
require_log_line "$PCSPKFFT_TXT" 'pcspkfft-done' "PC speaker FFT completion file"
require_log_line "$PCSPKDUAL_LOG" 'tone-hz=440' "PC speaker dual-tone first tone marker"
require_log_line "$PCSPKDUAL_LOG" 'tone-hz=523' "PC speaker dual-tone second tone marker"
require_log_line "$PCSPKDUAL_LOG" 'pcspkdual-end' "PC speaker dual-tone completion marker"
require_log_line "$PCSPKDUAL_TXT" 'pcspkdual-done' "PC speaker dual-tone completion file"

require_sfx_command_result() {
	local stem="$1"
	local log_marker="$2"
	local file_marker="$3"
	local log_file="$TEST_ROOT/$stem.LOG"
	local result_file="$TEST_ROOT/$stem.TXT"

	[ -f "$log_file" ] || die "missing sfx command log: $log_file"
	[ -f "$result_file" ] || die "missing sfx command result file: $result_file"
	require_log_line "$log_file" 'device-select=[[:space:]]*0' "$stem DEVICE SELECT success"
	require_log_line "$log_file" "$log_marker" "$stem completion marker"
	require_log_line "$result_file" "$file_marker" "$stem completion file"
}

require_sfx_command_result "SBBEEP" 'sbbeep-end' 'sbbeep-done'
require_sfx_command_result "SBSOUND" 'sbsound-end' 'sbsound-done'
require_sfx_command_result "SBSNDCH" 'sbsndch-end' 'sbsndch-done'
require_sfx_command_result "SBNOTE" 'sbnote-end' 'sbnote-done'
require_sfx_command_result "SBPLAY" 'sbplay-end' 'sbplay-done'
require_sfx_command_result "SBPLY2" 'sbply2-end' 'sbply2-done'
require_sfx_command_result "SBNOISE" 'sbnoise-end' 'sbnoise-done'
require_sfx_command_result "SBSFXW" 'sbsfxw-end' 'sbsfxw-done'
require_sfx_command_result "SBSFXM" 'sbsfxm-end' 'sbsfxm-done'
require_sfx_command_result "SBMUSW" 'sbmusw-end' 'sbmusw-done'
require_sfx_command_result "SBMUSM" 'sbmusm-end' 'sbmusm-done'
require_sfx_command_result "SBAUDW" 'sbaudw-end' 'sbaudw-done'
require_sfx_command_result "SBAUDM" 'sbaudm-end' 'sbaudm-done'
require_sfx_command_result "SBSTRW" 'sbstrw-end' 'sbstrw-done'
require_sfx_command_result "SBSTRM" 'sbstrm-end' 'sbstrm-done'

require_log_line "$TEST_ROOT/SBAUDW.LOG" 'audio-play=[[:space:]]*0' "AUDIO WAV playback start"
require_log_line "$TEST_ROOT/SBAUDM.LOG" 'audio-play=[[:space:]]*0' "AUDIO MP3 playback start"
require_log_line "$TEST_ROOT/SBSTRW.LOG" 'stream-open=[[:space:]]*0' "STREAM WAV open"
require_log_line "$TEST_ROOT/SBSTRW.LOG" 'stream-play=[[:space:]]*0' "STREAM WAV playback start"
require_log_line "$TEST_ROOT/SBSTRM.LOG" 'stream-open=[[:space:]]*0' "STREAM MP3 open"
require_log_line "$TEST_ROOT/SBSTRM.LOG" 'stream-play=[[:space:]]*0' "STREAM MP3 playback start"

SB_FIRST_NONZERO_SAMPLE="$(validate_wav_audio "$SB_CAPTURE_WAV")"
SBDUAL_FIRST_NONZERO_SAMPLE="$(validate_wav_audio "$SBDUAL_CAPTURE_WAV")"
PCSPK_FIRST_NONZERO_SAMPLE="$(validate_wav_audio "$PCSPK_CAPTURE_WAV")"
PCSPKDUAL_FIRST_NONZERO_SAMPLE="$(validate_wav_audio "$PCSPKDUAL_CAPTURE_WAV")"
SFX_CAPTURE_FIRST_NONZERO_SAMPLES=()

for wav in "${SFX_CAPTURE_WAVS[@]}"; do
	SFX_CAPTURE_FIRST_NONZERO_SAMPLES+=("$(validate_wav_audio "$wav")")
done

msg "FreeBASIC DOS test suite passed"
echo "Package: $PKGDIR"
echo "Trace log: $TRACE_LOG"
echo "Hello log: $HELLO_LOG"
echo "GFX log: $GFX_LOG"
echo "Sound log: $SOUND_LOG"
echo "No-BLASTER log: $NOBLAST_LOG"
echo "Sound Blaster capture: $SB_CAPTURE_WAV"
echo "Sound Blaster first non-zero sample: $SB_FIRST_NONZERO_SAMPLE"
echo "Sound Blaster FFT report: $SBFFT_REPORT"
echo "Sound Blaster dual-tone capture: $SBDUAL_CAPTURE_WAV"
echo "Sound Blaster dual-tone first non-zero sample: $SBDUAL_FIRST_NONZERO_SAMPLE"
echo "Sound Blaster dual-tone FFT report: $SBDUAL_REPORT"
echo "PC speaker capture: $PCSPK_CAPTURE_WAV"
echo "PC speaker first non-zero sample: $PCSPK_FIRST_NONZERO_SAMPLE"
echo "PC speaker FFT report: $PCSPKFFT_REPORT"
echo "PC speaker dual-tone capture: $PCSPKDUAL_CAPTURE_WAV"
echo "PC speaker dual-tone first non-zero sample: $PCSPKDUAL_FIRST_NONZERO_SAMPLE"
echo "PC speaker dual-tone FFT report: $PCSPKDUAL_REPORT"
echo "DOS sfxlib command/media captures:"
for i in "${!SFX_CAPTURE_WAVS[@]}"; do
	echo "  ${SFX_CAPTURE_LABELS[$i]}: ${SFX_CAPTURE_WAVS[$i]}"
	echo "    first non-zero sample: ${SFX_CAPTURE_FIRST_NONZERO_SAMPLES[$i]}"
	echo "    report: ${SFX_CAPTURE_REPORTS[$i]}"
done
run file "$SB_CAPTURE_WAV"
run file "$SBDUAL_CAPTURE_WAV"
run file "$PCSPK_CAPTURE_WAV"
run file "$PCSPKDUAL_CAPTURE_WAV"
for wav in "${SFX_CAPTURE_WAVS[@]}"; do
	run file "$wav"
done
