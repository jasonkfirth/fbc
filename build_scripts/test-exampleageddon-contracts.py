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


if __name__ == "__main__":
    unittest.main(verbosity=2)

# end of test-exampleageddon-contracts.py
