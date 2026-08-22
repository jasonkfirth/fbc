#!/usr/bin/env bash
#
#   Project: FreeBASIC Wii packaging
#   ---------------------------------
#
#   File: msys2-build-freebasic-wii.sh
#
#   Purpose:
#
#       Build a Windows/MSYS2 install tree, zip archive, and NSIS installer for the
#       FreeBASIC Wii target.
#
#   Responsibilities:
#
#       * build the host-side fbc-wii compiler driver
#       * build the Wii runtime, gfxlib2, and sfxlib libraries
#       * stage the FreeBASIC Wii files into a relocatable package tree
#       * optionally bundle the devkitPro Wii toolchain
#       * emit command-line launchers, a zip archive, and an NSIS installer
#
#   This file intentionally does NOT contain:
#
#       * Dolphin emulator test execution
#       * fbcunit or exampleageddon test policy
#       * devkitPro repository bootstrap logic
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

FBVERSION="$({ sed -n 's/^FBVERSION[[:space:]]*:=[[:space:]]*//p' "${ROOT_DIR}/mk/version.mk" 2>/dev/null || true; } | head -n 1)"
if [ -z "${FBVERSION}" ]; then
    FBVERSION="unknown"
fi

BUILDROOT="${BUILDROOT:-/tmp/freebasic-wii-build}"
WORKTREE="${WORKTREE:-${BUILDROOT}/source}"
STAGEDIR="${STAGEDIR:-${BUILDROOT}/stage}"
DISTDIR="${DISTDIR:-${BUILDROOT}/dist}"
INSTALL_SUBDIR="${INSTALL_SUBDIR:-FreeBASIC-${FBVERSION}-fbc-wii}"
PACKAGE_ROOT="${PACKAGE_ROOT:-${DISTDIR}/${INSTALL_SUBDIR}}"
OUT="${OUT:-${ROOT_DIR}/out/mingw32-wii}"
ARCHIVE_PATH="${ARCHIVE_PATH:-${OUT}/FreeBASIC-${FBVERSION}-fbc-wii.zip}"
INSTALLER_PATH="${INSTALLER_PATH:-${OUT}/FreeBASIC-${FBVERSION}-fbc-wii-setup.exe}"
NSIS_EXE="${NSIS_EXE:-/mingw64/bin/makensis.exe}"
JOBS="${JOBS:-}"
HOST_FBC_TARGET="${HOST_FBC_TARGET:-win64}"
WII_TARGET_TRIPLET="${WII_TARGET_TRIPLET:-powerpc-eabi}"
WII_TARGET_KEY="${WII_TARGET_KEY:-wii-powerpc}"
DEVKITPRO_ARG=""
DEVKITPPC_ARG=""
BUNDLE_DEVKITPRO=1
DO_DEPS=1
DO_SOURCE_SYNC=1
DO_BUILD=1
DO_PACKAGE=1
DO_INSTALLER=1
DO_VALIDATE=1
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

Build a Windows package containing the FreeBASIC Wii compiler driver,
Wii runtime libraries, and optionally a bundled devkitPro toolchain.

Options:
  --buildroot DIR          Temporary build root [${BUILDROOT}]
  --package-root DIR       Package directory to create [${PACKAGE_ROOT}]
  --archive PATH           Zip archive output [${ARCHIVE_PATH}]
  --installer PATH         NSIS installer output [${INSTALLER_PATH}]
  --out DIR                Output directory for default artifacts [${OUT}]
  --devkitpro DIR          devkitPro root [/opt/devkitpro or DEVKITPRO]
  --devkitppc DIR          devkitPPC root [DEVKITPRO/devkitPPC]
  --jobs N                 Parallel make jobs
  --no-bundle-devkitpro    Do not copy devkitPro into the installer tree
  --skip-deps              Do not install MSYS2 build dependencies
  --skip-source-sync       Build in the repository directly
  --reuse-worktree         Reuse WORKTREE without synchronizing the source
  --skip-build             Reuse an existing build tree
  --skip-package           Do not assemble the package tree
  --skip-installer         Do not build the NSIS installer
  --skip-validate          Do not compile the packaged hello-world test
  --keep-buildroot         Keep temporary package staging directories
  -h, --help               Show this help

Environment:
  NSIS_EXE                 makensis executable [${NSIS_EXE}]
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

first_existing()
{
    local path

    for path in "$@"; do
        if [ -e "${path}" ]; then
            printf '%s\n' "${path}"
            return 0
        fi
    done

    return 1
}

