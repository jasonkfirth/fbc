#!/usr/bin/env bash
#
# Project: FreeBASIC Debian/Ubuntu Windows CE package workflow
# -----------------------------------------------------------
#
# File: debianubuntu-build-freebasic-wince-package.sh
#
# Purpose:
#
#     Build an installable amd64 freebasic-wince package on a current
#     Debian or Ubuntu host, including both supported Windows CE targets.
#
# Responsibilities:
#
#     - install the host dependencies used by the existing WinCE builders
#     - build ARM and MIPS runtimes in one isolated source tree
#     - build and bundle the pinned target toolchains
#     - invoke the Debian package staging and smoke-validation workflow
#     - place artifacts in the standard distro/codename/architecture tree
#
# This file intentionally does NOT contain:
#
#     - compiler, runtime, or emulator implementation
#     - Windows CE ROM acquisition
#     - application-specific builds
#     - Win32 MinGW-w64 packaging
#

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

NO_BUILD=0
SKIP_DEPS=0
KEEP_WORK=0
JOBS="${JOBS:-2}"
BUILDROOT="${BUILDROOT:-$ROOT/.build-debianubuntu-wince-package}"
WINCE_BUILD_ROOT="$BUILDROOT/wince-output"
WORK_ROOT="$WINCE_BUILD_ROOT/work"
OUTBASE="${OUTBASE:-$ROOT/out}"
TOOLCHAIN_IMAGE="${WINCE_TOOLCHAIN_IMAGE:-freebasic-wince-toolchain:noble}"
PACKAGE_REVISION="${WINCE_PACKAGE_REVISION:-1}"

die() {
	echo "ERROR: $*" >&2
	exit 1
}

msg() {
	echo
	echo "==> $*"
}

run() {
	echo "==> $*"
	"$@"
}

run_root() {
	if [ "$(id -u)" -eq 0 ]; then
		run "$@"
	elif command -v sudo >/dev/null 2>&1; then
		run sudo "$@"
	else
		die "root privileges are required to install build dependencies"
	fi
}

cleanup() {
	[ "$KEEP_WORK" -eq 0 ] || return 0
	[ -d "$BUILDROOT" ] || return 0
	[[ "$BUILDROOT" == "$ROOT"/.build-debianubuntu-wince-package* ]] || return 0
	rm -rf -- "$BUILDROOT"
}

usage() {
	cat <<EOF
Usage: ./build_scripts/debianubuntu-build-freebasic-wince-package.sh [options]

Options:
  --no-build       Reuse the prepared isolated ARM/MIPS build tree.
  --skip-deps      Do not install APT build dependencies.
  --jobs N         Parallel build jobs. Default: $JOBS
  --revision N     Debian package revision. Default: $PACKAGE_REVISION
  --keep-work      Preserve the isolated build tree after success.
  -h, --help       Show this help text.

Environment:
  BUILDROOT        Isolated build root.
  OUTBASE          Artifact root. Default: out
  FBC_PACKAGE_OUTDIR
                   Full package output directory override.

Artifacts are written below:
  out/linux/<distro>/<codename>/amd64/wince/

On Ubuntu Resolute amd64 this produces an installer containing fbc-wince,
both Windows CE target runtimes, and their required PE toolchains.
EOF
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--no-build)
			NO_BUILD=1
			shift
			;;
		--skip-deps)
			SKIP_DEPS=1
			shift
			;;
		--jobs)
			[ "$#" -ge 2 ] || die "$1 requires a value"
			JOBS="$2"
			shift 2
			;;
		--revision)
			[ "$#" -ge 2 ] || die "$1 requires a value"
			PACKAGE_REVISION="$2"
			shift 2
			;;
		--keep-work)
			KEEP_WORK=1
			shift
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
case "$PACKAGE_REVISION" in
	''|*[!0-9]*|0) die "--revision must be a positive integer" ;;
esac
command -v apt-get >/dev/null 2>&1 ||
	die "this workflow requires a Debian or Ubuntu host"

if [ "$SKIP_DEPS" -eq 0 ]; then
	msg "installing Windows CE package build dependencies"
	export DEBIAN_FRONTEND=noninteractive
	run_root apt-get update -y
	run_root apt-get install -y --no-install-recommends \
		autoconf automake binutils bison build-essential ca-certificates clang coreutils curl \
		docker.io dpkg-dev fakeroot file flex git libtool llvm make patch \
		pkgconf rsync texinfo xz-utils zlib1g-dev
fi

for tool in clang docker dpkg fakeroot flex git llvm-dlltool make rsync; do
	command -v "$tool" >/dev/null 2>&1 || die "required tool not found: $tool"
done
[ "$(dpkg --print-architecture)" = amd64 ] ||
	die "this workflow creates an amd64 installer and requires an amd64 host"
docker info >/dev/null 2>&1 || die "Docker daemon is unavailable"

DISTRO_ID=unknown
CODENAME=unknown
if [ -f /etc/os-release ]; then
	DISTRO_ID="$(. /etc/os-release; printf '%s' "${ID:-unknown}")"
	CODENAME="$(. /etc/os-release; printf '%s' "${VERSION_CODENAME:-unknown}")"
fi
DISTRO_ID="${FBC_PACKAGE_DISTRO_ID:-$DISTRO_ID}"
CODENAME="${FBC_PACKAGE_CODENAME:-$CODENAME}"
PACKAGE_OUTDIR="${FBC_PACKAGE_OUTDIR:-$OUTBASE/linux/$DISTRO_ID/$CODENAME/amd64/wince}"

mkdir -p "$BUILDROOT" "$PACKAGE_OUTDIR"
if [ "$NO_BUILD" -eq 0 ]; then
	msg "building the isolated Windows CE ARM target"
	WINCE_OUTPUT_ROOT="$WINCE_BUILD_ROOT" \
	WINCE_WORK_ROOT="$WORK_ROOT" \
	WINCE_TOOLCHAIN_IMAGE="$TOOLCHAIN_IMAGE" \
		"$ROOT/build_scripts/debianubuntu-build-freebasic-wince.sh" \
		--jobs "$JOBS" \
		--skip-package \
		--skip-fbctests \
		--skip-exampleageddon \
		--skip-oma

	msg "building the isolated Windows CE MIPS target"
	(
		cd "$WORK_ROOT"
		WINCE_OUTPUT_ROOT="$WINCE_BUILD_ROOT" \
		WINCE_TOOLCHAIN_IMAGE="$TOOLCHAIN_IMAGE" \
			./build_scripts/wince/build-mips-libraries.sh --jobs "$JOBS"
	)
fi

msg "creating the installable Windows CE package"
WINCE_PACKAGE_REVISION="$PACKAGE_REVISION" \
	"$ROOT/build_scripts/wince-package-debian.sh" \
	--work-dir "$WORK_ROOT" \
	--toolchain-dir "$WINCE_BUILD_ROOT/mips-toolchain" \
	--image "$TOOLCHAIN_IMAGE" \
	--out-dir "$PACKAGE_OUTDIR" \
	--validation-dir "$WINCE_BUILD_ROOT/package-validation" \
	--revision "$PACKAGE_REVISION"

msg "Windows CE Debian/Ubuntu package workflow completed"
find "$PACKAGE_OUTDIR" -maxdepth 1 -type f \
	\( -name '*.deb' -o -name '*.sha256' \) -printf '%f\n' | sort

cleanup

# end of build_scripts/debianubuntu-build-freebasic-wince-package.sh
