#!/usr/bin/env python3
#
#   Project: FreeBASIC NuttX/RISC-V testing
#   ---------------------------------------
#
#   File: nuttx-riscv32-exampleageddon-qemu.py
#
#   Purpose:
#
#       Run exampleageddon-generated C programs under the NuttX RISC-V QEMU
#       smoke harness.
#
#   Responsibilities:
#
#       * read a nuttx-riscv32-qemu-suite style manifest
#       * rebuild one reusable NuttX app slot for each generated C program
#       * run each program under qemu-system-riscv32
#       * keep per-example logs and a CSV summary
#       * stop on the first real failing example unless asked to keep going
#
#   This file intentionally does NOT contain:
#
#       * FreeBASIC source classification
#       * generated-C production
#       * NuttX board setup
#       * release packaging
#

from __future__ import annotations

import argparse
import csv
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class ManifestEntry:
    source_name: str
    generated_c: str


@dataclass(frozen=True)
class RunResult:
    index: int
    total: int
    source_name: str
    generated_c: str
    status: str
    returncode: int
    seconds: str
    log: str


def die(message: str) -> None:
    print(f"nuttx-exampleageddon-qemu: {message}", file=sys.stderr)
    raise SystemExit(1)


def read_manifest(path: Path) -> list[ManifestEntry]:
    entries: list[ManifestEntry] = []

    with path.open("r", encoding="utf-8", newline="") as f:
        for raw_line in f:
            line = raw_line.strip()

            if not line or line.startswith("#"):
                continue

            source_name, sep, rest = line.partition(":")

            if not sep or not source_name or not rest:
                die(f"invalid manifest line: {raw_line.rstrip()}")

            generated_c = rest.split(":", 1)[0]
            entries.append(ManifestEntry(source_name, generated_c))

    return entries


def safe_log_name(name: str) -> str:
    result = []

    for ch in name:
        if ch.isalnum() or ch in "._-":
            result.append(ch)
        else:
            result.append("_")

    return "".join(result)


def write_csv(results: list[RunResult], path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)

    with path.open("w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(RunResult.__dataclass_fields__.keys()))
        writer.writeheader()

        for result in results:
            writer.writerow(result.__dict__)


def run_one(
    entry: ManifestEntry,
    index: int,
    total: int,
    args: argparse.Namespace,
    reuse_config: bool,
) -> RunResult:
    log_path = args.log_dir / f"{index:04d}-{safe_log_name(entry.source_name)}.log"
    command = [
        "bash",
        "build_scripts/nuttx-riscv32-qemu-smoke.sh",
        "--nuttx-workdir",
        str(args.nuttx_workdir),
        "--generated-c",
        entry.generated_c,
        "--app-name",
        args.app_name,
    ]

    if reuse_config:
        command.append("--reuse-config")

    env = None

    started = time.monotonic()
    log_path.parent.mkdir(parents=True, exist_ok=True)

    with log_path.open("w", encoding="utf-8", errors="replace") as log:
        log.write("$ " + " ".join(command) + "\n\n")
        log.flush()

        completed = subprocess.run(
            command,
            stdout=log,
            stderr=subprocess.STDOUT,
            env=env,
            check=False,
        )

    seconds = time.monotonic() - started
    text = log_path.read_text(encoding="utf-8", errors="replace")

    if completed.returncode == 0 and "NUTTX_RISCV32_FB_SMOKE_OK" in text:
        status = "pass"
    else:
        status = f"fail({completed.returncode})"

    return RunResult(
        index=index,
        total=total,
        source_name=entry.source_name,
        generated_c=entry.generated_c,
        status=status,
        returncode=completed.returncode,
        seconds=f"{seconds:.3f}",
        log=str(log_path),
    )


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run NuttX exampleageddon C output under QEMU")
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--nuttx-workdir", type=Path, default=Path("/work"))
    parser.add_argument("--outdir", type=Path, default=Path(".build-nuttx-exampleageddon-qemu"))
    parser.add_argument("--app-name", default="eg_exampleageddon")
    parser.add_argument("--start-index", type=int, default=1)
    parser.add_argument("--limit", type=int, default=0)
    parser.add_argument(
        "--reuse-config",
        action="store_true",
        help="reuse the existing NuttX configuration for the first case too",
    )
    parser.add_argument("--keep-going", action="store_true")

    args = parser.parse_args(argv)

    if args.start_index < 1:
        parser.error("--start-index must be 1 or greater")

    args.outdir = args.outdir.resolve()
    args.log_dir = args.outdir / "logs"

    return args


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    entries = read_manifest(args.manifest)

    if not entries:
        die("manifest did not contain any runnable entries")

    selected = entries[args.start_index - 1:]

    if args.limit > 0:
        selected = selected[:args.limit]

    total = len(entries)
    results: list[RunResult] = []
    csv_path = args.outdir / "results.csv"

    args.outdir.mkdir(parents=True, exist_ok=True)
    args.log_dir.mkdir(parents=True, exist_ok=True)

    for offset, entry in enumerate(selected):
        index = args.start_index + offset
        reuse_config = args.reuse_config or index != args.start_index

        print(f"[{index}/{total}] run {entry.source_name}", flush=True)
        result = run_one(entry, index, total, args, reuse_config)
        results.append(result)
        write_csv(results, csv_path)

        print(f"[{index}/{total}] {result.status} {entry.source_name} ({result.seconds}s)", flush=True)

        if result.status != "pass" and not args.keep_going:
            print(f"first failing log: {result.log}", file=sys.stderr)
            return 1

    failures = [result for result in results if result.status != "pass"]
    print()
    print(f"CSV: {csv_path}")
    print(f"Run passes: {sum(1 for result in results if result.status == 'pass')}")
    print(f"Run failures: {len(failures)}")

    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

# end of nuttx-riscv32-exampleageddon-qemu.py
