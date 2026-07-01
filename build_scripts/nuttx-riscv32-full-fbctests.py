#!/usr/bin/env python3
#
#   Project: FreeBASIC NuttX/RISC-V testing
#   ---------------------------------------
#
#   File: nuttx-riscv32-full-fbctests.py
#
#   Purpose:
#
#       Drive the official fbctests log-test inventory against the
#       NuttX RISC-V target.
#
#   Responsibilities:
#
#       * parse tests/log-tests-fb.inc
#       * run compile-only success and failure tests with the NuttX target
#       * generate C sources for tests that must run under QEMU/NuttX
#       * stage single-module and representable multi-module runtime tests
#       * write manifests for nuttx-riscv32-qemu-suite.sh
#       * write plain CSV reports describing what passed or still needs work
#
#   This file intentionally does NOT contain:
#
#       * QEMU execution logic
#       * NuttX app-tree mutation
#       * target runtime implementations
#       * broad policy decisions about unsupported external libraries
#

from __future__ import annotations

import argparse
import csv
import hashlib
import os
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


LOG_TEST_RE = re.compile(r"^(SRCLIST_[A-Z_]+)\s*\+=\s*(.+?)\s*$")
MAKE_ASSIGN_RE = re.compile(r"^([A-Za-z0-9_]+)\s*:?=\s*(.*?)\s*$")

FBCUNIT_SUPPORT = (
    "fbcunit",
    "fbcunit_qb",
    "fbcunit_console",
    "fbcunit_report",
)


@dataclass(frozen=True)
class TestEntry:
    category: str
    source: str


@dataclass(frozen=True)
class BmkInfo:
    main: str
    sources: tuple[str, ...]
    extra_objects: tuple[str, ...]
    compile_and_link: bool


def die(message: str) -> None:
    print(f"nuttx-full-fbctests: {message}", file=sys.stderr)
    raise SystemExit(1)


def find_repo_root(start: Path) -> Path:
    current = start.resolve()

    if current.is_file():
        current = current.parent

    while True:
        if (current / "build_scripts").is_dir() and (current / "tests" / "log-tests-fb.inc").is_file():
            return current

        if current.parent == current:
            die(f"could not find FreeBASIC repo root from {start}")

        current = current.parent


def default_fbc(repo: Path) -> Path:
    if os.name == "nt":
        return repo / "bin" / "fbc.exe"

    host_fbc = repo / "bin" / "fbc"

    if host_fbc.exists():
        return host_fbc

    return repo / "bin" / "fbc.exe"


def rel_from_tests(source: str) -> str:
    return source[2:] if source.startswith("./") else source


def read_log_tests(path: Path) -> dict[str, list[TestEntry]]:
    groups: dict[str, list[TestEntry]] = {}

    with path.open("r", encoding="utf-8", newline="") as f:
        for line in f:
            match = LOG_TEST_RE.match(line)
            if not match:
                continue

            category = match.group(1)
            source = match.group(2)
            groups.setdefault(category, []).append(TestEntry(category, source))

    return groups


def app_name_for(source: str) -> str:
    rel = rel_from_tests(source)
    stem = Path(rel).with_suffix("").as_posix()
    stem = re.sub(r"[^A-Za-z0-9_]+", "_", stem).strip("_").lower()
    digest = hashlib.sha1(rel.encode("utf-8")).hexdigest()[:8]
    name = f"fb_{stem[:34]}_{digest}"

    if not re.match(r"^[A-Za-z_]", name):
        name = f"fb_{name}"

    return name


def read_bmk(path: Path) -> BmkInfo:
    values: dict[str, str] = {}
    logical_lines: list[str] = []
    pending = ""

    with path.open("r", encoding="utf-8", newline="") as f:
        for raw_line in f:
            line = raw_line.rstrip("\r\n")

            if line.rstrip().endswith("\\"):
                pending += line.rstrip()[:-1] + " "
                continue

            line = pending + line
            pending = ""
            logical_lines.append(line)

    if pending:
        logical_lines.append(pending)

    for line in logical_lines:
        stripped = line.strip()

        if not stripped or stripped.startswith("#"):
            continue

        match = MAKE_ASSIGN_RE.match(stripped)
        if match:
            values[match.group(1)] = match.group(2).strip()

    main = values.get("MAIN", "")
    if not main:
        raise ValueError("missing MAIN assignment")

    sources = tuple(item for item in values.get("SRCS", "").split() if item)
    extra_objects = tuple(item for item in values.get("EXTRA_OBJS", "").split() if item)
    compile_and_link = values.get("COMPILE_AND_LINK", "").strip() == "1"

    return BmkInfo(main, sources, extra_objects, compile_and_link)


