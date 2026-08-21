#!/usr/bin/env bash
#
# Project: FreeBASIC Windows CE Exampleageddon workflow
# -----------------------------------------------------
#
# File: wince/run-exampleageddon.sh
#
# Purpose:
#
#     Compile the complete example tree and run every self-contained example
#     on Windows CE ARM or MIPS.
#
# Responsibilities:
#
#     - compile and classify all examples with the selected WinCE toolchain
#     - stage each runnable example in its own resource directory
#     - execute bounded batches in the matching Windows CE emulator
#     - preserve per-example statuses, emulator logs, and screenshots
#     - support resumable execution without trusting incomplete batch output
#
# This file intentionally does NOT contain:
#
#     - Exampleageddon classification policy
#     - Windows CE ROM or emulator installation
#     - interactive, graphics, audio, or network input automation
#     - Windows CE toolchain construction policy

set -euo pipefail
trap 'echo "ERROR: failed at line $LINENO: $BASH_COMMAND" >&2' ERR

##############################################################################
# Defaults
##############################################################################

SCRIPT_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

TOOLCHAIN_IMAGE="${WINCE_TOOLCHAIN_IMAGE:-freebasic-wince-toolchain:noble}"
WORK_ROOT="${WINCE_WORK_ROOT:-$ROOT/out/wince/work}"
MIPS_COMPILER="${WINCE_MIPS_FBC:-$ROOT/src/tools/wince/fbc-wince-mips}"
MIPS_TOOLCHAIN_ROOT="${WINCE_MIPS_TOOLCHAIN_ROOT:-$ROOT/out/wince/mips-toolchain}"
CERF_ROOT="${WINCE_CERF_ROOT:-}"
OUTPUT_ROOT="${WINCE_EXAMPLEAGEDDON_OUTDIR:-}"
TARGET_ARCH="${WINCE_ARCH:-arm}"

BATCH_SIZE=25
BOOT_SECONDS=15
COMPILE_ONLY=0
COMPILE_TIMEOUT=180
JOBS=""
RESUME=0
RUN_SECONDS_OVERRIDE=""
SKIP_COMPILE=0
START_BATCH=1

SHARE_STAGE=""

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

require_command() {
	command -v "$1" >/dev/null 2>&1 ||
		die "required host tool not found: $1"
}

cleanup() {
	if [ -n "$SHARE_STAGE" ] &&
	   [[ "$SHARE_STAGE" == "$CERF_ROOT/share"/.exampleageddon-stage.* ]] &&
	   [ -d "$SHARE_STAGE" ]; then
		rm -rf -- "$SHARE_STAGE"
	fi
}

usage() {
	cat <<EOF
Usage: ./build_scripts/wince/run-exampleageddon.sh [options]

Options:
  --arch ARCH          Target architecture: arm or mips. Default: arm
  --batch-size N       Examples per emulator boot. Default: 25
  --boot-seconds N     Guest shell initialization wait. Default: 15
  --compile-only       Compile and classify without launching the emulator
  --compile-timeout N  Per-example compile timeout. Default: 180
  --jobs N             Parallel compiler jobs. Default: detected CPUs
  --resume             Reuse complete, all-zero saved batch results
  --run-seconds N      Override the calculated per-batch emulator timeout
  --skip-compile       Reuse the existing compile inventory
  --start-batch N      Start guest execution at this one-based batch number
  --out-dir DIR        Result root. Default: out/wince/exampleageddon/ARCH
  -h, --help           Show this help

The compile report covers every examples/**/*.bas source. Guest execution is
limited to the self-contained category selected by the shared classifier.
EOF
}

trap cleanup EXIT

##############################################################################
# Command-line parsing
##############################################################################

JOBS="${JOBS:-$(detect_jobs)}"

