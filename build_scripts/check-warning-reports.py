#!/usr/bin/env python3
"""Project: FreeBASIC compiler tests
File: check-warning-reports.py
Purpose: Compare warning diagnostics with maintained reference reports.
Responsibilities: Isolate compiler output and update references only on request.
This file intentionally does NOT accept changed diagnostics as passing tests.
"""

import argparse
import difflib
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import tempfile


TARGETS = ("dos", "linux-x86", "linux-x86_64", "win32", "win64")
LOCATION = re.compile(r"^.*\.(?:bas|bi)\(.*\) warning .*\(.*\): ", re.MULTILINE)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=Path(__file__).resolve().parents[1] / "tests/warnings")
    parser.add_argument("--fbc", default=os.environ.get("FBC", "fbc"))
    parser.add_argument("--filter", default=os.environ.get("TEST_FILTER", "*.bas"))
    parser.add_argument("--update", action="store_true", help="Replace the selected reference reports")
    options = parser.parse_args()
    source = options.source.resolve()
    compiler = shutil.which(options.fbc)
    if compiler is None:
        parser.error("warning-test compiler not found: " + options.fbc)
    compiler = str(Path(compiler).resolve())
    inputs = sorted(source.glob(options.filter))
    if not inputs or any(path.parent != source or path.suffix != ".bas" for path in inputs):
        parser.error("filter must select top-level .bas files: " + options.filter)
    flags = shlex.split(os.environ.get("FBC_FLAGS", ""))
    failures = 0
    reports = []
    # The compiler writes intermediate C/assembly beside its input. Copy just
    # the small fixture sources, so a check cannot overwrite either the source
    # tree's intermediates or its reference reports, even when a command fails.
    with tempfile.TemporaryDirectory(prefix="fbc-warnings-") as temporary:
        working = Path(temporary)
        for path in source.iterdir():
            if path.is_file() and path.suffix in (".bas", ".bi"):
                shutil.copyfile(path, working / path.name)
        for target in TARGETS:
            for path in inputs:
                print("TEST " + target + " " + path.name, flush=True)
                command = [compiler, *flags, "-maxerr", "inf", "-w", "constness",
                           "-target", target, path.name, "-r", "-m", path.stem]
                result = subprocess.run(command, cwd=working, capture_output=True,
                                        text=True, errors="replace", timeout=60, check=False)
                # Compiler error fixtures deliberately return 1. A crash or
                # failed tool launch must never become a new expectation.
                if result.returncode not in (0, 1) or result.stderr:
                    print(result.stdout + result.stderr)
                    raise RuntimeError("warning-test command failed: " + target + " " + path.name)
                text = LOCATION.sub("\t", result.stdout)
                reports.append((source / "r" / target / (path.stem + ".txt"), text))
        # Generate every selected profile successfully before publishing any
        # updates. LF is canonical; reads also accept a checkout's CRLF form.
        for reference, text in reports:
            if options.update:
                reference.parent.mkdir(parents=True, exist_ok=True)
                staged = working / "reference.txt"
                staged.write_text(text, encoding="utf-8", newline="\n")
                # Use a same-directory temporary for atomic replacement even
                # when the system temporary directory is on another volume.
                with tempfile.NamedTemporaryFile(dir=reference.parent, prefix=".warning-", delete=False) as output:
                    pending = Path(output.name)
                    output.write(staged.read_bytes())
                try:
                    os.replace(pending, reference)
                finally:
                    pending.unlink(missing_ok=True)
            else:
                expected = reference.read_text(encoding="utf-8") if reference.is_file() else ""
                if not reference.is_file() or expected != text:
                    failures += 1
                    print("WARNING_REPORT_DIFF " + str(reference.relative_to(source)))
                    print("".join(difflib.unified_diff(expected.splitlines(True), text.splitlines(True),
                                                     fromfile=str(reference), tofile="generated")), end="")
        if options.filter == "*.bas":
            expected_names = {path.stem + ".txt" for path in inputs}
            for target in TARGETS:
                for extra in (source / "r" / target).glob("*.txt"):
                    if extra.name not in expected_names:
                        if options.update:
                            extra.unlink()
                        else:
                            failures += 1
                            print("WARNING_REPORT_EXTRA " + str(extra.relative_to(source)))
    mode = "UPDATED" if options.update else ("FAIL" if failures else "PASS")
    print(f"WARNING_REPORTS_{mode} reports={len(reports)} differences={failures}")
    return 1 if failures else 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        raise SystemExit("WARNING_REPORTS_FAIL " + str(error))

# end of check-warning-reports.py
