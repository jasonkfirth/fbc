#!/usr/bin/env bash
#
#   Project: FreeBASIC Wii testing
#   --------------------------------
#
#   File: msys2-test-freebasic-wii-gfx-sfx-smoke.sh
#
#   Purpose:
#
#       Build and run the focused Wii gfxlib/sfxlib smoke program in Dolphin.
#
#   Responsibilities:
#
#       * compile tests/wii/gfx_sfx_smoke.bas for the Wii target
#       * run the generated DOL in Dolphin with writable SD folder sync
#       * collect the BASIC pass/fail log from the SD folder
#       * verify Wii XFB PPM dumps contain red, green, and blue regions
#       * convert the sfxlib driver-edge sample dump into a WAV artifact
#
#   This file intentionally does NOT contain:
#
#       * full fbcunit policy
#       * exampleageddon policy
#       * Dolphin download or installation logic
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

BUILDROOT="${BUILDROOT:-build/wii-gfx-sfx-smoke}"
PACKAGE_ROOT="${PACKAGE_ROOT:-}"
FBC_WII="${FBC_WII:-}"
DOLPHIN="${DOLPHIN:-${ROOT_DIR}/build/tools/Dolphin-x64/Dolphin.exe}"
TIMEOUT_SECONDS="${TIMEOUT_SECONDS:-240}"
KEEP_BUILDROOT=0
NO_RUN=0

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

msg()
{
    printf '\n==> %s\n' "$*"
}

fail()
{
    printf 'error: %s\n' "$*" >&2
    exit 1
}

have()
{
    command -v "$1" >/dev/null 2>&1
}

run()
{
    printf '+'
    printf ' %q' "$@"
    printf '\n'
    "$@"
}

usage()
{
    cat <<EOF
Usage: $(basename "$0") [options]

Build and run the Wii gfxlib/sfxlib smoke test in Dolphin.

Options:
  --package-root DIR       Optional packaged FreeBASIC Wii tree
  --fbc-wii PATH           fbc-wii launcher to use
  --buildroot DIR          Temporary test root [${BUILDROOT}]
  --dolphin PATH           Dolphin executable [${DOLPHIN}]
  --timeout SECONDS        Dolphin timeout [${TIMEOUT_SECONDS}]
  --no-run                 Build the DOL but do not start Dolphin
  --keep-buildroot         Keep staged files
  -h, --help               Show this help
EOF
}

normalize_path()
{
    local path="$1"

    if [ -n "${path}" ] && have cygpath; then
        cygpath -u "${path}" 2>/dev/null || printf '%s\n' "${path}"
    else
        printf '%s\n' "${path}"
    fi
}

windows_path()
{
    local path="$1"

    if have cygpath; then
        cygpath -aw "${path}"
    else
        printf '%s\n' "${path}"
    fi
}

remove_if_requested()
{
    local path="$1"

    if [ "${KEEP_BUILDROOT}" -eq 0 ]; then
        rm -rf "${path}"
    fi
}

# ---------------------------------------------------------------------------
# Argument handling
# ---------------------------------------------------------------------------

while [ "$#" -gt 0 ]; do
    case "$1" in
        --package-root)
            PACKAGE_ROOT="$(normalize_path "$2")"
            shift 2
            ;;
        --fbc-wii)
            FBC_WII="$(normalize_path "$2")"
            shift 2
            ;;
        --buildroot)
            BUILDROOT="$(normalize_path "$2")"
            shift 2
            ;;
        --dolphin)
            DOLPHIN="$(normalize_path "$2")"
            shift 2
            ;;
        --timeout)
            TIMEOUT_SECONDS="$2"
            shift 2
            ;;
        --no-run)
            NO_RUN=1
            shift
            ;;
        --keep-buildroot)
            KEEP_BUILDROOT=1
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

BUILDROOT="$(normalize_path "${BUILDROOT}")"
PACKAGE_ROOT="$(normalize_path "${PACKAGE_ROOT}")"
DOLPHIN="$(normalize_path "${DOLPHIN}")"

if [ -z "${FBC_WII}" ]; then
    if [ -n "${PACKAGE_ROOT}" ] && [ -x "${PACKAGE_ROOT}/fbc-wii-package.sh" ]; then
        FBC_WII="${PACKAGE_ROOT}/fbc-wii-package.sh"
    elif [ -x "${ROOT_DIR}/src/tools/wii/fbc-wii" ]; then
        FBC_WII="${ROOT_DIR}/src/tools/wii/fbc-wii"
    else
        fail "could not find fbc-wii; pass --fbc-wii"
    fi
