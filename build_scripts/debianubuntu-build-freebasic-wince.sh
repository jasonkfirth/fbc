#!/usr/bin/env bash
#
# Project: FreeBASIC Windows CE release workflow
# ------------------------------------------------
#
# File: debianubuntu-build-freebasic-wince.sh
#
# Purpose:
#
#     Build, qualify, and package FreeBASIC for Windows CE ARM from a Debian
#     or Ubuntu host without installing obsolete cross tools on that host.
#
# Responsibilities:
#
#     - build the pinned CeGCC and CERF support container images
#     - prepare an isolated source copy below out/wince
#     - build the host compiler, libffi, rtlib, gfxlib2, and sfxlib
#     - create and emulator-validate the relocatable compiler cross-SDK
#     - run full fbctests, Exampleageddon, and available OMA launch checks
#     - preserve packages, logs, reports, and screenshots below out/wince
#
# This file intentionally does NOT contain:
#
#     - compiler, runtime, graphics, or sound implementation
#     - individual fbctests or example batching logic
#     - OMA source and asset definitions
#     - proprietary Windows CE ROM redistribution
#     - Windows CE MIPS toolchain policy
#
# Host dependency policy:
#
#     The host needs Docker and rsync only.  CeGCC, Wine, Xvfb, archive tools,
#     and build prerequisites remain in pinned containers so Ubuntu repository
#     contents cannot silently change the target supply chain.

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

OUTPUT_ROOT="${WINCE_OUTPUT_ROOT:-$ROOT/out/wince}"
WORK_ROOT="${WINCE_WORK_ROOT:-$ROOT/out/wince/work}"
CERF_ROOT="${WINCE_CERF_ROOT:-$ROOT/out/wince/emulator/cerf}"
TOOLCHAIN_IMAGE="${WINCE_TOOLCHAIN_IMAGE:-freebasic-wince-toolchain:noble}"
EMULATOR_IMAGE="${WINCE_EMULATOR_IMAGE:-freebasic-wince-emulator:noble}"

SKIP_TOOLCHAIN_IMAGE=0
SKIP_EMULATOR_IMAGE=0
REUSE_WORKTREE=0
INCREMENTAL=0
SKIP_PACKAGE=0
SKIP_FBCTESTS=0
SKIP_EXAMPLEAGEDDON=0
SKIP_OMA=0
RESUME_TESTS=0

##############################################################################
# Helpers
##############################################################################

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

	# Two simultaneous bootstrap compiler processes are a conservative default
	# for workstations also carrying the memory cost of the emulator.
	if [ "$jobs" -gt 2 ]; then
		jobs=2
	fi

	printf '%s\n' "$jobs"
}

require_value() {
	local option="$1"
	local value="${2-}"

	[ -n "$value" ] || die "$option requires a value"
}

require_command() {
	command -v "$1" >/dev/null 2>&1 ||
		die "required host tool not found: $1"
}

emulator_work_requested() {
	[ "$SKIP_FBCTESTS" -eq 0 ] ||
	[ "$SKIP_EXAMPLEAGEDDON" -eq 0 ] ||
	[ "$SKIP_OMA" -eq 0 ] ||
	[ "$SKIP_PACKAGE" -eq 0 ]
}

usage() {
	cat <<EOF
Usage: ./build_scripts/debianubuntu-build-freebasic-wince.sh [options]

Options:
  --jobs N                 Parallel build jobs. Default: detected, capped at 2
  --skip-toolchain-image   Reuse the existing pinned CeGCC image.
  --skip-emulator-image    Reuse the existing pinned Wine/CERF support image.
  --reuse-worktree         Reuse out/wince/work without refreshing source.
  --incremental            Preserve compatible compiler and library objects.
  --skip-package           Skip compiler archive creation and package smoke.
  --skip-fbctests          Skip the complete fbcunit run.
  --skip-exampleageddon    Skip Exampleageddon compilation and execution.
  --skip-oma               Skip OMA builds, packages, and launch checks.
  --resume                 Reuse matching successful test evidence.
  --work-dir DIR           Isolated source copy. Default: out/wince/work
  --out-dir DIR            Persistent output root. Default: out/wince
  --cerf-dir DIR           Prepared CERF/ROM tree.
  --toolchain-image NAME   CeGCC image name. Default: $TOOLCHAIN_IMAGE
  --emulator-image NAME    Emulator image name. Default: $EMULATOR_IMAGE
  -h, --help               Show this help.

Host prerequisites, if they are not already installed:

  sudo apt-get update
  sudo apt-get install -y docker.io rsync

The Windows CE ROM is not redistributed. See docs/wince.md for the exact CERF
layout and the user-supplied Microsoft emulator-image requirement.
EOF
}