while [ "$#" -gt 0 ]; do
	case "$1" in
		--arch)
			require_value "$1" "${2-}"
			TARGET_ARCH="$2"
			shift 2
			;;
		--batch-size)
			require_value "$1" "${2-}"
			BATCH_SIZE="$2"
			shift 2
			;;
		--boot-seconds)
			require_value "$1" "${2-}"
			BOOT_SECONDS="$2"
			shift 2
			;;
		--compile-only)
			COMPILE_ONLY=1
			shift
			;;
		--compile-timeout)
			require_value "$1" "${2-}"
			COMPILE_TIMEOUT="$2"
			shift 2
			;;
		--jobs)
			require_value "$1" "${2-}"
			JOBS="$2"
			shift 2
			;;
		--resume)
			RESUME=1
			shift
			;;
		--run-seconds)
			require_value "$1" "${2-}"
			RUN_SECONDS_OVERRIDE="$2"
			shift 2
			;;
		--skip-compile)
			SKIP_COMPILE=1
			shift
			;;
		--start-batch)
			require_value "$1" "${2-}"
			START_BATCH="$2"
			shift 2
			;;
		--out-dir)
			require_value "$1" "${2-}"
			OUTPUT_ROOT="$2"
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

case "$TARGET_ARCH" in
	arm|mips) ;;
	*) die "--arch must be arm or mips" ;;
esac
if [ -z "$OUTPUT_ROOT" ]; then
	OUTPUT_ROOT="$ROOT/out/wince/exampleageddon/$TARGET_ARCH"
fi
if [ -z "$CERF_ROOT" ]; then
	if [ "$TARGET_ARCH" = arm ]; then
		CERF_ROOT="$ROOT/out/wince/emulator/cerf"
	else
		CERF_ROOT="$ROOT/out/wince/emulator/cerf-mips"
	fi
fi

for positive_integer in \
	"$BATCH_SIZE" "$BOOT_SECONDS" "$COMPILE_TIMEOUT" "$JOBS" \
	"$START_BATCH"; do
	case "$positive_integer" in
		''|*[!0-9]*|0) die "batch, timeout, and jobs values must be positive integers" ;;
	esac
done
if [ -n "$RUN_SECONDS_OVERRIDE" ]; then
	case "$RUN_SECONDS_OVERRIDE" in
		''|*[!0-9]*|0) die "--run-seconds must be a positive integer" ;;
	esac
fi

##############################################################################
# Validation and paths
##############################################################################

for tool in awk cp find grep mkdir python3 sed sort; do
	require_command "$tool"
done

if [ "$TARGET_ARCH" = arm ]; then
	require_command docker
	[ -x "$WORK_ROOT/bin/fbc" ] || die "prepared host compiler not found"
	RUNTIME_ROOT="$WORK_ROOT/lib/freebasic/wince-arm"
else
	[ -x "$MIPS_COMPILER" ] || die "Windows CE MIPS compiler wrapper not found"
	[ -x "$MIPS_TOOLCHAIN_ROOT/bin/mips-wince-pe-ld" ] ||
		die "Windows CE MIPS linker not found"
	RUNTIME_ROOT="$ROOT/lib/freebasic/wince-mips32el"
fi
[ -f "$RUNTIME_ROOT/libfbmt.a" ] ||
	die "Windows CE $TARGET_ARCH multithreaded runtime not found"
[ -f "$RUNTIME_ROOT/libfbgfxmt.a" ] ||
	die "Windows CE $TARGET_ARCH gfxlib2 runtime not found"

EMULATOR_RUNNER="$SCRIPT_DIR/run-$TARGET_ARCH-emulator.sh"
if [ "$COMPILE_ONLY" -eq 0 ]; then
	[ -f "$ROOT/tests/wince/exampleageddon-runner.c" ] ||
		die "Windows CE Exampleageddon guest runner source not found"
	[ -x "$EMULATOR_RUNNER" ] ||
		die "Windows CE $TARGET_ARCH emulator runner is not executable"
	[ -d "$CERF_ROOT/share" ] || die "CERF shared directory not found"
	if [ "$TARGET_ARCH" = mips ]; then
		require_command clang
	fi
fi

mkdir -p "$OUTPUT_ROOT" "$OUTPUT_ROOT/batches" "$OUTPUT_ROOT/logs" \
	"$OUTPUT_ROOT/screenshots"
OUTPUT_ROOT="$(cd "$OUTPUT_ROOT" && pwd)"
INVENTORY="$OUTPUT_ROOT/results.csv"
MANIFEST="$OUTPUT_ROOT/manifest.tsv"
SHARE_ROOT="$CERF_ROOT/share"

##############################################################################
# Compilation and runnable manifest
##############################################################################

