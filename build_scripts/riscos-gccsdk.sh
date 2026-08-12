#!/usr/bin/env bash
#
# Project: FreeBASIC RISC OS toolchain workflow
# ---------------------------------------------
#
# File: riscos-gccsdk.sh
#
# Purpose:
#
#     Build the GCCSDK cross compiler and UnixLib environment used by the
#     FreeBASIC RISC OS target.
#
# Responsibilities:
#
#     - check the host tools required by GCCSDK
#     - create or update a GCCSDK Subversion checkout
#     - provide GCCSDK's install paths through gccsdk-params
#     - select host language modes accepted by GCCSDK's legacy dependencies
#     - select the static C-only UnixLib configuration used by FreeBASIC
#     - omit GCCSDK's dynamic-loader tools from that static configuration
#     - omit optional SharedCLibrary-only debugging modules in static mode
#     - optionally build static native GCC and binutils programs for RISC OS
#     - preserve static linkage through legacy binutils libtool program links
#     - keep host compiler overrides out of target module builds
#     - serialize legacy install rules which race under parallel make
#     - skip legacy GCC manuals rejected by current Texinfo releases
#     - disable the optional PPL/CLooG optimizer whose upstream archive is gone
#     - build GCC's bundled ARM libffi for FreeBASIC THREADCALL support
#     - run build-world and write a reusable environment file
#
# This file intentionally does NOT contain:
#
#     - host package installation
#     - general-purpose feature or optimizer patches to GCCSDK, GCC, or UnixLib
#     - FreeBASIC runtime compilation
#     - native FreeBASIC compiler construction or HostFS packaging
#     - system-wide compiler installation
#
# Work directory ownership:
#
#     The selected work directory is managed by this script.  In particular,
#     gcc4/gccsdk-params is regenerated atomically so build-world cannot use
#     stale install paths from an earlier invocation.
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

WORK_DIR="$ROOT/out/riscos/gccsdk"
GCCSDK_REVISION=""
UPDATE=0
BUILD_NATIVE=0

GCCSDK_URL="svn://svn.riscos.info/gccsdk/trunk/gcc4"

##############################################################################
# Helpers
##############################################################################

die() {
    echo "ERROR: $*" >&2
    exit 1
}

detect_jobs() {
    local jobs=1

    if command -v nproc >/dev/null 2>&1; then
        jobs="$(nproc)"
    elif getconf _NPROCESSORS_ONLN >/dev/null 2>&1; then
        jobs="$(getconf _NPROCESSORS_ONLN)"
    fi

    case "$jobs" in
        ''|*[!0-9]*|0) jobs=1 ;;
    esac

    printf '%s\n' "$jobs"
}

require_value() {
    local option="$1"
    local value="${2-}"

    [ -n "$value" ] || die "$option requires a value"
}

quote_for_shell() {
    local value="$1"

    value=${value//\'/\'\\\'\'}
    printf "'%s'" "$value"
}

usage() {
    cat <<EOF
Usage: ./build_scripts/riscos-gccsdk.sh [options]

Options:
  --workdir DIR       Managed GCCSDK checkout/install root.
                      Default: out/riscos/gccsdk
  --revision REV      Pin the GCCSDK Subversion checkout to REV.
  --update            Update an existing unpinned checkout before building.
  --with-native       Also build native RISC OS GCC and binutils programs.
  --jobs N            Parallel build jobs. Default: detected CPU count
  -h, --help          Show this help.

The result is installed below:
  <workdir>/cross/bin  Cross compiler and host-side tools
  <workdir>/cross/arm-unknown-riscos
                      Target headers and libraries
  <workdir>/env        GCCSDK Autobuilder wrapper environment
  <workdir>/env.sh     Environment settings for later shells
  <workdir>/gcc4/release-area/full/!GCC
                      Native compiler tree when --with-native is used

This script does not install host packages. On Debian/Ubuntu, GCCSDK normally
requires build-essential, subversion, m4, bison, flex, autogen, gperf, texinfo,
wget, bzip2, unzip, xsltproc, cmake, and their normal development tools.
EOF
}

##############################################################################
# Command-line parsing
##############################################################################

JOBS="$(detect_jobs)"

while [ "$#" -gt 0 ]; do
    case "$1" in
        --workdir)
            require_value "$1" "${2-}"
            WORK_DIR="$2"
            shift 2
            ;;
        --revision)
            require_value "$1" "${2-}"
            GCCSDK_REVISION="$2"
            shift 2
            ;;
        --update)
            UPDATE=1
            shift
            ;;
        --with-native)
            BUILD_NATIVE=1
            shift
            ;;
        --jobs)
            require_value "$1" "${2-}"
            JOBS="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            die "unknown option: $1"
            ;;
    esac
