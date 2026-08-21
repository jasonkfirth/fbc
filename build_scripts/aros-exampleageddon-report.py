#!/usr/bin/env python3

##############################################################################
# FreeBASIC AROS Exampleageddon reporting
##############################################################################
#
# File: aros-exampleageddon-report.py
#
# Purpose:
#
#   Merge one AROS architecture's Exampleageddon compile inventory with its
#   in-guest execution results.
#
# Responsibilities:
#
#   * preserve every compile and classification result
#   * attach emulator batch, return-code, and log evidence
#   * write machine-readable CSV and concise Markdown acceptance reports
#   * fail when any self-contained example did not compile and run cleanly
#
# This file intentionally does NOT contain:
#
#   * example discovery or classification policy
#   * compiler or emulator orchestration
#   * architecture-specific build rules
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
        writer.writerows(
            {field: row.get(field, "") for field in OUTPUT_FIELDS}
            for row in rows
        )


def write_report(rows: list[dict[str, str]], path: Path, target: str) -> int:
    compile_counts = Counter(row["compile_status"] for row in rows)
    classification_counts = Counter(row["group"] for row in rows)
    self_contained = [row for row in rows if row["group"] == "self-contained"]
    run_counts = Counter(row["run_status"] for row in self_contained)
    problems = [
        row
        for row in self_contained
        if row["compile_status"] != "pass" or row["run_status"] != "pass"
    ]

    lines = [
        f"# AROS {target} Exampleageddon Sweep",
        "",
        "This report combines the complete cross-compile inventory with",
        "in-guest execution of every self-contained example.",
        "",
        "## Summary",
        "",
        f"- Total `.bas` files: {len(rows)}",
        f"- Compile passes: {compile_counts['pass']}",
        f"- Compile failures/timeouts: {len(rows) - compile_counts['pass']}",
        f"- Self-contained examples: {len(self_contained)}",
        f"- AROS run passes: {run_counts['pass']}",
        f"- AROS run failures: {run_counts['fail']}",
        f"- AROS run timeouts: {run_counts['timeout']}",
        f"- Missing AROS results: {run_counts['missing']}",
        f"- Self-contained policy problems: {len(problems)}",
        "",
        "Compile failures outside the self-contained group are retained as",
        "coverage data and do not fail the unattended execution policy.",
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

    if problems:
        for row in problems:
            lines.append(
                f"| `{row['path']}` | {row['compile_status']} | "
                f"{row['run_status']} | {row.get('return_code', '')} | "
                f"{row.get('batch', '')} | `{row.get('run_log', '')}` |"
            )
    else:
        lines.append("| None | pass | pass | 0 |  |  |")

    lines.extend(["", "## Compile Status Counts", ""])
    for status, count in sorted(compile_counts.items()):
        lines.append(f"- {status}: {count}")

    lines.extend(["", "## Self-Contained Run Status Counts", ""])
    for status, count in sorted(run_counts.items()):
        lines.append(f"- {status}: {count}")

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return 1 if problems else 0


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Merge AROS Exampleageddon compile and guest results"
    )
    parser.add_argument("--target", required=True)
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
    return write_report(merged, args.report, args.target)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

# end of aros-exampleageddon-report.py