fi

FBC_WII="$(normalize_path "${FBC_WII}")"

if [ ! -x "${FBC_WII}" ]; then
    fail "fbc-wii launcher is not executable: ${FBC_WII}"
fi

# ---------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------

build_smoke()
{
    local src="${ROOT_DIR}/tests/wii/gfx_sfx_smoke.bas"
    local dol="${BUILDROOT}/wii-gfx-sfx-smoke.dol"
    local staged_prefix="${BUILDROOT}/repo-prefix"
    local compiler="${ROOT_DIR}/bin/fbc-wii-compiler.exe"
    local libdir="${staged_prefix}/lib/freebasic-wii/wii-powerpc"

    msg "Building Wii gfx/sfx smoke DOL"

    mkdir -p "${BUILDROOT}"

    if [ "${FBC_WII}" = "${ROOT_DIR}/src/tools/wii/fbc-wii" ]; then
        remove_if_requested "${staged_prefix}"
        mkdir -p "${libdir}" "${staged_prefix}/include/freebasic-wii"
        cp -f "${ROOT_DIR}/lib/freebasic/wii/"* "${libdir}/"
        cp -a "${ROOT_DIR}/inc/." "${staged_prefix}/include/freebasic-wii/"

        run env \
            FBWII_PREFIX="${staged_prefix}" \
            FBWII_COMPILER="${compiler}" \
            FBWII_INCDIR="${ROOT_DIR}/inc" \
            FBWII_LIBDIR="${libdir}" \
            FBWII_TMPDIR="${BUILDROOT}/fbwii-tmp" \
            "${FBC_WII}" \
            "${src}" \
            -mt \
            -x "${dol}"
    else
        run env \
            FBWII_TMPDIR="${BUILDROOT}/fbwii-tmp" \
            "${FBC_WII}" \
            "${src}" \
            -mt \
            -x "${dol}"
    fi

    if [ ! -f "${dol}" ]; then
        fail "smoke DOL was not produced: ${dol}"
    fi
}

# ---------------------------------------------------------------------------
# Dolphin runner
# ---------------------------------------------------------------------------

write_dolphin_runner()
{
    local script="$1"

    cat > "${script}" <<'EOF'
param(
    [Parameter(Mandatory=$true)][string]$DolphinPath,
    [Parameter(Mandatory=$true)][string]$DolPath,
    [Parameter(Mandatory=$true)][string]$UserPath,
    [Parameter(Mandatory=$true)][string]$SdRoot,
    [Parameter(Mandatory=$true)][string]$SdImage,
    [Parameter(Mandatory=$true)][int]$TimeoutSeconds
)

$ErrorActionPreference = 'Stop'

if (Test-Path -LiteralPath $UserPath) {
    Remove-Item -LiteralPath $UserPath -Recurse -Force
}

Remove-Item -LiteralPath $SdImage -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path (Join-Path $UserPath 'Config') | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $UserPath 'Dump') | Out-Null

$sdRootIni = $SdRoot.Replace('\', '/')
$sdImageIni = $SdImage.Replace('\', '/')

@"
[Analytics]
Enabled = False
PermissionAsked = True
[Interface]
UsePanicHandlers = False
ConfirmStop = False
[Core]
WiiSDCard = True
WiiSDCardAllowWrites = True
WiiSDCardEnableFolderSync = True
WiiSDCardFilesize = 0x0000000008000000
[General]
WiiSDCardPath = $sdImageIni
WiiSDCardSyncFolder = $sdRootIni
[Movie]
DumpFrames = False
DumpAudio = False
"@ | Set-Content -Encoding ASCII (Join-Path $UserPath 'Config\Dolphin.ini')

@"
[Settings]
Backend = Software Renderer
Crop = False
FrameDumpResolutionType = 2
PNGCompressionLevel = 1
[Hacks]
ImmediateXFB = True
SkipDuplicateXFBs = False
"@ | Set-Content -Encoding ASCII (Join-Path $UserPath 'Config\GFX.ini')

$process = Start-Process -FilePath $DolphinPath -ArgumentList @('--batch', '--exec', $DolPath, '--user', $UserPath) -PassThru -WindowStyle Hidden

if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
    Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
    throw "Dolphin timed out after $TimeoutSeconds seconds"
}

if ($process.ExitCode -ne 0) {
    throw "Dolphin exited with code $($process.ExitCode)"
}
EOF
}

