#!/usr/bin/env python3
#
# Project: FreeBASIC NuttX/RISC-V runtime audit
# ---------------------------------------------
#
# File: nuttx-riscv32-rtlib-audit.py
#
# Purpose:
#
#     Check that the NuttX QEMU smoke harness keeps using the generic
#     FreeBASIC runtime helpers that are already safe for the target.
#
# Responsibilities:
#
#     - verify that each approved generic rtlib substitution is still staged
#     - verify that the source files used by those substitutions still exist
#     - document the NuttX-specific shims that are intentionally not replaced
#       by generic rtlib code yet
#     - produce stable text for local validation and remote QEMU bring-up work
#
# This file intentionally does NOT contain:
#
#     - QEMU launch logic
#     - fbctests execution
#     - hardware flashing or mounted-drive writes
#     - automatic replacement of NuttX runtime shims
#

from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class GenericSubstitution:
    name: str
    generic_source: str
    staged_source: str
    smoke_script_markers: tuple[str, ...]
    note: str
    source_markers: tuple[tuple[str, str], ...] = ()


@dataclass(frozen=True)
class NuttXShim:
    name: str
    source: str
    reason: str


GENERIC_SUBSTITUTIONS: tuple[GenericSubstitution, ...] = (
    GenericSubstitution(
        "memory clear/copy helper",
        "src/rtlib/mem_copyclear.c",
        "fb_nuttx_generic_memory.c",
        (
            'USE_GENERIC_MEMORY="${FB_NUTTX_USE_GENERIC_MEMORY:-1}"',
            'cp "$ROOT/src/rtlib/mem_copyclear.c" '
            '"$APP_DIR/fb_nuttx_generic_memory.c"',
            "CFLAGS += -DFB_NUTTX_USE_GENERIC_MEMORY=$USE_GENERIC_MEMORY",
        ),
        "The local fallback shim is disabled while the generic helper is built.",
        (
            (
                "src/rtlib/nuttx/fb_nuttx_memory.c",
                "#if !defined(FB_NUTTX_USE_GENERIC_MEMORY)",
            ),
        ),
    ),
    GenericSubstitution(
        "GOSUB helper",
        "src/rtlib/gosub.c",
        "fb_nuttx_gosub.c",
        (
            'USE_GENERIC_GOSUB="${FB_NUTTX_USE_GENERIC_GOSUB:-1}"',
            'cp "$ROOT/src/rtlib/gosub.c" "$APP_DIR/fb_nuttx_generic_gosub.c"',
            "CFLAGS += -DFB_NUTTX_USE_GENERIC_GOSUB=$USE_GENERIC_GOSUB",
        ),
        "The smoke path defaults to the normal rtlib GOSUB implementation.",
        (
            (
                "src/rtlib/nuttx/fb_nuttx_gosub.c",
                "#if !defined(FB_NUTTX_USE_GENERIC_GOSUB)",
            ),
        ),
    ),
    GenericSubstitution(
        "object base helper",
        "src/rtlib/oop_object.c",
        "fb_nuttx_object.c",
        (
            'USE_GENERIC_OBJECT="${FB_NUTTX_USE_GENERIC_OBJECT:-1}"',
            'cp "$ROOT/src/rtlib/oop_object.c" "$APP_DIR/fb_nuttx_oop_object.c"',
            "CFLAGS += -DFB_NUTTX_USE_GENERIC_OBJECT=$USE_GENERIC_OBJECT",
        ),
        "Object construction and destruction stay aligned with normal rtlib.",
        (
            (
                "src/rtlib/nuttx/fb_nuttx_object.c",
                "#if !defined(FB_NUTTX_USE_GENERIC_OBJECT)",
            ),
        ),
    ),
    GenericSubstitution(
        "object type check helper",
        "src/rtlib/oop_istypeof.c",
        "fb_nuttx_oop_istypeof.c",
        (
            'USE_GENERIC_OBJECT="${FB_NUTTX_USE_GENERIC_OBJECT:-1}"',
            'cp "$ROOT/src/rtlib/oop_istypeof.c" "$APP_DIR/fb_nuttx_oop_istypeof.c"',
        ),
        "fb_IsTypeOf is built as a separate shared rtlib source.",
    ),
    GenericSubstitution(
        "FIX helper",
        "src/rtlib/math_fix.c",
        "fb_nuttx_math_fix.c",
        (
            'USE_GENERIC_MATH_FIX="${FB_NUTTX_USE_GENERIC_MATH_FIX:-1}"',
            'cp "$ROOT/src/rtlib/math_fix.c" "$APP_DIR/fb_nuttx_math_fix.c"',
            "CFLAGS += -DFB_NUTTX_USE_GENERIC_MATH_FIX=$USE_GENERIC_MATH_FIX",
        ),
        "The local duplicate FIX helper is removed from the staged shim.",
        (
            (
                "src/rtlib/nuttx/fb_nuttx_convert.c",
                "#if !defined(FB_NUTTX_USE_GENERIC_MATH_FIX)",
            ),
        ),
    ),
    GenericSubstitution(
        "MK/CV numeric helpers",
        "src/rtlib/math_cvn.c",
        "fb_nuttx_math_cvn.c",
        (
            'USE_GENERIC_MATH_CVN="${FB_NUTTX_USE_GENERIC_MATH_CVN:-1}"',
            'cp "$ROOT/src/rtlib/math_cvn.c" "$APP_DIR/fb_nuttx_math_cvn.c"',
            "CFLAGS += -DFB_NUTTX_USE_GENERIC_MATH_CVN=$USE_GENERIC_MATH_CVN",
        ),
        "The normal bit-conversion helpers are shared with the NuttX lab.",
        (
            (
                "src/rtlib/nuttx/fb_nuttx_convert.c",
                "#if !defined(FB_NUTTX_USE_GENERIC_MATH_CVN)",
            ),
        ),
    ),
    GenericSubstitution(
        "FRAC helper",
        "src/rtlib/math_frac.c",
        "fb_nuttx_math_frac.c",
        (
            'USE_GENERIC_MATH_FRAC="${FB_NUTTX_USE_GENERIC_MATH_FRAC:-1}"',
            'cp "$ROOT/src/rtlib/math_frac.c" "$APP_DIR/fb_nuttx_math_frac.c"',
            "CFLAGS += -DFB_NUTTX_USE_GENERIC_MATH_FRAC=$USE_GENERIC_MATH_FRAC",
            "could not find math_frac include marker",
        ),
        "The normal FRAC() helper is shared now that FIX() is generic.",
    ),
    GenericSubstitution(
        "integer LOG10 helpers",
        "src/rtlib/math_log10.c",
        "fb_nuttx_math_log10.c",
        (
            'USE_GENERIC_MATH_LOG10="${FB_NUTTX_USE_GENERIC_MATH_LOG10:-1}"',
            'cp "$ROOT/src/rtlib/math_log10.c" "$APP_DIR/fb_nuttx_math_log10.c"',
            "CFLAGS += -DFB_NUTTX_USE_GENERIC_MATH_LOG10=$USE_GENERIC_MATH_LOG10",
        ),
        "The normal integer base-10 helpers are shared with the NuttX lab.",
    ),
    GenericSubstitution(
        "RND/RANDOMIZE helpers",
        "src/rtlib/math_rnd.c",
        "fb_nuttx_math_rnd.c",
        (
            'USE_GENERIC_MATH_RND="${FB_NUTTX_USE_GENERIC_MATH_RND:-1}"',
            'cp "$ROOT/src/rtlib/math_rnd.c" "$APP_DIR/fb_nuttx_math_rnd.c"',
            "CFLAGS += -DFB_NUTTX_USE_GENERIC_MATH_RND=$USE_GENERIC_MATH_RND",
        ),
        "The normal PRNG implementation replaces the compact NuttX-only copy.",
        (
            (
                "src/rtlib/nuttx/fb_nuttx_convert.c",
                "#if !defined(FB_NUTTX_USE_GENERIC_MATH_RND)",
            ),
            (
                "src/rtlib/nuttx/fb_nuttx_minrt.c",
                "FB_RTLIB_CTX __fb_ctx",
            ),
        ),
    ),
    GenericSubstitution(
        "SGN helpers",
        "src/rtlib/math_sgn.c",
        "fb_nuttx_math_sgn.c",
        (
            'USE_GENERIC_MATH_SGN="${FB_NUTTX_USE_GENERIC_MATH_SGN:-1}"',
            'cp "$ROOT/src/rtlib/math_sgn.c" "$APP_DIR/fb_nuttx_math_sgn.c"',
            "CFLAGS += -DFB_NUTTX_USE_GENERIC_MATH_SGN=$USE_GENERIC_MATH_SGN",
        ),
        "The local duplicate SGN helpers are removed from the staged shim.",
        (
            (
                "src/rtlib/nuttx/fb_nuttx_math.c",
                "#if !defined(FB_NUTTX_USE_GENERIC_MATH_SGN)",
            ),
        ),
    ),
    GenericSubstitution(
        "date/time serial math helpers",
        "src/rtlib/time_core.c",
        "fb_nuttx_time_core.c",
        (
            'USE_GENERIC_DATETIME_MATH="${FB_NUTTX_USE_GENERIC_DATETIME_MATH:-1}"',
            'cp "$ROOT/src/rtlib/time_core.c" "$APP_DIR/fb_nuttx_time_core.c"',
            'cp "$ROOT/src/rtlib/time_dateadd.c" "$APP_DIR/fb_nuttx_time_dateadd.c"',
            'cp "$ROOT/src/rtlib/time_datediff.c" "$APP_DIR/fb_nuttx_time_datediff.c"',
            "#define FB_TIME_INTERVAL_INVALID        0",
            "CFLAGS += -DFB_NUTTX_USE_GENERIC_DATETIME_MATH=$USE_GENERIC_DATETIME_MATH",
        ),
        "The normal serial-date math handles DATEADD/DATEPART/DATEDIFF.",
        (
            (
                "src/rtlib/nuttx/fb_nuttx_datetime.c",
                "#if !defined(FB_NUTTX_USE_GENERIC_DATETIME_MATH)",
            ),
        ),
    ),
    GenericSubstitution(
        "date/time clock helpers",
        "src/rtlib/time_date.c",
        "fb_nuttx_time_date.c",
        (
            'USE_GENERIC_CLOCK="${FB_NUTTX_USE_GENERIC_CLOCK:-1}"',
            'cp "$ROOT/src/rtlib/time_date.c" "$APP_DIR/fb_nuttx_time_date.c"',
            'cp "$ROOT/src/rtlib/time_time.c" "$APP_DIR/fb_nuttx_time_time.c"',
            'cp "$ROOT/src/rtlib/unix/time_timer.c" "$APP_DIR/fb_nuttx_time_timer.c"',
            "#define FB_LOCK()",
            "CFLAGS += -DFB_NUTTX_USE_GENERIC_CLOCK=$USE_GENERIC_CLOCK",
        ),
        "The normal DATE$, TIME$, and TIMER helpers replace local copies.",
        (
            (
                "src/rtlib/nuttx/fb_nuttx_system.c",
                "#if !defined(FB_NUTTX_USE_GENERIC_CLOCK)",
            ),
        ),
    ),
    GenericSubstitution(
        "environment variable helpers",
        "src/rtlib/sys_environ.c",
        "fb_nuttx_sys_environ.c",
        (
            'USE_GENERIC_ENVIRON="${FB_NUTTX_USE_GENERIC_ENVIRON:-1}"',
            'cp "$ROOT/src/rtlib/sys_environ.c" "$APP_DIR/fb_nuttx_sys_environ.c"',
            "CFLAGS += -DFB_NUTTX_USE_GENERIC_ENVIRON=$USE_GENERIC_ENVIRON",
        ),
        "The normal POSIX ENVIRON/SETENVIRON helpers replace the local copy.",
        (
            (
                "src/rtlib/nuttx/fb_nuttx_env.c",
                "#if !defined(FB_NUTTX_USE_GENERIC_ENVIRON)",
            ),
        ),
    ),
    GenericSubstitution(
        "directory command helpers",
        "src/rtlib/sys_mkdir.c",
        "fb_nuttx_sys_mkdir.c",
        (
            'USE_GENERIC_DIR="${FB_NUTTX_USE_GENERIC_DIR:-1}"',
            'cp "$ROOT/src/rtlib/sys_mkdir.c" "$APP_DIR/fb_nuttx_sys_mkdir.c"',
            'cp "$ROOT/src/rtlib/sys_cdir.c" "$APP_DIR/fb_nuttx_sys_cdir.c"',
            "CFLAGS += -DFB_NUTTX_USE_GENERIC_DIR=$USE_GENERIC_DIR",
        ),
        "The normal POSIX MKDIR/RMDIR/CHDIR/CURDIR helpers replace the local copies.",
        (
            (
                "src/rtlib/nuttx/fb_nuttx_file.c",
                "#if !defined(FB_NUTTX_USE_GENERIC_DIR)",
            ),
        ),
    ),
    GenericSubstitution(
        "file copy helper",
        "src/rtlib/file_copy_crt.c",
        "fb_nuttx_file_copy_crt.c",
        (
            'USE_GENERIC_FILE_COPY="${FB_NUTTX_USE_GENERIC_FILE_COPY:-1}"',
            'cp "$ROOT/src/rtlib/file_copy_crt.c" '
            '"$APP_DIR/fb_nuttx_file_copy_crt.c"',
            "FILE *fb_hOpenFile(const char *path, const char *mode);",
            "CFLAGS += -DFB_NUTTX_USE_GENERIC_FILE_COPY=$USE_GENERIC_FILE_COPY",
        ),
        "The normal C stdio file-copy helper replaces the local copy loop.",
        (
            (
                "src/rtlib/nuttx/fb_nuttx_file.c",
                "#if !defined(FB_NUTTX_USE_GENERIC_FILE_COPY)",
            ),
            (
                "src/rtlib/nuttx/fb_nuttx_file.c",
                "FILE *fb_hOpenFile(const char *path, const char *mode)",
            ),
        ),
    ),
    GenericSubstitution(
        "base conversion string helpers",
        "src/rtlib/str_base.c",
        "fb_nuttx_str_base.c",
        (
            'USE_GENERIC_STR_BASE="${FB_NUTTX_USE_GENERIC_STR_BASE:-1}"',
            'cp "$ROOT/src/rtlib/str_base.c" "$APP_DIR/fb_nuttx_str_base.c"',
            'cp "$ROOT/src/rtlib/str_hex_lng.c" "$APP_DIR/fb_nuttx_str_hex_lng.c"',
            'cp "$ROOT/src/rtlib/str_bin_ptr.c" "$APP_DIR/fb_nuttx_str_bin_ptr.c"',
            "CFLAGS += -DFB_NUTTX_USE_GENERIC_STR_BASE=$USE_GENERIC_STR_BASE",
        ),
        "The normal HEX/OCT/BIN helpers replace the local base-conversion copy.",
        (
            (
                "src/rtlib/nuttx/fb_nuttx_convert.c",
                "#if !defined(FB_NUTTX_USE_GENERIC_STR_BASE)",
            ),
        ),
    ),
    GenericSubstitution(
        "string fill helpers",
        "src/rtlib/str_fill.c",
        "fb_nuttx_str_fill.c",
        (
            'USE_GENERIC_STR_FILL="${FB_NUTTX_USE_GENERIC_STR_FILL:-1}"',
            'cp "$ROOT/src/rtlib/str_misc.c" "$APP_DIR/fb_nuttx_str_misc.c"',
            'cp "$ROOT/src/rtlib/str_fill.c" "$APP_DIR/fb_nuttx_str_fill.c"',
            "CFLAGS += -DFB_NUTTX_USE_GENERIC_STR_FILL=$USE_GENERIC_STR_FILL",
        ),
        "The normal SPACE/STRING fill helpers replace the local copies.",
        (
            (
                "src/rtlib/nuttx/fb_nuttx_string.c",
                "#if !defined(FB_NUTTX_USE_GENERIC_STR_FILL)",
            ),
            (
                "src/rtlib/nuttx/fb_nuttx_convert.c",
                "#if !defined(FB_NUTTX_USE_GENERIC_STR_FILL)",
            ),
        ),
    ),
    GenericSubstitution(
        "string extra helpers",
        "src/rtlib/str_left.c",
        "fb_nuttx_str_left.c",
        (
            'USE_GENERIC_STR_EXTRA="${FB_NUTTX_USE_GENERIC_STR_EXTRA:-1}"',
            'cp "$ROOT/src/rtlib/str_hskip.c" "$APP_DIR/fb_nuttx_str_hskip.c"',
            'cp "$ROOT/src/rtlib/str_left.c" "$APP_DIR/fb_nuttx_str_left.c"',
            'cp "$ROOT/src/rtlib/str_trim.c" "$APP_DIR/fb_nuttx_str_trim.c"',
            "CFLAGS += -DFB_NUTTX_USE_GENERIC_STR_EXTRA=$USE_GENERIC_STR_EXTRA",
        ),
        "The normal LEFT/RIGHT/MID/INSTR/TRIM/LCASE/UCASE helpers replace the local copies.",
        (
            (
                "src/rtlib/nuttx/fb_nuttx_string_extra.c",
                "#if !defined(FB_NUTTX_USE_GENERIC_STR_EXTRA)",
            ),
        ),
    ),
    GenericSubstitution(
        "WSTRING operator helpers",
        "src/rtlib/strw_assign.c",
        "fb_nuttx_strw_assign.c",
        (
            'USE_GENERIC_WSTRING="${FB_NUTTX_USE_GENERIC_WSTRING:-1}"',
            'cp "$ROOT/src/rtlib/strw_assign.c" "$APP_DIR/fb_nuttx_strw_assign.c"',
            'cp "$ROOT/src/rtlib/strw_convconcat.c" "$APP_DIR/fb_nuttx_strw_convconcat.c"',
            'cp "$ROOT/src/rtlib/strw_hex_lng.c" "$APP_DIR/fb_nuttx_strw_hex_lng.c"',
            "CFLAGS += -DFB_NUTTX_USE_GENERIC_WSTRING=$USE_GENERIC_WSTRING",
        ),
        "The normal WSTRING assign/concat/case/fill/base helpers replace the local copies.",
        (
            (
                "src/rtlib/nuttx/fb_nuttx_wstring.c",
                "#if !defined(FB_NUTTX_USE_GENERIC_WSTRING)",
            ),
        ),
    ),
    GenericSubstitution(
        "assertion helpers",
        "src/rtlib/error_assert.c",
        "fb_nuttx_error_assert.c",
        (
            'USE_GENERIC_ASSERT="${FB_NUTTX_USE_GENERIC_ASSERT:-1}"',
            'cp "$ROOT/src/rtlib/error_assert.c" "$APP_DIR/fb_nuttx_error_assert.c"',
            'cp "$ROOT/src/rtlib/error_assert_wstr.c" \\',
            '"$APP_DIR/fb_nuttx_error_assert_wstr.c"',
            "CFLAGS += -DFB_NUTTX_USE_GENERIC_ASSERT=$USE_GENERIC_ASSERT",
        ),
        "The normal ASSERT and ASSERTWARN helpers replace the local copies.",
        (
            (
                "src/rtlib/nuttx/fb_nuttx_error.c",
                "#if !defined(FB_NUTTX_USE_GENERIC_ASSERT)",
            ),
            (
                "src/rtlib/nuttx/fb_nuttx_system.c",
                "__fb_ctx.errmsg",
            ),
        ),
    ),
)


