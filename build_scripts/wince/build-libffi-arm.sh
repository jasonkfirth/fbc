#!/usr/bin/env bash
# FreeBASIC Windows CE libffi build helper
# ----------------------------------------
#
# File: build_scripts/wince/build-libffi-arm.sh
#
# Purpose:
#
#     Build and stage a reproducible ARMv4T Windows CE libffi archive for the
#     FreeBASIC THREADCALL runtime.
#
# Responsibilities:
#
#     - build the pinned Windows CE container toolchain
#     - download and authenticate the libffi 3.5.2 source release
#     - apply the reviewed ARMv4T Windows CE compatibility patch
#     - validate every archive member and the required public symbols
#     - stage the archive, headers, and license in the target runtime tree
#
# This file intentionally does NOT contain:
#
#     - FreeBASIC runtime or compiler construction
#     - emulator startup policy
#     - MIPS Windows CE libffi policy
#     - package construction

set -euo pipefail

# ---------------------------------------------------------------------------
# Build identity
# ---------------------------------------------------------------------------

readonly LIBFFI_VERSION="3.5.2"
readonly LIBFFI_SHA256="f3a3082a23b37c293a4fcd1053147b371f2ff91fa7ea1b2a52e335676bac82dc"
readonly LIBFFI_URL="https://github.com/libffi/libffi/releases/download/v${LIBFFI_VERSION}/libffi-${LIBFFI_VERSION}.tar.gz"
readonly WINCE_IMAGE="${WINCE_IMAGE:-freebasic-wince-toolchain:noble}"

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly SCRIPT_DIR
REPO_ROOT="$(CDPATH= cd -- "${SCRIPT_DIR}/../.." && pwd)"
readonly REPO_ROOT
readonly WINCE_OUT="${REPO_ROOT}/out/wince"
readonly BUILD_LOG="${WINCE_OUT}/logs/libffi-arm-${LIBFFI_VERSION}.log"

# ---------------------------------------------------------------------------
# Host checks
# ---------------------------------------------------------------------------

if ! command -v docker >/dev/null 2>&1; then
	printf 'ERROR: Docker is required to build the Windows CE toolchain.\n' >&2
	exit 1
fi

if ! docker info >/dev/null 2>&1; then
	printf 'ERROR: Docker is installed, but the daemon is not available.\n' >&2
	exit 1
fi

if ! test -f "${SCRIPT_DIR}/patches/libffi-${LIBFFI_VERSION}-wince-arm.patch"; then
	printf 'ERROR: the Windows CE libffi patch is missing.\n' >&2
	exit 1
fi

mkdir -p "${WINCE_OUT}/deps" "${WINCE_OUT}/logs"

# Docker's layer cache makes this check inexpensive and ensures that changes
# to the pinned toolchain recipe cannot be hidden by an older local tag.
docker build --tag "${WINCE_IMAGE}" "${SCRIPT_DIR}"

# ---------------------------------------------------------------------------
# Target build and validation
# ---------------------------------------------------------------------------

docker run --rm \
	--interactive \
	--user "$(id -u):$(id -g)" \
	--volume "${REPO_ROOT}:/work" \
	--workdir /work \
	--env "LIBFFI_VERSION=${LIBFFI_VERSION}" \
	--env "LIBFFI_SHA256=${LIBFFI_SHA256}" \
	--env "LIBFFI_URL=${LIBFFI_URL}" \
	"${WINCE_IMAGE}" \
	bash -s <<'WINCE_LIBFFI_BUILD' 2>&1 | tee "${BUILD_LOG}"
set -euo pipefail

readonly archive="/work/out/wince/deps/libffi-${LIBFFI_VERSION}.tar.gz"
readonly partial_archive="${archive}.partial"
readonly patch_file="/work/build_scripts/wince/patches/libffi-${LIBFFI_VERSION}-wince-arm.patch"
readonly target_dir="/work/lib/freebasic/wince-arm"
build_root="$(mktemp -d "/work/out/wince/libffi-arm-build.XXXXXX")"
readonly build_root

cleanup()
{
	rm -rf -- "${build_root}"
}

trap cleanup EXIT

verify_download()
{
	printf '%s  %s\n' "${LIBFFI_SHA256}" "$1" | sha256sum --check --status
}

if ! test -f "${archive}" || ! verify_download "${archive}"; then
	rm -f -- "${partial_archive}"
	curl --fail --location --silent --show-error \
		--output "${partial_archive}" "${LIBFFI_URL}"
	verify_download "${partial_archive}"
	mv -f -- "${partial_archive}" "${archive}"
fi

readonly source_dir="${build_root}/source"
readonly object_dir="${build_root}/object"
readonly stage_dir="${build_root}/stage"
mkdir -p "${source_dir}" "${object_dir}" "${stage_dir}"

tar -xzf "${archive}" --strip-components=1 -C "${source_dir}"
patch --directory "${source_dir}" --strip=1 < "${patch_file}"

cd "${object_dir}"

readonly arm_flags="-O0 -g0 -march=armv4t -mfloat-abi=soft -marm -D_WIN32_WCE=0x0500"
CFLAGS="${arm_flags}" \
CCASFLAGS="${arm_flags}" \
"${source_dir}/configure" \
	--host=arm-mingw32ce \
	--prefix="${stage_dir}" \
	--disable-shared \
	--enable-static

make -j"$(getconf _NPROCESSORS_ONLN)"
make install

readonly built_archive="${stage_dir}/lib/libffi.a"
readonly validation_file="${build_root}/archive-validation.txt"

test -s "${built_archive}"
arm-mingw32ce-objdump -f "${built_archive}" > "${validation_file}"

if grep 'file format' "${validation_file}" | grep -Fv 'pe-arm-wince-little' >/dev/null; then
	printf 'ERROR: libffi contains a non-Windows CE object.\n' >&2
	exit 1
fi

if grep 'architecture:' "${validation_file}" | grep -Fv 'architecture: armv4t,' >/dev/null; then
	printf 'ERROR: libffi contains an object outside the ARMv4T baseline.\n' >&2
	exit 1
fi

readonly required_symbols="ffi_call ffi_closure_alloc ffi_prep_cif ffi_prep_closure_loc"
for symbol in ${required_symbols}; do
	if ! arm-mingw32ce-nm --defined-only "${built_archive}" \
		| awk '{ print $3 }' | grep -Fx "${symbol}" >/dev/null; then
		printf 'ERROR: libffi is missing required symbol %s.\n' "${symbol}" >&2
		exit 1
	fi
done

install -d "${target_dir}/include" "${target_dir}/licenses/libffi"
install -m 0644 "${built_archive}" "${target_dir}/libffi.a"
install -m 0644 "${stage_dir}/include/ffi.h" "${target_dir}/include/ffi.h"
install -m 0644 "${stage_dir}/include/ffitarget.h" "${target_dir}/include/ffitarget.h"
install -m 0644 "${source_dir}/LICENSE" "${target_dir}/licenses/libffi/LICENSE"

arm-mingw32ce-objdump -f "${target_dir}/libffi.a" | sed -n '1,18p'
printf 'Staged Windows CE ARM libffi in %s\n' "${target_dir}"
WINCE_LIBFFI_BUILD

# end of build_scripts/wince/build-libffi-arm.sh