done

case "$JOBS" in
    ''|*[!0-9]*|0) die "--jobs must be a positive integer" ;;
esac

##############################################################################
# Host and checkout validation
##############################################################################

for tool in svn make gcc g++ m4 bison flex autogen gperf makeinfo wget \
    bzip2 unzip xsltproc cmake sed; do
    command -v "$tool" >/dev/null 2>&1 ||
        die "required host tool not found: $tool"
done

mkdir -p "$WORK_DIR"
WORK_DIR="$(cd "$WORK_DIR" && pwd)"
[ "$WORK_DIR" != "/" ] || die "refusing to use the filesystem root as --workdir"

GCC4_DIR="$WORK_DIR/gcc4"
GCCSDK_INSTALL_CROSSBIN="$WORK_DIR/cross/bin"
GCCSDK_INSTALL_ENV="$WORK_DIR/env"
GCCSDK_TARGET_ENV="$WORK_DIR/cross/arm-unknown-riscos"
export GCCSDK_INSTALL_CROSSBIN GCCSDK_INSTALL_ENV GCCSDK_TARGET_ENV
export PATH="$GCCSDK_INSTALL_CROSSBIN:$PATH"

if [ ! -d "$GCC4_DIR/.svn" ]; then
    [ ! -e "$GCC4_DIR" ] ||
        die "$GCC4_DIR exists but is not a Subversion checkout"

    checkout_args=(checkout)
    if [ -n "$GCCSDK_REVISION" ]; then
        checkout_args+=(-r "$GCCSDK_REVISION")
    fi
    checkout_args+=("$GCCSDK_URL" "$GCC4_DIR")
    svn "${checkout_args[@]}"
elif [ -n "$GCCSDK_REVISION" ]; then
    # A requested revision is a reproducibility constraint, not merely an
    # update hint, so apply it even when --update was not supplied.
    svn update -r "$GCCSDK_REVISION" "$GCC4_DIR"
elif [ "$UPDATE" -eq 1 ]; then
    svn update "$GCC4_DIR"
fi

##############################################################################
# Host compatibility preparation
##############################################################################

GCCSDK_CONFIGURATION_CHANGED=0
GCCSDK_NATIVE_CONFIGURATION_CHANGED=0

# FreeBASIC emits C and currently links static UnixLib programs. Avoid building
# GCCSDK's C++, LTO, shared-library, and historical ABI multilib variants; none
# is consumed by this port, and each substantially increases clean build time.
if grep -qx 'GCC_LANGUAGES="c,c++"' "$GCC4_DIR/Makefile"; then
    sed -i 's/^GCC_LANGUAGES="c,c++"$/GCC_LANGUAGES="c"/' \
        "$GCC4_DIR/Makefile"
    GCCSDK_CONFIGURATION_CHANGED=1
fi

if grep -qx 'CROSS_ENABLE_SHARED=yes' "$GCC4_DIR/Makefile"; then
    sed -i 's/^CROSS_ENABLE_SHARED=yes$/CROSS_ENABLE_SHARED=no/' \
        "$GCC4_DIR/Makefile"
    GCCSDK_CONFIGURATION_CHANGED=1
fi

if grep -qx 'GCC_USE_LTO=yes' "$GCC4_DIR/Makefile"; then
    sed -i 's/^GCC_USE_LTO=yes$/GCC_USE_LTO=no/' "$GCC4_DIR/Makefile"
    GCCSDK_CONFIGURATION_CHANGED=1
