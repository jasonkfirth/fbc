#!/usr/bin/env bash
#
# FreeBASIC macOS smoke-test runner.
#
# This script compiles and runs small Darwin-specific gfxlib/sfxlib programs
# against the in-tree compiler.  It is intentionally focused on exercising the
# real macOS platform drivers after the build has completed.

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FBC="${FBC:-$ROOT_DIR/bin/fbc}"
OUT_DIR="${OUT_DIR:-$ROOT_DIR/out/macos-smoke}"
SKIP_STATUS=77
SMOKE_RESULT=""

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "macos-smoke: skipped, this runner requires Darwin"
    exit "$SKIP_STATUS"
fi

if [[ ! -x "$FBC" ]]; then
    echo "macos-smoke: fbc not found: $FBC" >&2
    exit 1
fi

mkdir -p "$OUT_DIR"

compile_smoke()
{
    local source="$1"
    local output="$2"

    "$FBC" -i "$ROOT_DIR/inc" -i "$ROOT_DIR/tests/sfx" "$source" -x "$output"
}

run_smoke()
{
    local name="$1"
    shift

    set +e
    "$@"
    local status=$?
    set -e

    if [[ "$status" -eq "$SKIP_STATUS" ]]; then
        echo "macos-smoke: skipped $name"
        SMOKE_RESULT="skipped"
        return 0
    fi

    if [[ "$status" -ne 0 ]]; then
        echo "macos-smoke: failed $name (status=$status)" >&2
        exit "$status"
    fi

    echo "macos-smoke: passed $name"
    SMOKE_RESULT="passed"
}

check_ppm_region_color()
{
    local file="$1"
    local x1="$2"
    local y1="$3"
    local x2="$4"
    local y2="$5"
    local want_r="$6"
    local want_g="$7"
    local want_b="$8"
    local tolerance="$9"

    perl - "$file" "$x1" "$y1" "$x2" "$y2" "$want_r" "$want_g" "$want_b" "$tolerance" <<'PERL'
use strict;
use warnings;

my ($file, $x1, $y1, $x2, $y2, $want_r, $want_g, $want_b, $tolerance) = @ARGV;
open my $fh, '<:raw', $file or die "cannot open $file: $!\n";

sub read_token {
    my ($fh) = @_;
    my $token = '';
    while (read($fh, my $ch, 1) == 1) {
        if ($ch =~ /\s/) {
            next if $token eq '';
            last;
        }
        if ($ch eq '#') {
            while (read($fh, $ch, 1) == 1 && $ch ne "\n") {
            }
            next if $token eq '';
            last;
        }
        $token .= $ch;
    }
    return $token;
}

my $magic = read_token($fh);
my $width = read_token($fh);
my $height = read_token($fh);
my $max = read_token($fh);
die "not a P6 PPM\n" unless $magic eq 'P6' && $max == 255;
my $data_start = tell($fh);
$x2 = $width - 1 if $x2 >= $width;
$y2 = $height - 1 if $y2 >= $height;
die "region outside image\n" if $x1 > $x2 || $y1 > $y2;

for my $y ($y1 .. $y2) {
    for my $x ($x1 .. $x2) {
        my $offset = $data_start + (($y * $width) + $x) * 3;
        seek($fh, $offset, 0) or die "seek failed: $!\n";
        read($fh, my $rgb, 3) == 3 or die "short PPM\n";
        my ($r, $g, $b) = unpack('C3', $rgb);
        next if abs($r - $want_r) > $tolerance;
        next if abs($g - $want_g) > $tolerance;
        next if abs($b - $want_b) > $tolerance;
        exit 0;
    }
}

die "color check failed: wanted $want_r,$want_g,$want_b in $x1,$y1-$x2,$y2\n";
PERL
}

check_audio_dump()
{
    local file="$1"

    perl - "$file" <<'PERL'
use strict;
use warnings;

my ($file) = @ARGV;
open my $fh, '<', $file or die "cannot open $file: $!\n";

my $count = 0;
my $sum = 0.0;
while (my $line = <$fh>) {
    next unless $line =~ /[-+0-9.]/;
    my $sample = 0.0 + $line;
    $sum += $sample * $sample;
    $count++;
}

die "empty audio dump\n" if $count < 128;
my $rms = sqrt($sum / $count);
die "silent audio dump rms=$rms\n" if $rms < 0.01;
PERL
}

