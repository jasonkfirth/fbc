#!/usr/bin/env bash
#
#   Project: FreeBASIC Wii testing
#   --------------------------------
#
#   File: msys2-test-freebasic-wii-fbcunit.sh
#
#   Purpose:
#
#       Build the fbcunit test runner with the packaged fbc-wii driver
#       and run it in Dolphin using an SD-card folder sync.
#
#   Responsibilities:
#
#       * locate the packaged FreeBASIC Wii install tree
#       * build tests/fbc-tests.dol for the Wii target
#       * stage the tests directory as Dolphin's writable SD-card root
#       * run Dolphin in batch mode and check the generated XML report
#
#   This file intentionally does NOT contain:
#
#       * installer construction
#       * exampleageddon policy
#       * Dolphin download or installation logic
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

FBVERSION="$({ sed -n 's/^FBVERSION[[:space:]]*:=[[:space:]]*//p' "${ROOT_DIR}/mk/version.mk" 2>/dev/null || true; } | head -n 1)"
if [ -z "${FBVERSION}" ]; then
    FBVERSION="unknown"
fi

BUILDROOT="${BUILDROOT:-/tmp/freebasic-wii-tests}"
PACKAGE_ROOT="${PACKAGE_ROOT:-/tmp/freebasic-wii-build/dist/FreeBASIC-${FBVERSION}-fbc-wii}"
DOLPHIN="${DOLPHIN:-${ROOT_DIR}/build/tools/Dolphin-x64/Dolphin.exe}"
TIMEOUT_SECONDS="${TIMEOUT_SECONDS:-1800}"
JOBS="${JOBS:-}"
NO_RUN=0
KEEP_BUILDROOT=0

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

Build fbcunit for Wii and run it in Dolphin.

Options:
  --package-root DIR       FreeBASIC Wii package tree [${PACKAGE_ROOT}]
  --buildroot DIR          Temporary test root [${BUILDROOT}]
  --dolphin PATH           Dolphin executable [${DOLPHIN}]
  --timeout SECONDS        Dolphin timeout [${TIMEOUT_SECONDS}]
  --jobs N                 Parallel make jobs
  --no-run                 Build fbc-tests.dol but do not start Dolphin
  --keep-buildroot         Keep staged Dolphin files
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

max_jobs()
{
    local count

    if [ -n "${JOBS}" ]; then
        printf '%s\n' "${JOBS}"
        return 0
    fi

    count="$(getconf _NPROCESSORS_ONLN 2>/dev/null || printf '2')"
    if [ -z "${count}" ] || [ "${count}" -lt 1 ] 2>/dev/null; then
        count=2
    fi

    printf '%s\n' "${count}"
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
        --jobs|-j)
            JOBS="$2"
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

if [ -z "${JOBS}" ]; then
    JOBS="$(max_jobs)"
fi

PACKAGE_ROOT="$(normalize_path "${PACKAGE_ROOT}")"
DOLPHIN="$(normalize_path "${DOLPHIN}")"

if [ ! -f "${PACKAGE_ROOT}/freebasic-wii-env.sh" ]; then
    fail "missing FreeBASIC Wii package tree: ${PACKAGE_ROOT}"
fi

# shellcheck disable=SC1090,SC1091
. "${PACKAGE_ROOT}/freebasic-wii-env.sh"

FBC_WII="${PACKAGE_ROOT}/fbc-wii-package.sh"
if [ ! -x "${FBC_WII}" ]; then
    fail "missing fbc-wii package launcher: ${FBC_WII}"
fi

# ---------------------------------------------------------------------------
# Build fbcunit
# ---------------------------------------------------------------------------

