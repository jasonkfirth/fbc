#!/usr/bin/env python3
#
# FreeBASIC Exampleageddon CI diagnostics
# ---------------------------------------
#
# File: show-exampleageddon-failures.py
#
# Purpose:
#
#     Print the bounded evidence needed to diagnose a failed Exampleageddon
#     qualification directly in a continuous-integration job log.
#
# Responsibilities:
#
#     * read Exampleageddon's CSV inventories and Markdown reports
#     * identify failing self-contained examples
#     * print the relevant compiler or guest-run log tails
#
# This file intentionally does NOT contain:
#
#     * example classification policy
#     * compilation or guest-emulator orchestration
#     * report generation or mutation
#

from __future__ import annotations

import csv
import sys
from pathlib import Path


MAX_PROBLEMS = 20
MAX_LOG_LINES = 80
MAX_REPORT_LINES = 180


def print_file_tail(path: Path, line_count: int) -> None:
    try:
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError as error:
        print(f"  could not read {path}: {error}")
        return

    for line in lines[-line_count:]:
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


def print_problem_logs(
    output_root: Path,
    rows: list[dict[str, str]],
    status_field: str,
    log_field: str,
) -> None:
    if not rows:
        return

    print()
    print("==> Failing self-contained Exampleageddon cases")
    for row in rows[:MAX_PROBLEMS]:
        path = row.get("path", "<unknown source>")
        status = row.get(status_field, "<unknown status>")
        reason = row.get("reason", "")
        log_name = row.get(log_field, "")
        log_path = output_root / log_name if log_name else None

        print()
        print(f"{path}: {status}")
        if reason:
            print(f"  classification: {reason}")
        if log_path is None:
            print("  no log path was recorded")
            continue

        print(f"  log: {log_name}")
        print_file_tail(log_path, MAX_LOG_LINES)

    if len(rows) > MAX_PROBLEMS:
        print(f"... {len(rows) - MAX_PROBLEMS} additional problems were omitted")


def main(argv: list[str]) -> int:
    if len(argv) != 1:
        print(f"Usage: {Path(sys.argv[0]).name} OUTPUT_DIRECTORY", file=sys.stderr)
        return 2

    output_root = Path(argv[0])
    print(f"==> Exampleageddon failure evidence: {output_root}")

    print_report(output_root / "report.md")
    print_report(output_root / "aros-report.md")
    print_report(output_root / "execution-report.md")

    compile_rows = [
        row
        for row in read_rows(output_root / "results.csv")
        if row.get("group") == "self-contained"
        and row.get("compile_status") != "pass"
    ]
    print_problem_logs(output_root, compile_rows, "compile_status", "compile_log")

    run_rows = [
        row
        for row in read_rows(output_root / "aros-results.csv")
        if row.get("group") == "self-contained"
        and row.get("run_status") != "pass"
    ]
    print_problem_logs(output_root, run_rows, "run_status", "run_log")

    if not compile_rows and not run_rows:
        print("No failing self-contained rows were found in the available CSV evidence.")

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))


# end of show-exampleageddon-failures.py