compile_inventory() {
	[ "$SKIP_COMPILE" -eq 0 ] || return 0

	msg "compiling all examples for Windows CE $TARGET_ARCH"
	if [ "$TARGET_ARCH" = arm ]; then
		docker run --rm --user "$(id -u):$(id -g)" \
			-v "$ROOT:/src" \
			-v "$WORK_ROOT:/work" \
			-v "$OUTPUT_ROOT:/output" \
			-w /src \
			"$TOOLCHAIN_IMAGE" \
			python3 /src/build_scripts/exampleageddon-freebasic.py \
				--root /src \
				--outdir /output \
				--fbc '/work/bin/fbc -target wince-arm -gen gcc -O 0 -mt -i /src/inc/wince' \
				--prefix /work \
				--include-dir /src/inc \
				--target-os wince \
				--jobs "$JOBS" \
				--compile-timeout "$COMPILE_TIMEOUT" \
				--no-run \
				--fail-on-self-contained
	else
		python3 "$ROOT/build_scripts/exampleageddon-freebasic.py" \
			--root "$ROOT" \
			--outdir "$OUTPUT_ROOT" \
			--fbc "$MIPS_COMPILER -O 0 -mt" \
			--no-prefix \
			--include-dir "$ROOT/inc/wince" \
			--target-os wince \
			--jobs "$JOBS" \
			--compile-timeout "$COMPILE_TIMEOUT" \
			--no-run \
			--fail-on-self-contained
	fi
}

write_manifest() {
	python3 - "$INVENTORY" "$OUTPUT_ROOT" "$ROOT" <<'PY'
import csv
import sys
from pathlib import Path

inventory = Path(sys.argv[1])
output_root = Path(sys.argv[2])
source_root = Path(sys.argv[3])
manifest = output_root / "manifest.tsv"

with inventory.open(newline="", encoding="utf-8") as stream:
    rows = list(csv.DictReader(stream))

selected = [
    row for row in rows
    if row["group"] == "self-contained" and row["compile_status"] == "pass"
]

with manifest.open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
    writer.writerow(("index", "case_id", "path", "program", "compile_directory"))
    for index, row in enumerate(selected, start=1):
        stem = Path(row["output"]).name
        program = output_root / "bin" / stem
        if not program.is_file():
            program = program.with_name(program.name + ".exe")
        compile_directory = output_root / "work" / stem / "compile"
        if not program.is_file():
            raise SystemExit(f"compiled WinCE executable is missing: {program}")
        if not compile_directory.is_dir():
            raise SystemExit(f"compile resource directory is missing: {compile_directory}")
        writer.writerow((f"{index:04d}", f"case{index:04d}", row["path"],
                         program, compile_directory))
PY
}

build_guest_runner() {
	msg "building Windows CE $TARGET_ARCH Exampleageddon guest runner"
	if [ "$TARGET_ARCH" = arm ]; then
		docker run --rm --user "$(id -u):$(id -g)" \
			-v "$ROOT:/src" \
			-v "$OUTPUT_ROOT:/output" \
			-w /src \
			"$TOOLCHAIN_IMAGE" \
			arm-mingw32ce-gcc \
				-O0 -march=armv4t -mfloat-abi=soft \
				-D_WIN32_WCE=0x0500 \
				tests/wince/exampleageddon-runner.c \
				-o /output/exampleageddon-runner.exe
	else
		clang --target=mipsel-pc-windows-msvc \
			-mcpu=mips2 -mno-check-zero-division -mabi=32 -msoft-float \
			-ffreestanding -fno-builtin -fno-exceptions \
			-fno-unwind-tables -fno-asynchronous-unwind-tables \
			-D__MINGW32CE__ -D__MINGW32__ -D__COREDLL__ \
			-D__GNUC__=4 -D__GNUC_MINOR__=2 -D_M_MRX000=4000 -DMIPS \
			-D_WIN32_WCE=0x0500 \
			-isystem "$MIPS_TOOLCHAIN_ROOT/include" -O0 \
			-c "$ROOT/tests/wince/exampleageddon-runner.c" \
			-o "$OUTPUT_ROOT/exampleageddon-runner.o"
		"$MIPS_COMPILER" -O 0 \
			"$OUTPUT_ROOT/exampleageddon-runner.o" \
			-x "$OUTPUT_ROOT/exampleageddon-runner.exe"
	fi
}

