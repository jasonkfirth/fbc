"""Project: FreeBASIC semantic transport qualification
File: test_binary_transport.py
Purpose: Verify producer/consumer agreement and content-based incremental reuse.
Responsibilities: Compare exact facts, reject damaged models, exercise cache invalidation.
This file intentionally does NOT replace the semantic graph regression suite.
"""

from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import time
import unittest

from semantic_binary import MAGIC, decode, read_rows
from sidecar import Model


class BinaryTransportTests(unittest.TestCase):
    root: Path
    compiler: Path
    linter: Path
    linter_root: Path
    link_compiler: Path
    receipts: list[dict] = []

    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix="fbc-binary-")
        self.addCleanup(self.directory.cleanup)
        self.work = Path(self.directory.name)
        self.source = self.work / "sample.bas"
        self.source.write_text(
            "' Project: Semantic transport regression\n"
            "' File: sample.bas\n"
            "' Purpose: Keep compiler facts observable across both transports.\n"
            "' Responsibilities: Exercise macros, calls, arithmetic and includes.\n"
            "' This file intentionally does NOT require external devices.\n"
            '#include once "crt/string.bi"\n'
            '#define DivideValue(a, b) ((a) \\ (b))\n'
            "function Quotient(byval numerator as long, byval divisor as long) as long\n"
            "    return DivideValue(numerator, divisor)\n"
            "end function\n"
            "print Quotient(7, 2)\n"
            "' end of sample.bas\n", encoding="ascii")

    def invoke(self, arguments: list[str], *, expected: int = 0, environment=None) -> subprocess.CompletedProcess:
        result = subprocess.run(arguments, cwd=self.work, env=environment, capture_output=True, text=True, timeout=180)
        self.assertEqual(result.returncode, expected, result.stdout + result.stderr)
        return result

    def compile(self, mode="full", text=False, bundle=False) -> Path:
        path = self.work / (mode + ("-text" if text else "-binary"))
        option = {"full": "-semantic-model", "bindings": "-semantic-model-bindings",
                  "expressions": "-semantic-model-expressions"}[mode]
        if bundle:
            option = "-semantic-bundle"
        command = [str(self.compiler), "-prefix", str(self.root), "-i", str(self.root / "inc"),
                   "-r", "-gen", "gcc", "-o", str(self.work / "module.c"), option, str(path)]
        if text:
            command += ["-semantic-model-text"]
        self.invoke(command + [str(self.source)])
        return path.with_suffix(".fbcsem") if bundle else path

    def lint(self, sources: list[Path], *, extra=(), expected=0, default_profile=False,
             environment=None) -> tuple[dict[str, int], str]:
        start = time.perf_counter()
        selection = ["--profile", "strict"] if default_profile else ["--select", "FBL-NUM-027"]
        configuration = [] if "--config" in extra else ["--no-config"]
        result = self.invoke([str(self.linter), *configuration, "--compiler", str(self.compiler),
                              "--compiler-prefix", str(self.root), *selection,
                              "--require-semantic", "--timings", "--jobs", "1", *extra,
                              *(str(source) for source in sources)], expected=expected, environment=environment)
        output = result.stdout + result.stderr
        values = {key: int(value) for key, value in re.findall(r"([a-z_]+)=\s*(\d+)", output)}
        values["wall_ms"] = round((time.perf_counter() - start) * 1000)
        if expected == 0:
            self.assertEqual(values.get("operational_errors"), 0, output)
            self.assertEqual(values.get("semantic_accepted"), len(sources), output)
        return values, output

    def test_shared_contract_copies(self):
        self.invoke([sys.executable, str(self.root / "build_scripts/update-semantic-binary-contract.py"),
                     "--root", str(self.root), "--linter-root", str(self.linter_root)])
        for filename in ("fbc-semantic-elements.bi", "fbc-semantic-binary.bi"):
            self.assertEqual((self.root / "inc" / filename).read_bytes(),
                             (self.linter_root / "src" / filename).read_bytes(), filename)

    def test_unavailable_compiler_status_stays_structured(self):
        for rule in ("FBL-ENUM-001", "FBL-ENUM-002", "FBL-ENUM-003", "FBL-ENUM-004",
                     "FBL-ENUM-005", "FBL-ENUM-006"):
            for required in (False, True):
                status_option = "--require-semantic" if required else "--show-semantic"
                result = self.invoke([str(self.linter), "--no-config", "--format", "jsonl", "--quiet",
                                      "--select", rule, status_option, "--compiler", str(self.work / "missing.exe"),
                                      str(self.source)], expected=2 if required else 0)
                rows = [json.loads(line) for line in result.stdout.splitlines() if line.strip()]
                statuses = [row for row in rows if row["type"] == "semantic"]
                self.assertEqual(len(statuses), 1, result.stdout)
                self.assertEqual(statuses[0]["reason"], "compiler-not-found", result.stdout)
                self.assertFalse(statuses[0]["accepted"], result.stdout)
                self.assertEqual(rows[-1]["operational_errors"], int(required), result.stdout)

    def test_native_codec_values_and_rollback(self):
        executable = self.work / "codec.exe"
        self.invoke([str(self.link_compiler), "-w", "all", "-exx", "-i", str(self.root / "inc"),
                     "-o", str(self.work / "codec.o"),
                     str(self.root / "tests/semantic-sidecar/test_binary_codec.bas"), "-x", str(executable)])
        result = self.invoke([str(executable), str(self.work / "codec.model")])
        self.assertIn("SEMANTIC_BINARY_CODEC_PASS", result.stdout)

    def test_constructor_observations_survive_binary_transport(self):
        self.source.write_text(
            'type Item\n'
            '    value as long\n'
            '    declare constructor()\n'
            '    declare constructor(byval initial as long)\n'
            'end type\n'
            'constructor Item()\n'
            '    value = 0\n'
            'end constructor\n'
            'constructor Item(byval initial as long)\n'
            '    value = initial\n'
            'end constructor\n'
            'dim named as Item\n'
            'dim converted as Item = 1\n'
            'var temporary = Item(2)\n'
            '#define MAKE_ITEM Item(3)\n'
            'dim expanded as Item = MAKE_ITEM\n'
            'dim elements() as Item\n'
            'redim elements(0 to 1)\n', encoding="ascii")
        for mode in ("full", "bindings", "expressions"):
            with self.subTest(mode=mode):
                binary = self.compile(mode)
                binary_emission = (self.work / "module.c").read_bytes()
                text = self.compile(mode, text=True)
                self.assertEqual((self.work / "module.c").read_bytes(), binary_emission)
                self.assertEqual(decode(binary.read_bytes()), text.read_bytes().replace(b"\r\n", b"\n"))
                model = Model.read(binary, bindings_only=mode == "bindings", expressions_only=mode == "expressions")
                for prefix, feature, width in (("construction-observation", "implicit-construction-inputs", 6),
                                               ("temporary-construction", "temporary-construction-inputs", 4)):
                    rows = [row for row in model.records["K"] if row[3].startswith(prefix + ":")]
                    self.assertEqual(model.capabilities[1][feature], "available" if mode == "full" else "unavailable")
                    if mode == "full":
                        self.assertTrue(rows)
                        self.assertTrue(all(len(row[4].split("\t")) == width for row in rows))
                        closure = [row[4] for row in model.records["K"] if row[3] == prefix + "-count:1"]
                        self.assertEqual(closure, [str(len(rows))])
                    else:
                        self.assertFalse(rows)
                if mode == "full":
                    self.assertTrue(any(row[3].startswith("construction-observation:") and
                                        row[4].split("\t")[2] == "redim-constructor" for row in model.records["K"]))
                    reader = self.build_native_reader()
                    self.invoke([str(reader), str(binary), "valid"])

    def test_virtual_call_assignment_storage_matches_expression(self):
        self.source.write_text(
            "' Project: Semantic transport regression\n' File: sample.bas\n"
            "' Purpose: Retain the source virtual callee in assignment storage.\n"
            "' Responsibilities: Match the original expression and storage facts.\n"
            "' This file intentionally does NOT lower a second executable call.\n"
            'type Phase extends Object\n'
            '    declare virtual function Value() as long\n'
            'end type\n'
            'function Phase.Value() as long\n    return 1\nend function\n'
            'dim instance as Phase\ndim receiver as Phase ptr = @instance\n'
            'dim result as long\nresult = receiver->Value()\nprint result\n'
            "' end of sample.bas\n", encoding="ascii")
        binary = self.compile()
        text = self.compile(text=True)
        self.assertEqual(decode(binary.read_bytes()), text.read_bytes().replace(b"\r\n", b"\n"))
        Model.read(binary)
        Model.read(text)
        reader = self.build_native_reader()
        self.invoke([str(reader), str(binary), "valid"])
        self.invoke([str(reader), str(text), "valid"])

    def test_portable_cache_io_and_lease(self):
        stable = self.work / "stable.bin"
        stable.write_bytes(b"A" * 4096)
        executable = self.work / "cache-io.exe"
        self.invoke([str(self.link_compiler), "-w", "all", "-mt", "-exx", "-o", str(self.work / "cache-io.o"),
                     str(self.linter_root / "test_semantic_io.bas"), "-x", str(executable)])
        arguments = [str(executable), str(stable), hashlib.sha256(stable.read_bytes()).hexdigest(), str(self.work)]
        result = self.invoke(arguments)
        self.assertIn("SEMANTIC_CACHE_PORTABLE_IO_PASS", result.stdout)
        # A second process must miss the busy lease. Killing its owner must
        # release the OS lock without a stale directory or manual recovery.
        holder = subprocess.Popen(arguments + ["hold"], stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                  text=True, encoding="utf-8")
        try:
            self.assertEqual(holder.stdout.readline().strip(), "SEMANTIC_CACHE_LEASE_HELD")
            blocked = self.invoke(arguments + ["probe"], expected=3)
        finally:
            holder.kill()
            holder.communicate(timeout=10)
        probe = self.invoke(arguments + ["probe"])
        self.assertIn("SEMANTIC_CACHE_LEASE_PASS", probe.stdout)

    def test_concurrent_processes_and_prefetch_reuse(self):
        sources = [self.source]
        for index in range(2):
            source = self.work / f"worker{index}.bas"
            source.write_bytes(self.source.read_bytes().replace(b"Quotient", f"Quotient{index}".encode("ascii")))
            sources.append(source)
        options = ("--semantic-cache", str(self.work / "cache"), "--jobs", "4")
        with ThreadPoolExecutor(max_workers=3) as workers:
            results = list(workers.map(lambda _: self.lint(sources, extra=options), range(3)))
        findings = lambda output: [line for line in output.splitlines() if "): " in line]
        for _, output in results[1:]:
            self.assertEqual(findings(results[0][1]), findings(output))
        warm, output = self.lint(sources, extra=options)
        self.assertEqual(warm.get("semantic_compiler_runs"), 0, output)
        self.assertEqual(warm.get("semantic_scan_cache_hits"), len(sources), output)

    def test_exact_facts_in_all_modes(self):
        receipt = {"test": "transport-parity", "modes": {}}
        for mode in ("full", "bindings", "expressions"):
            binary = self.compile(mode)
            text = self.compile(mode, text=True)
            self.assertTrue(binary.read_bytes().startswith(MAGIC))
            self.assertEqual(decode(binary.read_bytes()), text.read_bytes().replace(b"\r\n", b"\n"))
            Model.read(binary, bindings_only=mode == "bindings", expressions_only=mode == "expressions")
            # Host INTEGER width must not change the unsigned datatype wire
            # fields, especially the statement/label invalid-type sentinel.
            for row in read_rows(binary):
                fields = row.split(b"\t")
                dtype_field = {b"S": 4, b"T": 6, b"N": 8, b"E": 12}.get(fields[0])
                if dtype_field is not None:
                    self.assertGreaterEqual(int(fields[dtype_field]), 0)
                    self.assertLessEqual(int(fields[dtype_field]), 0xffffffff)
            receipt["modes"][mode] = {"binary_bytes": binary.stat().st_size, "text_bytes": text.stat().st_size}
        self.receipts.append(receipt)

    def test_dependency_receipts_and_frozen_reader(self):
        def load_script(filename):
            spec = importlib.util.spec_from_file_location(filename, self.root / "build_scripts" / filename)
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            return module

        self.source = self.source.rename(self.work / "sample%literal.bas")
        binary = self.compile()
        text = self.compile(text=True)
        self.assertEqual(list(read_rows(binary)), list(read_rows(text)))
        receipts = load_script("lint-compiler-sources.py")
        sources = receipts.model_sources(binary)
        self.assertEqual(sources, receipts.model_sources(text))
        self.assertIn(str(self.source.resolve()), sources)
        frozen = self.work / "frozen-reader"
        frozen.mkdir()
        load_script("compiler_semantic_audit.py").freeze_semantic_reader(self.root, frozen)
        self.invoke([sys.executable, "-I", "-c",
                     "import sys; from pathlib import Path; sys.path.insert(0, sys.argv[1]); "
                     "from sidecar import Model; Model.read(Path(sys.argv[2]))",
                     str(frozen), str(binary)])
        damaged = self.work / "incomplete.model"
        damaged.write_bytes(binary.read_bytes()[:-1])
        with self.assertRaises(ValueError):
            receipts.model_sources(damaged)

    def build_native_reader(self) -> Path:
        production = (self.linter_root / "src/fb_linter_semantic.bi").read_text(encoding="utf-8")
        pairs = [
            ("FBLINT_MAX_SEMANTIC_SYMBOL_TYPES", "FBLINT_TEST_SYMBOL_LIMIT"),
            ("FBLINT_MAX_SEMANTIC_MODEL_IDENTITIES", "FBLINT_TEST_MODEL_IDENTITY_LIMIT"),
            ("FBLINT_MAX_SEMANTIC_PROCEDURE_PARAMETERS", "FBLINT_TEST_PARAMETER_LIMIT"),
            ("FBLINT_SEMANTIC_MAX_INTEGER", "FBLINT_TEST_INTEGER_LIMIT"),
            *[("FBLINT_SEMANTIC_SYMBOL_CLASS_" + kind, "FBLINT_TEST_" + label + "_CLASS")
              for kind, label in [("VAR", "VARIABLE"), ("CONST", "CONSTANT"), ("PROC", "PROC"),
                                  ("PARAM", "PARAM"), ("FIELD", "FIELD"), ("ENUM", "ENUM"), ("STRUCT", "STRUCT")]],
            ("FBLINT_SEMANTIC_FBC_DATATYPE_ENUM", "FBLINT_TEST_DATATYPE_ENUM"),
        ]
        definitions = []
        for constant, parameter in pairs:
            # Production limits may share one named bound. Resolve only plain
            # decimal literals and constant aliases, without evaluating code.
            current = constant
            visited = set()
            while True:
                self.assertNotIn(current, visited, "Cyclic native reader constant: " + constant)
                visited.add(current)
                match = re.search(r"^Const " + current + r" As LongInt = ([A-Za-z_][A-Za-z_0-9]*|[0-9]+)\s*$",
                                  production, re.M)
                self.assertIsNotNone(match, current)
                value = match.group(1)
                if value.isdecimal():
                    break
                current = value
            definitions += ["-d", parameter + "=" + value]
        executable = self.work / "native-reader.exe"
        self.invoke([str(self.link_compiler), "-w", "all", "-exx", "-i", str(self.linter_root),
                     *definitions, "-o", str(self.work / "native-reader.o"),
                     str(self.linter_root / "test_semantic_schema27.bas"), "-x", str(executable)])
        return executable

    def test_native_field_index_matches_wire(self):
        executable = self.build_native_reader()
        self.invoke([str(executable), str(self.compile()), "valid"])
        self.invoke([str(executable), str(self.compile(text=True)), "valid"])

    def test_native_rejects_malformed_transport(self):
        model = self.compile(bundle=True)
        original = model.read_bytes()
        header_end = original.index(b"DATA\n") + len(b"DATA\n")
        header, body = original[:header_end], original[header_end:]
        first_size, = struct.unpack_from("<I", body)
        first_tag, first_count = body[4:6]
        self.assertEqual(first_count, 1)
        def first_frame(value: bytes) -> bytes:
            record = bytes([first_tag, first_count]) + value
            return header + struct.pack("<I", len(record)) + record + body[first_size + 4:]
        cases = {
            "truncated-header": header[:-3],
            "truncated-frame": original[:-1],
            "zero-length-frame": header + b"\0\0\0\0" + body,
            "oversized-frame": header + b"\xff\xff\xff\x7f" + body[4:],
            "trailing-data": original + b"\0",
            "forward-string-reference": first_frame(b"\x0b"),
            "noncanonical-integer": first_frame(b"\x81\0"),
            "empty-tuple": first_frame(b"\x07"),
            "nested-tuple": first_frame(b"\x17\x17\0\0\0"),
            "oversized-tuple": first_frame(b"\x8f\x10"),
            "tuple-without-capability": original.replace(b"cap\tstructured-values\t1\t1\n", b""),
            "duplicate-metadata": header.replace(b"DATA\n", b"producer\tduplicate\nDATA\n") + body,
            "missing-mode": original.replace(b"mode\tfull\n", b""),
            "unknown-mode": original.replace(b"mode\tfull\n", b"mode\tfuture\n"),
            "missing-layout": original.replace(next(line + b"\n" for line in header.splitlines()
                                                     if line.startswith(b"layout\t")), b""),
            "empty-capability": header.replace(b"DATA\n", b"cap\t\t1\t0\nDATA\n") + body,
            "invalid-capability-version": header.replace(b"DATA\n", b"cap\tfuture\t\t0\nDATA\n") + body,
            "header-control-byte": original.replace(b"dictionary\t", b"dictionary\t\x01", 1),
            "required-capability": header.replace(b"DATA\n", b"cap\tfuture-domain\t1\t1\nDATA\n") + body,
            "missing-capability": original.replace(b"cap\tsource-revisions\t1\t1\n", b""),
            "record-contract": original.replace(b"\tM\t2\t", b"\tM\t3\t", 1),
        }
        bundle = str(model.with_suffix(""))
        bundle_options = ("--semantic-bundle", bundle, "--semantic-root", str(self.source), "--no-semantic-cache")
        for name, damaged in cases.items():
            with self.subTest(name=name):
                model.write_bytes(damaged)
                with self.assertRaises(ValueError):
                    Model.read(model)
                values, output = self.lint([self.source], extra=bundle_options, expected=2)
                self.assertEqual(values.get("semantic_accepted"), 0, output)
                if name == "required-capability":
                    self.assertIn("unsupported-capability:future-domain", output)
        model.write_bytes(original.replace(b"DATA\n", b"cap\tfuture-domain\t1\t0\nDATA\n", 1))
        Model.read(model)
        self.lint([self.source], extra=bundle_options)

    def test_incremental_content_and_include_resolution(self):
        self.exercise_incremental(result_cache=False)

    def test_incremental_analysis_results(self):
        self.exercise_incremental(result_cache=True)

    def exercise_incremental(self, *, result_cache: bool):
        second = self.work / "second.bas"
        second.write_bytes(self.source.read_bytes().replace(b"Quotient", b"OtherQuotient"))
        sdk = self.work / "sdk"
        sdk.mkdir()
        header = sdk / "shared.bi"
        header.write_text("#define SHARED_NUMBER 7\n", encoding="ascii")
        for source in (self.source, second):
            source.write_bytes(b'#include once "shared.bi"\n' + source.read_bytes())
        options = ("--semantic-cache", str(self.work / "cache"), "--compiler-include", str(sdk))
        if not result_cache:
            options += ("--no-semantic-result-cache",)
        sources = [self.source, second]
        results = {}
        for phase in ("cold", "warm", "one-file", "include", "shadow", "project-index", "corrupt"):
            if phase == "one-file":
                metadata = self.source.stat()
                self.source.write_bytes(self.source.read_bytes().replace(b"Quotient(7, 2)", b"Quotient(9, 2)"))
                os.utime(self.source, ns=(metadata.st_atime_ns, metadata.st_mtime_ns))
            elif phase == "include":
                metadata = header.stat()
                header.write_text("#define SHARED_NUMBER 9\n", encoding="ascii")
                os.utime(header, ns=(metadata.st_atime_ns, metadata.st_mtime_ns))
            elif phase == "shadow":
                (self.work / "shared.bi").write_text("#define SHARED_NUMBER 5\n", encoding="ascii")
            elif phase == "project-index":
                second.write_bytes(second.read_bytes().replace(b"OtherQuotient", b"SecondQuotient"))
            elif phase == "corrupt":
                pattern = "scan-*.findings" if result_cache else "*.model"
                entries = sorted((self.work / "cache" / "fblint-v2").glob(pattern),
                                 key=lambda item: item.stat().st_mtime_ns, reverse=True)
                self.assertGreaterEqual(len(entries), 2)
                damaged = bytearray(entries[0].read_bytes())
                damaged[-1] ^= 1
                entries[0].write_bytes(damaged)
            values, output = self.lint(sources, extra=options)
            expected_runs = {"cold": 2, "warm": 0, "one-file": 1, "include": 2, "shadow": 2,
                             "project-index": 1, "corrupt": 1}[phase]
            if result_cache and phase == "corrupt":
                expected_runs = 0
            self.assertEqual(values.get("semantic_compiler_runs"), expected_runs, output)
            hits = values.get("semantic_disk_cache_hits", 0) + values.get("semantic_scan_cache_hits", 0)
            self.assertEqual(hits, 2 - expected_runs, output)
            if result_cache and phase in ("warm", "one-file"):
                self.assertEqual(values.get("semantic_scan_cache_hits"), 2 - expected_runs, output)
            self.assertEqual(values.get("warnings"), 2, output)
            results[phase] = values
        self.receipts.append({"test": "incremental-results" if result_cache else "incremental-models", "phases": results})

    def test_larger_module_analysis_reuse(self):
        functions = [
            f"function Quotient{index}(byval number as long) as long\n"
            "    return number \\ 2\nend function\n"
            for index in range(128)
        ]
        self.source.write_text("\n".join(functions) + '#include once "crt/string.bi"\nprint Quotient0(9)\n', encoding="ascii")
        binary = self.compile()
        text = self.compile(text=True)
        parsed = Model.read(binary)
        self.assertGreater(max(parsed.files), 5000)
        options = ("--semantic-cache", str(self.work / "cache"))
        cold, cold_output = self.lint([self.source], extra=options)
        warm, warm_output = self.lint([self.source], extra=options)
        self.assertEqual(cold.get("semantic_compiler_runs"), 1, cold_output)
        self.assertEqual(warm.get("semantic_compiler_runs"), 0, warm_output)
        self.assertEqual(warm.get("semantic_scan_cache_hits"), 1, warm_output)
        self.assertEqual([line for line in cold_output.splitlines() if ": warning " in line],
                         [line for line in warm_output.splitlines() if ": warning " in line])
        self.receipts.append({"test": "larger-module", "binary_bytes": binary.stat().st_size,
                              "text_bytes": text.stat().st_size, "cold": cold, "warm": warm})

    def test_default_profile_cached_findings_match(self):
        options = ("--semantic-cache", str(self.work / "cache"))
        cold, cold_output = self.lint([self.source], extra=options, default_profile=True)
        warm, warm_output = self.lint([self.source], extra=options, default_profile=True)
        self.assertEqual(cold.get("semantic_compiler_runs"), 1, cold_output)
        self.assertEqual(warm.get("semantic_compiler_runs"), 0, warm_output)
        self.assertEqual(warm.get("semantic_scan_cache_hits"), 1, warm_output)
        findings = lambda text: [line for line in text.splitlines() if "): " in line]
        self.assertEqual(findings(cold_output), findings(warm_output))
        self.receipts.append({"test": "default-profile", "cold": cold, "warm": warm})

    def test_shorter_cached_findings_replace_previous_bytes(self):
        self.source.write_text(
            "function Quotient(byval number as long) as long\n"
            "    return number \\ 2\nend function\nprint Quotient(7)\n", encoding="ascii")
        options = ("--semantic-cache", str(self.work / "cache"))
        first, output = self.lint([self.source], extra=options)
        self.assertEqual(first.get("warnings"), 1, output)
        self.source.write_bytes(self.source.read_bytes().replace(b"number \\ 2", b"number"))
        changed, output = self.lint([self.source], extra=options)
        self.assertEqual(changed.get("warnings"), 0, output)
        self.assertEqual(changed.get("semantic_scan_cache_stores"), 1, output)
        warm, output = self.lint([self.source], extra=options)
        self.assertEqual(warm.get("warnings"), 0, output)
        self.assertEqual(warm.get("semantic_scan_cache_hits"), 1, output)
        self.assertEqual(warm.get("semantic_compiler_runs"), 0, output)

    def test_cache_configuration_request_and_environment_identity(self):
        config = self.work / "lint.cfg"
        config.write_text("", encoding="ascii")
        options = ("--semantic-cache", str(self.work / "cache"), "--config", str(config))
        self.lint([self.source], extra=options)
        warm, output = self.lint([self.source], extra=options)
        self.assertEqual(warm.get("semantic_scan_cache_hits"), 1, output)
        # A valid configuration with different bytes invalidates findings,
        # while the unchanged compiler request can reuse its artifact.
        config.write_text("\n", encoding="ascii")
        changed, output = self.lint([self.source], extra=options)
        self.assertEqual(changed.get("semantic_scan_cache_hits"), 0, output)
        self.assertEqual(changed.get("semantic_disk_cache_hits"), 1, output)
        self.assertEqual(changed.get("semantic_compiler_runs"), 0, output)
        options += ("--compiler-define", "CACHE_REVISION=2")
        changed, output = self.lint([self.source], extra=options)
        self.assertEqual(changed.get("semantic_compiler_runs"), 1, output)
        environment = {**os.environ, "FBLINT_CACHE_TEST_STAMP": "different-environment"}
        changed, output = self.lint([self.source], extra=options, environment=environment)
        self.assertEqual(changed.get("semantic_compiler_runs"), 1, output)
        warm, output = self.lint([self.source], extra=options, environment=environment)
        self.assertEqual(warm.get("semantic_scan_cache_hits"), 1, output)
        uncached, output = self.lint([self.source], extra=(*options, "--no-semantic-cache"))
        self.assertEqual(uncached.get("semantic_compiler_runs"), 1, output)
        self.assertEqual(uncached.get("semantic_scan_cache_hits"), 0, output)
        self.assertEqual(uncached.get("semantic_disk_cache_stores"), 0, output)

    def test_shared_include_audit_is_deduplicated(self):
        sources = [self.source]
        for index in range(3):
            source = self.work / f"other{index}.bas"
            source.write_bytes(self.source.read_bytes().replace(b"Quotient", f"Quotient{index}".encode("ascii")))
            sources.append(source)
        options = ("--semantic-cache", str(self.work / "cache"))
        self.lint(sources, extra=options)
        warm, output = self.lint(sources, extra=options)
        unique_sources = set()
        for manifest in (self.work / "cache/fblint-v2").glob("scan-*.sources"):
            for line in manifest.read_text(encoding="ascii").splitlines():
                fields = line.split("\t")
                if fields[0] == "F":
                    unique_sources.add(fields[1])
        self.assertGreater(len(unique_sources), len(sources))
        self.assertEqual(warm.get("semantic_scan_cache_hits"), len(sources), output)
        self.assertEqual(warm.get("semantic_cache_source_hashes"), 2 * len(unique_sources), output)

    def test_volatile_predefined_inputs_are_not_cached(self):
        self.source.write_text('#define ReadClock __TIME__\nprint ReadClock\n', encoding="ascii")
        options = ("--semantic-cache", str(self.work / "cache"))
        for _ in range(2):
            values, output = self.lint([self.source], extra=options)
            self.assertEqual(values.get("semantic_compiler_runs"), 1, output)
            self.assertEqual(values.get("semantic_disk_cache_stores"), 0, output)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--fbc", type=Path, required=True)
    parser.add_argument("--linter", type=Path, required=True)
    parser.add_argument("--linter-root", type=Path, default=Path("C:/fblint"))
    parser.add_argument("--link-fbc", type=Path, default=Path("C:/FreeBASIC/fbc64.exe"))
    parser.add_argument("--receipt", type=Path)
    parser.add_argument("--test", action="append")
    arguments = parser.parse_args()
    BinaryTransportTests.root = arguments.root.resolve()
    BinaryTransportTests.compiler = arguments.fbc.resolve()
    BinaryTransportTests.linter = arguments.linter.resolve()
    BinaryTransportTests.linter_root = arguments.linter_root.resolve()
    BinaryTransportTests.link_compiler = arguments.link_fbc.resolve()
    suite = (unittest.TestSuite(BinaryTransportTests(name) for name in arguments.test)
             if arguments.test else unittest.defaultTestLoader.loadTestsFromTestCase(BinaryTransportTests))
    result = unittest.TextTestRunner(verbosity=2, stream=sys.stdout).run(suite)
    if arguments.receipt:
        arguments.receipt.write_text(json.dumps(BinaryTransportTests.receipts, indent=2) + "\n")
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    raise SystemExit(main())

# end of test_binary_transport.py