##############################################################################
# Command-line parsing
##############################################################################

JOBS="${JOBS:-$(detect_jobs)}"

while [ "$#" -gt 0 ]; do
	case "$1" in
		--jobs)
			require_value "$1" "${2-}"
			JOBS="$2"
			shift 2
			;;
		--skip-toolchain-image)
			SKIP_TOOLCHAIN_IMAGE=1
			shift
			;;
		--skip-emulator-image)
			SKIP_EMULATOR_IMAGE=1
			shift
			;;
		--reuse-worktree)
			REUSE_WORKTREE=1
			shift
			;;
		--incremental)
			INCREMENTAL=1
			shift
			;;
		--skip-package)
			SKIP_PACKAGE=1
			shift
			;;
		--skip-fbctests)
			SKIP_FBCTESTS=1
			shift
			;;
		--skip-exampleageddon)
			SKIP_EXAMPLEAGEDDON=1
			shift
			;;
		--skip-oma)
			SKIP_OMA=1
			shift
			;;
		--resume)
			RESUME_TESTS=1
			shift
			;;
		--work-dir)
			require_value "$1" "${2-}"
			WORK_ROOT="$2"
			shift 2
			;;
		--out-dir)
			require_value "$1" "${2-}"
			OUTPUT_ROOT="$2"
			shift 2
			;;
		--cerf-dir)
			require_value "$1" "${2-}"
			CERF_ROOT="$2"
			shift 2
			;;
		--toolchain-image)
			require_value "$1" "${2-}"
			TOOLCHAIN_IMAGE="$2"
			shift 2
			;;
		--emulator-image)
			require_value "$1" "${2-}"
			EMULATOR_IMAGE="$2"
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

for host_tool in docker realpath rsync; do
	require_command "$host_tool"
done
docker info >/dev/null 2>&1 || die "Docker is installed, but its daemon is unavailable"

##############################################################################
# Pinned target toolchain
##############################################################################

ROOT_REAL="$(realpath -m "$ROOT")"
OUTPUT_REAL="$(realpath -m "$OUTPUT_ROOT")"
WORK_REAL="$(realpath -m "$WORK_ROOT")"
CERF_REAL="$(realpath -m "$CERF_ROOT")"