##############################################################################
# Batch staging and validation
##############################################################################

install_share_stage() {
	local destination="$SHARE_ROOT/exampleageddon"

	[ -d "$SHARE_STAGE" ] || die "batch share stage is missing"
	[ "$destination" != "/" ] || die "refusing to replace the filesystem root"
	rm -rf -- "$destination"
	mv "$SHARE_STAGE" "$destination"
	SHARE_STAGE=""
}

stage_batch() {
	local row
	local index
	local case_id
	local source_path
	local program
	local compile_directory
	local case_directory

	SHARE_STAGE="$(mktemp -d "$SHARE_ROOT/.exampleageddon-stage.XXXXXX")"
	: > "$SHARE_STAGE/manifest.txt"

	for row in "${BATCH_ROWS[@]}"; do
		IFS=$'\t' read -r \
			index case_id source_path program compile_directory <<< "$row"
		case_directory="$SHARE_STAGE/$case_id"
		mkdir -p "$case_directory"
		cp -a "$compile_directory/." "$case_directory/"
		find "$case_directory" -type f \
			\( -name '*.o' -o -name '*.obj' -o -name '*.asm' \
			   -o -name '*.exe' \) -delete
		cp "$program" "$case_directory/runner.exe"
		printf '%s\r\n' "$case_id" >> "$SHARE_STAGE/manifest.txt"
	done

	install_share_stage
	cp "$SHARE_ROOT/exampleageddon/manifest.txt" \
		"$SHARE_ROOT/exampleageddon-manifest.txt"
	cp "$OUTPUT_ROOT/exampleageddon-runner.exe" \
		"$SHARE_ROOT/exampleageddon-runner.exe"
}

remove_guest_outputs() {
	local output_name

	for output_name in exampleageddon.result exampleageddon.done; do
		if [ -f "$SHARE_ROOT/$output_name" ]; then
			rm -f -- "$SHARE_ROOT/$output_name"
		fi
	done
}

validate_batch_result() {
	local expected_count="$2"
	local result_file="$1"
	local actual_count

	[ -s "$result_file" ] || return 1
	actual_count="$(awk 'NF >= 2 { count += 1 } END { print count + 0 }' "$result_file")"
	[ "$actual_count" -eq "$expected_count" ] || return 1
	awk 'NF < 2 || ($2 + 0) != 0 { failed = 1 } END { exit failed }' \
		"$result_file"
}

save_batch_evidence() {
	local batch_id="$1"
	local log_stem="exampleageddon-$TARGET_ARCH-$batch_id"

	cp "$SHARE_ROOT/exampleageddon.result" \
		"$OUTPUT_ROOT/batches/$batch_id.result"
	cp "$SHARE_ROOT/exampleageddon.done" \
		"$OUTPUT_ROOT/batches/$batch_id.done"
	if [ -f "$CERF_ROOT/logs/$log_stem.log" ]; then
		cp "$CERF_ROOT/logs/$log_stem.log" \
			"$OUTPUT_ROOT/logs/$batch_id.log"
	fi
	if [ -f "$CERF_ROOT/logs/$log_stem.png" ]; then
		cp "$CERF_ROOT/logs/$log_stem.png" \
			"$OUTPUT_ROOT/screenshots/$batch_id.png"
	fi
}

##############################################################################
# Execution and report
##############################################################################

compile_inventory
[ -s "$INVENTORY" ] || die "Exampleageddon compile inventory is missing"
write_manifest

mapfile -t MANIFEST_ROWS < <(tail -n +2 "$MANIFEST")
TOTAL_CASES="${#MANIFEST_ROWS[@]}"
[ "$TOTAL_CASES" -gt 0 ] || die "no self-contained examples were selected"

if [ "$COMPILE_ONLY" -eq 1 ]; then
	echo "==> Windows CE $TARGET_ARCH Exampleageddon compiled: $TOTAL_CASES runnable cases"
	exit 0
fi

build_guest_runner

TOTAL_BATCHES=$(( (TOTAL_CASES + BATCH_SIZE - 1) / BATCH_SIZE ))
[ "$START_BATCH" -le "$TOTAL_BATCHES" ] ||
	die "--start-batch exceeds the $TOTAL_BATCHES available batches"