NUTTX_SPECIFIC_SHIMS: tuple[NuttXShim, ...] = (
    NuttXShim(
        "DATA/READ/RESTORE",
        "src/rtlib/nuttx/fb_nuttx_data.c",
        "Generated C currently uses the compact FB_NUTTX_DATA_DESC layout.",
    ),
    NuttXShim(
        "dynamic arrays",
        "src/rtlib/nuttx/fb_nuttx_array.c",
        "Generated C currently uses FB_NUTTX_ARRAY instead of FBARRAY.",
    ),
    NuttXShim(
        "runtime error state",
        "src/rtlib/nuttx/fb_nuttx_error.c",
        "The mini runtime keeps one small error slot instead of FB_ERRORCTX.",
    ),
    NuttXShim(
        "file and device I/O",
        "src/rtlib/nuttx/fb_nuttx_file.c",
        "The target maps BASIC file numbers directly to NuttX stdio/socket use.",
    ),
    NuttXShim(
        "console and input",
        "src/rtlib/nuttx/fb_nuttx_console.c",
        "The target has serial, NSH, and future HDMI console paths to reconcile.",
    ),
    NuttXShim(
        "time/date platform edges",
        "src/rtlib/nuttx/fb_nuttx_datetime.c",
        "SET DATE/TIME and board-aware parsing edges stay platform-specific.",
    ),
    NuttXShim(
        "system commands",
        "src/rtlib/nuttx/fb_nuttx_system.c",
        "CHAIN, EXEC, RUN, and SHELL must stay honest about NuttX support.",
    ),
    NuttXShim(
        "thread bridge",
        "src/rtlib/nuttx/fb_nuttx_thread.c",
        "The current smoke path supports the generated-C threadcall ABI subset.",
    ),
)