stage_sd_card()
{
    local sdroot="${BUILDROOT}/sd"

    msg "Preparing Dolphin SD-card folder"

    remove_if_requested "${sdroot}"
    mkdir -p "${sdroot}"
}

run_dolphin()
{
    local sdroot="${BUILDROOT}/sd"
    local userdir="${BUILDROOT}/dolphin-user"
    local sdimage="${BUILDROOT}/wii-smoke-sd.raw"
    local ps1="${BUILDROOT}/run-dolphin.ps1"
    local dol="${BUILDROOT}/wii-gfx-sfx-smoke.dol"

    if [ "${NO_RUN}" -ne 0 ]; then
        msg "Skipping Dolphin run because --no-run was passed"
        return 0
    fi

    if [ ! -f "${DOLPHIN}" ]; then
        fail "Dolphin was not found: ${DOLPHIN}"
    fi

    msg "Running Wii gfx/sfx smoke in Dolphin"

    write_dolphin_runner "${ps1}"

    run powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$(windows_path "${ps1}")" \
        -DolphinPath "$(windows_path "${DOLPHIN}")" \
        -DolPath "$(windows_path "${dol}")" \
        -UserPath "$(windows_path "${userdir}")" \
        -SdRoot "$(windows_path "${sdroot}")" \
        -SdImage "$(windows_path "${sdimage}")" \
        -TimeoutSeconds "${TIMEOUT_SECONDS}"
}

# ---------------------------------------------------------------------------
# Artifact analysis
# ---------------------------------------------------------------------------