first_command()
{
    local tool

    for tool in "$@"; do
        if command -v "${tool}" >/dev/null 2>&1; then
            command -v "${tool}"
            return 0
        fi
    done

    return 1
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

extract_host_triplet()
{
    local triplet

    if have gcc; then
        triplet="$(gcc -dumpmachine 2>/dev/null || true)"
        if [ -n "${triplet}" ]; then
            printf '%s\n' "${triplet}"
            return 0
        fi
    fi

    printf 'x86_64-w64-mingw32\n'
}

detect_host_fbc()
{
    local candidate

    for candidate in \
        "${HOST_FBC:-}" \
        "${ROOT_DIR}/bin/fbc.exe" \
        "${ROOT_DIR}/bin/fbc" \
        "${ROOT_DIR}/bootstrap/win64/fbc.exe" \
        "${ROOT_DIR}/bootstrap/win64/fbc" \
        "$(command -v fbc 2>/dev/null || true)"
    do
        if [ -n "${candidate}" ] && [ -x "${candidate}" ]; then
            printf '%s\n' "${candidate}"
            return 0
        fi
    done

    return 1
}

copy_runtime_dlls()
{
    local exe="$1"
    local dest="$2"
    local dll

    if ! have ldd; then
        return 0
    fi

    mkdir -p "${dest}"

    while IFS= read -r dll; do
        if [ -f "${dll}" ]; then
            cp -f "${dll}" "${dest}/"
        fi
    done < <(ldd "${exe}" 2>/dev/null | awk '
        /=>/ && $3 ~ /^\// { print $3 }
        /^[[:space:]]*\// { print $1 }
    ')
}

copy_tool()
{
    local tool="$1"
    local dest="$2"
    local path

    path="$(command -v "${tool}" 2>/dev/null || true)"
    case "${path}" in
        /*) ;;
        *) return 0 ;;
    esac

    if [ ! -f "${path}" ]; then
        return 0
    fi

    mkdir -p "${dest}"
    cp -f "${path}" "${dest}/"
    copy_runtime_dlls "${path}" "${dest}"
}

remove_if_requested()
{
    local path="$1"

    if [ "${KEEP_BUILDROOT}" -eq 0 ]; then
        rm -rf "${path}"
    fi
}

sanitize_build_tree()
{
    local tree="$1"

    rm -rf \
        "${tree:?}/bin" \
        "${tree:?}/lib/freebasic" \
        "${tree:?}/obj" \
        "${tree:?}/.fbtmp" \
        "${tree:?}/tmp"
}

sync_source_tree()
{
    msg "Preparing isolated source tree"

    remove_if_requested "${WORKTREE}"
    mkdir -p "${WORKTREE}"

    run rsync -a --delete --delete-excluded --prune-empty-dirs \
        --exclude-from "${ROOT_DIR}/mk/source-copy-excludes.rsync" \
        "${ROOT_DIR}/" "${WORKTREE}/"

    sanitize_build_tree "${WORKTREE}"
}

# ---------------------------------------------------------------------------
# Argument handling
# ---------------------------------------------------------------------------

while [ "$#" -gt 0 ]; do
    case "$1" in
        --buildroot)
            BUILDROOT="$(normalize_path "$2")"
            shift 2
            ;;
        --package-root)
            PACKAGE_ROOT="$(normalize_path "$2")"
            shift 2
            ;;
        --archive)
            ARCHIVE_PATH="$(normalize_path "$2")"
            shift 2
            ;;
        --installer)
            INSTALLER_PATH="$(normalize_path "$2")"
            shift 2
            ;;
        --out)
            OUT="$(normalize_path "$2")"
            ARCHIVE_PATH="${OUT}/FreeBASIC-${FBVERSION}-fbc-wii.zip"
            INSTALLER_PATH="${OUT}/FreeBASIC-${FBVERSION}-fbc-wii-setup.exe"
            shift 2
            ;;
        --devkitpro)
            DEVKITPRO_ARG="$(normalize_path "$2")"
            shift 2
            ;;
        --devkitppc)
            DEVKITPPC_ARG="$(normalize_path "$2")"
            shift 2
            ;;
        --jobs|-j)
            JOBS="$2"
            shift 2
            ;;
        --no-bundle-devkitpro)
            BUNDLE_DEVKITPRO=0
            shift
            ;;
        --skip-deps)
            DO_DEPS=0
            shift
            ;;
        --skip-source-sync)
            DO_SOURCE_SYNC=0
            WORKTREE="${ROOT_DIR}"
            shift
            ;;
        --reuse-worktree)
            DO_SOURCE_SYNC=0
            shift
            ;;
        --skip-build)
            DO_BUILD=0
            shift
            ;;
        --skip-package)
            DO_PACKAGE=0
            shift
            ;;
        --skip-installer)
            DO_INSTALLER=0
            shift
            ;;
        --skip-validate)
            DO_VALIDATE=0
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

STAGEDIR="${BUILDROOT}/stage"
DISTDIR="$(dirname "${PACKAGE_ROOT}")"
HOST_TRIPLET="$(extract_host_triplet)"

DEVKITPRO="$(normalize_path "${DEVKITPRO_ARG:-${DEVKITPRO:-/opt/devkitpro}}")"
DEVKITPPC="$(normalize_path "${DEVKITPPC_ARG:-${DEVKITPPC:-${DEVKITPRO}/devkitPPC}}")"
HOST_CC="$(first_command "${HOST_CC:-}" x86_64-pc-cygwin-gcc gcc || true)"
HOST_CXX="$(first_command "${HOST_CXX:-}" x86_64-pc-cygwin-g++ g++ || true)"
HOST_AS="$(first_command "${HOST_AS:-}" /usr/bin/as as || true)"
HOST_AR="$(first_command "${HOST_AR:-}" /usr/bin/ar ar || true)"
HOST_LD="$(first_command "${HOST_LD:-}" /usr/bin/ld ld || true)"
HOST_RANLIB="$(first_command "${HOST_RANLIB:-}" /usr/bin/ranlib ranlib || true)"
HOST_STRIP="$(first_command "${HOST_STRIP:-}" /usr/bin/strip strip || true)"
POWERPC_GCC="$(first_existing "${DEVKITPPC}/bin/powerpc-eabi-gcc.exe" "${DEVKITPPC}/bin/powerpc-eabi-gcc" || true)"
POWERPC_GXX="$(first_existing "${DEVKITPPC}/bin/powerpc-eabi-g++.exe" "${DEVKITPPC}/bin/powerpc-eabi-g++" || true)"
POWERPC_AS="$(first_existing "${DEVKITPPC}/bin/powerpc-eabi-as.exe" "${DEVKITPPC}/bin/powerpc-eabi-as" || true)"
POWERPC_AR="$(first_existing "${DEVKITPPC}/bin/powerpc-eabi-ar.exe" "${DEVKITPPC}/bin/powerpc-eabi-ar" || true)"
POWERPC_RANLIB="$(first_existing "${DEVKITPPC}/bin/powerpc-eabi-ranlib.exe" "${DEVKITPPC}/bin/powerpc-eabi-ranlib" || true)"
ELF2DOL="$(first_existing "${DEVKITPRO}/tools/bin/elf2dol.exe" "${DEVKITPRO}/tools/bin/elf2dol" || true)"

# ---------------------------------------------------------------------------
# Dependency checks
# ---------------------------------------------------------------------------

install_dependencies()
{
    if [ "${DO_DEPS}" -eq 0 ]; then
        return 0
    fi

    if ! have pacman; then
        msg "Skipping MSYS2 dependency installation because pacman was not found"
        return 0
    fi

    msg "Installing MSYS2 build dependencies"

    run pacman --needed --noconfirm -S \
        base-devel \
        binutils \
        libffi-devel \
        ncurses-devel \
        pkgconf \
        rsync \
        zip \
        p7zip \
        mingw-w64-ucrt-x86_64-gcc \
        mingw-w64-x86_64-nsis
}

ensure_host_build_tools()
{
    local missing=0

    msg "Checking MSYS2 host build tools"

    if [ -z "${HOST_CC}" ] || [ ! -x "${HOST_CC}" ]; then
        printf 'missing: host C compiler\n' >&2
        missing=1
    fi

    if [ -z "${HOST_CXX}" ] || [ ! -x "${HOST_CXX}" ]; then
        printf 'missing: host C++ compiler\n' >&2
        missing=1
    fi

    if [ -z "${HOST_AS}" ] || [ ! -x "${HOST_AS}" ]; then
        printf 'missing: host assembler\n' >&2
        missing=1
    fi

    if [ -z "${HOST_AR}" ] || [ ! -x "${HOST_AR}" ]; then
        printf 'missing: host archiver\n' >&2
        missing=1
    fi

    if [ -z "${HOST_LD}" ] || [ ! -x "${HOST_LD}" ]; then
        printf 'missing: host linker\n' >&2
        missing=1
    fi

    if [ -z "${HOST_RANLIB}" ] || [ ! -x "${HOST_RANLIB}" ]; then
        printf 'missing: host ranlib\n' >&2
        missing=1
    fi

    if [ -z "${HOST_STRIP}" ] || [ ! -x "${HOST_STRIP}" ]; then
        printf 'missing: host strip\n' >&2
        missing=1
    fi

    if [ "${missing}" -ne 0 ]; then
        fail "install MSYS2 gcc/binutils or rerun without --skip-deps"
    fi
}

ensure_devkitpro()
{
    local missing=0

    msg "Checking devkitPro Wii toolchain"

    if [ -z "${POWERPC_GCC}" ] || [ ! -x "${POWERPC_GCC}" ]; then
        printf 'missing: %s/bin/powerpc-eabi-gcc\n' "${DEVKITPPC}" >&2
        missing=1
    fi

    if [ -z "${POWERPC_GXX}" ] || [ ! -x "${POWERPC_GXX}" ]; then
        printf 'missing: %s/bin/powerpc-eabi-g++\n' "${DEVKITPPC}" >&2
        missing=1
    fi

    if [ -z "${POWERPC_AS}" ] || [ ! -x "${POWERPC_AS}" ]; then
        printf 'missing: %s/bin/powerpc-eabi-as\n' "${DEVKITPPC}" >&2
        missing=1
    fi

    if [ -z "${POWERPC_AR}" ] || [ ! -x "${POWERPC_AR}" ]; then
        printf 'missing: %s/bin/powerpc-eabi-ar\n' "${DEVKITPPC}" >&2
        missing=1
    fi

    if [ -z "${POWERPC_RANLIB}" ] || [ ! -x "${POWERPC_RANLIB}" ]; then
        printf 'missing: %s/bin/powerpc-eabi-ranlib\n' "${DEVKITPPC}" >&2
        missing=1
    fi

    if [ -z "${ELF2DOL}" ] || [ ! -x "${ELF2DOL}" ]; then
        printf 'missing: %s/tools/bin/elf2dol\n' "${DEVKITPRO}" >&2
        missing=1
    fi

    if [ ! -d "${DEVKITPRO}/libogc/include" ]; then
        printf 'missing: %s/libogc/include\n' "${DEVKITPRO}" >&2
        missing=1
    fi

    if [ ! -d "${DEVKITPRO}/libogc/lib/wii" ]; then
        printf 'missing: %s/libogc/lib/wii\n' "${DEVKITPRO}" >&2
        missing=1
    fi

    if [ "${missing}" -ne 0 ]; then
        fail "install devkitPro devkitPPC/libogc or pass --devkitpro and --devkitppc"
    fi
}

# ---------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------

build_wii_freebasic()
{
    local build_fbc

    if [ "${DO_BUILD}" -eq 0 ]; then
        return 0
    fi

    if [ "${DO_SOURCE_SYNC}" -ne 0 ]; then
        sync_source_tree
    elif [ "${WORKTREE}" = "${ROOT_DIR}" ]; then
        msg "Building directly in repository tree"
    else
        msg "Reusing isolated source tree"
    fi

    build_fbc="$(detect_host_fbc)" || fail "could not find a host fbc; set HOST_FBC=/path/to/fbc"

    msg "Building Wii compiler and libraries"

    (
        cd "${WORKTREE}"

        run make -f GNUmakefile -j"${JOBS}" \
            TARGET_TRIPLET="${HOST_TRIPLET}" \
            TARGET="${HOST_TRIPLET}" \
            CC="${HOST_CC}" \
            CXX="${HOST_CXX}" \
            AS="${HOST_AS}" \
            AR="${HOST_AR}" \
            LD="${HOST_LD}" \
            RANLIB="${HOST_RANLIB}" \
            STRIP="${HOST_STRIP}" \
            FBC="${build_fbc}" \
            BUILD_FBC="${build_fbc}" \
            BUILD_FBC_TARGET="${HOST_FBC_TARGET}" \
            compiler-wii

        run env PATH="${DEVKITPPC}/bin:${DEVKITPRO}/tools/bin:${PATH}" \
            make -f GNUmakefile -j"${JOBS}" \
            TARGET_OS=wii \
            TARGET_TRIPLET="${WII_TARGET_TRIPLET}" \
            TARGET="${WII_TARGET_TRIPLET}" \
            MULTILIB= \
            BUILD_PREFIX=powerpc-eabi- \
            DEVKITPRO="${DEVKITPRO}" \
            DEVKITPPC="${DEVKITPPC}" \
            WII_LIBOGC_INC="${DEVKITPRO}/libogc/include" \
            WII_LIBOGC_LIB="${DEVKITPRO}/libogc/lib/wii" \
            ELF2DOL="${ELF2DOL}" \
            CC="${POWERPC_GCC}" \
            CXX="${POWERPC_GXX}" \
            AS="${POWERPC_AS}" \
            LD="${POWERPC_GCC}" \
            AR="${POWERPC_AR}" \
            RANLIB="${POWERPC_RANLIB}" \
            FBC="${build_fbc}" \
            BUILD_FBC="${build_fbc}" \
            BUILD_FBC_TARGET="${WII_TARGET_KEY}" \
            BUILD_FBC_BUILDPREFIX= \
            rtlib fbrt gfxlib2 sfxlib

        remove_if_requested "${STAGEDIR}"
        mkdir -p "${STAGEDIR}"

        run make -f GNUmakefile \
            TARGET_TRIPLET="${HOST_TRIPLET}" \
            TARGET="${HOST_TRIPLET}" \
            TARGET_OS=win32 \
            MULTILIB= \
            DESTDIR="${STAGEDIR}" \
            prefix="/${INSTALL_SUBDIR}" \
            DEVKITPRO="${DEVKITPRO}" \
            DEVKITPPC="${DEVKITPPC}" \
            ELF2DOL="${ELF2DOL}" \
            CC="${HOST_CC}" \
            CXX="${HOST_CXX}" \
            AS="${HOST_AS}" \
            AR="${HOST_AR}" \
            LD="${HOST_LD}" \
            RANLIB="${HOST_RANLIB}" \
            STRIP="${HOST_STRIP}" \
            FBC="${build_fbc}" \
            BUILD_FBC="${build_fbc}" \
            BUILD_FBC_TARGET="${HOST_FBC_TARGET}" \
            WII_BUILD_LIBDIR="${WORKTREE}/lib/freebasic/wii" \
            install-wii
    )
}

# ---------------------------------------------------------------------------
# Package assembly
# ---------------------------------------------------------------------------

write_launchers()
{
    local root="$1"

    cat > "${root}/fbc-wii-package.sh" <<'EOF'
#!/usr/bin/env bash

set -e

SCRIPT_PATH="${BASH_SOURCE[0]//\\//}"
case "${SCRIPT_PATH}" in
    */*) SCRIPT_DIR="${SCRIPT_PATH%/*}" ;;
    *) SCRIPT_DIR="." ;;
esac
SCRIPT_DIR="$(cd "${SCRIPT_DIR}" && pwd)"

if [ -d "${SCRIPT_DIR}/toolchain/devkitpro/devkitPPC" ]; then
    export DEVKITPRO="${FBWII_DEVKITPRO:-${SCRIPT_DIR}/toolchain/devkitpro}"
    export DEVKITPPC="${FBWII_DEVKITPPC:-${DEVKITPRO}/devkitPPC}"
else
    export DEVKITPRO="${FBWII_DEVKITPRO:-${DEVKITPRO:-/opt/devkitpro}}"
    export DEVKITPPC="${FBWII_DEVKITPPC:-${DEVKITPPC:-${DEVKITPRO}/devkitPPC}}"
fi

export FBWII_PREFIX="${SCRIPT_DIR}"
export FBWII_LIBROOT="${SCRIPT_DIR}/lib/freebasic-wii"
export FBWII_COMPILER="${SCRIPT_DIR}/lib/freebasic-wii/bin/fbc-wii-compiler.exe"
export FBWII_INCDIR="${SCRIPT_DIR}/include/freebasic-wii"
export FBWII_LIBDIR="${SCRIPT_DIR}/lib/freebasic-wii/wii-powerpc"
export PATH="${SCRIPT_DIR}/toolchain/msys2/usr/bin:${DEVKITPPC}/bin:${DEVKITPRO}/tools/bin:${PATH}"

if [ -x "${SCRIPT_DIR}/fbc-wii.exe" ]; then
    exec "${SCRIPT_DIR}/fbc-wii.exe" "$@"
fi

exec "${SCRIPT_DIR}/bin/fbc-wii" "$@"
EOF

    cat > "${root}/freebasic-wii-env.sh" <<'EOF'
#!/usr/bin/env bash

SCRIPT_PATH="${BASH_SOURCE[0]//\\//}"
case "${SCRIPT_PATH}" in
    */*) SCRIPT_DIR="${SCRIPT_PATH%/*}" ;;
    *) SCRIPT_DIR="." ;;
esac
SCRIPT_DIR="$(cd "${SCRIPT_DIR}" && pwd)"

if [ -d "${SCRIPT_DIR}/toolchain/devkitpro/devkitPPC" ]; then
    export DEVKITPRO="${FBWII_DEVKITPRO:-${SCRIPT_DIR}/toolchain/devkitpro}"
    export DEVKITPPC="${FBWII_DEVKITPPC:-${DEVKITPRO}/devkitPPC}"
else
    export DEVKITPRO="${FBWII_DEVKITPRO:-${DEVKITPRO:-/opt/devkitpro}}"
    export DEVKITPPC="${FBWII_DEVKITPPC:-${DEVKITPPC:-${DEVKITPRO}/devkitPPC}}"
fi

export FBWII_PREFIX="${SCRIPT_DIR}"
export FBWII_LIBROOT="${SCRIPT_DIR}/lib/freebasic-wii"
export FBWII_COMPILER="${SCRIPT_DIR}/lib/freebasic-wii/bin/fbc-wii-compiler.exe"
export FBWII_INCDIR="${SCRIPT_DIR}/include/freebasic-wii"
export FBWII_LIBDIR="${SCRIPT_DIR}/lib/freebasic-wii/wii-powerpc"
export PATH="${SCRIPT_DIR}/toolchain/msys2/usr/bin:${DEVKITPPC}/bin:${DEVKITPRO}/tools/bin:${SCRIPT_DIR}:${PATH}"
EOF

    cat > "${root}/fbc-wii.cmd" <<'EOF'
@echo off
setlocal
set "FBWII_ROOT=%~dp0"
set "FBWII_BASH=%FBWII_ROOT%toolchain\msys2\usr\bin\bash.exe"
if not exist "%FBWII_BASH%" (
    echo Missing bundled MSYS2 bash: %FBWII_BASH%
    exit /b 1
)
"%FBWII_BASH%" "%FBWII_ROOT%fbc-wii-package.sh" %*
exit /b %ERRORLEVEL%
EOF

    cat > "${root}/freebasic-wii-shell.cmd" <<'EOF'
@echo off
setlocal
set "FBWII_ROOT=%~dp0"
set "PATH=%FBWII_ROOT%;%FBWII_ROOT%toolchain\msys2\usr\bin;%FBWII_ROOT%toolchain\devkitpro\devkitPPC\bin;%FBWII_ROOT%toolchain\devkitpro\tools\bin;%PATH%"
set "DEVKITPRO=%FBWII_ROOT%toolchain\devkitpro"
set "DEVKITPPC=%FBWII_ROOT%toolchain\devkitpro\devkitPPC"
set "FBWII_PREFIX=%FBWII_ROOT%"
cmd /k
EOF

    chmod +x "${root}/fbc-wii-package.sh" "${root}/freebasic-wii-env.sh"
}

write_distribution_notes()
{
    local root="$1"

    msg "Writing fbc-wii package notes"

    cat > "${root}/readme-fbc-wii.txt" <<EOF
FreeBASIC Wii ${FBVERSION}

Use fbc-wii.cmd from cmd.exe or PowerShell:

    fbc-wii.cmd program.bas -x program.dol

For games that load files from the current directory, build a homebrew folder
instead.  The wrapper writes boot.dol and copies the selected asset tree beside
it, which is the layout Dolphin, SD-card launchers, and the Homebrew Channel
expect:

    fbc-wii.cmd --bundle build\\mygame --assets game-folder game.bas

The bundled fbc-wii driver still accepts normal FreeBASIC compiler options.
The --bundle and --assets options are only packaging conveniences; they do not
change the FreeBASIC language or runtime API.
EOF
}

copy_msys2_runtime()
{
    local root="$1"
    local bindir="${root}/toolchain/msys2/usr/bin"

    msg "Copying minimal MSYS2 runtime"

    mkdir -p "${bindir}" "${root}/toolchain/msys2/tmp"

    for tool in bash sh env dirname pwd cygpath realpath sed tr mkdir mktemp cp rm mv find uname make cat chmod; do
        copy_tool "${tool}" "${bindir}"
    done
}

copy_devkitpro_tree()
{
    local root="$1"

    if [ "${BUNDLE_DEVKITPRO}" -eq 0 ]; then
        msg "Not bundling devkitPro; package will use DEVKITPRO from the environment"
        return 0
    fi

    msg "Copying devkitPro Wii toolchain"

    mkdir -p "${root}/toolchain"

    # Package data is copied to NTFS and does not need POSIX metadata replay.
    run rsync -a --no-perms --no-owner --no-group --delete \
        --exclude '.git' \
        --exclude 'var/cache/pacman/pkg' \
        --exclude 'packages' \
        "${DEVKITPRO}/" "${root}/toolchain/devkitpro/"
}

copy_optional_tree()
{
    local source="$1"
    local dest="$2"

    if [ -d "${source}" ]; then
        run rsync -a --no-perms --no-owner --no-group --delete \
            "${source}/" "${dest}/"
    fi
}

assemble_package()
{
    local stage_prefix="${STAGEDIR}"

    if [ "${DO_PACKAGE}" -eq 0 ]; then
        return 0
    fi

    if [ ! -d "${stage_prefix}" ]; then
        fail "missing staged install tree: ${stage_prefix}"
    fi

    msg "Assembling package tree"

    remove_if_requested "${PACKAGE_ROOT}"
    mkdir -p "${PACKAGE_ROOT}"

    run rsync -a --no-perms --no-owner --no-group --delete \
        "${stage_prefix}/" "${PACKAGE_ROOT}/"

    mkdir -p "${PACKAGE_ROOT}/doc" "${PACKAGE_ROOT}/examples"
    copy_optional_tree "${WORKTREE}/doc" "${PACKAGE_ROOT}/doc"
    copy_optional_tree "${WORKTREE}/examples" "${PACKAGE_ROOT}/examples"

    if [ -f "${WORKTREE}/changelog.txt" ]; then
        cp -f "${WORKTREE}/changelog.txt" "${PACKAGE_ROOT}/"
    fi

    if [ -f "${WORKTREE}/readme.txt" ]; then
        cp -f "${WORKTREE}/readme.txt" "${PACKAGE_ROOT}/"
    fi

    copy_msys2_runtime "${PACKAGE_ROOT}"
    copy_devkitpro_tree "${PACKAGE_ROOT}"
    write_launchers "${PACKAGE_ROOT}"
    write_distribution_notes "${PACKAGE_ROOT}"
}

# ---------------------------------------------------------------------------
# Validation
# ---------------------------------------------------------------------------

validate_package()
{
    local testdir="${BUILDROOT}/validate"
    local src="${testdir}/hello.bas"
    local out="${testdir}/hello.dol"
    local assetdir="${testdir}/assets"
    local bundledir="${testdir}/bundle"

    if [ "${DO_VALIDATE}" -eq 0 ]; then
        return 0
    fi

    msg "Validating packaged fbc-wii driver"

    remove_if_requested "${testdir}"
    mkdir -p "${testdir}"

    cat > "${src}" <<'EOF'
screenres 320, 240, 32
cls
print "hello from wii"
sleep
EOF

    run env PATH="/c/Windows/System32:/c/Windows" \
        cmd.exe //C "$(windows_path "${PACKAGE_ROOT}/fbc-wii.cmd")" "$(windows_path "${src}")" -x "$(windows_path "${out}")"

    if [ ! -f "${out}" ]; then
        fail "packaged fbc-wii did not produce ${out}"
    fi

    mkdir -p "${assetdir}"
    printf 'asset smoke\n' > "${assetdir}/readme.txt"

    run env PATH="/c/Windows/System32:/c/Windows" \
        cmd.exe //C "$(windows_path "${PACKAGE_ROOT}/fbc-wii.cmd")" \
        --bundle "$(windows_path "${bundledir}")" \
        --assets "$(windows_path "${assetdir}")" \
        "$(windows_path "${src}")"

    if [ ! -f "${bundledir}/boot.dol" ]; then
        fail "packaged fbc-wii --bundle did not produce ${bundledir}/boot.dol"
    fi

    if [ ! -f "${bundledir}/readme.txt" ]; then
        fail "packaged fbc-wii --assets did not copy ${bundledir}/readme.txt"
    fi
}

# ---------------------------------------------------------------------------
# Archive
# ---------------------------------------------------------------------------

build_archive()
{
    local package_name

    if [ "${DO_PACKAGE}" -eq 0 ]; then
        return 0
    fi

    have zip || fail "zip was not found"

    package_name="$(basename "${PACKAGE_ROOT}")"
    mkdir -p "$(dirname "${ARCHIVE_PATH}")"

    msg "Creating Wii distribution zip"
    rm -f "${ARCHIVE_PATH}"
    (
        cd "$(dirname "${PACKAGE_ROOT}")"
        run zip -qr "${ARCHIVE_PATH}" "${package_name}"
    )
}

# ---------------------------------------------------------------------------
# Installer
# ---------------------------------------------------------------------------

write_nsis_script()
{
    local script="$1"
    local payload_zip="$2"
    local payload_win
    local installer_win

    payload_win="$(windows_path "${payload_zip}")"
    installer_win="$(windows_path "${INSTALLER_PATH}")"

    cat > "${script}" <<EOF
!define PRODUCT_NAME "FreeBASIC Wii"
!define PRODUCT_VERSION "${FBVERSION}"

Name "\${PRODUCT_NAME} \${PRODUCT_VERSION}"
OutFile "${installer_win}"
InstallDir "\$PROGRAMFILES64\\FreeBASIC-Wii"
RequestExecutionLevel admin
SetCompressor zlib

Page directory
Page instfiles

Section "FreeBASIC Wii" SEC01
    InitPluginsDir
    SetOutPath "\$PLUGINSDIR"
    SetCompress off
    File /oname=freebasic-wii-payload.zip "${payload_win}"
    SetCompress auto
    IfFileExists "\$SYSDIR\\WindowsPowerShell\\v1.0\\powershell.exe" 0 no_powershell
    SetOutPath "\$INSTDIR"
    ;
    ; Keep the package tree as a normal zip payload so makensis does not need
    ; to mmap every bundled FreeBASIC and devkitPro file individually.
    FileOpen \$0 "\$PLUGINSDIR\\extract-payload.ps1" w
    FileWrite \$0 "param([string] \$\$PayloadZip, [string] \$\$Destination)$\r$\n"
    FileWrite \$0 "\$\$ErrorActionPreference = 'Stop'$\r$\n"
    FileWrite \$0 "Expand-Archive -LiteralPath \$\$PayloadZip -DestinationPath \$\$Destination -Force -ErrorAction Stop$\r$\n"
    FileClose \$0
    nsExec::ExecToLog '"\$SYSDIR\\WindowsPowerShell\\v1.0\\powershell.exe" -NoLogo -NoProfile -ExecutionPolicy Bypass -File "\$PLUGINSDIR\\extract-payload.ps1" "\$PLUGINSDIR\\freebasic-wii-payload.zip" "\$INSTDIR"'
    Pop \$0
    StrCmp \$0 "0" payload_done
        Abort "Failed to extract the FreeBASIC Wii payload. PowerShell exit code: \$0"
    payload_done:
    CreateDirectory "\$SMPROGRAMS\\FreeBASIC Wii"
    CreateShortCut "\$SMPROGRAMS\\FreeBASIC Wii\\FreeBASIC Wii Shell.lnk" "\$INSTDIR\\freebasic-wii-shell.cmd"
    CreateShortCut "\$SMPROGRAMS\\FreeBASIC Wii\\Uninstall.lnk" "\$INSTDIR\\uninstall.exe"
    WriteUninstaller "\$INSTDIR\\uninstall.exe"
    Goto install_done
    no_powershell:
        Abort "Windows PowerShell is required to extract this installer."
    install_done:
SectionEnd

Section "Uninstall"
    Delete "\$SMPROGRAMS\\FreeBASIC Wii\\FreeBASIC Wii Shell.lnk"
    Delete "\$SMPROGRAMS\\FreeBASIC Wii\\Uninstall.lnk"
    RMDir "\$SMPROGRAMS\\FreeBASIC Wii"
    RMDir /r "\$INSTDIR"
SectionEnd
EOF
}

build_installer()
{
    local makensis="${NSIS_EXE}"
    local nsis_script="${BUILDROOT}/freebasic-wii.nsi"
    local installer_payload_zip="${BUILDROOT}/freebasic-wii-installer-payload.zip"

    if [ "${DO_INSTALLER}" -eq 0 ]; then
        return 0
    fi

    have zip || fail "zip was not found; install zip or pass --skip-installer"

    if [ -n "${makensis}" ]; then
        makensis="$(normalize_path "${makensis}")"
    fi

    if [ -z "${makensis}" ]; then
        makensis="$(first_command makensis makensis.exe || true)"
    fi

    if [ -z "${makensis}" ] || [ ! -x "${makensis}" ]; then
        fail "makensis not found at ${makensis:-<PATH>}; install mingw-w64-x86_64-nsis or set NSIS_EXE"
    fi

    msg "Building NSIS installer"

    mkdir -p "$(dirname "${INSTALLER_PATH}")"
    msg "Creating Wii NSIS payload zip"
    rm -f "${installer_payload_zip}"
    (
        cd "${PACKAGE_ROOT}"
        run zip -qr "${installer_payload_zip}" .
    )

    write_nsis_script "${nsis_script}" "${installer_payload_zip}"
    rm -f "${INSTALLER_PATH}"
    if ! run "${makensis}" "${nsis_script}"; then
        rm -f "${installer_payload_zip}"
        fail "makensis failed while creating FreeBASIC Wii installer"
    fi
    rm -f "${installer_payload_zip}"
}

validate_installer()
{
    if [ "${DO_VALIDATE}" -eq 0 ] || [ "${DO_INSTALLER}" -eq 0 ] || [ "${DO_PACKAGE}" -eq 0 ]; then
        return 0
    fi

    if [ ! -f "${INSTALLER_PATH}" ]; then
        fail "missing installer for smoke test: ${INSTALLER_PATH}"
    fi

    run bash "${ROOT_DIR}/build_scripts/msys2-test-freebasic-installer.sh" \
        --installer "${INSTALLER_PATH}" \
        --kind wii
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

msg "FreeBASIC Wii Windows package"
printf 'source root : %s\n' "${ROOT_DIR}"
printf 'work tree   : %s\n' "${WORKTREE}"
printf 'package     : %s\n' "${PACKAGE_ROOT}"
printf 'archive     : %s\n' "${ARCHIVE_PATH}"
printf 'installer   : %s\n' "${INSTALLER_PATH}"
printf 'devkitPro   : %s\n' "${DEVKITPRO}"
printf 'devkitPPC   : %s\n' "${DEVKITPPC}"
printf 'jobs        : %s\n' "${JOBS}"

install_dependencies
ensure_host_build_tools
ensure_devkitpro
build_wii_freebasic
assemble_package
validate_package
build_archive
build_installer
validate_installer

msg "Wii package complete"
printf 'package tree: %s\n' "${PACKAGE_ROOT}"
if [ "${DO_PACKAGE}" -ne 0 ]; then
    printf 'archive     : %s\n' "${ARCHIVE_PATH}"
fi
if [ "${DO_INSTALLER}" -ne 0 ]; then
    printf 'installer   : %s\n' "${INSTALLER_PATH}"
fi

# end of msys2-build-freebasic-wii.sh