fi

if ! grep -qx 'GCC_CONFIG_ARGS += --disable-multilib' \
    "$GCC4_DIR/Makefile"; then
    sed -i \
        '/^GCC_CONFIG_ARGS += --enable-checking=release$/a GCC_CONFIG_ARGS += --disable-multilib' \
        "$GCC4_DIR/Makefile"
    GCCSDK_CONFIGURATION_CHANGED=1
fi

grep -qx 'GCC_LANGUAGES="c"' "$GCC4_DIR/Makefile" ||
    die "failed to select GCC's C-only build"
grep -qx 'CROSS_ENABLE_SHARED=no' "$GCC4_DIR/Makefile" ||
    die "failed to select GCC's static-library build"
grep -qx 'GCC_USE_LTO=no' "$GCC4_DIR/Makefile" ||
    die "failed to disable GCC's unused LTO support"
grep -qx 'GCC_CONFIG_ARGS += --disable-multilib' "$GCC4_DIR/Makefile" ||
    die "failed to disable GCC's unused ABI multilibs"

if [ "$BUILD_NATIVE" -eq 1 ]; then
    # Native FreeBASIC needs GCC, the GNU assembler, and the linker, but not
    # GCCSDK's optional SharedCLibrary helper applications.  Keep the native
    # toolchain self-contained and do not make those helpers a completion
    # dependency for this static configuration.
    if grep -qx 'RONATIVE_ENABLE_SHARED=yes' "$GCC4_DIR/Makefile"; then
        sed -i 's/^RONATIVE_ENABLE_SHARED=yes$/RONATIVE_ENABLE_SHARED=no/' \
            "$GCC4_DIR/Makefile"
        GCCSDK_NATIVE_CONFIGURATION_CHANGED=1
    fi

    if grep -Fqx \
        'ronative-done: ronative-gcc-built ronative-binutils-built ronative-riscostools-built' \
        "$GCC4_DIR/Makefile"; then
        sed -i \
            's/^ronative-done: ronative-gcc-built ronative-binutils-built ronative-riscostools-built$/ronative-done: ronative-gcc-built ronative-binutils-built/' \
            "$GCC4_DIR/Makefile"
        GCCSDK_NATIVE_CONFIGURATION_CHANGED=1
    fi

    grep -qx 'RONATIVE_ENABLE_SHARED=no' "$GCC4_DIR/Makefile" ||
        die "failed to select GCCSDK's static native compiler build"
    grep -Fqx \
        'ronative-done: ronative-gcc-built ronative-binutils-built' \
        "$GCC4_DIR/Makefile" ||
        die "failed to omit unused native RISC OS helper applications"
fi

# build-it mixes Ubuntu utilities with RISC OS modules. Let each configure
# script choose its own compiler instead of inheriting the GCC bootstrap's
# host-only CC/CXX values, and make its RISC OS link checks match this static
# toolchain. Dropping -f also lets an interrupted utility build resume.
if grep -Fqx $'\tcd $(RISCOSTOOLSDIR) && ./build-it -f cross' \
    "$GCC4_DIR/Makefile"; then
    sed -i \
        's|./build-it -f cross|env -u CC -u CXX LDFLAGS=-static ./build-it -static cross|' \
        "$GCC4_DIR/Makefile"
fi

if grep -Fqx $'\tcd $(RISCOSTOOLSDIR) && ./build-it -f -static cross' \
    "$GCC4_DIR/Makefile"; then
    sed -i \
        's|./build-it -f -static cross|env -u CC -u CXX LDFLAGS=-static ./build-it -static cross|' \
        "$GCC4_DIR/Makefile"
fi

if grep -Fqx \
    $'\tcd $(RISCOSTOOLSDIR) && env -u CC -u CXX ./build-it -static cross' \
    "$GCC4_DIR/Makefile"; then
    sed -i \
        's|env -u CC -u CXX ./build-it -static cross|env -u CC -u CXX LDFLAGS=-static ./build-it -static cross|' \
        "$GCC4_DIR/Makefile"
fi