build_fbcunit()
{
    msg "Building fbcunit for Wii"

    (
        cd "${ROOT_DIR}/tests"

        # unit-tests.mk keeps target-neutral object names.  A desktop test run
        # can therefore leave objects that the PowerPC linker cannot consume.
        # Clean with the same target settings before starting the Wii build.
        run make -f unit-tests.mk \
            FBC="${FBC_WII}" \
            ARCH=powerpc \
            GEN=gcc \
            TARGET_OS=wii \
            TARGET_EXEEXT=.dol \
            ENABLE_CONSOLE_OUTPUT=1 \
            WII_DEVKITPRO="${DEVKITPRO}" \
            WII_FB_PREFIX="${PACKAGE_ROOT}" \
            clean

        run make -f unit-tests.mk -j"${JOBS}" \
            FBC="${FBC_WII}" \
            ARCH=powerpc \
            GEN=gcc \
            TARGET_OS=wii \
            TARGET_EXEEXT=.dol \
            ENABLE_CONSOLE_OUTPUT=1 \
            WII_DEVKITPRO="${DEVKITPRO}" \
            WII_FB_PREFIX="${PACKAGE_ROOT}" \
            build_tests
    )

    if [ ! -f "${ROOT_DIR}/tests/fbc-tests.dol" ]; then
        fail "tests/fbc-tests.dol was not produced"
    fi
}

# ---------------------------------------------------------------------------
# Dolphin runner
# ---------------------------------------------------------------------------

stage_sd_card()
{
    local sdroot="${BUILDROOT}/sd"

    msg "Staging Dolphin SD-card directory"

    remove_if_requested "${sdroot}"
    mkdir -p "${sdroot}"

    run rsync -a --delete \
        --exclude '*.o' \
        --exclude '*.a' \
        --exclude '*.elf' \
        --exclude '*.dol' \
        --exclude '*.exe' \
        --exclude '__fb_ct*' \
        "${ROOT_DIR}/tests/" "${sdroot}/"
}

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
"@ | Set-Content -Encoding ASCII (Join-Path $UserPath 'Config\Dolphin.ini')

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

run_dolphin()
{
    local sdroot="${BUILDROOT}/sd"
    local userdir="${BUILDROOT}/dolphin-user"
    local sdimage="${BUILDROOT}/fbctests-sd.raw"
    local report="${sdroot}/fbctests-wii.xml"
    local ps1="${BUILDROOT}/run-dolphin.ps1"

    if [ "${NO_RUN}" -ne 0 ]; then
        msg "Skipping Dolphin run because --no-run was passed"
        return 0
    fi

    if [ ! -f "${DOLPHIN}" ]; then
        fail "Dolphin was not found: ${DOLPHIN}"
    fi

    msg "Running fbcunit in Dolphin"

    mkdir -p "${BUILDROOT}"
    rm -f "${report}"
    write_dolphin_runner "${ps1}"

    run powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$(windows_path "${ps1}")" \
        -DolphinPath "$(windows_path "${DOLPHIN}")" \
        -DolPath "$(windows_path "${ROOT_DIR}/tests/fbc-tests.dol")" \
        -UserPath "$(windows_path "${userdir}")" \
        -SdRoot "$(windows_path "${sdroot}")" \
        -SdImage "$(windows_path "${sdimage}")" \
        -TimeoutSeconds "${TIMEOUT_SECONDS}"

    if [ ! -f "${report}" ]; then
        fail "Dolphin run completed but ${report} was not created"
    fi

    if grep -Eq 'failures="[1-9]|errors="[1-9]' "${report}"; then
        printf 'fbcunit report contains failures or errors: %s\n' "${report}" >&2
        grep -E 'failures="[1-9]|errors="[1-9]' "${report}" >&2 || true
        exit 1
    fi

    if have python; then
        python - "${report}" <<'PY'
import sys
import xml.etree.ElementTree as ET

root = ET.parse(sys.argv[1]).getroot()
tests = failures = errors = 0

for suite in root.iter('testsuite'):
    tests += int(suite.attrib.get('tests', '0'))
    failures += int(suite.attrib.get('failures', '0'))
    errors += int(suite.attrib.get('errors', '0'))

print(f'fbcunit: tests={tests} failures={failures} errors={errors}')
PY
    fi

    printf 'report: %s\n' "${report}"
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

msg "FreeBASIC Wii fbcunit test"
printf 'package : %s\n' "${PACKAGE_ROOT}"
printf 'dolphin : %s\n' "${DOLPHIN}"
printf 'build   : %s\n' "${BUILDROOT}"
printf 'jobs    : %s\n' "${JOBS}"

build_fbcunit
stage_sd_card
run_dolphin

msg "Wii fbcunit test complete"

# end of msys2-test-freebasic-wii-fbcunit.sh
