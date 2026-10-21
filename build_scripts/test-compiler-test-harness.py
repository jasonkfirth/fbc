#!/usr/bin/env python3
"""FreeBASIC compiler log harness regression tests.

File: test-compiler-test-harness.py
Purpose: Ensure log status, failure details, and failed-test retries are reliable.
Responsibilities: Exercise the real make rules in an isolated temporary test tree.
This file intentionally does NOT run the complete language suite.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile


def test_haiku_grep(root: Path, working: Path) -> bool:
    """Haiku must count result records with the same status as native grep."""
    # The Haiku helper requires GNU awk extensions. Other hosts may only
    # provide BSD awk or mawk, which cannot run this platform-specific probe.
    awk = shutil.which("gawk") or shutil.which("awk")
    if awk is None:
        print("Haiku grep: skipped (GNU awk unavailable)")
        return True
    version = subprocess.run([awk, "--version"], text=True, capture_output=True,
                             timeout=10, check=False)
    if "GNU Awk" not in version.stdout:
        print("Haiku grep: skipped (GNU awk unavailable)")
        return True
    environment = dict(os.environ, AWK=awk)
    working.mkdir()
    (working / "matches.log").write_text(
        "probe : RESULT=PASSED\nignored\nprobe : RESULT=FAILED\n", encoding="utf-8")
    (working / "empty.log").write_text("", encoding="utf-8")
    helper = ["sh", str(root / "tests/haiku/grep.sh")]
    pattern = r"^.* : RESULT=(PASSED|FAILED)( |$)"
    for label, arguments, input_text in (
        ("result counts", ["-c", "-E", pattern, "matches.log"], None),
        ("empty result count", ["-c", "-E", pattern, "empty.log"], None),
        ("per-file counts", ["-c", "-E", pattern, "matches.log", "empty.log"], None),
        ("stdin count", ["-c", "-E", pattern], "probe : RESULT=PASSED\n"),
        ("forced file name", ["-H", "-E", pattern, "matches.log"], None),
        ("forced file-name count", ["-H", "-c", "-E", pattern, "matches.log"], None),
        ("forced stdin name", ["-H", "-E", pattern], "probe : RESULT=PASSED\n"),
        ("quiet count", ["-c", "-q", "-E", pattern, "matches.log"], None),
        ("file-name count", ["-c", "-l", "-E", pattern, "matches.log", "empty.log"], None),
        ("missing file", ["-c", "-E", pattern, "missing.log"], None),
    ):
        expected = subprocess.run(["grep", *arguments], cwd=working, input=input_text,
                                  text=True, capture_output=True, timeout=10, check=False)
        actual = subprocess.run(helper + arguments, cwd=working, input=input_text,
                                text=True, capture_output=True, timeout=10, check=False,
                                env=environment)
        if (actual.returncode, actual.stdout) != (expected.returncode, expected.stdout):
            print(label + ": wrong Haiku grep result\n" + actual.stdout + actual.stderr)
            return False
        print("Haiku grep " + label + ": passed")
    return True


def test_compiler_selection(root: Path, working: Path, make: str) -> bool:
    """A compiler built by this invocation must replace the native fallback."""
    working.mkdir()
    (working / ".maketests-host").mkdir()
    for path, label in ((working / "new-fbc", "built"),
                        (working / ".maketests-host/fbc", "preserved")):
        path.write_text("#!/bin/sh\nprintf '%s\\n' '" + label + "'\n", encoding="utf-8")
        path.chmod(0o755)
    makefile = working / "GNUmakefile"
    makefile.write_text(
        "FBC_EXE := bin/fbc\nCC := gcc\nCROSS_BUILD :=\n"
        "include " + (root / "mk/tests/harness.mk").as_posix() + "\n"
        "all: bin/fbc\n\t@$(TEST_FBC_CMD)\n"
        "fallback:\n\t@$(TEST_FBC_CMD)\n"
        "bin/fbc:\n\t@mkdir -p bin\n\t@cp new-fbc bin/fbc\n",
        encoding="utf-8",
    )
    for label, arguments, expected in (
        ("native fallback", ["fallback"], "preserved"),
        ("compiler built by prerequisite", ["all"], "built"),
        ("cross build host compiler", ["all", "CROSS_BUILD=yes"], "preserved"),
    ):
        result = subprocess.run([make, "--no-print-directory", "-s", *arguments],
                                cwd=working, text=True, capture_output=True,
                                timeout=30, check=False)
        if result.returncode != 0 or result.stdout.strip() != expected:
            print(label + ": wrong compiler selection\n" + result.stdout + result.stderr)
            return False
        print(label + ": passed")
    return True


def test_log_recovery(root: Path, working: Path, make: str) -> bool:
    """Saved logs must identify failures and bound retry cleanup to those tests."""
    working.mkdir()
    (working / "cases").mkdir()
    for name in ("common.mk", "log-tests.mk", "bmk-make.mk"):
        shutil.copyfile(root / "tests" / name, working / name)
    (working / "dirlist.mk").write_text("DIRLIST_FB := cases\n", encoding="utf-8")
    command = [make, "--no-print-directory", "-s", "-f", "log-tests.mk",
               "FB_LANG=fb", "FBC=false", "rootdir=" + root.as_posix()]

    def run(*targets: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(command + list(targets), cwd=working, text=True,
                              capture_output=True, timeout=30, check=False)

    def save_log(stem: str, mode: str, result: str) -> None:
        suffix = ".bmk" if mode.startswith("MULTI_MODULE") else ".bas"
        source = working / ("cases/" + stem + suffix)
        # These are dependency placeholders. Only saved-log handling runs;
        # FBC=false makes an accidental compilation fail the regression.
        source.touch()
        if suffix == ".bmk":
            source.write_text("MAIN := " + stem + ".bas\n", encoding="utf-8")
        (working / ("cases/" + stem + ".log")).write_text(
            str(source.relative_to(working).as_posix()) + " : TEST_MODE=" + mode + "\n"
            "diagnostic from " + stem + "\n"
            + str(source.relative_to(working).as_posix()) + " : RESULT=" + result + "\n",
            encoding="utf-8",
        )
        # Native Windows Make can round file times differently from Python.
        # Saved logs must remain newer than their placeholder sources, even
        # when the fixture replaces several files within one clock tick.
        stamp = (working / ("cases/" + stem + ".log")).stat().st_mtime - 4
        os.utime(source, (stamp, stamp))

    selected = working / "log-tests-fb.inc"
    selected.write_text("SRCLIST_COMPILE_ONLY_OK += cases/single.bas\n", encoding="utf-8")
    save_log("single", "COMPILE_ONLY_OK", "FAILED")
    result = run("results")
    success = result.returncode != 0 and "diagnostic from single" in result.stdout
    print("single failure details: " + ("passed" if success else "FAILED"))
    if not success:
        print(result.stdout + result.stderr)

    modes = ("COMPILE_ONLY_OK", "COMPILE_ONLY_FAIL", "COMPILE_AND_RUN_OK",
             "COMPILE_AND_RUN_FAIL", "MULTI_MODULE_OK", "MULTI_MODULE_FAIL")
    rows = []
    for mode in modes:
        suffix = ".bmk" if mode.startswith("MULTI_MODULE") else ".bas"
        rows.append("SRCLIST_" + mode + " += cases/" + mode + suffix)
        save_log(mode, mode, "FAILED")
        (working / ("cases/" + mode + ".o")).touch()
    save_log("passing", "COMPILE_ONLY_OK", "PASSED")
    (working / "cases/passing.o").touch()
    (working / "cases/single.log").unlink()
    selected.write_text("\n".join(rows) + "\n"
                        "SRCLIST_COMPILE_ONLY_OK += cases/passing.bas\n", encoding="utf-8")
    # The fixture replaces its selection within one filesystem timestamp tick
    # on some Windows hosts. Discard the earlier single-test manifests.
    for name in ("log-tests-log-fb.lst", "log-tests-results-fb.log"):
        (working / name).unlink(missing_ok=True)
    failed = working / "failed-log-tests-fb.inc"
    failed.unlink(missing_ok=True)
    result = run("FAILED_ONLY=1", "all")
    recovered = {line for line in failed.read_text().splitlines()
                 if line and not line.startswith("#")}
    recovery_ok = result.returncode == 0 and recovered == set(rows)
    print("failed list recovery (six modes): " + ("passed" if recovery_ok else "FAILED"))
    if not recovery_ok:
        print(result.stdout + result.stderr + str(sorted(recovered)))
        return False

    with failed.open("a", encoding="utf-8") as stream:
        stream.write(rows[0] + "\n")
    result = run("FAILED_ONLY=1", "mostlyclean")
    cleanup_ok = (result.returncode == 0
                  and (working / "cases/passing.log").exists()
                  and (working / "cases/passing.o").exists()
                  and all(not (working / ("cases/" + mode + suffix)).exists()
                          for mode in modes for suffix in (".log", ".o")))
    print("failed retry preserves passing artifacts: " + ("passed" if cleanup_ok else "FAILED"))
    if not cleanup_ok:
        print(result.stdout + result.stderr)
        return False

    # A retry summary must inspect the selected tests even when an unrelated
    # saved log is incomplete. Repeated cache entries must not repeat failures.
    (working / "cases/passing.log").write_text("", encoding="utf-8")
    for mode in modes:
        save_log(mode, mode, "FAILED" if mode in modes[:2] else "PASSED")
    result = run("FAILED_ONLY=1", "results")
    summary = working / "log-tests-results-failed-fb.log"
    details_ok = (result.returncode != 0 and len(summary.read_text().splitlines()) == 2
                  and all("diagnostic from " + mode in result.stdout for mode in modes[:2]))
    print("multiple failure details without duplicates: " + ("passed" if details_ok else "FAILED"))
    if not details_ok:
        print(result.stdout + result.stderr)
        return False

    for mode in modes[:2]:
        save_log(mode, mode, "PASSED")
    summary.unlink()
    result = run("FAILED_ONLY=1", "results")
    summary_ok = result.returncode == 0
    print("failed retry summary scope: " + ("passed" if summary_ok else "FAILED"))
    if not summary_ok:
        print(result.stdout + result.stderr)
        return False

    result = run("results")
    full_ok = result.returncode != 0 and "Incomplete test log: cases/passing.log" in result.stdout
    print("full summary after failed retry: " + ("passed" if full_ok else "FAILED"))
    if not full_ok:
        print(result.stdout + result.stderr)
        return False

    save_log("passing", "COMPILE_ONLY_OK", "PASSED")
    failed.unlink()
    for name in ("log-tests-log-failed-fb.lst", "log-tests-obj-failed-fb.lst",
                 "log-tests-results-failed-fb.log"):
        (working / name).unlink(missing_ok=True)
    result = run("FAILED_ONLY=1", "all", "results")
    empty_ok = result.returncode == 0 and not summary.read_text()
    print("empty failed selection: " + ("passed" if empty_ok else "FAILED"))
    if not empty_ok:
        print(result.stdout + result.stderr)
    return success and empty_ok


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--fbc", type=Path, required=True)
    parser.add_argument("--make", default=shutil.which("gmake") or shutil.which("make") or "make")
    options = parser.parse_args()
    root = options.root.resolve()
    with tempfile.TemporaryDirectory(prefix="fbc-log-harness-") as temporary:
        if not test_haiku_grep(root, Path(temporary) / "grep"):
            return 1
        if not test_compiler_selection(root, Path(temporary) / "selection", options.make):
            return 1
        if not test_log_recovery(root, Path(temporary) / "recovery", options.make):
            return 1
        working = Path(temporary) / "tests"
        (working / "cases").mkdir(parents=True)
        for name in ("common.mk", "log-tests.mk"):
            shutil.copyfile(root / "tests" / name, working / name)
        (working / "dirlist.mk").write_text("DIRLIST_FB := cases\n", encoding="utf-8")
        fixture = working / "cases/probe.bas"
        fixture.write_text("'' TEST_MODE : COMPILE_ONLY_OK\nend 0\n", encoding="utf-8")
        # Paths passed by tests/common.mk must refer to the actual repository,
        # while generated sources, objects, and logs remain in the fixture.
        # This value is a shell command inside Make, rather than a subprocess
        # argument vector. Forward slashes keep MSYS timeout from consuming
        # Windows path separators; shell quoting also preserves spaces.
        fbc = shlex.join([options.fbc.resolve().as_posix(), "-prefix", root.as_posix(),
                          "-i", (root / "inc").as_posix()])
        command = [options.make, "--no-print-directory", "-s", "-f", "log-tests.mk",
                   "FB_LANG=fb", "FBC=" + fbc, "rootdir=" + root.as_posix()]

        def run(expected_success: bool, label: str, *targets: str) -> bool:
            result = subprocess.run(command + list(targets), cwd=working, text=True,
                                    capture_output=True, timeout=60, check=False)
            if (result.returncode == 0) != expected_success:
                print(label + ": wrong make status\n" + result.stdout + result.stderr)
                return False
            print(label + ": passed")
            return True

        if not run(True, "passing test", "all", "results"):
            return 1
        (working / "cases/probe.log").write_text("", encoding="utf-8")
        (working / "log-tests-results-fb.log").unlink()
        if not run(False, "incomplete record", "results"):
            return 1
        (working / "cases/probe.log").write_text(
            "probe : RESULT=PASSED\nprobe : RESULT=FAILED\n", encoding="utf-8")
        (working / "log-tests-results-fb.log").unlink(missing_ok=True)
        if not run(False, "duplicate records", "results"):
            return 1
        fixture.write_text("'' TEST_MODE : COMPILE_ONLY_OK\n#error deliberate harness failure\n", encoding="utf-8")
        (working / "cases/probe.log").unlink()
        if not run(False, "failing test", "all", "results"):
            return 1

        timeout_tool = shutil.which("timeout")
        if timeout_tool is not None:
            tool = subprocess.run([timeout_tool, "--version"], capture_output=True,
                                  text=True, timeout=5, check=False)
            if tool.returncode == 0 and "GNU coreutils" in tool.stdout:
                fixture.write_text("'' TEST_MODE : COMPILE_ONLY_FAIL\nend 0\n", encoding="utf-8")
                for name in ("cases/probe.log", "log-tests-fb.inc",
                             "log-tests-results-fb.log"):
                    (working / name).unlink(missing_ok=True)
                # This process stands in for a compiler stuck on invalid input.
                # Its timeout must not satisfy COMPILE_ONLY_FAIL's exit-1 rule.
                sleeper = working / "sleep-compiler.sh"
                sleeper.write_text("#!/bin/sh\nexec sleep 5\n", encoding="utf-8")
                sleeper.chmod(0o755)
                if not run(False, "compiler timeout", "FBC=" + shlex.join(["sh", sleeper.as_posix()]),
                           "LOG_TEST_TIMEOUT_TOOL=" + shlex.quote(Path(timeout_tool).as_posix()),
                           "LOG_TEST_TIMEOUT=1", "all", "results"):
                    return 1
                if "unexpected fbc exit code 124" not in (working / "cases/probe.log").read_text():
                    print("compiler timeout: missing timeout record")
                    return 1
            else:
                print("compiler timeout: skipped (GNU timeout unavailable)")
        else:
            print("compiler timeout: skipped (GNU timeout unavailable)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# end of test-compiler-test-harness.py