GFX_PALETTED="$OUT_DIR/gfx-darwin-paletted-smoke"
GFX_TRUECOLOR="$OUT_DIR/gfx-darwin-truecolor-smoke"
GFX_SCREEN_MODES="$OUT_DIR/gfx-darwin-screen-modes-smoke"
SFX_PLAYBACK="$OUT_DIR/sfx-coreaudio-smoke"
SFX_CAPTURE="$OUT_DIR/sfx-capture-smoke"

compile_smoke "$ROOT_DIR/tests/macos/gfx-darwin-paletted-smoke.bas" "$GFX_PALETTED"
compile_smoke "$ROOT_DIR/tests/macos/gfx-darwin-truecolor-smoke.bas" "$GFX_TRUECOLOR"
compile_smoke "$ROOT_DIR/tests/macos/gfx-darwin-screen-modes-smoke.bas" "$GFX_SCREEN_MODES"
compile_smoke "$ROOT_DIR/tests/macos/sfx-coreaudio-smoke.bas" "$SFX_PLAYBACK"
compile_smoke "$ROOT_DIR/tests/macos/sfx-capture-smoke.bas" "$SFX_CAPTURE"

rm -f "$OUT_DIR"/gfx-screen-*.ppm "$OUT_DIR/gfx-paletted.ppm" "$OUT_DIR/gfx-truecolor.ppm" "$OUT_DIR/sfx-coreaudio.txt"

run_smoke "gfx paletted" env FBGFX=Darwin FBGFX_DARWIN_DUMP="$OUT_DIR/gfx-paletted.ppm" "$GFX_PALETTED"
if [[ "$SMOKE_RESULT" == "passed" ]]; then
    check_ppm_region_color "$OUT_DIR/gfx-paletted.ppm" 8 8 23 23 255 0 0 8
    check_ppm_region_color "$OUT_DIR/gfx-paletted.ppm" 24 8 39 23 0 255 0 8
    check_ppm_region_color "$OUT_DIR/gfx-paletted.ppm" 40 8 55 23 0 0 255 8
fi
GFX_AVAILABLE="$SMOKE_RESULT"

run_smoke "gfx truecolor" env FBGFX=Darwin FBGFX_DARWIN_DUMP="$OUT_DIR/gfx-truecolor.ppm" "$GFX_TRUECOLOR"
if [[ "$SMOKE_RESULT" == "passed" ]]; then
    check_ppm_region_color "$OUT_DIR/gfx-truecolor.ppm" 8 8 23 23 255 0 0 0
    check_ppm_region_color "$OUT_DIR/gfx-truecolor.ppm" 24 8 39 23 0 255 0 0
    check_ppm_region_color "$OUT_DIR/gfx-truecolor.ppm" 40 8 55 23 0 0 255 0
fi

if [[ "$GFX_AVAILABLE" == "passed" ]]; then
    for mode in 0 1 2 3 4 5 6 7 8 9 10 11 12 13; do
        dump="$OUT_DIR/gfx-screen-$mode.ppm"
        rm -f "$dump"
        run_smoke "gfx SCREEN $mode" \
            env FBGFX=Darwin FBGFX_SMOKE_MODE="$mode" FBGFX_DARWIN_DUMP="$dump" "$GFX_SCREEN_MODES"

        case "$mode" in
            0|3|4|5|6)
                ;;
            2|10|11)
                check_ppm_region_color "$dump" 8 0 23 63 255 255 255 8
                ;;
            *)
                check_ppm_region_color "$dump" 8 0 23 63 255 0 0 8
                check_ppm_region_color "$dump" 32 0 47 63 0 255 0 8
                check_ppm_region_color "$dump" 56 0 71 63 0 0 255 8
                ;;
        esac
    done
else
    echo "macos-smoke: skipped gfx SCREEN modes"
fi

run_smoke "sfx CoreAudio playback" \
    env SFXLIB_DRIVER=CoreAudio SFXLIB_DRIVER_DUMP="$OUT_DIR/sfx-coreaudio.txt" SFXLIB_DRIVER_DUMP_FRAMES=4096 "$SFX_PLAYBACK"
if [[ "$SMOKE_RESULT" == "passed" ]]; then
    check_audio_dump "$OUT_DIR/sfx-coreaudio.txt"
fi

run_smoke "sfx CoreAudio capture" env SFXLIB_DRIVER=CoreAudio "$SFX_CAPTURE"

echo "macos-smoke: all runnable smoke tests passed"
