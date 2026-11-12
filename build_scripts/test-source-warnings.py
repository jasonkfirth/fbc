#!/usr/bin/env python3
"""Project: FreeBASIC source diagnostic qualification
File: test-source-warnings.py
Purpose: Verify source warnings independently of the selected code generator.
Responsibilities: Check levels, locations, target ranges, conservative exclusions,
warning promotion, and optional emitted-output identity against a prior compiler.
This file intentionally does NOT execute the memory-access examples.
"""

from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile


WARNING = re.compile(r"^(.+?)\((\d+)\)(?: error)? warning (\d+)\((\d+)\): (.+)$", re.M)


def diagnostic_path(filename: str) -> Path:
    # MSYS Python sees /d/... while the native compiler reports D:\... .
    # Convert only native drive paths; keep ordinary POSIX paths unchanged.
    if os.name != "nt" and re.match(r"^[A-Za-z]:[\\/]", filename):
        filename = subprocess.check_output(["cygpath", "-u", filename], text=True).strip()
    return Path(filename).resolve()


def expected(case: int, bits: int, level: str) -> Counter:
    if level == "none":
        return Counter()
    result = Counter()
    if level == "all":
        if case == 1:
            result.update({54: 1, 55: 2})
        elif case == 10:
            result[54] = 2
        elif case == 11:
            result[55] = 1
        elif case == 17:
            result[55] = 2
        elif case == 18:
            result[55] = 1
        elif case == 19:
            result[54] = 1
    if case == 2 and bits == 64:
        result[56] = 1
    elif case in (3, 12, 21):
        result[57] = 1
    elif case == 9:
        result[56] = 4 if bits == 64 else 3
    elif case == 23:
        result[56] = 1
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fbc", type=Path, required=True)
    parser.add_argument("--prefix", type=Path, required=True)
    parser.add_argument("--reference", type=Path)
    parser.add_argument("--work-dir", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    source = root / "tests/compiler-support/source-warnings.bas"
    include = source.with_name("source-warning-location.bi")
    declaration_line = next(i for i, line in enumerate(include.read_text().splitlines(), 1)
                            if line.startswith("dim included_unused_value"))
    state = {"checks_complete": False, "applications_executed": False, "checks": [],
             "emitted_output_identities": 0, "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest()}
    # One directory and one output slot are reused for every frontend invocation.
    with tempfile.TemporaryDirectory(prefix="source-warnings-", dir=args.work_dir) as temporary:
        output = Path(temporary) / "unit.out"

        def emit(compiler, generator, bits, case, level, flags=(), promote=False):
            command = [str(compiler.resolve()), "-prefix", str(args.prefix.resolve()),
                       "-gen", generator, "-asm", "intel", "-r", "-m", source.stem,
                       "-target", "linux", "-arch", "686" if bits == 32 else "x86_64",
                       *([] if level == "default" else ["-w", level]),
                       *(["-w", "error"] if promote else []), *flags,
                       "-d", f"SOURCE_WARNING_CASE={case}", str(source), "-o", str(output)]
            result = subprocess.run(command, cwd=root, capture_output=True, timeout=600, check=False)
            text = (result.stdout + result.stderr).decode(errors="replace").replace("\r", "")
            return result.returncode, text, hashlib.sha256(output.read_bytes()).hexdigest() if output.exists() else None

        def check(generator, bits, case, level, flags=(), promote=False):
            output.unlink(missing_ok=True)
            status, text, digest = emit(args.fbc, generator, bits, case, level, flags, promote)
            matches = list(WARNING.finditer(text))
            found = Counter(int(m[3]) for m in matches)
            wanted = expected(case, bits, level)
            assert found == wanted, (generator, bits, case, level, flags, wanted, found, text)
            assert status == (1 if promote and wanted else 0), (status, text)
            assert all(int(m[4]) == (0 if int(m[3]) in (54, 55) else 1) for m in matches), text
            for match in matches:
                filename, line, code = match[1], int(match[2]), int(match[3])
                if case == 11:
                    assert diagnostic_path(filename) == include.resolve() and line == declaration_line, text
                else:
                    assert diagnostic_path(filename) == source.resolve(), text
                    assert 0 < line <= len(source.read_text().splitlines()), text
                if case == 1 and code == 54:
                    assert match[5].endswith(", IGNORED"), text
            row = {"generator": generator, "bits": bits, "case": case, "level": level,
                   "flags": list(flags), "promoted": promote, "exit": status,
                   "warnings": dict(found), "output": text, "emission_sha256": digest}
            if args.reference and not promote:
                output.unlink(missing_ok=True)
                old_status, old_text, old_digest = emit(args.reference, generator, bits, case, level, flags)
                assert old_status == 0, old_text
                assert old_digest == digest, (generator, bits, case, level, flags, old_digest, digest)
                state["emitted_output_identities"] += 1
                row["reference_emission_sha256"] = old_digest
            state["checks"].append(row)
            output.unlink(missing_ok=True)

        for generator, bits in (("gcc", 64), ("clang", 64), ("gas64", 64),
                                ("gcc", 32), ("clang", 32), ("gas", 32)):
            for case in range(1, 24):
                for level in ("default", "all"):
                    check(generator, bits, case, level)
            for flags in (("-O", "0"), ("-O", "2", "-g"), ("-O", "2", "-exx")):
                for case in (1, 2, 3, 4, 5, 9, 23):
                    check(generator, bits, case, "all", flags)
            for case in (1, 2, 3, 9, 10, 11, 23):
                check(generator, bits, case, "none")
            check(generator, bits, 10, "all", promote=True)
            check(generator, bits, 3, "default", promote=True)
            check(generator, bits, 23, "default", promote=True)
            print(f"SOURCE_WARNINGS_BACKEND_PASS {generator} {bits}", flush=True)
    state["checks_complete"] = True
    if args.report:
        args.report.write_text(json.dumps(state, separators=(",", ":")) + "\n", encoding="utf-8")
    print(f"SOURCE_WARNINGS_PASS {len(state['checks'])} checks, "
          f"{state['emitted_output_identities']} emitted-output identities; examples not executed", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of test-source-warnings.py
