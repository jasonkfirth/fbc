#!/usr/bin/env python3
"""Project: FreeBASIC compiler semantic tooling
File: validate-compiler-semantic-model.py
Purpose: Validate one compiler semantic sidecar as an untrusted artifact.
Responsibilities: Select the schema mode, enforce reader limits, and verify sources.
This file intentionally does NOT contain: semantic inference, lint policy, or source writes.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys


def positive_integer(value: str) -> int:
    """Parse a command-line resource limit without accepting zero."""
    try:
        result = int(value)
    except ValueError as error:
        raise argparse.ArgumentTypeError("must be an integer") from error
    if result <= 0:
        raise argparse.ArgumentTypeError("must be greater than zero")
    return result


def emit_result(payload: dict[str, object], output_format: str, *, failed: bool = False) -> None:
    """Keep machine output stable while retaining a small human summary."""
    if output_format == "json":
        print(json.dumps(payload, sort_keys=True))
        return
    stream = sys.stderr if failed else sys.stdout
    if failed:
        print("invalid: " + str(payload["error"]), file=stream)
        return
    print("valid: yes", file=stream)
    print("schema: " + str(payload["schema"]), file=stream)
    print("compiler: " + str(payload["compiler"]), file=stream)
    print("footer: " + str(payload["footer"]), file=stream)
    print("modules: " + str(payload["modules"]), file=stream)
    print("files: " + str(payload["files"]), file=stream)
    print("locations: " + str(payload["locations"]), file=stream)
    print("source validation: " + str(payload["source_validation"]), file=stream)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("model", type=Path, help="FBCSEM sidecar to validate")
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1],
                        help="FreeBASIC source root containing the independent reader")
    parser.add_argument("--mode", choices=("full", "bindings", "expressions"), default="full")
    parser.add_argument("--allow-recovery", action="store_true",
                        help="Accept an explicitly provisional RECOVERY footer")
    parser.add_argument("--validate-sources", action="store_true",
                        help="Rehash every regular source revision")
    parser.add_argument("--validate-locations", action="store_true",
                        help="Also verify physical byte and UTF-16 coordinates")
    parser.add_argument("--format", choices=("text", "json"), default="text")
    parser.add_argument("--max-sidecar-bytes", type=positive_integer, default=512 * 1024 * 1024)
    parser.add_argument("--max-record-bytes", type=positive_integer, default=64 * 1024 * 1024)
    parser.add_argument("--max-records", type=positive_integer, default=24_000_000)
    parser.add_argument("--max-source-bytes", type=positive_integer, default=512 * 1024 * 1024)
    parser.add_argument("--max-source-cache-bytes", type=positive_integer,
                        default=512 * 1024 * 1024)
    options = parser.parse_args()

    root = options.root.resolve()
    reader_directory = root / "tests" / "semantic-sidecar"
    if not reader_directory.is_dir():
        parser.error("Independent semantic reader is missing: " + str(reader_directory))
    sys.path.insert(0, str(reader_directory))
    from sidecar import Model, ReaderLimits, SCHEMA

    limits = ReaderLimits(max_sidecar_bytes=options.max_sidecar_bytes,
                          max_record_bytes=options.max_record_bytes,
                          max_records=options.max_records,
                          max_source_bytes=options.max_source_bytes,
                          max_cached_source_bytes=options.max_source_cache_bytes)
    try:
        model = Model.read(options.model,
                           expressions_only=options.mode == "expressions",
                           bindings_only=options.mode == "bindings",
                           allow_recovery=options.allow_recovery,
                           limits=limits)
        source_validation = "not requested"
        if options.validate_locations:
            model.validate_physical_locations()
            source_validation = "revisions and locations verified"
        elif options.validate_sources:
            model.validate_source_revisions()
            source_validation = "revisions verified"
    except (OSError, ValueError) as error:
        emit_result({"valid": False, "error": str(error)}, options.format, failed=True)
        return 1

    payload: dict[str, object] = {
        "valid": True,
        "schema": SCHEMA,
        "compiler": model.rows[0][2],
        "footer": model.footer[0],
        "modules": len(model.records["M"]),
        "files": len(model.files),
        "locations": len(model.records["LOC"]),
        "source_validation": source_validation,
        "records": {tag: len(rows) for tag, rows in sorted(model.records.items()) if rows},
    }
    emit_result(payload, options.format)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of validate-compiler-semantic-model.py