grep -Fqx \
    $'\tcd $(RISCOSTOOLSDIR) && env -u CC -u CXX LDFLAGS=-static ./build-it -static cross' \
    "$GCC4_DIR/Makefile" ||
    die "failed to select GCCSDK's static RISC OS tools"

# GDBServer and SysLogD are optional SharedCLibrary modules. Their -mlibscl
# links require multilib startup files, so GCCSDK's static mode must skip them
# along with the shared-object loader it already omits.
if ! grep -Fq '# FreeBASIC static toolchain: omit SCL-only modules.' \
    "$GCC4_DIR/riscos/build-it"; then
    sed -i \
        '/^  # GDBServer module:$/i\  # FreeBASIC static toolchain: omit SCL-only modules.\n  if ! $GCCSDK_BUILD_ISSTATIC ; then' \
        "$GCC4_DIR/riscos/build-it"
fi

# The action-selection block contains the same elif text earlier in the file.
# Remove stale closers, then constrain insertion to the second occurrence.
sed -i '/^  fi # End of SCL-only modules\.$/d' \
    "$GCC4_DIR/riscos/build-it"
sed -i \
    '0,/^elif \[ "$GCCSDK_BUILD_ACTION" == "riscos" \] ; then$/! { /^elif \[ "$GCCSDK_BUILD_ACTION" == "riscos" \] ; then$/i\  fi # End of SCL-only modules.
}' \
    "$GCC4_DIR/riscos/build-it"

grep -Fq '# FreeBASIC static toolchain: omit SCL-only modules.' \
    "$GCC4_DIR/riscos/build-it" ||
    die "failed to omit GCCSDK's SCL-only modules"
grep -Fq '  fi # End of SCL-only modules.' \
    "$GCC4_DIR/riscos/build-it" ||
    die "failed to close GCCSDK's SCL-only module guard"
bash -n "$GCC4_DIR/riscos/build-it" ||
    die "GCCSDK's patched RISC OS tool build is not valid shell"

# Several old install targets create the same destination directory from
# parallel submakes. Modern make reaches those rules concurrently and one
# mkdir then fails because the other already created the directory. Keep the
# expensive compile steps parallel while making each install phase serial.
if grep -Fq '$(MAKE) install' "$GCC4_DIR/Makefile"; then
    sed -i 's/$(MAKE) install/$(MAKE) -j1 install/g' "$GCC4_DIR/Makefile"
fi

if grep -Fq '$(MAKE) install' "$GCC4_DIR/Makefile"; then
    die "failed to serialize GCCSDK install rules"
fi

# GCC 4.7's manuals contain Texinfo constructs rejected by current makeinfo.
# They are not part of the compiler, target headers, libraries, or RISC OS
# tools. Override MAKEINFO only for GCC's recursive build and all install
# phases so a documentation incompatibility cannot prevent the SDK build.
if grep -Fq '$(MAKE) $(GCC_BUILD_FLAGS)' "$GCC4_DIR/Makefile"; then
    sed -i \
        's/$(MAKE) $(GCC_BUILD_FLAGS)/$(MAKE) MAKEINFO=true $(GCC_BUILD_FLAGS)/g' \
        "$GCC4_DIR/Makefile"
fi

if grep -Fq '$(MAKE) -j1 install' "$GCC4_DIR/Makefile"; then
    sed -i 's/$(MAKE) -j1 install/$(MAKE) -j1 MAKEINFO=true install/g' \
        "$GCC4_DIR/Makefile"
fi

if grep -Fq '$(MAKE) $(GCC_BUILD_FLAGS)' "$GCC4_DIR/Makefile" ||
    grep -Fq '$(MAKE) -j1 install' "$GCC4_DIR/Makefile"; then
    die "failed to disable incompatible GCC documentation rules"
fi

# PPL and CLooG only provide an optional loop optimizer. GCCSDK's pinned PPL
# download now returns HTTP 403, while GCC itself remains complete without it.
if grep -qx 'GCC_USE_PPL_CLOOG=yes' "$GCC4_DIR/Makefile"; then
    sed -i 's/^GCC_USE_PPL_CLOOG=yes$/GCC_USE_PPL_CLOOG=no/' \
        "$GCC4_DIR/Makefile"
