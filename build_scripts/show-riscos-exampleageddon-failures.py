#!/usr/bin/env python3

##############################################################################
# FreeBASIC RISC OS Exampleageddon CI diagnostics
##############################################################################
#
# File: show-riscos-exampleageddon-failures.py
#
# Purpose:
#
#     Print the bounded report and guest logs needed to diagnose a failed
#     RISC OS Exampleageddon qualification directly in a CI job log.
#
# Responsibilities:
#
#     * print the cross-compile and RISC OS execution reports
#     * identify failing self-contained examples from the merged CSV
#     * print each relevant compiler or guest batch log once
#
# This file intentionally does NOT contain:
#
#     * Exampleageddon classification policy
#     * compilation or emulator orchestration
#     * report generation or mutation
#
##############################################################################

from __future__ import annotations

import csv
import sys
from pathlib import Path


MAX_REPORT_LINES = 180
MAX_PROBLEMS = 20
MAX_LOG_LINES = 80


def print_report(path: Path) -> None:
    if not path.is_file():
        return

    print()
    print(f"==> {path}")
    try:
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError as error:
        print(f"Could not read report: {error}")
        return

    for line in lines[:MAX_REPORT_LINES]:
        print(line)
    if len(lines) > MAX_REPORT_LINES:
        print(f"... report truncated after {MAX_REPORT_LINES} lines")


def print_log(path: Path) -> None:
    try:
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError as error:
        print(f"  could not read {path}: {error}")
        return

    for line in lines[-MAX_LOG_LINES:]:
        print(f"  {line}")


def read_rows(path: Path) -> list[dict[str, str]]:
    if not path.is_file():
        return []

    try:
        with path.open(newline="", encoding="utf-8") as stream:
            return list(csv.DictReader(stream))
    except (OSError, csv.Error) as error:
        print(f"Could not read {path}: {error}")
        return []


def print_problems(output_root: Path, rows: list[dict[str, str]]) -> None:
    problems = [
        row
        for row in rows
        if row.get("group") == "self-contained"
        and (
            row.get("compile_status") != "pass"
            or row.get("run_status") in {"fail", "timeout", "missing"}
        )
    ]
    if not problems:
        print("No failing self-contained rows were found in the available CSV evidence.")
        return

    print()
    print("==> Failing self-contained RISC OS Exampleageddon cases")
    shown_logs: set[str] = set()
    for row in problems[:MAX_PROBLEMS]:
        source = row.get("path", "<unknown source>")
        compile_status = row.get("compile_status", "<unknown>")
        run_status = row.get("run_status", "<unknown>")
        batch = row.get("batch", "")
        return_code = row.get("return_code", "")
        print(
            f"{source}: compile={compile_status}, run={run_status}, "
            f"batch={batch or '<none>'}, return_code={return_code or '<none>'}"
        )

        if compile_status != "pass":
            log_name = row.get("compile_log", "")
        else:
            log_name = row.get("run_log", "")
        if not log_name or log_name in shown_logs:
            continue

        shown_logs.add(log_name)
        log_path = output_root / log_name
        print(f"  log: {log_name}")
        print_log(log_path)

    if len(problems) > MAX_PROBLEMS:
        print(f"... {len(problems) - MAX_PROBLEMS} additional problems were omitted")


def main(argv: list[str]) -> int:
    if len(argv) != 1:
        print(
            f"Usage: {Path(sys.argv[0]).name} OUTPUT_DIRECTORY",
            file=sys.stderr,
        )
        return 2

    output_root = Path(argv[0])
    print(f"==> RISC OS Exampleageddon failure evidence: {output_root}")
    print_report(output_root / "report.md")
    print_report(output_root / "riscos-report.md")
    rows = read_rows(output_root / "riscos-results.csv")
    if not rows:
        rows = read_rows(output_root / "results.csv")
    print_problems(output_root, rows)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

##############################################################################
# end of show-riscos-exampleageddon-failures.py
##############################################################################