def generated_c_for(source: Path) -> Path:
    return source.with_suffix(".c")


def preserve_generated(path: Path) -> bytes | None:
    if path.exists():
        return path.read_bytes()

    return None


def restore_generated(path: Path, original: bytes | None) -> None:
    if original is None:
        try:
            path.unlink()
        except FileNotFoundError:
            pass
        return

    path.write_bytes(original)


def fbc_command(
    args: argparse.Namespace,
    source: Path,
    main_module: str | None = None,
    runtime_test: bool = False,
) -> list[str]:
    command = [
        str(args.fbc),
        "-target",
        args.target,
        "-i",
        str(args.repo / "inc"),
        "-i",
        str(args.repo / "tests" / "fbcunit" / "inc"),
        "-gen",
        "gcc",
        "-r",
    ]

    if runtime_test:
        command.extend(["-g", "-w", "0"])

    if main_module:
        command.extend(["-m", main_module])

    command.append(str(source))
    return command


def run_fbc(
    args: argparse.Namespace,
    source: Path,
    main_module: str | None = None,
    runtime_test: bool = False,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        fbc_command(args, source, main_module, runtime_test),
        cwd=args.repo,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=args.timeout,
    )


def write_csv(path: Path, rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)

    fieldnames = ["category", "source", "status", "detail"]

    with path.open("w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()

        for row in rows:
            writer.writerow(row)


def record(rows: list[dict[str, str]], category: str, source: str, status: str, detail: str = "") -> None:
    rows.append(
        {
            "category": category,
            "source": source,
            "status": status,
            "detail": detail.replace("\r", " ").replace("\n", " ")[:1000],
        }
    )


def compile_only(
    args: argparse.Namespace,
    entry: TestEntry,
    expect_success: bool,
    rows: list[dict[str, str]],
    compile_manifest_lines: list[str],
) -> bool:
    source = args.repo / "tests" / rel_from_tests(entry.source)

    if not source.exists():
        record(rows, entry.category, entry.source, "FAIL", f"missing source: {source}")
        return False

    generated = generated_c_for(source)
    original = preserve_generated(generated)

    try:
        result = run_fbc(args, source)
        success = result.returncode == 0
        passed = success == expect_success

        if not success and not expect_success:
            record(rows, entry.category, entry.source, "PASS")
            return True

        if not success and expect_success:
            record(rows, entry.category, entry.source, "FAIL", f"expected success, got failure: {result.stdout}")
            return False

        if not generated.exists():
            record(rows, entry.category, entry.source, "FAIL", f"generated C not found: {generated}")
            return False

        app_name = app_name_for(entry.source)
        staged_name = f"compile/{app_name}.c"
        staged_path = args.out_dir / staged_name
        staged_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(generated, staged_path)

        if expect_success:
            compile_manifest_lines.append(f"ok:{staged_name}:{entry.source}")
            record(rows, entry.category, entry.source, "STAGED_COMPILE_OK")
        else:
            compile_manifest_lines.append(f"fail:{staged_name}:{entry.source}")
            record(rows, entry.category, entry.source, "STAGED_COMPILE_FAIL")

        return True
    finally:
        restore_generated(generated, original)


def stage_one_source(
    args: argparse.Namespace,
    source: Path,
    staged_name: str,
    main_module: str | None,
) -> tuple[bool, str]:
    if not source.exists():
        return False, f"missing source: {source}"

    generated = generated_c_for(source)
    original = preserve_generated(generated)

    try:
        result = run_fbc(args, source, main_module, True)

        if result.returncode != 0:
            return False, result.stdout

        if not generated.exists():
            return False, f"generated C not found: {generated}"

        shutil.copyfile(generated, args.out_dir / staged_name)
        return True, ""
    finally:
        restore_generated(generated, original)


def stage_fbcunit_support(args: argparse.Namespace) -> bool:
    src_dir = args.repo / "tests" / "fbcunit" / "src"
    include_dir = args.repo / "tests" / "fbcunit" / "inc"

    for name in FBCUNIT_SUPPORT:
        source = src_dir / f"{name}.bas"
        generated = src_dir / f"{name}.c"
        original = preserve_generated(generated)

        try:
            command = [
                str(args.fbc),
                "-target",
                args.target,
                "-d",
                "FBCU_NO_INCLIB",
                "-i",
                str(args.repo / "inc"),
                "-i",
                str(include_dir),
                "-i",
                str(src_dir),
                "-gen",
                "gcc",
                "-r",
                str(source),
            ]
            result = subprocess.run(
                command,
                cwd=args.repo,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=args.timeout,
            )

            if result.returncode != 0:
                print(result.stdout, file=sys.stderr)
                return False

            if not generated.exists():
                print(f"missing generated fbcunit support: {generated}", file=sys.stderr)
                return False

            shutil.copyfile(generated, args.out_dir / f"{name}.c")
        finally:
            restore_generated(generated, original)

    return True


def manifest_mode(source: str, expect_fail: bool) -> str:
    rel = rel_from_tests(source)
    with_gfx = rel.startswith("gfx/") or rel == "command-sweep/gfxlib-command-sweep.bas"

    if with_gfx and expect_fail:
        return "gfx-runfail"

    if with_gfx:
        return "gfx"

    if expect_fail:
        return "runfail"

    return ""


def stage_runtime_bas(
    args: argparse.Namespace,
    entry: TestEntry,
    rows: list[dict[str, str]],
    manifest_lines: list[str],
    expect_fail: bool,
) -> bool:
    source = args.repo / "tests" / rel_from_tests(entry.source)
    app_name = app_name_for(entry.source)
    staged_name = f"{app_name}.c"
    main_module = str(source.with_suffix(""))

    ok, detail = stage_one_source(args, source, staged_name, main_module)

    if not ok:
        record(rows, entry.category, entry.source, "FAIL", detail)
        return False

    text = (args.out_dir / staged_name).read_text(encoding="utf-8", errors="ignore")
    mode = manifest_mode(entry.source, expect_fail)

    if "_ZN4FBCU" in text:
        args.need_fbcunit = True

    if mode:
        manifest_lines.append(f"{app_name}:{staged_name}:{mode}")
    else:
        manifest_lines.append(f"{app_name}:{staged_name}")

    record(rows, entry.category, entry.source, "STAGED")
    return True


def stage_runtime_bmk(
    args: argparse.Namespace,
    entry: TestEntry,
    rows: list[dict[str, str]],
    manifest_lines: list[str],
    expect_fail: bool,
) -> bool:
    bmk_path = args.repo / "tests" / rel_from_tests(entry.source)

    try:
        info = read_bmk(bmk_path)
    except ValueError as exc:
        record(rows, entry.category, entry.source, "UNSUPPORTED", str(exc))
        return False

    test_dir = bmk_path.parent
    main_source = test_dir / info.main
    module_sources = [main_source] + [test_dir / item for item in info.sources]

    for source in module_sources:
        if source.suffix.lower() != ".bas":
            record(rows, entry.category, entry.source, "UNSUPPORTED", f"non-BASIC source: {source.name}")
            return False

        if not source.exists():
            record(rows, entry.category, entry.source, "FAIL", f"missing source: {source}")
            return False

    app_name = app_name_for(entry.source)
    staged_names: list[str] = []
    main_module = str(main_source.with_suffix(""))

    for index, source in enumerate(module_sources):
        staged_name = f"{app_name}_{index}.c"
        ok, detail = stage_one_source(args, source, staged_name, main_module)

        if not ok:
            record(rows, entry.category, entry.source, "FAIL", detail)
            return False

        staged_names.append(staged_name)

    combined_text = ""

    for staged_name in staged_names:
        combined_text += (args.out_dir / staged_name).read_text(encoding="utf-8", errors="ignore")

    if "_ZN4FBCU" in combined_text:
        args.need_fbcunit = True

    mode = manifest_mode(entry.source, expect_fail)
    extra_specs: list[str] = []

    for index, extra_object in enumerate(info.extra_objects):
        ok, spec_or_detail = stage_extra_object_source(args, test_dir, app_name, index, extra_object)

        if not ok:
            record(rows, entry.category, entry.source, "UNSUPPORTED", spec_or_detail)
            return False

        extra_specs.append(spec_or_detail)

    c_spec = "+".join(staged_names + extra_specs)

    if mode:
        manifest_lines.append(f"{app_name}:{c_spec}:{mode}")
    else:
        manifest_lines.append(f"{app_name}:{c_spec}")

    record(rows, entry.category, entry.source, "STAGED")
    return True


def stage_extra_object_source(
    args: argparse.Namespace,
    test_dir: Path,
    app_name: str,
    index: int,
    extra_object: str,
) -> tuple[bool, str]:
    object_stem = Path(extra_object).with_suffix("").name

    candidates = [
        (test_dir / f"{object_stem}.c", "c"),
        (test_dir / f"{object_stem}.cpp", "cxx"),
        (test_dir / f"{object_stem}.cxx", "cxx"),
        (test_dir / f"{object_stem}.cc", "cxx"),
        (test_dir / f"{object_stem}.S", "asm"),
        (test_dir / f"{object_stem}.s", "asm"),
    ]

    for source, kind in candidates:
        if not source.exists():
            continue

        staged_name = f"{app_name}_extra_{index}{source.suffix}"
        shutil.copyfile(source, args.out_dir / staged_name)
        return True, f"{kind}:{staged_name}"

    return False, f"could not find source for extra object {extra_object}"


def run(args: argparse.Namespace) -> int:
    groups = read_log_tests(args.repo / "tests" / "log-tests-fb.inc")
    rows: list[dict[str, str]] = []
    manifest_lines: list[str] = []
    compile_manifest_lines: list[str] = []

    args.out_dir.mkdir(parents=True, exist_ok=True)
    args.report_dir.mkdir(parents=True, exist_ok=True)
    args.need_fbcunit = False

    for entry in groups.get("SRCLIST_COMPILE_ONLY_OK", []):
        compile_only(args, entry, True, rows, compile_manifest_lines)

    for entry in groups.get("SRCLIST_COMPILE_ONLY_FAIL", []):
        compile_only(args, entry, False, rows, compile_manifest_lines)

    for entry in groups.get("SRCLIST_COMPILE_AND_RUN_OK", []):
        stage_runtime_bas(args, entry, rows, manifest_lines, False)

    for entry in groups.get("SRCLIST_COMPILE_AND_RUN_FAIL", []):
        stage_runtime_bas(args, entry, rows, manifest_lines, True)

    for entry in groups.get("SRCLIST_MULTI_MODULE_OK", []):
        stage_runtime_bmk(args, entry, rows, manifest_lines, False)

    for entry in groups.get("SRCLIST_MULTI_MODULE_FAIL", []):
        stage_runtime_bmk(args, entry, rows, manifest_lines, True)

    if args.need_fbcunit and not stage_fbcunit_support(args):
        record(rows, "FBCUNIT_SUPPORT", "tests/fbcunit/src", "FAIL", "could not stage fbcunit support")

    manifest_path = args.out_dir / "manifest.txt"
    manifest_path.write_text("\n".join(manifest_lines) + "\n", encoding="utf-8", newline="\n")

    compile_manifest_path = args.out_dir / "compile-manifest.txt"
    compile_manifest_path.write_text("\n".join(compile_manifest_lines) + "\n", encoding="utf-8", newline="\n")

    report_path = args.report_dir / "nuttx-full-fbctests-log.csv"
    write_csv(report_path, rows)

    failed = sum(1 for row in rows if row["status"] == "FAIL")
    unsupported = sum(1 for row in rows if row["status"] == "UNSUPPORTED")
    staged = sum(1 for row in rows if row["status"] == "STAGED")
    staged_compile = sum(1 for row in rows if row["status"].startswith("STAGED_COMPILE"))
    passed = sum(1 for row in rows if row["status"] == "PASS")

    print(f"nuttx-full-fbctests: fbc-only pass rows {passed}")
    print(f"nuttx-full-fbctests: staged compile rows {staged_compile}")
    print(f"nuttx-full-fbctests: staged runtime rows {staged}")
    print(f"nuttx-full-fbctests: unsupported rows {unsupported}")
    print(f"nuttx-full-fbctests: failed rows {failed}")
    print(f"nuttx-full-fbctests: manifest {manifest_path}")
    print(f"nuttx-full-fbctests: compile manifest {compile_manifest_path}")
    print(f"nuttx-full-fbctests: report {report_path}")

    return 1 if failed or unsupported else 0


def main() -> int:
    repo = find_repo_root(Path(__file__))

    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", type=Path, default=repo)
    parser.add_argument("--fbc", type=Path, default=default_fbc(repo))
    parser.add_argument("--target", default="nuttx-riscv32")
    parser.add_argument("--out-dir", type=Path)
    parser.add_argument("--report-dir", type=Path)
    parser.add_argument("--timeout", type=int, default=30)
    args = parser.parse_args()

    args.repo = find_repo_root(args.repo)
    args.fbc = args.fbc.resolve()

    if args.out_dir is None:
        args.out_dir = args.repo / "generated" / "fbctests-nuttx-full-log"

    if args.report_dir is None:
        args.report_dir = args.repo / ".build-nuttx-full-fbctests"

    args.out_dir = args.out_dir.resolve()
    args.report_dir = args.report_dir.resolve()

    if not args.fbc.exists():
        die(f"missing fbc: {args.fbc}")

    return run(args)


if __name__ == "__main__":
    raise SystemExit(main())

# end of nuttx-riscv32-full-fbctests.py