fi

grep -qx 'GCC_USE_PPL_CLOOG=no' "$GCC4_DIR/Makefile" ||
    die "failed to disable GCCSDK's unavailable PPL/CLooG dependency"

if [ "$GCCSDK_CONFIGURATION_CHANGED" -eq 1 ]; then
    # The configure recipe recreates its build directory, but only when this
    # stamp is absent. Invalidate dependent stamps whenever our configuration
    # changes so an interrupted older build cannot silently retain its ABI set.
    rm -f \
        "$GCC4_DIR/buildstepsdir/cross-gcc-configured" \
        "$GCC4_DIR/buildstepsdir/cross-gcc-built" \
        "$GCC4_DIR/buildstepsdir/cross-riscostools-built"

    if [ "$BUILD_NATIVE" -eq 1 ]; then
        GCCSDK_NATIVE_CONFIGURATION_CHANGED=1
    fi
fi

##############################################################################
# GCCSDK configuration and build
##############################################################################

CROSSBIN_QUOTED="$(quote_for_shell "$GCCSDK_INSTALL_CROSSBIN")"
ENV_QUOTED="$(quote_for_shell "$GCCSDK_INSTALL_ENV")"
TARGET_ENV_QUOTED="$(quote_for_shell "$GCCSDK_TARGET_ENV")"
PARAMS_TMP="$(mktemp "$WORK_DIR/gccsdk-params.XXXXXX")"

printf '%s\n' \
    '# Generated by FreeBASIC build_scripts/riscos-gccsdk.sh' \
    "export GCCSDK_INSTALL_CROSSBIN=$CROSSBIN_QUOTED" \
    "export GCCSDK_INSTALL_ENV=$ENV_QUOTED" \
    '# end of gccsdk-params' \
    > "$PARAMS_TMP"
mv "$PARAMS_TMP" "$GCC4_DIR/gccsdk-params"

export MAKEFLAGS="${MAKEFLAGS:+$MAKEFLAGS }-j$JOBS"

# GCCSDK 4.7 builds GMP 5.0.1 and GCC 4.7.4. Their configure checks contain
# constructs which modern GCC no longer accepts in its default language modes.
# Keep the historical C and C++ modes explicit, and downgrade only the three
# diagnostics that GCC 14 and later promoted to errors.
HOST_CC="${CC:-gcc}"
HOST_CXX="${CXX:-g++}"
HOST_C_FLAGS="-std=gnu89"
HOST_C_FLAGS+=" -Wno-error=implicit-function-declaration"
HOST_C_FLAGS+=" -Wno-error=implicit-int"
HOST_C_FLAGS+=" -Wno-error=incompatible-pointer-types"
export CC="$HOST_CC $HOST_C_FLAGS"
export CXX="$HOST_CXX -std=gnu++98"

(
    cd "$GCC4_DIR"
    ./build-world
)

[ -x "$GCCSDK_INSTALL_CROSSBIN/arm-unknown-riscos-gcc" ] ||
    die "build-world finished without arm-unknown-riscos-gcc"
[ -x "$GCCSDK_INSTALL_CROSSBIN/elf2aif" ] ||
    die "build-world finished without elf2aif"
SUL_SOURCE="$GCCSDK_INSTALL_CROSSBIN/sul"
if [ ! -s "$SUL_SOURCE" ]; then
    SUL_SOURCE="$GCCSDK_INSTALL_CROSSBIN/arm-unknown-riscos-sul"
fi

[ -s "$SUL_SOURCE" ] ||
    die "build-world finished without the SharedUnixLibrary module"

# UnixLib installs this target-independent module with a target prefix. Keep a
# stable unprefixed name for the FreeBASIC staging workflow and documentation.
if [ "$SUL_SOURCE" != "$GCCSDK_INSTALL_CROSSBIN/sul" ]; then
    cp "$SUL_SOURCE" "$GCCSDK_INSTALL_CROSSBIN/sul"
fi
[ -d "$GCCSDK_TARGET_ENV/include" ] ||
    die "build-world finished without the UnixLib target headers"