analyze_artifacts()
{
    local sdroot="${BUILDROOT}/sd"
    local artifacts="${BUILDROOT}/artifacts"
    local log="${sdroot}/wii-gfx-sfx-smoke.log"
    local dump="${sdroot}/wii-sfx-driver.tmp"
    local wav="${artifacts}/wii-sfx-driver.wav"
    local analyzer="${BUILDROOT}/analyze-artifacts.ps1"

    if [ "${NO_RUN}" -ne 0 ]; then
        return 0
    fi

    msg "Analyzing Wii smoke artifacts"

    mkdir -p "${artifacts}"

    if [ ! -f "${log}" ]; then
        fail "smoke log was not created: ${log}"
    fi

    cp -f "${log}" "${artifacts}/"

    if grep -q '^FAIL ' "${log}"; then
        cat "${log}" >&2
        fail "smoke program reported failures"
    fi

    if ! grep -qx 'RESULT PASS' "${log}"; then
        cat "${log}" >&2
        fail "smoke program did not report RESULT PASS"
    fi

    if [ ! -s "${dump}" ]; then
        fail "sfxlib driver dump was not created: ${dump}"
    fi

    cp -f "${dump}" "${artifacts}/"
    cp -f "${sdroot}"/wii-xfb-*.ppm "${artifacts}/" 2>/dev/null || true

    cat > "${analyzer}" <<'EOF'
param(
    [Parameter(Mandatory=$true)][string]$SdRoot,
    [Parameter(Mandatory=$true)][string]$DumpPath,
    [Parameter(Mandatory=$true)][string]$WavPath
)

$ErrorActionPreference = 'Stop'
$Culture = [Globalization.CultureInfo]::InvariantCulture

function Read-Token([IO.BinaryReader]$Reader) {
    $bytes = New-Object System.Collections.Generic.List[byte]

    while ($true) {
        $value = $Reader.Read()
        if ($value -lt 0) {
            break
        }

        $byte = [byte]$value

        if ($byte -eq 35) {
            while ($true) {
                $value = $Reader.Read()
                if ($value -lt 0 -or $value -eq 10) {
                    break
                }
            }
            continue
        }

        if ($byte -eq 9 -or $byte -eq 10 -or $byte -eq 13 -or $byte -eq 32) {
            if ($bytes.Count -gt 0) {
                break
            }
            continue
        }

        $bytes.Add($byte)
    }

    return [Text.Encoding]::ASCII.GetString($bytes.ToArray())
}

function Read-Ppm([string]$Path) {
    $stream = [IO.File]::OpenRead($Path)
    try {
        $reader = New-Object IO.BinaryReader($stream)
        $magic = Read-Token $reader
        if ($magic -ne 'P6') {
            throw "${Path}: expected P6 PPM"
        }

        $width = [int](Read-Token $reader)
        $height = [int](Read-Token $reader)
        $maxval = [int](Read-Token $reader)
        if ($maxval -ne 255) {
            throw "${Path}: expected maxval 255"
        }

        $data = $reader.ReadBytes($width * $height * 3)
        if ($data.Length -ne ($width * $height * 3)) {
            throw "${Path}: truncated PPM"
        }

        return @{
            Width = $width
            Height = $height
            Data = $data
        }
    } finally {
        $stream.Close()
    }
}

function Get-Pixel($Image, [int]$X, [int]$Y) {
    $offset = (($Y * $Image.Width) + $X) * 3
    return @(
        [int]$Image.Data[$offset],
        [int]$Image.Data[$offset + 1],
        [int]$Image.Data[$offset + 2]
    )
}

function Strong-Red($Rgb) {
    return $Rgb[0] -gt 140 -and $Rgb[0] -gt ($Rgb[1] + 45) -and $Rgb[0] -gt ($Rgb[2] + 45)
}

function Strong-Green($Rgb) {
    return $Rgb[1] -gt 110 -and $Rgb[1] -gt ($Rgb[0] + 35) -and $Rgb[1] -gt ($Rgb[2] + 35)
}

function Strong-Blue($Rgb) {
    return $Rgb[2] -gt 110 -and $Rgb[2] -gt ($Rgb[0] + 35) -and $Rgb[2] -gt ($Rgb[1] + 35)
}

function Dark-Pixel($Rgb) {
    return $Rgb[0] -lt 35 -and $Rgb[1] -lt 35 -and $Rgb[2] -lt 35
}

function Bright-Pixel($Rgb) {
    return $Rgb[0] -gt 150 -and $Rgb[1] -gt 150 -and $Rgb[2] -gt 150
}

function Test-RgbBars($Ppm) {
    $image = Read-Ppm $Ppm.FullName
    $y = [int]($image.Height / 2)
    $red = Get-Pixel $image ([int]($image.Width / 6)) $y
    $green = Get-Pixel $image ([int]($image.Width / 2)) $y
    $blue = Get-Pixel $image ([int](($image.Width * 5) / 6)) $y

    if (-not ((Strong-Red $red) -and (Strong-Green $green) -and (Strong-Blue $blue))) {
        throw "$($Ppm.Name): unexpected RGB samples red=$red green=$green blue=$blue"
    }
}

function Test-MonoBars($Ppm) {
    $image = Read-Ppm $Ppm.FullName
    $y = [int]($image.Height / 2)
    $left = Get-Pixel $image ([int]($image.Width / 6)) $y
    $middle = Get-Pixel $image ([int]($image.Width / 2)) $y
    $right = Get-Pixel $image ([int](($image.Width * 5) / 6)) $y

    if (-not ((Dark-Pixel $left) -and (Bright-Pixel $middle) -and (Bright-Pixel $right))) {
        throw "$($Ppm.Name): unexpected mono samples left=$left middle=$middle right=$right"
    }
}

function Test-CgaScreen1Bars($Ppm) {
    $image = Read-Ppm $Ppm.FullName
    $y = [int]($image.Height / 2)
    $left = Get-Pixel $image ([int]($image.Width / 6)) $y
    $middle = Get-Pixel $image ([int]($image.Width / 2)) $y
    $right = Get-Pixel $image ([int](($image.Width * 5) / 6)) $y

    if (-not ((Strong-Green $left) -and (Strong-Red $middle) -and
              ($right[0] -gt 120) -and ($right[1] -gt 50) -and ($right[2] -lt 60))) {
        throw "$($Ppm.Name): unexpected SCREEN 1 CGA samples left=$left middle=$middle right=$right"
    }
}

function Last-PpmForPrefix([string]$Prefix) {
    $matches = @(Get-ChildItem -LiteralPath $SdRoot -Filter "$Prefix-*.ppm" | Sort-Object Name)
    if ($matches.Count -lt 1) {
        throw "expected an XFB dump for $Prefix"
    }

    return @($matches | Select-Object -Last 1)[0]
}

$ppms = @(Get-ChildItem -LiteralPath $SdRoot -Filter 'wii-xfb-*.ppm' | Sort-Object Name)
if ($ppms.Count -lt 15) {
    throw "expected at least 15 XFB dumps, found $($ppms.Count)"
}

$colorModes = @(7, 8, 9, 12, 13)
for ($mode = 1; $mode -le 13; ++$mode) {
    $prefix = "wii-xfb-screen-{0:00}" -f $mode
    $ppm = Last-PpmForPrefix $prefix

    if ($mode -eq 1) {
        Test-CgaScreen1Bars $ppm
    } elseif ($colorModes -contains $mode) {
        Test-RgbBars $ppm
    } else {
        Test-MonoBars $ppm
    }
}

Test-RgbBars (Last-PpmForPrefix 'wii-xfb-screenset-a')
Test-RgbBars (Last-PpmForPrefix 'wii-xfb-screenset-b')

$samples = New-Object System.Collections.Generic.List[double]
foreach ($line in [IO.File]::ReadLines($DumpPath)) {
    $value = 0.0
    if ([double]::TryParse($line.Trim(),
            [Globalization.NumberStyles]::Float,
            $Culture,
            [ref]$value)) {
        $samples.Add($value)
    }
}

if ($samples.Count -lt 16000) {
    throw "expected at least 16000 audio samples, found $($samples.Count)"
}

$sumSquares = 0.0
$peak = 0.0
foreach ($sample in $samples) {
    $sumSquares += $sample * $sample
    $abs = [Math]::Abs($sample)
    if ($abs -gt $peak) {
        $peak = $abs
    }
}

$rms = [Math]::Sqrt($sumSquares / [double]$samples.Count)
if ($rms -lt 0.02 -or $peak -lt 0.08) {
    throw ("audio signal too quiet: rms={0:N4} peak={1:N4}" -f $rms, $peak)
}

New-Item -ItemType Directory -Force -Path (Split-Path -Parent $WavPath) | Out-Null

$dataBytes = $samples.Count * 2
$stream = [IO.File]::Create($WavPath)
try {
    $writer = New-Object IO.BinaryWriter($stream)
    $ascii = [Text.Encoding]::ASCII

    $writer.Write($ascii.GetBytes('RIFF'))
    $writer.Write([int](36 + $dataBytes))
    $writer.Write($ascii.GetBytes('WAVE'))
    $writer.Write($ascii.GetBytes('fmt '))
    $writer.Write([int]16)
    $writer.Write([int16]1)
    $writer.Write([int16]1)
    $writer.Write([int]48000)
    $writer.Write([int](48000 * 2))
    $writer.Write([int16]2)
    $writer.Write([int16]16)
    $writer.Write($ascii.GetBytes('data'))
    $writer.Write([int]$dataBytes)

    foreach ($sample in $samples) {
        if ($sample -gt 1.0) {
            $sample = 1.0
        } elseif ($sample -lt -1.0) {
            $sample = -1.0
        }

        $writer.Write([int16]($sample * 32767.0))
    }
} finally {
    $stream.Close()
}

Write-Host ("xfb frames={0}" -f $ppms.Count)
Write-Host ("audio samples={0} rms={1:N4} peak={2:N4}" -f $samples.Count, $rms, $peak)
Write-Host ("audio wav={0}" -f $WavPath)
EOF

    run powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$(windows_path "${analyzer}")" \
        -SdRoot "$(windows_path "${sdroot}")" \
        -DumpPath "$(windows_path "${dump}")" \
        -WavPath "$(windows_path "${wav}")"

    msg "Smoke artifacts"
    printf 'log : %s\n' "${artifacts}/wii-gfx-sfx-smoke.log"
    printf 'wav : %s\n' "${wav}"
    printf 'xfb : %s\n' "${artifacts}/wii-xfb-*.ppm"
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*)
        ;;
    *)
        fail "this script is intended to run under MSYS2 on Windows"
        ;;
esac

msg "FreeBASIC Wii gfx/sfx smoke"
printf 'fbc-wii  : %s\n' "${FBC_WII}"
printf 'dolphin  : %s\n' "${DOLPHIN}"
printf 'build    : %s\n' "${BUILDROOT}"

build_smoke
stage_sd_card
run_dolphin
analyze_artifacts

msg "Wii gfx/sfx smoke complete"

# end of msys2-test-freebasic-wii-gfx-sfx-smoke.sh