[ "$OUTPUT_REAL" != / ] || die "refusing to use / as the output root"
[ "$WORK_REAL" != / ] || die "refusing to use / as the work root"
[ "$WORK_REAL" != "$ROOT_REAL" ] || die "the isolated work root cannot be the source root"
[[ "$WORK_REAL" == "$OUTPUT_REAL"/* ]] ||
	die "the work root must be below the selected Windows CE output root"

if [ "$SKIP_TOOLCHAIN_IMAGE" -eq 0 ]; then
	msg "building the pinned ARM CeGCC image"
	# The Dockerfile imports the pinned CeGCC image and needs no repository
	# context. Feeding it on stdin avoids sending this large worktree to Docker.
	docker build -t "$TOOLCHAIN_IMAGE" - < "$ROOT_REAL/build_scripts/wince/Dockerfile"
else
	docker image inspect "$TOOLCHAIN_IMAGE" >/dev/null 2>&1 ||
		die "CeGCC image does not exist: $TOOLCHAIN_IMAGE"
fi

##############################################################################
# Guarded workspace preparation
##############################################################################

SOURCE_EXCLUDES="$ROOT/mk/source-copy-excludes.rsync"
[ -f "$SOURCE_EXCLUDES" ] || die "source copy exclusions are missing"

mkdir -p "$OUTPUT_REAL" "$WORK_REAL"

# Older versions of this workflow ran target builds as root in the container.
# Normalize only the disposable isolated worktree so rsync and the unprivileged
# build can replace those stale outputs.  The developer's source tree is never
# mounted by this repair step.
run docker run --rm \
	--mount "type=bind,src=$WORK_REAL,dst=/work" \
	"$TOOLCHAIN_IMAGE" \
	bash -ceu '
		find /work -xdev \( ! -uid "$1" -o ! -gid "$2" \) \
			-exec chown "$1:$2" {} +
	' -- "$(id -u)" "$(id -g)"

if [ "$REUSE_WORKTREE" -eq 0 ]; then
	msg "refreshing the isolated Windows CE source tree"
	run rsync -a --delete --delete-excluded \
		--exclude-from="$SOURCE_EXCLUDES" \
		"$ROOT_REAL/" "$WORK_REAL/"
fi

##############################################################################
# Pinned emulator environment
##############################################################################

if emulator_work_requested; then
	[ -f "$CERF_REAL/cerf.exe" ] || die "CERF is not prepared: $CERF_REAL/cerf.exe"
	[ -f "$CERF_REAL/roms-wince-arm.bin" ] ||
		die "Windows CE ARM ROM is not prepared: $CERF_REAL/roms-wince-arm.bin"
	[ -d "$CERF_REAL/share" ] || die "CERF share directory is missing"

	if [ "$SKIP_EMULATOR_IMAGE" -eq 0 ]; then
		msg "building the pinned CERF support image"
		run docker build \
			-f "$ROOT_REAL/build_scripts/wince/emulator/Dockerfile" \
			-t "$EMULATOR_IMAGE" \
			"$ROOT_REAL/build_scripts/wince/emulator"
	else
		docker image inspect "$EMULATOR_IMAGE" >/dev/null 2>&1 ||
			die "CERF support image does not exist: $EMULATOR_IMAGE"
	fi
fi

##############################################################################
# Compiler and libraries
##############################################################################

container_arguments=( --jobs "$JOBS" )
if [ "$INCREMENTAL" -eq 1 ]; then
	container_arguments+=( --incremental )
fi

msg "building FreeBASIC and Windows CE ARM libraries"
run docker run --rm \
	--user "$(id -u):$(id -g)" \
	--mount "type=bind,src=$WORK_REAL,dst=/work" \
	--mount "type=bind,src=$OUTPUT_REAL,dst=/wince-output" \
	"$TOOLCHAIN_IMAGE" \
	/work/build_scripts/wince/container-build.sh "${container_arguments[@]}"

##############################################################################
# Packages and emulator qualification
##############################################################################

export WINCE_OUTPUT_ROOT="$OUTPUT_REAL"
export WINCE_WORK_ROOT="$WORK_REAL"
export WINCE_CERF_ROOT="$CERF_REAL"
export WINCE_TOOLCHAIN_IMAGE="$TOOLCHAIN_IMAGE"
export WINCE_EMULATOR_IMAGE="$EMULATOR_IMAGE"

if [ "$SKIP_PACKAGE" -eq 0 ]; then
	msg "creating and validating the Windows CE ARM compiler package"
	run "$ROOT_REAL/build_scripts/wince-package-freebasic.sh" \
		--work-dir "$WORK_REAL" \
		--image "$TOOLCHAIN_IMAGE"

	PACKAGE_SMOKE="$OUTPUT_REAL/packages/validation/arm/package-basic-file.exe"
	[ -s "$PACKAGE_SMOKE" ] || die "package-built emulator smoke is missing"
	cp "$PACKAGE_SMOKE" "$CERF_REAL/share/package-basic-file.exe"
	if [ -e "$CERF_REAL/share/fb-wince-smoke.txt" ]; then
		unlink "$CERF_REAL/share/fb-wince-smoke.txt"
	fi

	run "$ROOT_REAL/build_scripts/wince/run-arm-emulator.sh" \
		--program package-basic-file.exe \
		--completion fb-wince-smoke.txt \
		--log-stem package-arm-basic-file \
		--boot-seconds 15 \
		--run-seconds 20
	grep -Fq 'FreeBASIC Windows CE runtime smoke passed' \
		"$CERF_REAL/share/fb-wince-smoke.txt" ||
		die "package-built executable did not produce its expected guest output"
fi

if [ "$SKIP_FBCTESTS" -eq 0 ]; then
	msg "running the complete Windows CE ARM fbcunit suite"
	fbctests_arguments=()
	if [ "$RESUME_TESTS" -eq 1 ]; then
		fbctests_arguments+=( --resume )
	fi
	run "$ROOT_REAL/build_scripts/wince/run-fbctests.sh" \
		"${fbctests_arguments[@]}"
fi

if [ "$SKIP_EXAMPLEAGEDDON" -eq 0 ]; then
	msg "running Windows CE ARM Exampleageddon"
	example_arguments=()
	if [ "$RESUME_TESTS" -eq 1 ]; then
		example_arguments+=( --resume )
	fi
	run "$ROOT_REAL/build_scripts/wince/run-exampleageddon.sh" \
		"${example_arguments[@]}"
fi

if [ "$SKIP_OMA" -eq 0 ]; then
	msg "building, packaging, and launch-qualifying the OMA corpus"
	oma_arguments=( --stability 15 )
	if [ "$RESUME_TESTS" -eq 1 ]; then
		oma_arguments+=( --resume )
	fi
	run "$ROOT_REAL/build_scripts/wince/run-oma-games.sh" \
		"${oma_arguments[@]}"
fi

msg "Windows CE ARM release workflow completed"
find "$OUTPUT_REAL/packages" -type f \
	\( -name '*.tar.xz' -o -name '*.zip' -o -name '*.SHA256SUMS' \) \
	-printf '%P\n' | sort

# end of build_scripts/debianubuntu-build-freebasic-wince.sh