##############################################################################
# Target libffi
##############################################################################

LIBFFI_SOURCE="$GCC4_DIR/srcdir.orig/gcc-trunk/libffi"
LIBFFI_BUILD="$WORK_DIR/libffi-build"
LIBFFI_LIBRARY="$LIBFFI_BUILD/.libs/libffi.a"

[ -x "$LIBFFI_SOURCE/configure" ] ||
    die "GCC's bundled libffi source was not found: $LIBFFI_SOURCE"

# GCCSDK does not select target-libffi in its C-only top-level GCC build, but
# FreeBASIC THREADCALL needs the matching ARM call marshaller. Build the copy
# bundled with GCC 4.7 separately, using the same ARMv4 soft-float ABI as the
# rest of the port. The static link flag is also required for configure's link
# probes because this GCCSDK configuration deliberately omits shared support.
if [ ! -f "$LIBFFI_BUILD/Makefile" ]; then
    [ "$LIBFFI_BUILD" != "/" ] ||
        die "refusing to replace the filesystem root as the libffi build tree"
    rm -rf -- "$LIBFFI_BUILD"
    mkdir -p "$LIBFFI_BUILD"

    (
        cd "$LIBFFI_BUILD"
        env -u CC -u CXX \
            CFLAGS='-O2 -march=armv4 -mfloat-abi=soft' \
            LDFLAGS='-static' \
            "$LIBFFI_SOURCE/configure" \
                --build="$(gcc -dumpmachine)" \
                --host=arm-unknown-riscos \
                --prefix="$GCCSDK_TARGET_ENV" \
                --disable-shared \
                --enable-static
    )
fi

make -C "$LIBFFI_BUILD" -j"$JOBS"
[ -s "$LIBFFI_LIBRARY" ] ||
    die "target libffi build did not produce $LIBFFI_LIBRARY"
[ -s "$LIBFFI_BUILD/include/ffi.h" ] ||
    die "target libffi build did not produce ffi.h"
[ -e "$LIBFFI_BUILD/include/ffitarget.h" ] ||
    die "target libffi build did not produce ffitarget.h"

mkdir -p "$GCCSDK_TARGET_ENV/include" "$GCCSDK_TARGET_ENV/lib"
cp "$LIBFFI_LIBRARY" "$GCCSDK_TARGET_ENV/lib/libffi.a"
cp "$LIBFFI_BUILD/include/ffi.h" "$GCCSDK_TARGET_ENV/include/ffi.h"
cp -L "$LIBFFI_BUILD/include/ffitarget.h" \
    "$GCCSDK_TARGET_ENV/include/ffitarget.h"

##############################################################################
# Optional native RISC OS toolchain
##############################################################################