def find_project_root(start: Path) -> Path:
    search = start.resolve()

    while True:
        if (search / "build_scripts").is_dir() and (
            (search / "GNUmakefile").is_file()
            or (search / "makefile").is_file()
            or (search / "Makefile").is_file()
        ):
            return search

        if search.parent == search:
            raise RuntimeError("could not locate FreeBASIC project root")

        search = search.parent


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def audit_generic_substitution(
    root: Path, smoke_script_text: str, substitution: GenericSubstitution
) -> list[str]:
    failures: list[str] = []

    if not (root / substitution.generic_source).is_file():
        failures.append(f"missing generic source: {substitution.generic_source}")

    for marker in substitution.smoke_script_markers:
        if marker not in smoke_script_text:
            failures.append(
                f"{substitution.name}: smoke script missing marker: {marker}"
            )

    for rel_path, marker in substitution.source_markers:
        source_path = root / rel_path

        if not source_path.is_file():
            failures.append(f"{substitution.name}: missing source: {rel_path}")
            continue

        if marker not in read_text(source_path):
            failures.append(
                f"{substitution.name}: {rel_path} missing marker: {marker}"
            )

    return failures


def audit_nuttx_shim(root: Path, shim: NuttXShim) -> list[str]:
    if not (root / shim.source).is_file():
        return [f"missing NuttX shim: {shim.source}"]

    return []


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Audit the NuttX/RISC-V mini-runtime staging rules."
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=None,
        help="FreeBASIC source root. Default: search upward from cwd.",
    )

    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    root = args.root.resolve() if args.root is not None else find_project_root(Path.cwd())
    smoke_script = root / "build_scripts" / "nuttx-riscv32-qemu-smoke.sh"
    failures: list[str] = []

    if not smoke_script.is_file():
        print(f"nuttx-rtlib-audit: missing smoke script: {smoke_script}", file=sys.stderr)
        return 2

    smoke_script_text = read_text(smoke_script)

    print(f"nuttx-rtlib-audit: root {root}")

    for substitution in GENERIC_SUBSTITUTIONS:
        missing = audit_generic_substitution(root, smoke_script_text, substitution)

        if missing:
            failures.extend(missing)
            print(f"nuttx-rtlib-audit: FAIL generic {substitution.name}")
        else:
            print(f"nuttx-rtlib-audit: PASS generic {substitution.name}")
            print(f"  {substitution.note}")

    print("nuttx-rtlib-audit: NuttX-specific shims still expected")

    for shim in NUTTX_SPECIFIC_SHIMS:
        missing = audit_nuttx_shim(root, shim)

        if missing:
            failures.extend(missing)
            print(f"nuttx-rtlib-audit: FAIL shim {shim.name}")
        else:
            print(f"nuttx-rtlib-audit: KEEP shim {shim.name}")
            print(f"  {shim.reason}")

    if failures:
        print("nuttx-rtlib-audit: failures", file=sys.stderr)

        for failure in failures:
            print(f"  - {failure}", file=sys.stderr)

        return 1

    print("nuttx-rtlib-audit: all runtime staging checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
