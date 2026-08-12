#!/usr/bin/env python3

##############################################################################
# FreeBASIC RISC OS Exampleageddon reporting
##############################################################################
#
# File: riscos-exampleageddon-report.py
#
# Purpose:
#
#   Merge the portable Exampleageddon compile inventory with execution results
#   captured from RISC OS, then write machine-readable and reviewable reports.
#
# Responsibilities:
#
#   * preserve every compile/classification result from Exampleageddon
#   * attach RPCEmu batch, return-code, and log data to runnable examples
#   * identify missing, failed, and timed-out self-contained executions
#   * write a RISC OS CSV and Markdown acceptance report
#   * return non-zero when the self-contained RISC OS policy is not satisfied
#
# This file intentionally does NOT contain:
#
#   * example discovery or classification policy
#   * FreeBASIC compilation
#   * RISC OS filesystem staging
#   * RPCEmu process control
#
##############################################################################

from __future__ import annotations

import argparse
import csv
import sys
from collections import Counter
from pathlib import Path


OUTPUT_FIELDS = (
    "path",
    "group",
    "reason",
    "compile_status",
    "run_status",
    "compile_seconds",
    "run_seconds",
    "output",
    "compile_log",
    "run_log",
    "batch",
    "return_code",
)


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def read_run_status(path: Path) -> dict[str, dict[str, str]]:
    if not path.is_file():
        return {}

    with path.open(newline="", encoding="utf-8") as stream:
        rows = csv.DictReader(stream, delimiter="\t")
        return {row["path"]: row for row in rows}


def merge_results(
    compile_rows: list[dict[str, str]],
    run_rows: dict[str, dict[str, str]],
) -> list[dict[str, str]]:
    merged = []

    for source_row in compile_rows:
        row = dict(source_row)
        row["batch"] = ""
        row["return_code"] = ""

        if row["group"] == "self-contained" and row["compile_status"] == "pass":
            run_row = run_rows.get(row["path"])
            if run_row is None:
                row["run_status"] = "missing"
            else:
                row["run_status"] = run_row["run_status"]
                row["run_seconds"] = run_row.get("run_seconds", "")
                row["run_log"] = run_row["run_log"]
                row["batch"] = run_row["batch"]
                row["return_code"] = run_row["return_code"]

        merged.append(row)

    return merged


def write_csv(rows: list[dict[str, str]], path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=OUTPUT_FIELDS)
        writer.writeheader()
        writer.writerows({field: row.get(field, "") for field in OUTPUT_FIELDS} for row in rows)


def markdown_problem_rows(rows: list[dict[str, str]]) -> list[str]:
    result = []
    for row in rows:
        result.append(
            f"| `{row['path']}` | {row['compile_status']} | {row['run_status']} | "
            f"{row.get('return_code', '')} | {row.get('batch', '')} | "
            f"`{row.get('run_log', '')}` |"
        )
    return result


def write_report(rows: list[dict[str, str]], path: Path) -> int:
    compile_failures = [row for row in rows if row["compile_status"] != "pass"]
    self_contained = [row for row in rows if row["group"] == "self-contained"]
    self_contained_problems = [
        row
        for row in self_contained
        if row["compile_status"] != "pass" or row["run_status"] != "pass"
    ]
    run_counts = Counter(row["run_status"] for row in self_contained)
    compile_counts = Counter(row["compile_status"] for row in rows)
    classification_counts = Counter(row["group"] for row in rows)

    lines = [
        "# RISC OS Exampleageddon Sweep",
        "",
        "This report combines the target-wide cross-compile inventory with",
        "execution of every self-contained target AIF under RISC OS Open.",
        "",
        "## Summary",
        "",
        f"- Total `.bas` files: {len(rows)}",
        f"- Compile passes: {compile_counts['pass']}",
        f"- Compile failures/timeouts: {len(compile_failures)}",
        f"- Self-contained examples: {len(self_contained)}",
        f"- RISC OS run passes: {run_counts['pass']}",
        f"- RISC OS run failures: {run_counts['fail']}",
        f"- RISC OS run timeouts: {run_counts['timeout']}",
        f"- Missing RISC OS results: {run_counts['missing']}",
        f"- Self-contained policy problems: {len(self_contained_problems)}",
        "",
        "Compile failures outside the self-contained group are retained as",
        "coverage data for platform-specific and external-library examples.",
        "They do not fail the unattended RISC OS execution policy.",
        "",
        "## Classification Counts",
        "",
    ]

    for group, count in sorted(classification_counts.items()):
        lines.append(f"- {group}: {count}")

    lines.extend(
        [
            "",
            "## Self-Contained Problems",
            "",
            "| Example | Compile | Run | Return code | Batch | Run log |",
            "| --- | --- | --- | --- | --- | --- |",
        ]
    )

    if self_contained_problems:
        lines.extend(markdown_problem_rows(self_contained_problems))
    else:
        lines.append("| None | pass | pass | 0 |  |  |")

    lines.extend(
        [
            "",
            "## Compile Status Counts",
            "",
        ]
    )
    for status, count in sorted(compile_counts.items()):
        lines.append(f"- {status}: {count}")

    lines.extend(
        [
            "",
            "## Self-Contained Run Status Counts",
            "",
        ]
    )
    for status, count in sorted(run_counts.items()):
        lines.append(f"- {status}: {count}")

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return 1 if self_contained_problems else 0


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Merge RISC OS Exampleageddon compile and run results"
    )
    parser.add_argument("--compile-results", type=Path, required=True)
    parser.add_argument("--run-status", type=Path, required=True)
    parser.add_argument("--output-csv", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    compile_rows = read_csv(args.compile_results)
    run_rows = read_run_status(args.run_status)
    merged = merge_results(compile_rows, run_rows)
    write_csv(merged, args.output_csv)
    return write_report(merged, args.report)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

##############################################################################
# end of riscos-exampleageddon-report.py
##############################################################################