for ((batch_number = START_BATCH; batch_number <= TOTAL_BATCHES; ++batch_number)); do
	batch_id="batch$(printf '%04d' "$batch_number")"
	batch_start=$(( (batch_number - 1) * BATCH_SIZE ))
	BATCH_ROWS=("${MANIFEST_ROWS[@]:batch_start:BATCH_SIZE}")
	expected_count="${#BATCH_ROWS[@]}"
	saved_result="$OUTPUT_ROOT/batches/$batch_id.result"

	if [ "$RESUME" -eq 1 ] &&
	   validate_batch_result "$saved_result" "$expected_count"; then
		echo "==> PASS (saved): $batch_id ($expected_count examples)"
		continue
	fi

	msg "running Windows CE $TARGET_ARCH Exampleageddon $batch_id/$TOTAL_BATCHES"
	stage_batch
	remove_guest_outputs

	if [ -n "$RUN_SECONDS_OVERRIDE" ]; then
		run_seconds="$RUN_SECONDS_OVERRIDE"
	else
		run_seconds=$((expected_count * 15 + 45))
	fi

	"$EMULATOR_RUNNER" \
		--program exampleageddon-runner.exe \
		--completion exampleageddon.done \
		--log-stem "exampleageddon-$TARGET_ARCH-$batch_id" \
		--boot-seconds "$BOOT_SECONDS" \
		--run-seconds "$run_seconds"

	[ -s "$SHARE_ROOT/exampleageddon.done" ] ||
		die "$batch_id did not publish its completion marker"
	validate_batch_result "$SHARE_ROOT/exampleageddon.result" \
		"$expected_count" || {
		save_batch_evidence "$batch_id"
		die "$batch_id contains a failed or incomplete example"
	}
	save_batch_evidence "$batch_id"
	echo "==> PASS: $batch_id ($expected_count examples)"
done

for ((batch_number = 1; batch_number <= TOTAL_BATCHES; ++batch_number)); do
	batch_id="batch$(printf '%04d' "$batch_number")"
	batch_start=$(( (batch_number - 1) * BATCH_SIZE ))
	remaining=$((TOTAL_CASES - batch_start))
	expected_count="$BATCH_SIZE"
	if [ "$remaining" -lt "$BATCH_SIZE" ]; then
		expected_count="$remaining"
	fi
	validate_batch_result "$OUTPUT_ROOT/batches/$batch_id.result" \
		"$expected_count" || die "saved result is incomplete: $batch_id"
done

python3 - "$MANIFEST" "$OUTPUT_ROOT" "$TARGET_ARCH" <<'PY'
import csv
import sys
from pathlib import Path

manifest_path = Path(sys.argv[1])
output_root = Path(sys.argv[2])
target_arch = sys.argv[3]

with manifest_path.open(newline="", encoding="utf-8") as stream:
    rows = list(csv.DictReader(stream, delimiter="\t"))

statuses = {}
for result_path in sorted((output_root / "batches").glob("batch*.result")):
    for line in result_path.read_text(encoding="utf-8").splitlines():
        fields = line.split("\t")
        if len(fields) >= 2:
            statuses[fields[0]] = int(fields[1])

result_path = output_root / "execution-results.tsv"
with result_path.open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
    writer.writerow(("index", "case_id", "path", "status"))
    for row in rows:
        writer.writerow((row["index"], row["case_id"], row["path"],
                         statuses[row["case_id"]]))

report = output_root / "execution-report.md"
report.write_text(
    f"# Windows CE {target_arch} Exampleageddon execution\n\n"
    f"- Self-contained examples run: {len(rows)}\n"
    "- Passed: " + str(sum(status == 0 for status in statuses.values())) + "\n"
    "- Failed: " + str(sum(status != 0 for status in statuses.values())) + "\n"
    "- Per-example evidence: `execution-results.tsv`\n"
    "- Per-batch emulator evidence: `logs/` and `screenshots/`\n",
    encoding="utf-8",
)
PY

echo
echo "==> Windows CE $TARGET_ARCH Exampleageddon passed: $TOTAL_CASES/$TOTAL_CASES examples"
echo "    Compile report:   $OUTPUT_ROOT/report.md"
echo "    Execution report: $OUTPUT_ROOT/execution-report.md"

# end of build_scripts/wince/run-exampleageddon.sh
