#!/usr/bin/env python3
"""FreeBASIC Exampleageddon result contract regressions.
File: test-exampleageddon-contracts.py
Purpose: Exercise target output names and resources passed to guest runners.
Responsibilities: Cross-host products, missing-output rejection and CSV staging.
This file does not claim target compilation or emulator runtime qualification.
"""
import csv
import importlib.util
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("exampleageddon", ROOT / "build_scripts/exampleageddon-freebasic.py")
runner = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = runner
spec.loader.exec_module(runner)


class ResultContracts(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="fbc-example-contract-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.source = self.root / "examples/arrays.bas"
        self.source.parent.mkdir()
        self.source.write_text("print 1\n", encoding="ascii")
        self.output = self.root / "output"
        for child in ("bin", "logs"):
            (self.output / child).mkdir(parents=True, exist_ok=True)
        self.compiler = self.root / "compiler.py"
        self.compiler.write_text(
            "import pathlib, sys\n"
            "output = pathlib.Path(sys.argv[sys.argv.index('-x') + 1])\n"
            "if not output.name.endswith('.exe'):\n"
            "    output = output.with_name(output.name + '.exe')\n"
            "output.write_bytes(b'controlled target product')\n", encoding="ascii")

    def compile(self, target="wince"):
        args = runner.parse_args([
            "--root", str(self.root), "--outdir", str(self.output),
            "--fbc", sys.executable, "--no-prefix", "--target-os", target,
            "--no-run", "--keep-work",
        ])
        args.fbc = [sys.executable, str(self.compiler)]
        return runner.compile_one(self.source, self.root, args)

    def test_cross_target_product_has_target_suffix(self):
        result = self.compile()
        self.assertEqual(result.compile_status, "pass")
        self.assertEqual(result.run_status, "skipped-no-run")
        self.assertTrue(Path(result.output).is_file())
        self.assertTrue(result.output.endswith(".exe"))
        self.assertEqual(runner.executable_name("example", "riscos"), "example")
        self.assertEqual(runner.executable_name("example", "windows"), "example.exe")
        self.assertEqual(runner.executable_name("example", "msys_nt-10.0"), "example.exe")

    def test_zero_exit_without_product_still_fails(self):
        self.compiler.write_text("# Deliberately return zero without linking a product.\n", encoding="ascii")
        result = self.compile()
        self.assertEqual(result.compile_status, "fail(missing-output)")
        self.assertEqual(result.output, "")

    def test_retained_resources_reach_each_guest_manifest(self):
        result = self.compile()
        runner.write_csv([result], self.output / "results.csv")
        directory = self.output / result.compile_directory
        self.assertTrue((directory / self.source.name).is_file())
        # Exercise the actual embedded manifest programs, rather than repeating
        # their path calculation in the test. Relative CSV paths survive mounts.
        wrappers = (
            ("build_scripts/riscos-run-exampleageddon.sh", 'python3 - "$COMPILE_RESULTS" "$OUTPUT_ROOT" "$MANIFEST"'),
            ("build_scripts/aros-run-exampleageddon.sh", 'python3 - "$target_output/results.csv" "$target_output"'),
            ("build_scripts/wince/run-exampleageddon.sh", 'python3 - "$INVENTORY" "$OUTPUT_ROOT" "$ROOT"'),
        )
        for filename, marker in wrappers:
            with self.subTest(wrapper=filename):
                text = (ROOT / filename).read_text(encoding="utf-8")
                begin = text.index("\n", text.index(marker)) + 1
                program = text[begin:text.index("\nPY", begin)]
                manifest = self.output / "manifest.tsv"
                manifest.unlink(missing_ok=True)
                arguments = ([str(self.output / "results.csv"), str(self.output), str(self.root)]
                             if "wince" in filename else
                             [str(self.output / "results.csv"), str(self.output), str(manifest)])
                completed = subprocess.run([sys.executable, "-", *arguments], input=program,
                                           capture_output=True, text=True, timeout=10)
                self.assertEqual(completed.returncode, 0, completed.stdout + completed.stderr)
                with manifest.open(newline="", encoding="utf-8") as stream:
                    row = next(csv.DictReader(stream, delimiter="\t"))
                self.assertEqual(Path(row["compile_directory"]), directory)
                self.assertTrue(Path(row["compile_directory"]).is_dir())

    def test_guest_tasks_pass_the_serialization_filename(self):
        bash = shutil.which("bash")
        if bash is None:
            self.skipTest("Bash is required to generate the guest startup scripts")
        rows = ["0001\texamples/manual/fileio/serialization.bas\tbinary\tcompile",
                "0002\texamples/arrays.bas\tbinary\tcompile"]
        definitions = "BATCH_ROWS=(" + " ".join(shlex.quote(row) for row in rows) + ")\n"
        work = self.root / "guest"
        startup = self.root / "startup"
        definitions += ("EXAMPLE_WORK=" + shlex.quote(work.as_posix()) + "\n"
                        "RUNTIME_LOGS=" + shlex.quote((self.root / "logs").as_posix()) + "\n"
                        "BOOT_TASK=" + shlex.quote((self.root / "boot").as_posix()) + "\n"
                        "TASKWINDOW_MIB=192\nTASKWINDOW_KIB=196608\n")
        for filename, function, invocation, output in (
            ("build_scripts/riscos-run-exampleageddon.sh", "write_boot_task",
             "write_boot_task batch01 key bas", work / "RunBatch,feb"),
            ("build_scripts/aros-run-exampleageddon.sh", "write_startup",
             "write_startup batch01 " + shlex.quote(startup.as_posix()), startup),
        ):
            text = (ROOT / filename).read_text(encoding="utf-8")
            start = text.index(function + "() {")
            body = text[start:text.index("\n}", start) + 2]
            for target in (("riscos",) if "riscos" in filename else ("x86_64", "m68k")):
                with self.subTest(wrapper=filename, target=target):
                    script = definitions + "CURRENT_TARGET=" + target + "\n" + body + "\n" + invocation + "\n"
                    result = subprocess.run([bash, "-s"], input=script, capture_output=True,
                                            text=True, timeout=10)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    lines = output.read_text().splitlines()
                    calls = [line.split(" >", 1)[0].strip() for line in lines
                             if line.startswith(("Run runner", "runner "))]
                    prefix = "Run runner" if target == "riscos" else "runner"
                    self.assertEqual(calls, [prefix + " saved-state", prefix])


if __name__ == "__main__":
    unittest.main(verbosity=2)

# end of test-exampleageddon-contracts.py
