#!/usr/bin/env python3
"""Project: FreeBASIC compiler tests
File: test-compiler-semantic-projects.py
Purpose: Audit semantic exports from corpus programs whose GCC controls built.
Responsibilities: Replay captured source flags, validate models, and retain evidence.
This file intentionally does NOT contain: project source repairs or alternate BASIC parsing.
"""

from __future__ import annotations

import argparse
from concurrent.futures import ProcessPoolExecutor, as_completed
import gzip
import json
import os
from pathlib import Path
import shutil
import sys

from compiler_semantic_audit import audit_exports, digest
from fb_corpus_builds import prepare_snapshot


def audit_case(snapshot: Path, output: Path, plan: dict, backend: str, timeout: int) -> dict:
    toolchain = snapshot / "inputs/toolchain"
    work = output / backend / plan["id"]
    work.mkdir(parents=True)
    modules = [Path(name) for name in plan["sources"]]
    # Artifact paths are private to this invocation. Source paths and source
    # options stay the same in every mode, including main/library/dialect policy.
    common = [str(toolchain / "fbc"), "-prefix", str(toolchain), "-i", str(toolchain / "inc"),
              *plan["flags"], "-gen", backend, "-r"]
    try:
        record = audit_exports(common, modules, work, backend, timeout, private_emissions=True,
                               cwd=Path(plan["cwd"]), keep_going=True)
    except (ValueError, OSError) as error:
        record = {"backend": backend, "passed": False, "error": str(error)}
    record.update({key: plan[key] for key in ("id", "project", "name", "baseline", "control_log")})
    # Keep the exact model and emission bytes, while limiting space used by
    # large GUI headers and generated RPG source modules.
    for path in sorted(work.rglob("*")):
        if path.is_file() and path.suffix in (".tsv", ".c", ".ll", ".asm"):
            with path.open("rb") as source, gzip.open(str(path) + ".gz", "wb", compresslevel=1) as target:
                shutil.copyfileobj(source, target)
            path.unlink()
    return record


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    inputs = parser.add_mutually_exclusive_group(required=True)
    inputs.add_argument("--snapshot", type=Path,
                        help="Replay a frozen GCC-controlled corpus with audit-plan.json")
    inputs.add_argument("--corpus", type=Path, help="Capture fresh GCC controls from this project corpus")
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--fbc", type=Path, help="Compiler for fresh GCC controls and semantic audits")
    parser.add_argument("--registry", type=Path, default=Path(__file__).with_name("fb-corpus-semantic-projects.json"))
    parser.add_argument("--prepare-only", action="store_true", help="Capture GCC controls without auditing exports")
    parser.add_argument("--resume", action="store_true", help="Resume complete results from the same frozen audit")
    parser.add_argument("--output", type=Path, required=True, help="New result directory")
    parser.add_argument("--backend", choices=("gcc", "llvm", "clang"), action="append")
    parser.add_argument("--project", action="append", help="Select project names from the captured plan")
    parser.add_argument("--case", action="append", help="Select invocation IDs from the captured plan")
    parser.add_argument("--jobs", type=int, default=3)
    parser.add_argument("--timeout", type=int, default=240)
    parser.add_argument("--build-timeout", type=int, default=1800)
    parser.add_argument("--source-date-epoch", default="0",
                        help="Fixed SOURCE_DATE_EPOCH for compiler date/time builtins (default: 0)")
    options = parser.parse_args()
    if options.jobs < 1 or options.timeout < 1 or options.build_timeout < 1:
        parser.error("--jobs, --timeout, and --build-timeout must be positive")
    if not options.source_date_epoch.isdecimal():
        parser.error("--source-date-epoch must be an unsigned integer")
    # Build-time banners must not change between export modes just because a
    # large header took another second to compile. FBC validates the epoch.
    os.environ["SOURCE_DATE_EPOCH"] = options.source_date_epoch
    output = options.output.resolve()
    if options.resume:
        if not options.snapshot or not (output / "results.json").is_file():
            parser.error("--resume requires --snapshot and an existing results.json")
    else:
        output.mkdir(parents=True, exist_ok=False)
    if options.corpus:
        if not options.fbc or not options.fbc.is_file():
            parser.error("Fresh corpus controls require an existing --fbc")
        snapshot = output / "snapshot"
        prepare_snapshot(options.root.resolve(), options.fbc.resolve(), options.corpus.resolve(),
                         snapshot, options.registry.resolve(), options.project, options.jobs,
                         options.timeout, options.build_timeout)
    else:
        snapshot = options.snapshot.resolve()
    if options.prepare_only:
        if not options.corpus:
            parser.error("--prepare-only requires --corpus")
        return 0
    plan = json.loads((snapshot / "audit-plan.json").read_text())
    if options.project:
        plan = [item for item in plan if item["project"] in options.project]
    if options.case:
        plan = [item for item in plan if item["id"] in options.case]
    if not plan:
        parser.error("No GCC-controlled source invocations were selected")
    if not options.resume:
        shutil.copy2(snapshot / "audit-plan.json", output / "captured-plan.json")
    sys.path.insert(0, str(snapshot / "inputs/validation"))
    backends = list(dict.fromkeys(options.backend or ("gcc", "llvm", "clang")))
    identity = {"compiler": digest(snapshot / "inputs/toolchain/fbc"),
                "reader": digest(snapshot / "inputs/validation/sidecar.py"),
                "helper": digest(Path(__file__).with_name("compiler_semantic_audit.py")),
                "plan": digest(snapshot / "audit-plan.json"), "backends": backends,
                "cases": [item["id"] for item in plan], "timeout": options.timeout,
                "source_date_epoch": options.source_date_epoch}
    if options.resume:
        if json.loads((output / "run-identity.json").read_text()) != identity:
            parser.error("Resume inputs differ from the saved compiler, validator, plan, or selections")
        records = json.loads((output / "results.json").read_text())
    else:
        (output / "run-identity.json").write_text(json.dumps(identity, indent=2) + "\n")
        records = []
    completed = {(entry["backend"], entry["id"]) for entry in records}
    pending = [(item, backend) for backend in backends for item in plan
               if (backend, item["id"]) not in completed]
    if options.resume:
        # An interrupted case never reaches results.json. Remove only its
        # private artifact directory before rebuilding all of its export modes.
        for item, backend in pending:
            work = output / backend / item["id"]
            if work.exists():
                shutil.rmtree(work)
    total = len(plan) * len(backends)
    with ProcessPoolExecutor(max_workers=options.jobs) as workers:
        futures = {workers.submit(audit_case, snapshot, output, item, backend, options.timeout):
                   (item, backend) for item, backend in pending}
        for future in as_completed(futures):
            item, backend = futures[future]
            try:
                record = future.result()
            except (ValueError, OSError) as error:
                record = {"id": item["id"], "project": item["project"], "name": item["name"],
                          "backend": backend, "passed": False, "error": str(error)}
            records.append(record)
            records.sort(key=lambda entry: (entry["backend"], entry["id"]))
            # Replacement leaves the last complete progress file readable if
            # this long corpus run is interrupted between completed cases.
            progress = output / "results.tmp"
            progress.write_text(json.dumps(records, indent=2) + "\n")
            progress.replace(output / "results.json")
            print(f"{len(records)}/{total} {backend} {item['name']}: "
                  + ("passed" if record["passed"] else record["error"]), flush=True)
    failures = sum(not entry["passed"] for entry in records)
    print(f"{len(records) - failures}/{len(records)} GCC-controlled corpus audit cases passed; {output}")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of test-compiler-semantic-projects.py