if [ "$BUILD_NATIVE" -eq 1 ]; then
    # Binutils 2.24 uses libtool for its native programs.  A plain -static is
    # consumed as a libtool library-selection option and never reaches GCC.
    # The resulting all-static-library but dynamic-driver link crashes the old
    # RISC OS BFD backend.  -all-static is libtool's program-link spelling and
    # preserves the intended static GCC invocation.
    for makefile in \
        "$GCC4_DIR/srcdir/binutils/gas/Makefile.in" \
        "$GCC4_DIR/srcdir/binutils/binutils/Makefile.in" \
        "$GCC4_DIR/srcdir/binutils/gprof/Makefile.in" \
        "$GCC4_DIR/srcdir/binutils/ld/Makefile.in"; do
        [ -f "$makefile" ] ||
            die "native binutils makefile not found: $makefile"

        if grep -q '^AM_LDFLAGS = ' "$makefile"; then
            grep -qx 'AM_LDFLAGS = -all-static' "$makefile" ||
                die "unexpected native binutils AM_LDFLAGS in $makefile"
        else
            if grep -q '^noinst_PROGRAMS = ' "$makefile"; then
                sed -i '/^noinst_PROGRAMS = /a AM_LDFLAGS = -all-static' \
                    "$makefile"
            elif grep -q '^bin_PROGRAMS = ' "$makefile"; then
                sed -i '/^bin_PROGRAMS = /a AM_LDFLAGS = -all-static' \
                    "$makefile"
            else
                die "program list not found in native binutils makefile: $makefile"
            fi
            GCCSDK_NATIVE_CONFIGURATION_CHANGED=1
        fi
    done

    if [ "$GCCSDK_NATIVE_CONFIGURATION_CHANGED" -eq 1 ]; then
        # Native configure results retain compiler and link flags.  Invalidate
        # the complete native lane when its static policy changes so cached
        # configure answers cannot produce a mixed toolchain.
        rm -f "$GCC4_DIR"/buildstepsdir/ronative-*
        rm -rf \
            "$GCC4_DIR"/builddir/ronative-* \
            "$GCC4_DIR/release-area/full"
    fi

    # Target programs use flags understood by GCCSDK GCC 4.7.  Generator
    # programs run on Ubuntu and need the separate modern-host compatibility
    # modes already used by build-world.
    (
        cd "$GCC4_DIR"
        env -u CC -u CXX \
            CFLAGS='-O2 -static' \
            CXXFLAGS='-O2 -static' \
            LDFLAGS='-static' \
            CFLAGS_FOR_BUILD="-O2 $HOST_C_FLAGS" \
            CXXFLAGS_FOR_BUILD='-O2 -std=gnu++98' \
            make ronative
    )

    GCCSDK_NATIVE_ROOT="$GCC4_DIR/release-area/full"
    [ -s "$GCCSDK_NATIVE_ROOT/!GCC/bin/gcc" ] ||
        die "native GCC build finished without gcc"
    [ -s "$GCCSDK_NATIVE_ROOT/!GCC/bin/as" ] ||
        die "native GCC build finished without as"
    [ -s "$GCCSDK_NATIVE_ROOT/!GCC/bin/ld" ] ||
        die "native GCC build finished without ld"

    # Native fbc invokes this GCC tree, so mirror the separately built target
    # library and headers into the target sysroot shipped by !GCC.
    NATIVE_TARGET_ENV="$GCCSDK_NATIVE_ROOT/!GCC/arm-unknown-riscos"
    mkdir -p "$NATIVE_TARGET_ENV/include" "$NATIVE_TARGET_ENV/lib"
    cp "$GCCSDK_TARGET_ENV/lib/libffi.a" "$NATIVE_TARGET_ENV/lib/libffi.a"
    cp "$GCCSDK_TARGET_ENV/include/ffi.h" "$NATIVE_TARGET_ENV/include/ffi.h"
    cp "$GCCSDK_TARGET_ENV/include/ffitarget.h" \
        "$NATIVE_TARGET_ENV/include/ffitarget.h"
fi

##############################################################################
# Reusable shell environment
##############################################################################

ENV_TMP="$(mktemp "$WORK_DIR/env.sh.XXXXXX")"
env_lines=( \
    '# Generated by FreeBASIC build_scripts/riscos-gccsdk.sh' \
    "export GCCSDK_INSTALL_CROSSBIN=$CROSSBIN_QUOTED" \
    "export GCCSDK_INSTALL_ENV=$ENV_QUOTED" \
    "export GCCSDK_TARGET_ENV=$TARGET_ENV_QUOTED" \
    'export PATH="$GCCSDK_INSTALL_CROSSBIN:$PATH"' )

if [ "$BUILD_NATIVE" -eq 1 ]; then
    NATIVE_ROOT_QUOTED="$(quote_for_shell "$GCCSDK_NATIVE_ROOT")"
    env_lines+=("export GCCSDK_NATIVE_ROOT=$NATIVE_ROOT_QUOTED")
fi

env_lines+=('# end of env.sh')
printf '%s\n' "${env_lines[@]}" > "$ENV_TMP"
mv "$ENV_TMP" "$WORK_DIR/env.sh"

echo "==> GCCSDK ready"
echo "    source '$WORK_DIR/env.sh'"
if [ "$BUILD_NATIVE" -eq 1 ]; then
    echo "    native tools: '$GCCSDK_NATIVE_ROOT/!GCC'"
fi

# end of riscos-gccsdk.sh
