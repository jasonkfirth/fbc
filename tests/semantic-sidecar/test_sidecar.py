"""Project: FreeBASIC semantic sidecar tests
File: test_sidecar.py
Purpose: Verify exported compiler facts and their deliberate exclusions.
Responsibilities: Exercise every record family, source provenance, and transactions.
This file intentionally does NOT contain: alternate BASIC parsing or name resolution.
"""

from __future__ import annotations

from collections import Counter
import codecs
import hashlib
from pathlib import Path
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
import unittest

from sidecar import SCHEMA, DETAIL_TAGS, PROVENANCE_TAGS, IMPLICIT_KINDS, PRIMITIVE_NAMES, Model, source_range, unescape


class SidecarTests(unittest.TestCase):
    root: Path
    compiler: Path
    toolchain_prefix: Path
    backends: list[str]
    native_windows: bool

    @classmethod
    def setUpClass(cls) -> None:
        result = subprocess.run([str(cls.compiler), "-print", "host"], text=True,
                                capture_output=True, timeout=30, check=True)
        cls.native_windows = result.stdout.strip().split("-", 1)[0] in ("win32", "win64")

    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory(prefix="fbc-semantic-")
        self.addCleanup(self.temporary.cleanup)
        self.working = Path(self.temporary.name)
        self.sequence = 0
        self.compiler_paths: dict[Path, str] = {}

    def compiler_path(self, path: Path) -> str:
        # MSYS2 converts process arguments, but cannot convert paths recorded
        # in a native compiler's output. Compare with the compiler's spelling
        # without rewriting the exported facts or confusing host and target.
        if not self.native_windows:
            return str(path)
        if os.name == "nt":
            return str(path).replace("/", "\\")
        if path not in self.compiler_paths:
            converted = subprocess.run(["cygpath", "-a", "-w", str(path)], text=True,
                                       capture_output=True, timeout=10, check=True)
            self.compiler_paths[path] = converted.stdout.rstrip("\r\n")
            self.assertTrue(self.compiler_paths[path], "cygpath returned an empty path")
        return self.compiler_paths[path]

    def create_symlink(self, target: str, destination: Path) -> None:
        if self.native_windows and sys.platform == "cygwin":
            # MSYS2's default symlink mode copies the target. Start ln with
            # strict native links so the fixture tests an actual alias.
            variable = "MSYS" if "MSYSTEM" in os.environ else "CYGWIN"
            environment = dict(os.environ)
            flags = [flag for flag in environment.get(variable, "").split()
                     if not flag.startswith("winsymlinks:")]
            environment[variable] = " ".join(flags + ["winsymlinks:nativestrict"])
            linked = subprocess.run(["ln", "-s", "--", target, str(destination)],
                                    env=environment, text=True, capture_output=True,
                                    timeout=10, check=False)
            if linked.returncode:
                self.skipTest("Native Windows symlinks are unavailable: " + linked.stderr.strip())
            self.assertTrue(destination.is_symlink(), "ln did not create a symlink")
        else:
            try:
                destination.symlink_to(target)
            except OSError as error:
                if self.native_windows:
                    self.skipTest("Native Windows symlinks are unavailable: " + str(error))
                raise

    def fixture(self, name: str) -> Path:
        target = self.working / name
        shutil.copyfile(self.root / "tests/semantic-sidecar" / name, target)
        return target

    def source(self, text: str, name: str = "probe.bas") -> Path:
        path = self.working / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")
        return path

    def invoke(self, sources: list[Path], *, mode: str = "full", backend: str = "gcc",
               success: bool = True, extra: tuple[str, ...] = (), emit: bool = True,
               environment: dict[str, str] | None = None
               ) -> tuple[subprocess.CompletedProcess[str], Path]:
        self.sequence += 1
        model = self.working / f"model-{self.sequence}.tsv"
        command = [str(self.compiler), "-prefix", str(self.toolchain_prefix), "-i", str(self.root / "inc"),
                   "-gen", backend, "-maxerr", "20"]
        if backend == "gas" and "-target" not in extra:
            command += ["-target", "linux-x86"]
        if emit:
            command.append("-r")
        if mode != "off":
            option = {
                "full": "-semantic-model",
                "bindings": "-semantic-model-bindings",
                "expressions": "-semantic-model-expressions",
            }[mode]
            command += [option, str(model)]
        command += [*extra, *(str(path) for path in sources)]
        result = subprocess.run(command, cwd=self.working, text=True, capture_output=True,
                                timeout=120, check=False,
                                env=None if environment is None else {**os.environ, **environment})
        if success:
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        else:
            self.assertNotEqual(result.returncode, 0, "Invalid fixture compiled successfully")
            self.assertEqual(result.returncode, 1, "Compiler did not report a normal source error: "
                             + result.stdout + result.stderr)
        return result, model

    def compile(self, source: Path, *, mode: str = "full", backend: str = "gcc",
                extra: tuple[str, ...] = ()) -> Model:
        _, path = self.invoke([source], mode=mode, backend=backend, extra=extra)
        return Model.read(path, expressions_only=mode == "expressions",
                          bindings_only=mode == "bindings")

    def one(self, model: Model, name: str, kind: str | None = None) -> int:
        identities = model.named(name, kind)
        self.assertEqual(len(identities), 1, (name, kind, identities))
        return identities[0]

    def span(self, path: Path, anchor: str, token: str | None = None,
             occurrence: int = 0) -> tuple[str, int, int, int, int]:
        lines = path.read_text(encoding="utf-8-sig").splitlines()
        matches = [(index, line) for index, line in enumerate(lines, 1) if anchor in line]
        self.assertGreater(len(matches), occurrence, anchor)
        line_number, line = matches[occurrence]
        start = line.index(anchor)
        if token is not None:
            start = line.index(token, start)
        else:
            token = anchor
        start_column = len(line[:start].encode("utf-16-le")) // 2
        end_column = start_column + len(token.encode("utf-16-le")) // 2
        return self.compiler_path(path), line_number, start_column, line_number, end_column

    def binding(self, model: Model, span: tuple[str, int, int, int, int],
                role: str | None = None) -> int:
        matches = [row for row in model.records["B"]
                   if source_range(row, 4) == span and (role is None or row[2] == role)]
        self.assertTrue(matches, f"Missing {role or ''} binding at {span}")
        self.assertEqual(len({row[1] for row in matches}), 1, matches)
        self.assertTrue(all(row[3] == "1" for row in matches), matches)
        return int(matches[0][1])

    def expressions(self, model: Model, span: tuple[str, int, int, int, int]) -> list[list[str]]:
        return [row for row in model.records["E"] if source_range(row, 3) == span]

    def assert_physical_ranges(self, model: Model, sources: dict[str, str]) -> None:
        sources = {self.compiler_path(Path(path)): text for path, text in sources.items()}
        for tag, flag_column, range_column in (("B", 3, 4), ("E", 2, 3), ("I", 5, 6), ("O", 4, 5)):
            for row in model.records[tag]:
                if row[flag_column] != "1":
                    continue
                path, start_line, start_column, end_line, end_column = source_range(row, range_column)
                self.assertIn(path, sources, row)
                lines = sources[path].splitlines()
                self.assertLessEqual(end_line, len(lines), row)
                self.assertLessEqual(start_column, len(lines[start_line - 1].encode("utf-16-le")) // 2, row)
                self.assertLessEqual(end_column, len(lines[end_line - 1].encode("utf-16-le")) // 2, row)

    def test_record_shapes_references_and_totals(self) -> None:
        for backend in self.backends:
            for name in ("bindings.bas", "types.bas", "procedures.bas", "lifetimes.bas", "control-flow.bas"):
                with self.subTest(backend=backend, fixture=name):
                    source = self.fixture(name)
                    model = self.compile(source, backend=backend)
                    self.assertEqual(model.footer[11], "1")
                    self.assertTrue(model.records["T"])
                    if name == "types.bas":
                        self.assertTrue(model.records["NT"])
                    if name == "control-flow.bas":
                        self.assertTrue(model.records["PH"])
                    self.assert_physical_ranges(model, {str(source): source.read_text()})

    def test_vocabulary_covers_compiler_symbol_and_ast_classes(self) -> None:
        for declaration, exporter, enumeration, prefix in (
            ("ast/ast.bi", "semantic-nodes.bas", "AST_NODECLASS", "AST_NODECLASS_"),
            ("symbols/symb.bi", "semantic-symbols.bas", "FB_SYMBCLASS", "FB_SYMBCLASS_"),
        ):
            with self.subTest(enumeration=enumeration):
                definitions = (self.root / "src/compiler" / declaration).read_text()
                block = re.search(r"^enum " + enumeration + r"\b(.*?)^end enum",
                                  definitions, re.MULTILINE | re.DOTALL)
                self.assertIsNotNone(block)
                classes = set(re.findall(r"^\s*(" + prefix + r"[A-Z0-9_]+)\b", block[1], re.MULTILINE))
                vocabulary = (self.root / "src/compiler/tooling" / exporter).read_text()
                names = set(re.findall(r"case (" + prefix + r"[A-Z0-9_]+): return", vocabulary))
                self.assertEqual(names, classes)
        definitions = (self.root / "src/compiler/symbols/symb.bi").read_text()
        block = re.search(r"^enum FB_DATATYPE\b(.*?)^end enum",
                          definitions, re.MULTILINE | re.DOTALL)
        self.assertIsNotNone(block)
        datatypes = re.findall(r"^\s*FB_DATATYPE_[A-Z0-9_]+\b", block[1], re.MULTILINE)
        self.assertEqual(len(datatypes), len(PRIMITIVE_NAMES))

    def test_declarations_references_and_scope_shadowing(self) -> None:
        source = self.fixture("bindings.bas")
        model = self.compile(source)
        library = self.binding(model, self.span(source, "namespace Library", "Library"), "declaration")
        self.assertEqual(self.binding(model, self.span(source, "using Library", "Library"), "reference"), library)
        field = self.binding(model, self.span(source, "item.value = outer", "value"), "reference")
        self.assertEqual(self.binding(model, self.span(source, ".value += 1", "value"), "reference"), field)
        self.assertEqual(model.types[field][3], "field")
        outer = self.binding(model, self.span(source, "dim outer as long", "outer"), "declaration")
        inner = self.binding(model, self.span(source, "dim outer as double", "outer"), "declaration")
        self.assertNotEqual(outer, inner)
        self.assertEqual(self.binding(model, self.span(source, "print outer", "outer", 0)), inner)
        self.assertEqual(self.binding(model, self.span(source, "print outer", "outer", 1)), outer)
        self.assertNotEqual(model.types[outer][8], model.types[inner][8])
        self.assertEqual(model.types[int(model.types[inner][8])][3], "scope")
        iterator = self.binding(model, self.span(source, "for iterator", "iterator"), "declaration")
        self.assertEqual(self.binding(model, self.span(source, "next iterator", "iterator")), iterator)
        self.assertEqual(self.binding(model, self.span(source, "mov eax, outer", "outer")), outer)
        for token in ("mov", "asm_local"):
            span = self.span(source, "mov eax" if token == "mov" else "jmp asm_local", token)
            self.assertFalse([row for row in model.records["B"] if source_range(row, 4) == span])

    def test_bindings_only_mode_keeps_resolved_calls_without_ast_details(self) -> None:
        source = self.fixture("lifetimes.bas")
        model = self.compile(source, mode="bindings")

        self.assertTrue(model.records["S"])
        self.assertTrue(model.records["B"])
        self.assertTrue(model.records["I"])
        for tag in ("P", "V", "N", "E"):
            self.assertFalse(model.records[tag], tag)
        self.assertFalse(any(model.records[tag] for tag in DETAIL_TAGS - PROVENANCE_TAGS))
        self.assertTrue(model.records["FILE"])
        self.assertTrue(model.records["SRC"])
        self.assertTrue(model.records["SRE"])

    def test_namespace_members_and_import_relationships(self) -> None:
        model = self.compile(self.fixture("bindings.bas"))
        library = self.one(model, "Library", "namespace")
        item = self.one(model, "Item", "type")
        field = self.one(model, "value", "field")
        self.assertEqual(int(model.types[item][8]), library)
        self.assertEqual(int(model.types[field][8]), item)
        imports = model.relations("imports")
        self.assertTrue(any(int(row[4]) == library for row in imports))
        self.assertTrue(all(model.types[int(row[2])][3] == "namespace-import" for row in imports))
        alias = self.one(model, "ItemAlias", "typedef")
        self.assertEqual(int(model.types[alias][7]), item)

    def test_finalized_layouts_inheritance_and_visibility(self) -> None:
        model = self.compile(self.fixture("types.bas"))
        packed = self.one(model, "Packed", "type")
        self.assertEqual(model.types[packed][13], "13")
        self.assertEqual(model.layouts[packed][4:6], ["1", "13"])
        fields = {row[2].lower(): row for row in model.types.values()
                  if row[3] == "field" and int(row[8]) == packed}
        self.assertEqual({name: int(row[14]) for name, row in fields.items()},
                         {"first": 0, "second": 1, "third": 5})
        overlay = self.one(model, "Overlay", "union")
        self.assertEqual(model.types[overlay][13], "4")
        self.assertTrue(all(row[14] == "0" for row in model.types.values()
                            if row[3] == "field" and int(row[8]) == overlay))
        base = self.one(model, "BaseType", "type")
        derived = self.one(model, "DerivedType", "type")
        self.assertEqual(int(model.layouts[derived][3]), base)
        self.assertEqual(model.types[self.one(model, "private_value", "field")][16], "private")
        self.assertEqual(model.types[self.one(model, "protected_value", "field")][16], "protected")
        self.assertTrue(model.relations("overrides"))
        bits = self.one(model, "Bits", "type")
        bit_fields = {row[2].lower(): int(row[1]) for row in model.types.values()
                      if row[3] == "field" and int(row[8]) == bits}
        self.assertEqual(model.properties["symbol", bit_fields["first"]]["bit-width"], "3")
        self.assertEqual(model.properties["symbol", bit_fields["second"]]["bit-position"], "3")
        alias = self.one(model, "ForwardAlias", "typedef")
        later = self.one(model, "LaterType", "type")
        self.assertEqual(int(model.types[alias][7]), later)
        self.assertTrue(any(int(row[4]) == later for row in model.relations("canonical-symbol")))

    def test_array_rank_bounds_and_type_categories(self) -> None:
        model = self.compile(self.fixture("types.bas"))
        fixed = self.one(model, "fixed_array", "variable")
        rows = [row[2:] for row in model.records["A"] if int(row[1]) == fixed]
        self.assertEqual(rows, [["2", "0", "fixed", "-2", "2"], ["2", "1", "fixed", "3", "5"]])
        dynamic = [identity for identity in model.named("dynamic_array", "variable")
                   if model.types[identity][4] == "numeric"]
        self.assertEqual(len(dynamic), 1)
        self.assertTrue(any(row[1] == str(dynamic[0]) and row[4:] == ["runtime", "", ""]
                            for row in model.records["A"]))
        expected = {"text_value": "dynamic-string", "unicode_value": "dynamic-string",
                    "fixed_text": "fixed-string", "byte_text": "fixed-string",
                    "wide_text": "fixed-string", "packed_value": "aggregate",
                    "pointer_value": "pointer", "callback_value": "pointer"}
        for name, kind in expected.items():
            self.assertEqual(model.types[self.one(model, name, "variable")][4], kind, name)
        pointer = self.one(model, "pointer_value", "variable")
        self.assertEqual(model.types[pointer][5], "const long ptr")
        external = self.one(model, "external_value", "variable")
        self.assertEqual(model.types[external][15:18], ["external%name", "public", "external"])

    def test_exact_constant_values_and_enum_elements(self) -> None:
        model = self.compile(self.fixture("types.bas"))
        expected = {"SignedValue": ("signed", "-9223372036854775807"),
                    "UnsignedValue": ("unsigned", "18446744073709551615"),
                    "FloatingValue": ("float64-bits", "0x3FF8000000000000"),
                    "ByteText": ("bytes", "41004209250A"),
                    "WideText": ("wide-units", "000000410000000000000042")}
        for name, value in expected.items():
            self.assertEqual(model.constants["symbol", self.one(model, name, "constant")], value, name)
        bindings = self.compile(self.fixture("bindings.bas"))
        shade = self.one(bindings, "Shade", "enum")
        self.assertEqual(bindings.layouts[shade][9], "2")
        self.assertEqual(bindings.constants["symbol", self.one(bindings, "Dark", "constant")], ("signed", "-3"))

    def test_procedure_signatures_defaults_and_canonical_parameters(self) -> None:
        model = self.compile(self.fixture("procedures.bas"))
        modes = self.one(model, "Modes", "procedure")
        self.assertEqual(model.signatures[modes][2:7], ["sub", "cdecl", "4", "1", "0"])
        parameters = model.parameters[modes]
        self.assertEqual([parameters[index][4] for index in range(4)], ["byval", "byref", "bydesc", "byval"])
        self.assertEqual(parameters[3][5], "1")
        self.assertEqual(parameters[2][6], "-1")
        self.assertTrue(all(int(row[7]) in model.symbols for row in parameters.values()))
        optional = int(parameters[3][2])
        self.assertTrue(any(int(row[2]) == optional and row[3] == "node"
                            for row in model.relations("default-initializer")))
        variadic = self.one(model, "Variadic", "procedure")
        self.assertEqual(model.parameters[variadic][1][4], "vararg")
        reference = self.one(model, "RefResult", "procedure")
        self.assertEqual(model.signatures[reference][6], "1")
        canonical = model.relations("canonical-symbol")
        self.assertTrue(canonical)
        for row in canonical:
            self.assertNotEqual(row[2], row[4])
        choices = model.named("Choose", "procedure")
        self.assertEqual(len(choices), 2)
        self.assertTrue(any(int(row[2]) in choices and int(row[4]) in choices
                            for row in model.relations("overload-next")))

    def test_selected_overloads_addresses_and_indirect_call_exclusion(self) -> None:
        source = self.fixture("procedures.bas")
        model = self.compile(source)
        choices = {model.types[identity][5]: identity for identity in model.named("Choose", "procedure")}
        self.assertEqual(self.binding(model, self.span(source, "Choose(value)", "Choose")), choices["long"])
        self.assertEqual(self.binding(model, self.span(source, "Choose(reference)", "Choose")), choices["double"])
        self.assertEqual(self.binding(model, self.span(source, "= @Choose", "Choose")), choices["long"])
        callback_span = self.span(source, "callback(value)", "callback")
        callback = self.binding(model, callback_span)
        self.assertEqual(model.types[callback][3], "variable")
        self.assertNotIn(callback, choices.values())
        calls = [identity for identity in model.nodes
                 if model.properties["node", identity].get("call-kind") == "indirect"]
        self.assertTrue(calls)
        self.assertTrue(all(model.signatures[int(model.nodes[identity][9])][2] == "procedure-pointer"
                            for identity in calls))

    def test_interned_callback_headers_preserve_parameter_bindings(self) -> None:
        source = self.source(
            "type Payload\n value as long\nend type\n"
            "type Dispatch\n"
            " first as function cdecl(byval initial as Payload ptr, byref label as string, "
            "values() as long, byval amount as long = 3) as long\n"
            " second as function cdecl(byval nextvalue as Payload ptr, byref nextlabel as string, "
            "nextvalues() as long, byval nextamount as long = 3) as long\n"
            " distinct as function cdecl(byval item as Payload ptr, byref text as string, "
            "items() as long, byval count as long = 4) as long\n"
            " variadicfirst as function cdecl(byval formatptr as zstring ptr, ...) as long\n"
            " variadicsecond as function cdecl(byval nextformat as zstring ptr, ...) as long\n"
            "end type\n")
        for backend in self.backends:
            with self.subTest(backend=backend):
                model = self.compile(source, backend=backend)
                signatures = {name: int(model.types[self.one(model, name, "field")][7])
                              for name in ("first", "second", "distinct", "variadicfirst", "variadicsecond")}
                self.assertEqual(signatures["first"], signatures["second"])
                self.assertNotEqual(signatures["first"], signatures["distinct"])
                self.assertEqual(signatures["variadicfirst"], signatures["variadicsecond"])
                replacements = {int(row[2]): int(row[4]) for row in model.relations("canonical-symbol")}
                for ordinal, (name, mode) in enumerate((("nextvalue", "byval"), ("nextlabel", "byref"),
                                                      ("nextvalues", "bydesc"), ("nextamount", "byval"))):
                    temporary = self.binding(model, self.span(source, " second as", name), "declaration")
                    selected = model.parameters[signatures["first"]][ordinal]
                    self.assertEqual(replacements[temporary], int(selected[2]))
                    self.assertEqual(selected[4], mode)
                    owner = int(model.symbols[temporary][11])
                    self.assertEqual(replacements[owner], signatures["first"])
                temporary = self.binding(model, self.span(source, " variadicsecond as", "nextformat"), "declaration")
                self.assertEqual(replacements[temporary], int(model.parameters[signatures["variadicfirst"]][0][2]))
                self.assertEqual(model.parameters[signatures["variadicfirst"]][1][4], "vararg")

    def test_llvm_escaped_literal_padding_uses_decoded_length(self) -> None:
        if "llvm" not in self.backends:
            self.skipTest("LLVM backend is not selected")
        source = self.source(
            "declare sub Take(byval text as wstring ptr)\n"
            'Take(!"\\u0041\\u0042\\u0043\\u0044")\n'
            'Take(!"\\u0000")\n'
            'dim shared PaddedText as wstring * 9 = !"\\u0041"\n'
            'Take(PaddedText)\n')
        expected = {text.encode("utf-32-le") for text in ("ABCD", "A", "")}
        baseline = None
        for mode in ("off", "full", "expressions"):
            with self.subTest(mode=mode):
                # glibc's value 165 fills fresh allocations with 0x5A. This
                # exposes padding reads that fresh zero-filled pages can hide.
                _, path = self.invoke([source], mode=mode, backend="llvm",
                                      extra=("-target", "linux-x86_64"),
                                      environment={"MALLOC_PERTURB_": "165"})
                if mode != "off":
                    Model.read(path, expressions_only=mode == "expressions")
                code = source.with_suffix(".ll").read_text()
                arrays = re.findall(r'^(@[^ ]+) = .*\[(\d+) x i8\] c"([^"\n]*)"(?:, align (\d+))?$',
                                    code, re.MULTILINE)
                arrays = [row for row in arrays if row[3] == "4" or row[0].startswith("@PADDEDTEXT")]
                self.assertGreaterEqual(len(arrays), 3)
                for _, size, literal, _ in arrays:
                    tokens = re.findall(r"\\[0-9A-F]{2}|[^\\]", literal)
                    data = bytes(int(token[1:], 16) if token.startswith("\\") else ord(token)
                                 for token in tokens)
                    self.assertEqual(len(data), int(size))
                    self.assertIn(data.rstrip(b"\0"), {value.rstrip(b"\0") for value in expected})
                if baseline is not None:
                    self.assertEqual(code, baseline)
                baseline = code

    def test_address_nodes_never_read_recycled_branch_payloads(self) -> None:
        # Repeated bodies recycle AST pool slots from calls, branches, and
        # debug nodes before new address expressions occupy the same slots.
        source = self.source("declare sub Observe(byref value as long)\n" + "\n".join(
            f"sub Owner{index}(byval selector as long)\n"
            "dim value as long\n"
            "for counter as long = 0 to selector\n"
            "if counter > 2 then Observe(value)\n"
            "next counter\n"
            "dim pointer_value as long ptr = @value\n"
            "Observe(*pointer_value)\n"
            "end sub\n" for index in range(24)))
        for backend in self.backends:
            with self.subTest(backend=backend):
                model = self.compile(source, backend=backend)
                addresses = {identity for identity in model.nodes
                             if model.properties["node", identity]["kind"] == "address-of"}
                self.assertTrue(addresses)
                self.assertFalse([row for row in model.relations("branch-target")
                                  if row[1] == "node" and int(row[2]) in addresses])

    def test_implicit_lifetime_selection_and_exclusions(self) -> None:
        source = self.fixture("lifetimes.bas")
        model = self.compile(source)
        kinds = {row[4] for row in model.records["I"]}
        self.assertEqual(kinds, IMPLICIT_KINDS)
        tracked = self.one(model, "Tracked", "type")
        plain = self.one(model, "Plain", "type")
        for row in model.records["I"]:
            self.assertEqual(int(row[3]), tracked)
            self.assertNotIn(plain, [int(value) for value in row[1:4]])
            self.assertIn(model.signatures[int(row[2])][2], ("constructor", "destructor"))
            if row[4] in ("temporary-destructor", "scope-exit-destructor"):
                self.assertEqual(row[5], "1")
        exits = [row for row in model.records["I"] if row[4] == "scope-exit-destructor"]
        self.assertEqual({model.symbol_name(row[1]).lower() for row in exits}, {"leaving_value", "loop_value"})
        for row in exits:
            span = source_range(row, 6)
            self.assertFalse([binding for binding in model.records["B"]
                              if binding[1] == row[2] and source_range(binding, 4) == span])
        macro_temp = self.span(source, "TakeValue(MACRO_TEMP)", "MACRO_TEMP")
        self.assertFalse([row for row in model.records["I"]
                          if row[4] == "temporary-destructor" and source_range(row, 6)[1] == macro_temp[1]])
        new_calls = [row for row in model.records["I"] if row[4] == "new-constructor"]
        self.assertEqual(len(new_calls), 2)
        self.assertNotEqual(new_calls[0][11], new_calls[1][11])

    def test_node_payloads_calls_conversions_and_jump_tables(self) -> None:
        model = self.compile(self.fixture("control-flow.bas"), extra=("-exx",))
        kinds = {properties["kind"] for (domain, _), properties in model.properties.items()
                 if domain == "node"}
        self.assertTrue({"assignment", "conversion", "branch", "jump-table", "call", "argument",
                         "constant", "variable", "scope-begin", "scope-end"} <= kinds)
        case_values = set()
        for row in model.records["J"]:
            bias = int(model.properties["node", int(row[1])]["jump-bias"])
            case_values.add(int(row[3]) + bias)
        self.assertEqual(case_values, {1, 3, 5})
        self.assertTrue(model.relations("branch-target"))
        self.assertTrue(model.relations("default-target"))
        self.assertTrue(any(value[0] == "signed" for (domain, _), value in model.constants.items()
                            if domain == "node"))
        for (domain, identity), properties in model.properties.items():
            if domain != "node" or properties.get("kind") != "conversion":
                continue
            self.assertEqual(model.nodes[identity][5], "-1")
            self.assertIn(properties["conversion"], ("0", "1"))

    def test_full_and_compact_expression_prefixes(self) -> None:
        source = self.source("type Leaf\n value as double\nend type\n"
                             "type Branch\n leaves(0 to 1) as Leaf\nend type\n"
                             "dim item as Branch\ndim factor as double = 2\n"
                             "print item.leaves(0).value + factor * 3\n"
                             "print 1 + 2 + factor\nprint -factor\n")
        spans = {"item.leaves(0)": "LEAF", "item.leaves(0).value": "double",
                 "factor * 3": "double", "1 + 2": "integer", "1 + 2 + factor": "double",
                 "-factor": "double"}
        for mode in ("full", "expressions"):
            model = self.compile(source, mode=mode)
            for expression, type_name in spans.items():
                rows = self.expressions(model, self.span(source, expression))
                self.assertTrue(rows, (mode, expression))
                self.assertTrue(any(row[2] == "1" and row[15].casefold() == type_name.casefold() for row in rows))
            if mode == "expressions":
                self.assertFalse(model.records["B"])
                self.assertFalse(any(model.records[tag] for tag in DETAIL_TAGS - PROVENANCE_TAGS))
                self.assertTrue(model.records["FILE"])
                self.assertTrue(all(row[13:15] == ["0", "0"] for row in model.records["E"]))

    def test_macros_and_line_remaps_never_become_editable(self) -> None:
        source = self.source("dim value as long = 1\n#define USE_VALUE value\n"
                             "print USE_VALUE + 2\n#line 30 \"logical.bas\"\n"
                             "print value + 3\n")
        for mode in ("full", "expressions"):
            model = self.compile(source, mode=mode)
            # The literal 2 is written outside the macro expansion. Its own
            # narrow range remains physical while the enclosing sum does not.
            rows = [row for row in model.records["E"]
                    if (row[4] == "3" and row[7] == "19" and row[5] != "18")
                    or row[3] == "logical.bas"]
            self.assertTrue(rows)
            self.assertTrue(all(row[2] == "0" for row in rows), rows)
            self.assertNotIn("logical.bas", [row[1] for row in model.records["D"]])
            if mode == "full":
                bindings = [row for row in model.records["B"] if row[5] == "3" or row[4] == "logical.bas"]
                self.assertTrue(bindings)
                self.assertTrue(all(row[3] == "0" for row in bindings))

    def test_unicode_encodings_utf16_columns_and_long_lines(self) -> None:
        text = "dim value as long = 1\nprint len(\"😀\") + value\nprint " + " " * 1500 + "value + 2\n"
        encodings = (("utf-8", codecs.BOM_UTF8), ("utf-16-le", codecs.BOM_UTF16_LE),
                     ("utf-16-be", codecs.BOM_UTF16_BE), ("utf-32-le", codecs.BOM_UTF32_LE),
                     ("utf-32-be", codecs.BOM_UTF32_BE))
        for encoding, bom in encodings:
            with self.subTest(encoding=encoding):
                source = self.working / (encoding + ".bas")
                source.write_bytes(bom + text.encode(encoding))
                model = self.compile(source)
                self.assert_physical_ranges(model, {str(source): text})
                columns = [int(row[6]) for row in model.records["B"] if row[5] == "2" and row[2] == "reference"]
                self.assertIn(18, columns)
                self.assertTrue(any(row[2] == "1" and row[4:8] == ["3", "1506", "3", "1515"]
                                    for row in model.records["E"]))

    def test_dependencies_include_order_once_preinclude_and_escaping(self) -> None:
        folder = self.working / "include%folder"
        folder.mkdir()
        preinclude = self.source("const PreValue = 1\n", "preinclude.bi")
        second = self.source("#pragma once\nconst SecondValue = 2\n", "include%folder/second%.bi")
        first = self.source('#include once "second%.bi"\n', "include%folder/first.bi")
        source = self.source('#include once "include%folder/first.bi"\n'
                             '#include once "include%folder/first.bi"\n'
                             '#if 0\n#include "absent.bi"\n#endif\n'
                             'print PreValue + SecondValue\n', "module%.bas")
        for mode in ("full", "expressions"):
            model = self.compile(source, mode=mode, extra=("-include", str(preinclude)))
            self.assertEqual([row[1] for row in model.records["D"]],
                             [self.compiler_path(path) for path in (source, preinclude, first, second)])
            self.assertEqual(model.footer[11], "1")
        self.assertEqual(unescape("%2525%09%0D%0A"), "%25\t\r\n")

    def test_dependency_limit_is_explicit(self) -> None:
        includes = self.working / "includes"
        includes.mkdir()
        paths = []
        for index in range(5001):
            path = includes / f"{index}.bi"
            path.write_text("' bounded dependency fixture\n", encoding="utf-8")
            paths.append(path)
        # Paths inside BASIC source never pass through MSYS2's argument
        # conversion. Relative includes work with both compiler hosts.
        source = self.source("".join(f'#include "includes/{path.name}"\n' for path in paths))
        model = self.compile(source, mode="expressions")
        self.assertEqual(len(model.records["D"]), 5000)
        self.assertEqual(model.footer[11], "0")
        self.assertEqual(model.records["D"][0][1], self.compiler_path(source))

    def test_multi_module_ids_and_transaction_rollback(self) -> None:
        first = self.source("dim first_value as long = 1\nprint first_value\n", "first.bas")
        second = self.source("dim second_value as double = 2\nprint second_value\n", "second.bas")
        _, path = self.invoke([first, second])
        model = Model.read(path)
        self.assertEqual(len(model.records["M"]), 2)
        self.assertNotEqual(self.one(model, "first_value", "variable"), self.one(model, "second_value", "variable"))
        second.write_text("dim stale_value as long = 2\nprint undeclared_name\n", encoding="utf-8")
        _, path = self.invoke([first, second], success=False)
        text = path.read_text()
        self.assertNotIn("stale_value".upper(), text)
        self.assertEqual(sum(line.startswith("M\t") for line in text.splitlines()), 1)
        with self.assertRaises(ValueError):
            Model(text)

    def test_parser_restart_discards_attempts(self) -> None:
        source = self.source('#lang "fblite"\ndim value as long = 1\nprint value + 2\n')
        for mode in ("full", "expressions"):
            model = self.compile(source, mode=mode)
            self.assertEqual(len(model.records["M"]), 1)
            self.assertEqual(len(model.records["D"]), 1)
            self.assertEqual(len([row for row in model.records["E"] if row[4:8] == ["3", "6", "3", "15"]]), 1)

    def test_failed_full_output_and_explicit_compact_recovery(self) -> None:
        source = self.source("dim factor as double = 1\nprint factor * 2 +\nprint factor * 3\n")
        _, path = self.invoke([source], success=False)
        text = path.read_text()
        self.assertFalse(any(line.startswith(("M\t", "S\t", "E\t", "END\t", "RECOVERY\t"))
                             for line in text.splitlines()))
        with self.assertRaises(ValueError):
            Model(text)
        _, path = self.invoke([source], mode="expressions", success=False)
        recovered = Model.read(path, expressions_only=True, allow_recovery=True)
        self.assertEqual(recovered.footer[0], "RECOVERY")
        self.assertTrue(any(row[2] == "1" and row[4] == "3" and row[15] == "double"
                            for row in recovered.records["E"]))
        for options in ({}, {"expressions_only": True}):
            with self.assertRaises(ValueError):
                Model.read(path, **options)

    def test_interrupted_compile_has_no_completeness_marker(self) -> None:
        # MSYS2 exposes mkfifo to Python, but its FIFO is not a blocking
        # source stream for the native Windows compiler.
        if self.native_windows or not hasattr(os, "mkfifo"):
            self.skipTest("FIFO source requires POSIX")
        source = self.working / "blocked.bas"
        os.mkfifo(source)
        sidecar = self.working / "interrupted.tsv"
        process = subprocess.Popen([str(self.compiler), "-prefix", str(self.root), "-r", "-gen", "gcc",
                                    "-semantic-model", str(sidecar), str(source)],
                                   cwd=self.working, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        try:
            deadline = time.monotonic() + 10
            while not list(self.working.glob(".fb-semantic-*/model.tmp")) and process.poll() is None and time.monotonic() < deadline:
                time.sleep(0.01)
            self.assertTrue(list(self.working.glob(".fb-semantic-*/model.tmp")), "Compiler never staged its sidecar")
            self.assertIsNone(process.poll(), "FIFO fixture failed to hold compilation")
        finally:
            if process.poll() is None:
                process.terminate()
            process.communicate(timeout=10)
        self.assertFalse(sidecar.exists(), "Interrupted output was published")

    def test_output_aliases_preserve_sources_and_includes(self) -> None:
        for mode in ("full", "expressions"):
            for alias in ("same", "relative", "hardlink", "symlink", "include", "preinclude"):
                with self.subTest(mode=mode, alias=alias):
                    folder = self.working / (mode + "-" + alias)
                    folder.mkdir()
                    source = folder / "input.bas"
                    included = folder / "input.bi"
                    source.write_text('print 1\n')
                    included.write_text('const IncludedValue = 17\n')
                    protected = included if alias in ("include", "preinclude") else source
                    if alias == "include":
                        source.write_text('#include "input.bi"\nprint IncludedValue\n')
                    destination = protected
                    if alias == "relative":
                        destination = folder / ".." / folder.name / protected.name
                    elif alias in ("hardlink", "symlink"):
                        destination = folder / "model.tsv"
                        if alias == "hardlink":
                            os.link(protected, destination)
                        else:
                            self.create_symlink(protected.name, destination)
                    original = protected.read_bytes()
                    extra = ("-include", str(included)) if alias == "preinclude" else ()
                    option = {
                        "full": "-semantic-model",
                        "bindings": "-semantic-model-bindings",
                        "expressions": "-semantic-model-expressions",
                    }[mode]
                    result, _ = self.invoke([source], mode="off", success=False,
                                            extra=(*extra, option, str(destination)))
                    self.assertEqual(protected.read_bytes(), original)
                    self.assertIn("could not write semantic model", result.stdout + result.stderr)

    def test_output_cannot_replace_emissions_or_executable(self) -> None:
        source = self.source("print 17\n")
        for backend in self.backends:
            with self.subTest(backend=backend):
                suffix = ".asm" if backend in ("gas", "gas64") else ".ll" if backend == "llvm" else ".c"
                destination = source.with_suffix(suffix)
                result, _ = self.invoke([source], mode="off", backend=backend, success=False,
                                        extra=("-semantic-model", str(destination)))
                self.assertIn("could not write semantic model", result.stdout + result.stderr)
                self.assertTrue(destination.is_file())
                self.assertFalse(destination.read_bytes().startswith(b"FBCSEM"))
        executable = self.working / "program"
        result, _ = self.invoke([source], mode="off", emit=False, success=False,
                                extra=("-x", str(executable), "-semantic-model", str(executable)))
        self.assertIn("could not write semantic model", result.stdout + result.stderr)
        self.assertTrue(executable.is_file())
        self.assertFalse(executable.read_bytes().startswith(b"FBCSEM"))
        preprocessed = source.with_suffix(".pp.bas")
        result, _ = self.invoke([source], mode="off", success=False,
                                extra=("-pp", "-semantic-model", str(preprocessed)))
        self.assertIn("could not write semantic model", result.stdout + result.stderr)
        self.assertFalse(preprocessed.read_bytes().startswith(b"FBCSEM"))

    def test_successful_publication_replaces_existing_model(self) -> None:
        source = self.source("dim value as long = 17\nprint value\n")
        destination = self.working / "published.tsv"
        destination.write_bytes(b"old output\n")
        self.invoke([source], mode="off", extra=("-semantic-model", str(destination)))
        model = Model.read(destination)
        self.assertTrue(model.records["E"])
        self.assertFalse(list(self.working.glob(".fb-semantic-*")))

    def test_special_output_destinations_are_rejected(self) -> None:
        if not Path("/dev/full").exists():
            self.skipTest("Full device requires POSIX")
        source = self.source("print 17\n")
        original = source.read_bytes()
        for mode in ("-semantic-model", "-semantic-model-expressions"):
            result, _ = self.invoke([source], mode="off", success=False, extra=(mode, "/dev/full"))
            self.assertIn("could not write semantic model", result.stdout + result.stderr)
            self.assertEqual(source.read_bytes(), original)

    def test_publication_write_flush_close_and_replace_failures(self) -> None:
        executable = self.working / ("publication-test.exe" if self.native_windows else "publication-test")
        command = [str(self.compiler), "-prefix", self.compiler_path(self.toolchain_prefix), "-exx", "-w", "pedantic",
                   "-i", self.compiler_path(self.root / "inc"), "-i", self.compiler_path(self.root / "src/compiler"),
                   self.compiler_path(self.root / "tests/semantic-sidecar/semantic-output-test.bas"),
                   "-x", self.compiler_path(executable)]
        built = subprocess.run(command, cwd=self.working, text=True, capture_output=True, timeout=60)
        self.assertEqual(built.returncode, 0, built.stdout + built.stderr)
        tested = subprocess.run([str(executable)], cwd=self.working, text=True, capture_output=True, timeout=30)
        self.assertEqual(tested.returncode, 0, tested.stdout + tested.stderr)
        self.assertIn("semantic publication failures passed", tested.stdout)
        self.assertFalse(list(self.working.glob(".fb-semantic-*")))

    def test_unopened_sources_are_not_dependencies(self) -> None:
        source = self.working / "absent.bas"
        _, path = self.invoke([source], success=False)
        self.assertNotIn(self.compiler_path(source), [line.split("\t")[1] for line in path.read_text().splitlines()
                                       if line.startswith("D\t")])

    def test_output_open_failure_is_reported(self) -> None:
        source = self.source("print 1\n")
        result, _ = self.invoke([source], mode="off", success=False,
                                extra=("-semantic-model", str(self.working / "missing" / "model.tsv")))
        self.assertIn("could not write semantic model", result.stdout + result.stderr)

    def test_opt_in_preserves_language_configuration_and_runtime_behavior(self) -> None:
        source = self.source("#assert __FB_ERR__ = 0\n"
                             "function calculate(byval x as long) as long\nreturn x * x + 1\nend function\n"
                             "print calculate(7)\n")
        outputs = []
        for mode in ("off", "full", "expressions"):
            executable = self.working / (mode + (".exe" if os.name == "nt" else ""))
            _, path = self.invoke([source], mode=mode, emit=False, extra=("-x", str(executable)))
            result = subprocess.run([str(executable)], cwd=self.working, capture_output=True, timeout=20, check=False)
            outputs.append((result.returncode, result.stdout, result.stderr))
            self.assertEqual(path.exists(), mode != "off")
        self.assertEqual(outputs[0], outputs[1])
        self.assertEqual(outputs[0], outputs[2])
        self.assertEqual(outputs[0][0], 0)
        self.assertEqual(outputs[0][1].strip(), b"50")

    def test_normalized_types_and_procedure_flow_contract(self) -> None:
        source = self.source("""type FirstItem
    value as long
end type
type SecondItem
    value as long
end type
dim first_value as FirstItem
dim second_value as SecondItem
dim fixed_text as string * 12
dim pointer_value as const long ptr
dim value as long = 1
if value > 0 then
    value = value + 2
end if
print value
""")
        _, path = self.invoke([source])
        text = path.read_text()
        model = Model(text)
        first = model.normalized_types["symbol", self.one(model, "first_value", "variable"), "value"]
        second = model.normalized_types["symbol", self.one(model, "second_value", "variable"), "value"]
        self.assertEqual(first[4], "type")
        self.assertNotEqual(first[5], second[5])
        fixed = model.normalized_types["symbol", self.one(model, "fixed_text", "variable"), "value"]
        self.assertEqual(fixed[4], "fixed-string")
        self.assertEqual(fixed[9], "12")
        pointer = model.normalized_types["symbol", self.one(model, "pointer_value", "variable"), "value"]
        self.assertEqual(pointer[4:9], ["long", "0", "1", "0", "01"])
        self.assertIn(pointer[9], ("4", "8"))
        for tag in ("PH", "NP", "EV", "CB", "CN", "CE", "CL"):
            self.assertTrue(model.records[tag], tag)
        self.assertTrue(any(row[4] == "conditional-label" for row in model.records["CE"]))
        assignment = next(identity for identity, row in model.nodes.items()
                          if row[6] == "assign" and {child[3] for child in model.nodes.values()
                                                    if int(child[2]) == identity and child[3] != "root"} >= {"left", "right"})
        order = sorted((int(row[3]), model.nodes[int(row[2])][3])
                       for row in model.records["EV"] if int(row[1]) == assignment)
        self.assertEqual(order, [(0, "right"), (1, "left")])
        rows = [row.copy() for row in model.rows]
        changes = {"NT": (8, "2"), "PH": (3, "unknown"), "NP": (2, "999999"),
                   "EV": (4, "unknown"), "CB": (2, "999999"), "CN": (2, "999999"),
                   "CE": (4, "unknown"), "CL": (3, "999999")}
        for tag, (field, value) in changes.items():
            with self.subTest(corrupt=tag), self.assertRaises(ValueError):
                changed = [row.copy() for row in rows]
                next(row for row in changed if row[0] == tag)[field] = value
                Model("\n".join("\t".join(row) for row in changed) + "\n")
        with self.assertRaisesRegex(ValueError, "phase membership"):
            changed = [row.copy() for row in rows if row[0] != "NP"]
            changed[-1][12] = str(int(changed[-1][12]) - len(model.records["NP"]))
            Model("\n".join("\t".join(row) for row in changed) + "\n")

    def test_reader_rejects_corrupt_or_falsely_complete_output(self) -> None:
        source = self.source("dim value as long = 1\nprint value + 2\n")
        _, path = self.invoke([source])
        text = path.read_text()
        lines = text.splitlines(keepends=True)
        footer = lines[-1].rstrip("\n").split("\t")
        malformed = {
            "truncated": text[:-1], "missing-footer": "".join(lines[:-1]),
            "extra-record": text + "D\tunexpected.bas\n",
            "unknown-record": text.replace("M\t", "UNKNOWN\t", 1),
            "unsupported-schema": text.replace("FBCSEM\t" + SCHEMA, "FBCSEM\t999", 1),
            "invalid-escape": text.replace("M\t", "M\t%ZZ", 1),
            "short-record": re.sub(r"(?m)^M\t[^\n]*", "M", text, count=1),
        }
        for index in range(2, 11):
            changed = footer.copy()
            changed[index] = str(int(changed[index]) + 1)
            malformed[f"count-{index}"] = "".join(lines[:-1]) + "\t".join(changed) + "\n"
        changed = footer.copy()
        changed[12] = str(int(changed[12]) + 1)
        malformed["metadata-count"] = "".join(lines[:-1]) + "\t".join(changed) + "\n"
        for name, corrupted in malformed.items():
            with self.subTest(corruption=name), self.assertRaises(ValueError):
                Model(corrupted)

    def test_binary_unary_and_assignment_operator_vocabulary(self) -> None:
        binary = {
            "+": "add", "-": "subtract", "*": "multiply", "/": "divide",
            "\\": "integer-divide", "mod": "modulo", "^": "power", "and": "and",
            "or": "or", "xor": "xor", "eqv": "equivalence", "imp": "implication",
            "shl": "shift-left", "shr": "shift-right", "andalso": "logical-and",
            "orelse": "logical-or", "=": "equal", "<>": "not-equal", "<": "less-than",
            ">": "greater-than", "<=": "less-or-equal", ">=": "greater-or-equal",
        }
        lines = ["dim left_value as long = 7", "dim right_value as long = 2",
                 "dim text_value as string = \"a\"", "dim other_text as string = \"b\""]
        lines += [f"print left_value {token} right_value" for token in binary]
        lines += ["print +left_value", "print -left_value", "print not left_value",
                  "print text_value & other_text", "left_value = right_value"]
        self_codes = {token: code + "-assign" for token, code in binary.items()
                      if token not in ("=", "<>", "<", ">", "<=", ">=")}
        lines += [f"left_value {token}= right_value" for token in self_codes]
        lines.append("text_value &= other_text")
        source = self.source("\n".join(lines) + "\n")
        expected = set(binary.values()) | set(self_codes.values()) | {
            "unary-plus", "negate", "not", "concatenate", "concatenate-assign", "assign"}
        for backend in self.backends:
            with self.subTest(backend=backend):
                model = self.compile(source, backend=backend)
                operations = model.records["O"]
                self.assertEqual({row[1] for row in operations}, expected)
                self.assertTrue(all(row[2:5] == ["builtin", "0", "1"] for row in operations))
                self.assert_physical_ranges(model, {str(source): source.read_text()})

    def test_math_and_conversion_operator_vocabulary(self) -> None:
        math = {"abs": "absolute-value", "sgn": "sign", "sin": "sine", "asin": "arcsine",
                "cos": "cosine", "acos": "arccosine", "tan": "tangent", "atn": "arctangent",
                "sqr": "square-root", "log": "logarithm", "exp": "exponential",
                "int": "floor", "fix": "truncate", "frac": "fractional-part"}
        casts = {"cbool": "convert-to-boolean", "cbyte": "convert-to-integer",
                 "cubyte": "convert-to-integer", "cshort": "convert-to-integer",
                 "cushort": "convert-to-integer", "cint": "convert-to-integer",
                 "cuint": "convert-to-integer", "clng": "convert-to-integer",
                 "culng": "convert-to-integer", "clngint": "convert-to-integer",
                 "culngint": "convert-to-integer", "csng": "convert-to-float",
                 "cdbl": "convert-to-float", "csign": "convert-to-signed", "cunsg": "convert-to-unsigned"}
        lines = ["dim value as double = 0.5", "dim integer_value as long = 2"]
        lines += [f"print {name}(value)" for name in math]
        lines += [f"print {name}(integer_value)" for name in casts]
        lines.append("print atan2(value, value)")
        source = self.source("\n".join(lines) + "\n")
        for backend in self.backends:
            with self.subTest(backend=backend):
                model = self.compile(source, backend=backend)
                self.assertEqual({row[1] for row in model.records["O"]},
                                 set(math.values()) | set(casts.values()) | {"arctangent2"})
                for name, code in {**math, **casts}.items():
                    value = "value" if name in math else "integer_value"
                    rows = self.expressions(model, self.span(source, f"{name}({value})"))
                    self.assertTrue(any(row[10:12] == [code, "builtin"] for row in rows), (name, rows))

    def test_overloaded_operators_are_not_builtin_or_identifier_bindings(self) -> None:
        source = self.source("type Number\n value as long\nend type\n"
                             "operator +(byref a as Number, byref b as Number) as Number\n"
                             " return type<Number>(a.value + b.value)\nend operator\n"
                             "operator +(byref a as Number) as Number\nreturn a\nend operator\n"
                             "dim a as Number\ndim b as Number\ndim c as Number\nc = a + b\nc = +a\n")
        model = self.compile(source)
        for expression, code in (("a + b", "add"), ("+a", "unary-plus")):
            rows = self.expressions(model, self.span(source, expression))
            self.assertTrue(any(row[10:12] == [code, "overloaded"] for row in rows), rows)
            operations = [row for row in model.records["O"] if row[1:3] == [code, "overloaded"]]
            self.assertTrue(operations)
            self.assertTrue(all(model.signatures[int(row[3])][2] == "operator" for row in operations))
            for row in operations:
                self.assertFalse([binding for binding in model.records["B"]
                                  if source_range(binding, 4) == source_range(row, 5)])

    def test_erased_operations_do_not_create_guessed_ast_operators(self) -> None:
        source = self.source("dim value as long = 1\nprint 1 + 2\nprint value\nprint 3\n")
        model = self.compile(source)
        rows = self.expressions(model, self.span(source, "1 + 2"))
        self.assertTrue(rows)
        self.assertTrue(all(row[10:12] == ["", "none"] for row in rows))
        self.assertTrue(all(model.constants["expression", int(row[1])] == ("signed", "3") for row in rows))
        self.assertTrue(any(row[1:3] == ["add", "builtin"] for row in model.records["O"]))
        for expression in ("print value", "print 3"):
            rows = self.expressions(model, self.span(source, expression, expression.split()[1]))
            self.assertTrue(rows)
            self.assertTrue(all(row[10:12] == ["", "none"] for row in rows))

    def test_symbol_pool_reuse_keeps_distinct_local_identities(self) -> None:
        source = self.source("\n".join(f"sub Scope{index}()\ndim reused as long = {index}\nprint reused\nend sub"
                                        for index in range(100)) + "\n")
        model = self.compile(source)
        identities = model.named("reused", "variable")
        self.assertEqual(len(identities), 100)
        self.assertEqual(len({model.types[identity][8] for identity in identities}), 100)
        for row in model.records["B"]:
            if model.symbol_name(row[1]).lower() == "reused":
                self.assertIn(int(row[1]), identities)

    def test_procedure_headers_do_not_reuse_released_local_identities(self) -> None:
        source = self.source("\n".join(
            f"sub Owner{index}()\n"
            "dim released_local as long\n"
            "goto finished\nfinished:\n"
            "print released_local\nend sub\n"
            f"declare function Later{index}(byval argument as long) as long\n"
            f"function Later{index}(byval argument as long) as long\n"
            "return argument\nend function\n" for index in range(32)))
        for backend in self.backends:
            with self.subTest(backend=backend):
                model = self.compile(source, backend=backend)
                locals = set(model.named("released_local", "variable"))
                self.assertEqual(len(locals), 32)
                for index in range(32):
                    procedure = self.one(model, f"Later{index}", "procedure")
                    self.assertNotIn(procedure, locals)
                    self.assertEqual(model.symbols[procedure][3], "3")
                    self.assertEqual(model.signatures[procedure][4], "1")

    def test_repeated_includes_preserve_contexts_and_unique_dependencies(self) -> None:
        header = self.source("dim included_value as SELECTED_TYPE\n"
                             "function ReadIncluded() as SELECTED_TYPE\nreturn included_value\nend function\n", "shared.bi")
        source = self.source('#define SELECTED_TYPE long\nnamespace First\n#include "shared.bi"\nend namespace\n'
                             '#undef SELECTED_TYPE\n#define SELECTED_TYPE double\nnamespace Second\n'
                             '#include "shared.bi"\nend namespace\n')
        model = self.compile(source)
        self.assertEqual([row[1] for row in model.records["D"]],
                         [self.compiler_path(source), self.compiler_path(header)])
        rows = self.expressions(model, self.span(header, "return included_value", "included_value"))
        self.assertEqual({row[15] for row in rows}, {"long", "double"})
        self.assertTrue(all(row[2] == "1" for row in rows))
        self.assertEqual(len(model.named("included_value", "variable")), 2)

    def test_label_bindings_in_legacy_dialects(self) -> None:
        source = self.source("dim selector as integer\non selector goto 10, 20\n"
                             "if selector then 10 else 20\non error goto handler\n"
                             "restore values\ngosub worker\ngoto done\n"
                             "values:\ndata 1\nworker:\nreturn done\n"
                             "handler:\nresume\n10:\ngoto done\n20:\ngoto done\ndone:\n")
        model = self.compile(source, extra=("-lang", "qb"))
        for name, declaration, reference in (("10", "10:", "on selector goto 10"),
                                               ("20", "20:", "on selector goto 10, 20"),
                                               ("handler", "handler:", "on error goto handler"),
                                               ("values", "values:", "restore values"),
                                               ("worker", "worker:", "gosub worker"),
                                               ("done", "done:", "return done")):
            declaration_id = self.binding(model, self.span(source, declaration, name), "declaration")
            self.assertEqual(self.binding(model, self.span(source, reference, name), "reference"), declaration_id)
            self.assertEqual(model.types[declaration_id][3], "label")

    def test_argument_constructor_after_overload_selection(self) -> None:
        source = self.fixture("lifetimes.bas")
        with source.open("a", encoding="utf-8") as output:
            output.write("\nsub SelectArgument overload(byval item as Tracked)\nend sub\n"
                         "sub SelectArgument overload(byval item as string)\nend sub\n"
                         "SelectArgument(17)\n")
        model = self.compile(source)
        # SUB calls allow optional parentheses; here (17) is the parsed actual
        # expression, including its written grouping parentheses.
        span = self.span(source, "SelectArgument(17)", "(17)")
        rows = [row for row in model.records["I"] if row[4] == "argument-constructor"
                and source_range(row, 6) == span]
        self.assertTrue(rows)
        for row in rows:
            self.assertEqual(model.types[int(row[1])][3], "parameter")
            proc = int(model.types[int(row[1])][8])
            self.assertEqual(model.symbol_name(proc).lower(), "selectargument")
            self.assertEqual(model.types[int(row[3])][2], "TRACKED")

    def test_target_context_and_primitive_storage(self) -> None:
        source = self.source("dim value as integer\ndim pointer_value as long ptr\nprint value\n")
        for architecture, pointer_size in (("x86", 4), ("x86_64", 8)):
            with self.subTest(architecture=architecture):
                model = self.compile(source, extra=("-target", "linux", "-arch", architecture))
                self.assertEqual(len(model.records["Q"]), 1)
                context = model.records["Q"][0]
                self.assertEqual(context[1], self.compiler_path(source))
                cpu = "686" if architecture == "x86" else "x86-64"
                self.assertEqual(context[2:5], ["linux-" + architecture, cpu, "fb"])
                self.assertEqual(context[6:9], [str(pointer_size), "little", "4"])
                primitives = {row[2]: row for row in model.records["Y"]}
                self.assertEqual(primitives["integer"][4], str(pointer_size))
                self.assertEqual(primitives["pointer"][4], str(pointer_size))
                self.assertEqual(primitives["long"][3:5], ["integer", "4"])
                self.assertEqual(primitives["long"][6], "1")
                self.assertEqual(primitives["ulong"][6], "0")
                self.assertEqual(primitives["string"][4], str(pointer_size * 3))
                self.assertEqual(primitives["ustring"][4], str(pointer_size * 3))
                self.assertEqual(primitives["va_list"][4:6], ["", ""])
                self.assertEqual(primitives["xmmword"][4:6], ["16", ""])
                self.assertEqual(len(primitives), len(model.records["Y"]))

    def test_reader_rejects_invalid_identities_ranges_flags_and_payloads(self) -> None:
        _, path = self.invoke([self.fixture("types.bas")])
        rows = [line.split("\t") for line in path.read_text().splitlines()]
        mutations = (("B", 1, "999999999"), ("B", 3, "-1"), ("B", 7, "0"),
                     ("E", 2, "2"), ("E", 6, "0"), ("E", 13, "999999999"),
                     ("G", 4, "unknown"), ("G", 7, "999999999"),
                     ("C", 3, "unknown"), ("H", 3, "unknown"),
                     ("T", 3, "unknown"), ("T", 4, "unknown"), ("T", 8, "999999999"),
                     ("T", 17, "unknown"), ("Q", 6, "3"), ("F", 2, "unknown"),
                     ("F", 3, "unknown"), ("U", 2, "unknown"))
        for tag, column, replacement in mutations:
            with self.subTest(tag=tag, column=column, replacement=replacement):
                copy = [row.copy() for row in rows]
                next(row for row in copy if row[0] == tag)[column] = replacement
                with self.assertRaises(ValueError):
                    Model("\n".join("\t".join(row) for row in copy) + "\n")
        copy = [row.copy() for row in rows]
        node = next(row for row in copy if row[0] == "N" and row[3] != "root")
        node[2] = node[1]
        with self.subTest(corruption="node-parent-cycle"), self.assertRaises(ValueError):
            Model("\n".join("\t".join(row) for row in copy) + "\n")

    def test_optional_runtime_headers_preserve_target_calling_convention(self) -> None:
        source = self.source('#include once "string.bi"\n#include once "file.bi"\n'
                             'dim mask as ustring = "0.0"\n'
                             'print format(12.5, mask), format(12.5, wstr("0.0"))\n'
                             'print format(12.5, "0.0"), FileLen(""), FileLen(wstr(""))\n')
        aliases = {"fb_StrFormat", "fb_TextFormat_w", "fb_UStrFormat", "fb_FileLen", "fb_WideFileLen"}
        for target, architecture, convention in (
            ("linux", "x86", "cdecl"), ("linux", "x86_64", "cdecl"),
            # Win64 retains the Windows source convention in metadata;
            # the backend maps it to the platform's single x64 convention.
            ("win32", "x86", "stdcall"), ("win32", "x86_64", "stdcall"),
        ):
            with self.subTest(target=target, architecture=architecture):
                model = self.compile(source, extra=("-target", target, "-arch", architecture))
                selected = {identity: row[15] for identity, row in model.types.items() if row[15] in aliases}
                self.assertEqual(set(selected.values()), aliases)
                for identity, alias in selected.items():
                    self.assertEqual(model.signatures[identity][3], convention, alias)

    def test_reader_rejects_missing_metadata_even_with_correct_totals(self) -> None:
        _, path = self.invoke([self.fixture("types.bas")])
        text = path.read_text()
        model = Model(text)
        identities = {"T": self.one(model, "Packed", "type"),
                      "U": self.one(model, "Packed", "type")}
        # Inheritance provides two ReadValue methods; remove one exact signature.
        identities["F"] = next(identity for identity, signature in model.signatures.items()
                               if model.symbol_name(identity).lower() == "readvalue")
        for tag in ("T", "U", "F", "G", "Q", "Y"):
            with self.subTest(tag=tag):
                rows = [line.split("\t") for line in text.splitlines()]
                if tag in identities:
                    retained = [row for row in rows if not (row[0] == tag and int(row[1]) == identities[tag])]
                else:
                    retained = [row for row in rows if row[0] != tag]
                removed = len(rows) - len(retained)
                self.assertGreater(removed, 0)
                retained[-1][12] = str(int(retained[-1][12]) - removed)
                with self.assertRaises(ValueError):
                    Model("\n".join("\t".join(row) for row in retained) + "\n")

        rows = [line.split("\t") for line in text.splitlines()]
        last = next(row for row in rows if row[0] == "Y" and row[2] == PRIMITIVE_NAMES[-1])
        rows.remove(last)
        rows[-1][12] = str(int(rows[-1][12]) - 1)
        with self.subTest(omission="last-primitive"), self.assertRaises(ValueError):
            Model("\n".join("\t".join(row) for row in rows) + "\n")

        rows = [line.split("\t") for line in text.splitlines()]
        edge = next(row for row in rows if row[0] == "H" and row[5] == "canonical-symbol")
        edge[4] = edge[2]
        with self.subTest(corruption="canonical-cycle"), self.assertRaises(ValueError):
            Model("\n".join("\t".join(row) for row in rows) + "\n")

    def test_local_types_keep_scope_ownership(self) -> None:
        source = self.source("sub Owner()\n"
                             "type LocalType\n value as long\nend type\n"
                             "dim item as LocalType\nitem.value = 1\n"
                             "print item.value\nend sub\nOwner()\n")
        model = self.compile(source)
        local = self.one(model, "LocalType", "type")
        owner = self.one(model, "Owner", "procedure")
        self.assertEqual(int(model.types[local][8]), owner)
        self.assertTrue(model.records["N"])

    def test_expression_values_are_exact_and_compact_output_omits_them(self) -> None:
        source = self.source('dim variable_value as long = 1\nprint 1 + 2, 1.5, !"A\\0B", variable_value\n')
        model = self.compile(source)
        for expression, value in (("1 + 2", ("signed", "3")),
                                  ("1.5", ("float64-bits", "0x3FF8000000000000")),
                                  ('!"A\\0B"', ("bytes", "410042"))):
            rows = self.expressions(model, self.span(source, expression))
            self.assertTrue(rows)
            self.assertTrue(all(model.constants["expression", int(row[1])] == value for row in rows))
        rows = self.expressions(model, self.span(source, "print 1 + 2", "variable_value"))
        self.assertTrue(rows)
        self.assertTrue(all(("expression", int(row[1])) not in model.constants for row in rows))
        compact = self.compile(source, mode="expressions")
        self.assertFalse(compact.records["C"])
        self.assertTrue(compact.records["E"])

    def test_virtual_dispatch_retains_static_target_and_override(self) -> None:
        source = self.fixture("types.bas")
        with source.open("a", encoding="utf-8") as output:
            output.write("\nsub Dispatch(byval item as BaseType ptr)\nprint item->ReadValue()\nend sub\n")
        model = self.compile(source)
        calls = [row for row in model.records["N"]
                 if model.properties["node", int(row[1])].get("call-kind") == "virtual"]
        self.assertTrue(calls)
        base = self.one(model, "BaseType", "type")
        derived = self.one(model, "DerivedType", "type")
        for row in calls:
            targets = [edge for edge in model.relations("static-target") if edge[2] == row[1]]
            self.assertEqual(len(targets), 1)
            target = int(targets[0][4])
            self.assertEqual(int(model.types[target][8]), base)
            self.assertTrue(any(int(edge[4]) == target and int(model.types[int(edge[2])][8]) == derived
                                for edge in model.relations("overrides")))

    def test_preprocessor_parameters_replacements_and_undefined_history(self) -> None:
        source = self.source('#define REMOVED_VALUE 7\nprint REMOVED_VALUE\n#undef REMOVED_VALUE\n'
                             '#define COMBINE(left_arg, right_arg) left_arg + right_arg\nprint COMBINE(1, 2)\n'
                             '#if 0\n#define INACTIVE_VALUE 9\n#endif\n')
        model = self.compile(source)
        removed = self.one(model, "REMOVED_VALUE", "define")
        combine = self.one(model, "COMBINE", "define")
        self.assertFalse(model.named("INACTIVE_VALUE"))
        self.assertEqual(model.properties["symbol", removed]["macro-kind"], "text")
        self.assertEqual(model.properties["symbol", combine]["macro-argument-count"], "2")
        parameters = [row[4] for row in model.records["Z"] if row[1] == str(combine) and row[3] == "parameter"]
        self.assertEqual([value.lower() for value in parameters], ["left_arg", "right_arg"])
        references = [row[4] for row in model.records["Z"]
                      if row[1] == str(combine) and row[3] == "parameter-reference"]
        self.assertEqual(references, ["0", "1"])
        self.assertTrue(any(row[1] == str(removed) and row[3] == "text" and row[4].strip() == "7"
                            for row in model.records["Z"]))
        self.assertEqual(self.binding(model, self.span(source, "#define REMOVED_VALUE", "REMOVED_VALUE"), "declaration"), removed)
        callback = self.one(model, "__DATE__", "define")
        self.assertEqual(model.properties["symbol", callback]["macro-kind"], "callback")
        self.assertFalse([row for row in model.records["Z"] if row[1] == str(callback)])

    def test_preprocessing_output_keeps_semantic_validation(self) -> None:
        source = self.source("dim value as long = 1\nprint value\n")
        for mode in ("full", "expressions"):
            _, path = self.invoke([source], mode=mode, extra=("-pp",))
            model = Model.read(path, expressions_only=mode == "expressions")
            self.assertTrue(model.records["E"])
        source.write_text("print unresolved_name\n", encoding="utf-8")
        _, path = self.invoke([source], extra=("-pp",), success=False)
        with self.assertRaises(ValueError):
            Model.read(path)

    def test_malformed_encoded_lines_are_not_editable(self) -> None:
        for encoding, bom, invalid_unit in (("utf-16-le", codecs.BOM_UTF16_LE, b"\x00\xd8"),
                                           ("utf-32-le", codecs.BOM_UTF32_LE, b"\x00\x00\x11\x00")):
            with self.subTest(encoding=encoding):
                source = self.working / (encoding + ".bas")
                source.write_bytes(bom + "print 1 ' ".encode(encoding) + invalid_unit + "\n".encode(encoding))
                model = self.compile(source)
                self.assertTrue(model.records["E"])
                self.assertTrue(all(row[2] == "0" for row in model.records["E"]))


    def test_prototype_allocation_does_not_reuse_label_or_local_identity(self) -> None:
        declarations = []
        for index in range(100):
            declarations.append(f"sub Body{index}()\n"
                                f"goto Target{index}\nTarget{index}:\n"
                                f"dim local_value as long = {index}\nprint local_value\nend sub\n"
                                f"declare function Prototype{index}(byval value as long) as long\n")
        model = self.compile(self.source("".join(declarations)))
        prototypes = [identity for identity, row in model.types.items()
                      if row[3] == "procedure" and row[2].lower().startswith("prototype")]
        labels = [identity for identity, row in model.types.items()
                  if row[3] == "label" and row[2].lower().startswith("target")]
        self.assertEqual(len(prototypes), 100)
        self.assertEqual(len(labels), 100)
        self.assertFalse(set(prototypes) & set(labels))

    def test_symbol_inventory_handles_many_sibling_prototypes(self) -> None:
        # A wide header has many parameter tables but little nesting. Its
        # sibling count must not exhaust the nested-table traversal stack.
        count = 4500
        source = self.source("".join(f"declare sub FlatCallback{index}(byval value as long)\n"
                                   for index in range(count)))
        model = self.compile(source)
        callbacks = [identity for identity, row in model.types.items()
                     if row[3] == "procedure" and row[2].lower().startswith("flatcallback")]
        self.assertEqual(len(callbacks), count)
        for identity in callbacks:
            self.assertEqual(model.signatures[identity][4], "1")
            self.assertEqual(model.parameters[identity][0][4], "byval")

    def test_temporary_initializer_scopes_keep_metadata(self) -> None:
        source = self.source("""type InitializerOptions
  width as long = 1
end type
declare sub WithDefault(byref settings as InitializerOptions = InitializerOptions())
WithDefault()
""")
        for backend in self.backends:
            with self.subTest(backend=backend):
                model = self.compile(source, backend=backend)
                scopes = [identity for identity, row in model.types.items() if row[3] == "scope"]
                self.assertTrue(scopes)
                for row in model.symbols.values():
                    if row[11] != "0":
                        self.assertIn(int(row[11]), model.types)


    def test_wire_bytes_round_trip_without_unicode_loss(self) -> None:
        payload = bytes(range(256))
        escaped = "".join(f"%{value:02X}" for value in payload)
        self.assertEqual(unescape(escaped).encode("utf-8", errors="surrogateescape"), payload)
        self.assertEqual(unescape("%F0%9F%98%80"), "😀")
        self.assertEqual(unescape("%25E9"), "%E9")
        for malformed in ("%", "%1", "%GG", "%e9", "\x00", "\x7f", "é"):
            with self.subTest(malformed=malformed), self.assertRaises(ValueError):
                unescape(malformed)

    def test_legacy_literal_and_macro_bytes_remain_readable(self) -> None:
        for macro in (False, True):
            with self.subTest(macro=macro):
                source = self.working / ("legacy-macro.bas" if macro else "legacy-literal.bas")
                original = (b'#define TEXT !"A\\0\xe9"\ndim value as string = TEXT\n' if macro else
                            b'dim value as string = !"A\\0\xe9"\n')
                source.write_bytes(original)
                emissions = []
                for mode in ("off", "full", "expressions"):
                    _, path = self.invoke([source], mode=mode)
                    emissions.append(source.with_suffix(".c").read_bytes())
                    self.assertEqual(source.read_bytes(), original)
                    if mode != "off":
                        wire = path.read_bytes()
                        self.assertTrue(all(value < 128 for value in wire))
                        model = Model.read(path, expressions_only=mode == "expressions")
                        if mode == "full":
                            self.assertIn(("bytes", "4100E9"), model.constants.values())
                            if macro:
                                self.assertTrue(any(b"\xe9" in row[4].encode("utf-8", errors="surrogateescape")
                                                    for row in model.records["Z"]))
                self.assertEqual(emissions[0], emissions[1])
                self.assertEqual(emissions[0], emissions[2])

    def test_legacy_and_malformed_utf8_bytes_keep_source_columns(self) -> None:
        payloads = (b"\xf0", b"\xf0\x9f", b"\xe0\x80\x80", b"\xed\xa0\x80",
                    b"\xf4\x90\x80\x80", b"\xf0\x9f\x98\x80", b"\xe2\x82\xac")
        for payload in payloads:
            original = b'dim value as string = "' + payload + b'" : print value\n'
            source = self.working / "legacy-columns.bas"
            source.write_bytes(original)
            text = original.decode("utf-8", errors="surrogateescape")
            width = len(text.rstrip("\n").encode("utf-16-le", errors="surrogatepass")) // 2
            self.invoke([source], mode="off")
            emitted = source.with_suffix(".c").read_bytes()
            for mode in ("full", "bindings", "expressions"):
                with self.subTest(payload=payload, mode=mode):
                    _, path = self.invoke([source], mode=mode)
                    self.assertEqual(source.with_suffix(".c").read_bytes(), emitted)
                    model = Model.read(path, expressions_only=mode == "expressions", bindings_only=mode == "bindings")
                    self.assertTrue(model.records["E"] if mode != "bindings" else model.records["B"])
                    for tag, physical, column in (("B", 3, 4), ("E", 2, 3), ("I", 5, 6), ("O", 4, 5)):
                        for row in model.records[tag]:
                            if row[physical] == "1":
                                span = source_range(row, column)
                                self.assertLessEqual(span[4], width, row)
                    if mode != "expressions":
                        references = [row for row in model.records["B"] if row[2] == "reference"]
                        self.assertTrue(any(source_range(row, 4)[4] == width for row in references))

    def test_layout_abi_status_and_initialized_node_payloads(self) -> None:
        for backend in self.backends:
            with self.subTest(backend=backend):
                model = self.compile(self.fixture("types.bas"), backend=backend)
                packed = self.one(model, "Packed", "type")
                properties = model.properties["symbol", packed]
                self.assertEqual(properties["natural-alignment"], "8")
                self.assertEqual(properties["packing-alignment"], "1")
                self.assertEqual(properties["layout-finalized"], "1")
                base = self.one(model, "BaseType", "type")
                for relation in ("virtual-table", "runtime-type-info"):
                    edges = [row for row in model.relations(relation) if row[2] == str(base)]
                    self.assertEqual(len(edges), 1)
                    self.assertEqual(model.types[int(edges[0][4])][3], "variable")
                source = self.source("declare sub ArrayAPI(values(any, any) as long)\n"
                                     "sub InitializeEarly() constructor 201\nend sub\n"
                                     "function Calculate(byval value as long) as long\n"
                                     "return value + 1\nend function\n"
                                     "dim value as long = 1\nvalue = 2\nprint Calculate(value)\n")
                model = self.compile(source, backend=backend)
                early = self.one(model, "InitializeEarly", "procedure")
                self.assertEqual(model.properties["symbol", early]["startup-priority"], "201")
                calculate = self.one(model, "Calculate", "procedure")
                self.assertEqual(model.properties["symbol", calculate]["return-used"], "1")
                api = self.one(model, "ArrayAPI", "procedure")
                formal = int(model.parameters[api][0][2])
                descriptor = [row for row in model.relations("parameter-descriptor-type") if int(row[2]) == formal]
                self.assertEqual(len(descriptor), 1)
                self.assertEqual(model.types[int(descriptor[0][4])][3], "type")
                self.assertEqual(model.properties["symbol", formal]["argument-register"], "0")
                assignments = [properties for (domain, _), properties in model.properties.items()
                               if domain == "node" and properties["kind"] == "assignment"]
                self.assertTrue(assignments)
                self.assertTrue(any(properties["initialization"] == "0" for properties in assignments))
                self.assertTrue(all("operator-options" in properties for properties in assignments))

    def test_assembly_tokens_preserve_operands_and_unknown_effects(self) -> None:
        source = self.source("sub AssemblyProbe()\ndim slot as long\nasm\n"
                             "mov eax, slot\nmov slot, eax\naudit_asm_label:\nnop\nend asm\nend sub\n")
        for backend in self.backends:
            with self.subTest(backend=backend):
                model = self.compile(source, backend=backend)
                tokens = model.records["ASM"]
                self.assertTrue(tokens)
                text = "".join(row[5] for row in tokens if row[3] == "text").lower()
                self.assertIn("eax", text)
                self.assertIn("audit_asm_label", text)
                self.assertIn("nop", text)
                operands = [row for row in tokens if row[3] == "symbol"]
                self.assertEqual(len(operands), 2)
                self.assertEqual({model.symbol_name(row[4]) for row in operands}, {"SLOT"})
                self.assertFalse(model.named("audit_asm_label"))
                for row in tokens:
                    self.assertEqual(model.properties["node", int(row[1])]["assembly-effects"],
                                     "unknown-memory-registers-control")

    def test_deferred_copyback_has_destination_temporary_and_call_order(self) -> None:
        source = self.source("declare sub Mutate(byref left_value as string, byref right_value as string)\n"
                             "dim first_value as string * 8\ndim second_value as string * 16\n"
                             "Mutate(first_value, second_value)\n"
                             "dim ordinary_value as string\nMutate(ordinary_value, ordinary_value)\n")
        for backend in self.backends:
            with self.subTest(backend=backend):
                model = self.compile(source, backend=backend)
                calls = [row for row in model.records["N"] if model.properties["node", int(row[1])]["kind"] == "call"
                         and model.symbol_name(row[9]) == "MUTATE"]
                self.assertEqual(sorted(model.properties["node", int(row[1])]["copyback-count"] for row in calls), ["0", "2"])
                call = next(row for row in calls if model.properties["node", int(row[1])]["copyback-count"] == "2")
                destinations = [row for row in model.records["N"] if row[2] == call[1] and row[3] == "copyback"]
                self.assertEqual(len(destinations), 2)
                self.assertEqual({model.symbol_name(row[9]) for row in destinations}, {"FIRST_VALUE", "SECOND_VALUE"})
                self.assertEqual(sorted(int(model.properties["node", int(row[1])]["auxiliary-ordinal"])
                                        for row in destinations), [0, 1])
                temporary_edges = model.relations("copyback-temporary")
                self.assertEqual(len(temporary_edges), 2)
                self.assertTrue(all(model.types[int(row[4])][18] == "compiler" for row in temporary_edges))

    def test_reader_rejects_missing_or_corrupt_active_payload(self) -> None:
        _, path = self.invoke([self.fixture("control-flow.bas")])
        original = [line.split("\t") for line in path.read_text().splitlines()]
        for property_name, replacement in (("kind", "pretend-kind"), ("bytes", "nonnumeric"),
                                            ("conversion", "2"), ("operator-options", "256")):
            with self.subTest(property_name=property_name):
                rows = [row.copy() for row in original]
                row = next(row for row in rows if row[0] == "K" and row[1] == "node" and row[3] == property_name)
                row[4] = replacement
                with self.assertRaises(ValueError):
                    Model("\n".join("\t".join(row) for row in rows) + "\n")
        rows = [row.copy() for row in original]
        removed = next(row for row in rows if row[0] == "K" and row[1] == "node" and row[3] == "passing-mode")
        rows.remove(removed)
        rows[-1][12] = str(int(rows[-1][12]) - 1)
        with self.assertRaises(ValueError):
            Model("\n".join("\t".join(row) for row in rows) + "\n")

    def test_declaration_initializers_do_not_enter_procedure_phases(self) -> None:
        source = self.source("sub DefaultValue(byval value as integer = 17)\n"
                             "print value\nend sub\nDefaultValue()\n")
        for backend in self.backends:
            with self.subTest(backend=backend):
                model = self.compile(source, backend=backend)
                defaults = model.relations("default-initializer")
                self.assertTrue(defaults)
                phased = {int(row[1]) for row in model.records["NP"]}
                for row in defaults:
                    root = int(row[4])
                    self.assertNotIn(root, phased)
                    self.assertEqual(model.nodes[root][2], row[2])
                self.assertTrue(model.records["PH"])
                self.assertTrue(model.records["CN"])

    def test_formal_parameter_spans_keep_names_and_argument_boundaries(self) -> None:
        source = self.fixture("parameter-spans.bas")
        text = source.read_text(encoding="utf-8")
        lines = text.splitlines()
        expected = {
            "FirstValue": ("byval FirstValue", "FirstValue as long", False),
            "SecondValue": ("byval SecondValue", "SecondValue as long", False),
            "BodyFirst": ("byval BodyFirst", "BodyFirst as long", False),
            "BodySecond": ("byval BodySecond", "BodySecond as long", False),
            "PrototypeValue": ("byval PrototypeValue", "    as long)", True),
            "BodyValue": ("byval BodyValue", "    as long)", True),
            "DefaultValue": ("byval DefaultValue", 'iif(-1, len("a,b"), 7))', True),
            "PrototypeAmount": ("byval PrototypeAmount", "        as long)", True),
            "BodyAmount": ("byval BodyAmount", "    as long)", True),
            "InputValue": ("byval InputValue", "    as long) as long", True),
            "NestedValue": ("byval NestedValue", "NestedValue as long", False),
            "Handler": ("byval Handler", "NestedValue as long) as long", True),
            "FormatText": ("byval FormatText", "FormatText as zstring ptr", False),
            "Values": ("Values(", "    as long = any)", True),
        }
        for backend in self.backends:
            targets = ((), ("-target", "linux-x86_64")) if backend == "gas64" else (
                (), ("-target", "linux-x86_64"), ("-target", "win32", "-arch", "686"))
            for target in targets:
                with self.subTest(backend=backend, target=target):
                    model = self.compile(source, backend=backend, extra=target)
                    self.assertEqual(model.capabilities[1]["formal-parameter-spans"], "available")
                    declarations = [row for row in model.records["DCL"]
                                    if row[3] in ("parameter-prototype", "parameter-definition")]
                    by_name = {row[4]: row for row in declarations if row[4]}
                    self.assertTrue(set(expected) <= set(by_name))
                    for name, (start_text, end_text, multiline) in expected.items():
                        declaration = by_name[name]
                        span = model.physical_locations["declaration", int(declaration[1]), "formal"]
                        self.assertEqual(span[11], "mapped", name)
                        self.assertEqual(int(span[5]), int(declaration[7]), name)
                        self.assertEqual(int(span[6]), lines[int(span[5]) - 1].index(start_text), name)
                        self.assertEqual(int(span[5]) != int(span[7]), multiline, name)
                        self.assertIn(end_text, lines[int(span[7]) - 1], name)
                        name_span = model.physical_locations["declaration", int(declaration[1]), "range"]
                        self.assertEqual(int(name_span[5]), int(name_span[7]), name)
                        self.assertEqual(text.encode()[int(name_span[9]):int(name_span[10])].decode(), name)
                        self.assertLessEqual(int(span[9]), int(name_span[9]), name)
                        self.assertGreaterEqual(int(span[10]), int(name_span[10]), name)
                    unnamed = [row for row in declarations if not row[4]]
                    self.assertEqual(len(unnamed), 3, "Two unnamed prototype formals and actual vararg")
                    self.assertEqual(sum(int(model.physical_locations["declaration", int(row[1]), "formal"][5]) !=
                                         int(model.physical_locations["declaration", int(row[1]), "formal"][7])
                                         for row in unnamed), 2)
                    for name in ("Item", "ExpandedValue", "FromMacro"):
                        parameter = self.one(model, name, "parameter")
                        self.assertEqual(model.properties["symbol", parameter]["formal-span-kind"], "generated")
                        for declaration in declarations:
                            if declaration[2] == str(parameter):
                                self.assertNotIn(("declaration", int(declaration[1]), "formal"), model.physical_locations)
                    for name in expected:
                        parameter = int(by_name[name][2])
                        self.assertEqual(model.properties["symbol", parameter]["formal-span-kind"], "physical")
                    method = self.one(model, "Method", "procedure")
                    # G uses ordinal -1 for the implicit receiver; zero is the
                    # first actual caller-supplied formal.
                    receiver = model.parameters[method][-1][2]
                    self.assertFalse(any(row[2] == receiver for row in declarations), "Implicit receiver has no written formal")

    def test_reader_rejects_forged_formal_parameter_spans(self) -> None:
        source = self.fixture("parameter-spans.bas")
        foreign = self.source("'\n" * 600 + "declare sub Foreign(byval value as long)\n", "foreign.bi")
        source.write_text('#include "foreign.bi"\n' + source.read_text(encoding="utf-8"), encoding="utf-8")
        _, path = self.invoke([source])
        original = [line.split("\t") for line in path.read_text().splitlines()]
        for mutation in ("missing-span", "missing-kind", "unknown-kind", "conflicting-kind",
                         "nonparameter", "foreign-source", "wrong-name-range", "generated-as-physical"):
            with self.subTest(mutation=mutation):
                rows = [row.copy() for row in original]
                span = next(row for row in rows if row[0] == "LOC" and row[1] == "declaration" and
                            row[3] == "formal" and row[5] != row[7])
                declaration = next(row for row in rows if row[0] == "DCL" and row[1] == span[2])
                kind = next(row for row in rows if row[:4] == ["K", "symbol", declaration[2], "formal-span-kind"])
                if mutation == "missing-span":
                    rows.remove(span)
                elif mutation == "missing-kind":
                    rows.remove(kind)
                elif mutation == "unknown-kind":
                    kind[4] = "guess"
                elif mutation == "conflicting-kind":
                    duplicate = kind.copy()
                    duplicate[4] = "generated"
                    rows.insert(-1, duplicate)
                elif mutation == "nonparameter":
                    span[2] = next(row[1] for row in rows if row[0] == "DCL" and row[3] == "procedure-prototype")
                elif mutation == "foreign-source":
                    span[4] = next(str(identity) for identity in Model.read(path).source_contexts if str(identity) != span[4])
                elif mutation == "wrong-name-range":
                    name = next(row for row in rows if row[:4] == ["LOC", "declaration", span[2], "range"])
                    span[5:11] = name[5:11]
                    span[6] = str(int(name[6]) + 1)
                    span[9] = str(int(name[9]) + 1)
                else:
                    generated = next(row for row in rows if row[:2] == ["LOC", "declaration"] and row[3] == "formal-generated")
                    generated[3] = "formal"
                rows[-1][12] = str(int(original[-1][12]) + len(rows) - len(original))
                with self.assertRaises(ValueError):
                    Model("\n".join("\t".join(row) for row in rows) + "\n")
        for mode in ("bindings", "expressions"):
            model = self.compile(source, mode=mode)
            self.assertEqual(model.capabilities[1]["formal-parameter-spans"], "unavailable")
            self.assertFalse(any(row[3] == "formal-span-kind" for row in model.records["K"]))

    def test_reader_rejects_cross_owned_procedure_phase(self) -> None:
        _, path = self.invoke([self.source("sub First()\nprint 1\nend sub\n"
                                         "sub Second()\nprint 2\nend sub\nFirst()\nSecond()\n")])
        rows = [line.split("\t") for line in path.read_text().splitlines()]
        phase = next(row for row in rows if row[0] == "PH")
        foreign = next(row for row in rows if row[0] == "PH" and row[2] != phase[2])
        phase[2] = foreign[2]
        with self.assertRaisesRegex(ValueError, "Phase root belongs to another procedure"):
            Model("\n".join("\t".join(row) for row in rows) + "\n")

    def test_written_override_receipts_are_parser_owned(self) -> None:
        source = self.source("#define ExplicitCheck OVERRIDE\n"
                             "#define EmptyCheck\n"
                             "type MarkerBase extends object\n"
                             "declare virtual function Compute() as integer\nend type\n"
                             "type Checked extends MarkerBase\n"
                             "declare virtual function Compute() as integer ExplicitCheck\nend type\n"
                             "type Implicit extends MarkerBase\n"
                             "declare function Compute() as integer EmptyCheck\nend type\n"
                             "function MarkerBase.Compute() as integer\nreturn 1\nend function\n"
                             "function Checked.Compute() as integer\nreturn 2\nend function\n"
                             "function Implicit.Compute() as integer\nreturn 3\nend function\n")
        for backend in self.backends:
            for target in ((), ("-target", "linux-x86_64"), ("-target", "win32", "-arch", "686")):
                with self.subTest(backend=backend, target=target):
                    # The ASM family has separate CPU-width emitters. A Win32
                    # frontend case must not ask gas64 to emit i686 assembly.
                    selected_backend = "gas" if backend == "gas64" and "win32" in target else backend
                    model = self.compile(source, backend=selected_backend, extra=target)
                    expected = {"MARKERBASE": "0", "CHECKED": "1", "IMPLICIT": "0"}
                    seen = {}
                    for identity in model.named("COMPUTE", "procedure"):
                        owner = model.symbol_name(model.types[identity][8])
                        seen[owner] = model.properties["symbol", identity]["written-override"]
                        if owner != "MARKERBASE":
                            self.assertNotEqual(model.signatures[identity][11], "0")
                    self.assertEqual(seen, expected)
                    receipts = [row for row in model.records["K"] if row[3] == "written-override"]
                    self.assertEqual(len(receipts), 3, "Body headers must not overwrite declaration receipts")
        for mode in ("bindings", "expressions"):
            model = self.compile(source, mode=mode)
            self.assertFalse(any(row[3] == "written-override" for row in model.records["K"]))

    def test_reader_rejects_invalid_written_override_receipts(self) -> None:
        source = self.source("type MarkerBase extends object\n"
                             "declare virtual sub Compute()\nend type\n"
                             "type Checked extends MarkerBase\n"
                             "declare sub Compute() override\nend type\n")
        _, path = self.invoke([source])
        original = [line.split("\t") for line in path.read_text().splitlines()]
        for mutation in ("flag", "domain", "nonprocedure", "contradiction", "missing-base"):
            with self.subTest(mutation=mutation):
                rows = [row.copy() for row in original]
                receipt = next(row for row in rows if row[0] == "K" and row[3] == "written-override" and row[4] == "1")
                if mutation == "flag":
                    receipt[4] = "2"
                elif mutation == "domain":
                    receipt[1] = "node"
                elif mutation == "nonprocedure":
                    receipt[2] = next(row[1] for row in rows if row[0] == "T" and row[2] == "CHECKED")
                elif mutation == "contradiction":
                    duplicate = receipt.copy()
                    duplicate[4] = "0"
                    rows.insert(-1, duplicate)
                    rows[-1][12] = str(int(rows[-1][12]) + 1)
                else:
                    next(row for row in rows if row[0] == "F" and row[1] == receipt[2])[11] = "0"
                with self.assertRaises(ValueError):
                    Model("\n".join("\t".join(row) for row in rows) + "\n")

    def test_prototype_names_special_headers_and_implicit_declarations(self) -> None:
        source = self.source("declare function API(byval ExplicitName as long, byref BorrowedName as double) as long\n"
                             "type Tracked\nvalue as long\ndeclare constructor(byval amount as long)\n"
                             "declare destructor()\nend type\n"
                             "constructor Tracked(byval amount as long)\nthis.value = amount\nend constructor\n"
                             "destructor Tracked()\nend destructor\n"
                             "operator +(byref left_value as Tracked, byref right_value as Tracked) as long\n"
                             "return left_value.value + right_value.value\nend operator\n")
        for backend in self.backends:
            with self.subTest(backend=backend):
                model = self.compile(source, backend=backend)
                api = self.one(model, "API", "procedure")
                names = [model.symbol_name(model.parameters[api][index][2]) for index in range(2)]
                self.assertEqual(names, ["ExplicitName", "BorrowedName"])
                occurrences = model.records["DCL"]
                for kind in ("constructor", "destructor", "operator"):
                    identities = {identity for identity, row in model.signatures.items() if row[2] == kind
                                  and model.types[identity][18] == "source"}
                    self.assertTrue(identities, kind)
                    self.assertTrue(all(any(row[2] == str(identity) and row[3] == "procedure-definition"
                                             for row in occurrences) for identity in identities))
                implicit = self.source('#lang "fblite"\nprint implicit_value\n', "implicit.bas")
                model = self.compile(implicit, backend=backend)
                variable = self.one(model, "implicit_value", "variable")
                self.assertEqual(model.types[variable][18], "source")
                self.assertTrue(any(row[2] == str(variable) and row[3] == "implicit-variable" for row in model.records["DCL"]))

    def test_initial_forward_type_use_resolves_without_fake_self_binding(self) -> None:
        source = self.source("type LinkType as FutureType ptr\n"
                             "type FutureType\nvalue as long\nend type\n"
                             "type SelfType as SelfType\n")
        model = self.compile(source)
        selected = self.binding(model, self.span(source, "type LinkType as FutureType", "FutureType"), "reference")
        self.assertEqual(model.symbol_name(selected), "FUTURETYPE")
        canonical = {int(row[2]): int(row[4]) for row in model.relations("canonical-symbol")}
        self.assertEqual(canonical.get(selected, selected), self.one(model, "FutureType", "type"))
        self.assertFalse([row for row in model.records["B"] if row[2] == "reference"
                          and source_range(row, 4) == self.span(source, "as SelfType", "SelfType")])

    def test_effective_context_changes_follow_node_construction_and_module_reset(self) -> None:
        first = self.source('#lang "fblite"\noption base 1\ndim first_values(2) as integer\n'
                            'option base 0\ndim second_values(2) as integer\n'
                            'print first_values(1), second_values(0)\n', "context-first.bas")
        second = self.source('dim final_value as long = 1\nprint final_value\n', "context-second.bas")
        for backend in self.backends:
            with self.subTest(backend=backend):
                _, path = self.invoke([first, second], backend=backend)
                model = Model.read(path)
                for name, expected in (("first_values", 1), ("second_values", 0)):
                    symbols = set(model.named(name))
                    declarations = [row for row in model.records["N"] if int(row[9]) in symbols
                                    and model.properties["node", int(row[1])]["kind"] == "declaration"]
                    self.assertTrue(declarations, name)
                    for row in declarations:
                        context = model.context_uses["node", int(row[1])]
                        self.assertEqual(model.options[context]["language-default", "base"], expected)
                self.assertTrue(any(module == 2 for module in model.configurations.values()))
                for context, module in model.configurations.items():
                    options = model.options[context]
                    self.assertEqual(options["compiler", "debug"], 0)
                    self.assertEqual(options["compiler", "debuginfo"], 0)
                    self.assertEqual(options["compiler", "nullptrchk"], 0)
                    self.assertIn(("language-policy", "int64literaldtype"), options)
                    if module == 2:
                        self.assertEqual(options["language-default", "base"], 0)
                rows = [row.copy() for row in model.rows]
                changed = next(row for row in rows if row[0] == "USE")
                changed[3] = "999999999"
                with self.assertRaises(ValueError):
                    Model("\n".join("\t".join(row) for row in rows) + "\n")

    def test_file_revisions_include_occurrences_and_remaps(self) -> None:
        preinclude = self.source("const PreValue = 1\n", "preinclude.bi")
        repeated = self.source("dim occurrence_value as long\n", "repeated.bi")
        guarded = self.source("#pragma once\nconst GuardedValue = 2\n", "guarded.bi")
        source = self.source('#include "guarded.bi"\n#include "guarded.bi"\n'
                             'namespace FirstNamespace\n#include "repeated.bi"\nend namespace\n'
                             'namespace SecondNamespace\n#include "repeated.bi"\nend namespace\n'
                             '#line 200 "logical.bas"\nprint PreValue + GuardedValue\n')
        for mode in ("full", "expressions"):
            with self.subTest(mode=mode):
                model = self.compile(source, mode=mode, extra=("-include", str(preinclude)))
                model.validate_source_revisions()
                for row in model.files.values():
                    original = Path(row[2]).read_bytes()
                    self.assertEqual(row[3], str(len(original)))
                    self.assertEqual(row[4], hashlib.sha256(original).hexdigest())
                repeated_contexts = [row for row in model.source_contexts.values()
                                     if model.files[int(row[3])][2] == self.compiler_path(repeated)]
                self.assertEqual(len(repeated_contexts), 2)
                self.assertNotEqual(repeated_contexts[0][1], repeated_contexts[1][1])
                self.assertEqual(repeated_contexts[0][2], repeated_contexts[1][2])
                self.assertEqual(sum(row[5] == "preinclude" for row in model.source_contexts.values()), 1)
                self.assertEqual([row[5] for row in model.records["INC"]].count("pragma-once"), 1)
                self.assertTrue(any(row[3] == "logical.bas" for row in model.records["MAP"]))
                self.assertFalse(any(row[2] == "logical.bas" for row in model.files.values()))
                self.assertEqual(set(model.source_contexts), set(model.source_endings))
                self.assertEqual(set(model.source_endings.values()), {"verified"})
                if mode == "full":
                    occurrence_symbols = model.named("occurrence_value", "variable")
                    origins = set()
                    for identity, row in enumerate(model.records["B"], 1):
                        if int(row[1]) in occurrence_symbols and row[2] == "declaration":
                            origin = model.origins.get(("binding", identity))
                            if origin in {int(item[1]) for item in repeated_contexts}:
                                origins.add(origin)
                    self.assertEqual(origins, {int(item[1]) for item in repeated_contexts})
                repeated.write_text("dim changed_value as long\n")
                with self.assertRaisesRegex(ValueError, "stale"):
                    model.validate_source_revisions()
                repeated.write_text("dim occurrence_value as long\n")

    def test_source_hash_boundaries_position_and_change_detection(self) -> None:
        executable = self.working / ("source-revision.exe" if self.native_windows else "source-revision")
        result = subprocess.run([str(self.compiler), "-prefix", self.compiler_path(self.toolchain_prefix), "-exx", "-w", "pedantic",
                                 "-i", self.compiler_path(self.root / "inc"), "-i", self.compiler_path(self.root / "src/compiler"),
                                 self.compiler_path(self.root / "tests/semantic-sidecar/semantic-source-file-test.bas"),
                                 "-x", self.compiler_path(executable)], cwd=self.working,
                                capture_output=True, text=True, timeout=60)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        for size in (0, 1, 55, 56, 63, 64, 65, 16383, 16384, 16385):
            with self.subTest(size=size):
                data = bytes(index % 256 for index in range(size))
                path = self.working / "revision.bin"
                path.write_bytes(data)
                tested = subprocess.run([str(executable), self.compiler_path(path), hashlib.sha256(data).hexdigest(),
                                         str(size), str(min(size, 17))], cwd=self.working,
                                        capture_output=True, text=True, timeout=30)
                self.assertEqual(tested.returncode, 0, tested.stdout + tested.stderr)
                self.assertIn("semantic source revision passed", tested.stdout)

    def test_conditional_directive_vocabulary_and_selections(self) -> None:
        source = self.source("""#define FLAG 1
#if 0
dim excluded_first as long
#elseifdef FLAG
dim selected_first as long
#elseif 1
dim excluded_later as long
#else
dim excluded_else as long
#endif
#if not defined(MISSING)
  #ifndef FLAG
  dim excluded_nested as long
  #elseifndef MISSING
  dim selected_nested as long
  #elseifdef FLAG
  dim excluded_nested_later as long
  #else
  dim excluded_nested_else as long
  #endif
#endif
#if "a string"
dim excluded_string as long
#else
dim selected_string as long
#endif
""")
        expected = {2: ("evaluated", "0", "0"), 4: ("evaluated", "1", "1"),
                    6: ("evaluated", "1", "0"), 8: ("unconditional", "", "0"),
                    11: ("evaluated", "1", "1"), 12: ("evaluated", "0", "0"),
                    14: ("evaluated", "1", "1"), 16: ("evaluated", "1", "0"),
                    18: ("unconditional", "", "0"), 22: ("evaluated", "0", "0"),
                    24: ("unconditional", "", "1")}
        for backend in self.backends:
            for mode in ("full", "bindings", "expressions"):
                with self.subTest(backend=backend, mode=mode):
                    model = self.compile(source, backend=backend, mode=mode)
                    actual = {int(branch[10]): tuple(model.conditional_decisions[identity][2:5])
                              for identity, branch in model.conditional_branches.items()}
                    self.assertEqual(actual, expected)
                    self.assertEqual({row[7] for row in model.records["PPB"]},
                                     {"if", "ifdef", "ifndef", "elseif", "elseifdef", "elseifndef", "else"} - {"ifdef"})
                    nested = [row for row in model.records["PPB"] if int(row[10]) in (12, 14, 16, 18)]
                    outer = next(row for row in model.records["PPB"] if row[10] == "11")
                    self.assertEqual({row[2] for row in nested}, {outer[1]})
                    if mode != "expressions":
                        names = {row[2].lower() for row in model.records["S"]}
                        self.assertTrue({"selected_first", "selected_nested", "selected_string"} <= names)
                        self.assertFalse(any(name.startswith("excluded_") for name in names))
                    probes = {(row[5].lower(), row[6]) for row in model.records["PPT"]}
                    self.assertEqual(probes, {("flag", "1"), ("missing", "0")})
                    for skipped in model.records["PPS"]:
                        self.assertEqual(model.conditional_decisions[int(skipped[2])][4], "0")
                    spans = {(int(row[6]), int(row[7]), int(row[8]), int(row[9])) for row in model.records["PPS"]}
                    self.assertTrue({(3, 0, 4, 0), (7, 0, 8, 0), (9, 0, 10, 0),
                                     (13, 0, 14, 2), (17, 0, 18, 2), (19, 0, 20, 2), (23, 0, 24, 0)} <= spans)

    def test_inactive_conditionals_do_not_parse_or_repeat_callbacks(self) -> None:
        source = self.source("""__FB_UNIQUEID_PUSH__(KEPT)
dim __FB_UNIQUEID__(KEPT) as long = 17
#if 0
  #if __FB_UNIQUEID_POP__(KEPT) 1
  this is invalid BASIC
  #elseif defined(NEVER_QUERIED)
  #else
  #endif
  #include "absent.bi"
  #error "inactive error"
#endif
#if 1
print __FB_UNIQUEID__(KEPT)
#elseif __FB_UNIQUEID_PUSH__(ONCE) 1
this is also invalid BASIC
#endif
__FB_UNIQUEID_POP__(ONCE)
#if __FB_QUOTE__(__FB_UNIQUEID__(ONCE)) <> ""
#error "callback was evaluated twice"
#endif
__FB_UNIQUEID_POP__(KEPT)
""")
        for backend in self.backends:
            outputs = []
            for mode in ("off", "full", "bindings", "expressions"):
                with self.subTest(backend=backend, mode=mode):
                    _, path = self.invoke([source], backend=backend, mode=mode)
                    suffix = ".ll" if backend == "llvm" else ".asm" if backend in ("gas", "gas64") else ".c"
                    outputs.append(source.with_suffix(suffix).read_bytes())
                    if mode == "off":
                        continue
                    model = Model.read(path, expressions_only=mode == "expressions", bindings_only=mode == "bindings")
                    branches = {int(row[10]): row for row in model.records["PPB"]}
                    for line in (4, 6, 7):
                        self.assertEqual(model.conditional_decisions[int(branches[line][1])][2:5],
                                         ["parent-inactive", "", "0"])
                    self.assertEqual(model.conditional_decisions[int(branches[14][1])][2:5],
                                     ["evaluated", "1", "0"])
                    self.assertFalse(any(row[5] == "NEVER_QUERIED" for row in model.records["PPT"]))
                    self.assertFalse(model.records["INC"])
                    self.assertEqual([row[1] for row in model.records["D"]], [self.compiler_path(source)])
                    if mode != "expressions":
                        self.assertFalse(any(row[2].lower() == "never_queried" for row in model.records["S"]))
                        self.assertFalse(any(int(row[5]) in (5, 15) for row in model.records["B"]))
            self.assertTrue(all(output == outputs[0] for output in outputs[1:]), backend)

    def test_conditional_contexts_include_remap_and_module_reset(self) -> None:
        header = self.working / "conditional.bi"
        header.write_text("#ifdef FLAG\ndim present_value as long\n#else\ndim absent_value as long\n#endif\n")
        source = self.source("""#if 1
#define FLAG 1
namespace First
#include "conditional.bi"
end namespace
#undef FLAG
namespace Second
#include "conditional.bi"
end namespace
#endif
#line 600 "conditional-logical.bas"
#if 0
not_valid BASIC
#else
dim mapped_value as long
#endif
""")
        second = self.source("#if 1\ndim separate_value as long\n#endif\n", "second.bas")
        for mode in ("full", "bindings", "expressions"):
            with self.subTest(mode=mode):
                _, path = self.invoke([source, second], mode=mode)
                model = Model.read(path, expressions_only=mode == "expressions", bindings_only=mode == "bindings")
                included = [row for row in model.records["PPB"] if row[9] == self.compiler_path(header)]
                self.assertEqual(len({row[4] for row in included}), 2)
                self.assertEqual([model.conditional_decisions[int(row[1])][4] for row in included if row[7] == "ifdef"],
                                 ["1", "0"])
                self.assertTrue(all(row[2] != "0" for row in included))
                mapped = [row for row in model.records["PPB"] if row[9] == "conditional-logical.bas"]
                self.assertEqual(len(mapped), 2)
                self.assertTrue(all(row[8] == "0" for row in mapped))
                following = [row for row in model.records["PPB"] if row[9] == self.compiler_path(second)]
                self.assertEqual(len(following), 1)
                self.assertEqual(following[0][2], "0")
                self.assertEqual(model.source_contexts[int(following[0][4])][4], "2")
                self.assertNotIn("conditional-logical.bas", [row[1] for row in model.records["D"]])

    def test_inactive_conditional_nesting_can_exceed_parser_limit(self) -> None:
        source = self.source("#if 0\n" + "#if invalid expression\n" * 80 +
                             "this must not parse\n" + "#endif\n" * 81 + "dim accepted_value as long\n")
        for mode in ("full", "expressions"):
            model = self.compile(source, mode=mode)
            self.assertEqual(len(model.records["PPB"]), 81)
            self.assertEqual(len(model.records["PPE"]), 81)
            self.assertEqual(sum(row[2] == "parent-inactive" for row in model.records["PPD"]), 80)

    def test_conditional_reader_rejects_fabricated_graphs(self) -> None:
        source = self.source("#define FLAG 1\n#if 1\ndim chosen as long\n#else\nnot valid\n#endif\n#ifdef FLAG\n#endif\n")
        _, path = self.invoke([source])
        rows = [line.split("\t") for line in path.read_text().splitlines()]
        changed_fields = {
            "PPB": ((1, "0"), (2, "999999"), (3, "999999"), (4, "999999"),
                    (5, "999999"), (6, "2"), (7, "unknown"), (8, "2")),
            "PPD": ((1, "999999"), (2, "unknown"), (3, ""), (4, "0")),
            "PPE": ((1, "999999"), (2, "999999")),
            "PPT": ((2, "999999"), (2, next(row[1] for row in rows if row[0] == "PPB")),
                    (3, "999999"), (6, "0")),
            "PPS": ((2, next(row[1] for row in rows if row[0] == "PPB")), (3, "999999")),
        }
        for tag, mutations in changed_fields.items():
            index = next(index for index, row in enumerate(rows) if row[0] == tag)
            for field, value in mutations:
                with self.subTest(tag=tag, field=field), self.assertRaises(ValueError):
                    changed = [row.copy() for row in rows]
                    changed[index][field] = value
                    Model("\n".join("\t".join(row) for row in changed) + "\n")
        for tag in ("PPD", "PPE"):
            with self.subTest(missing=tag), self.assertRaisesRegex(ValueError, "[Cc]onditional"):
                changed = [row.copy() for row in rows if row[0] != tag]
                changed[-1][12] = str(int(changed[-1][12]) - sum(row[0] == tag for row in rows))
                Model("\n".join("\t".join(row) for row in changed) + "\n")

    def test_conditional_recovery_does_not_claim_valid_evaluation(self) -> None:
        source = self.source("dim runtime_value as long\n#if runtime_value\nprint 1\n#endif\n")
        _, path = self.invoke([source], mode="expressions", success=False)
        model = Model.read(path, expressions_only=True, allow_recovery=True)
        self.assertEqual([row[2:5] for row in model.records["PPD"]], [["invalid", "", "0"]])
        self.assertTrue(model.diagnostics)
        self.assertTrue(all(row[2] == "error" and int(row[3]) > 0 for row in model.diagnostics.values()))
        self.assertTrue(any(row[7] == self.compiler_path(source) for row in model.diagnostics.values()))
        for field, value in ((2, "information"), (3, "-1"), (9, "999999"), (10, "999999"), (11, "999999")):
            with self.subTest(diagnostic_field=field), self.assertRaises(ValueError):
                changed = [row.copy() for row in model.rows]
                next(row for row in changed if row[0] == "DI")[field] = value
                Model("\n".join("\t".join(row) for row in changed) + "\n",
                      expressions_only=True, allow_recovery=True)
        unclosed = self.source("#if 1\nprint 1\n", "unclosed.bas")
        _, path = self.invoke([unclosed], mode="expressions", success=False)
        model = Model.read(path, expressions_only=True, allow_recovery=True)
        self.assertEqual(model.footer[0], "RECOVERY")
        self.assertEqual(len(model.records["PPB"]), 1)
        self.assertFalse(model.records["PPE"])

    def test_generated_conditional_locations_remain_noneditable(self) -> None:
        text = """#macro SelectValue()
  #if 1
  dim expanded_value as long
  #else
  invalid BASIC
  #endif
#endmacro
SelectValue()
"""
        for encoding in ("ascii", "utf-16le"):
            source = self.working / (encoding + ".bas")
            source.write_bytes(text.encode("ascii") if encoding == "ascii" else codecs.BOM_UTF16_LE + text.encode("utf-16le"))
            for mode in ("full", "bindings", "expressions"):
                with self.subTest(encoding=encoding, mode=mode):
                    _, path = self.invoke([source], mode=mode)
                    model = Model.read(path, expressions_only=mode == "expressions", bindings_only=mode == "bindings")
                    self.assertEqual(len(model.records["PPB"]), 2)
                    self.assertEqual([row[4] for row in model.records["PPD"]], ["1", "0"])
                    for tag, flag_column in (("PPB", 8), ("PPD", 5), ("PPE", 3)):
                        self.assertTrue(all(row[flag_column] == "0" for row in model.records[tag]))
                    self.assertTrue(any(row[7:11] == [row[7], row[8], row[7], row[8]]
                                        for row in model.records["PPD"]))
                    if mode == "full":
                        rows = path.read_text().splitlines()
                        index = next(index for index, line in enumerate(rows) if line.startswith("PPD\t"))
                        corrupted = rows[index].split("\t")
                        corrupted[5] = "1"
                        rows[index] = "\t".join(corrupted)
                        with self.assertRaises(ValueError):
                            Model("\n".join(rows) + "\n")

    def test_line_only_remaps_do_not_restore_physical_locations(self) -> None:
        header = self.working / "after-remap.bi"
        header.write_text("#if 1\ndim actual_include_value as long\n#endif\n")
        source = self.source("""dim first_value as long
#line 1
#if 1
dim remapped_value as long
#endif
#line 2
#include "after-remap.bi"
#if 1
print remapped_value
#endif
""")
        for mode in ("full", "bindings", "expressions"):
            with self.subTest(mode=mode):
                model = self.compile(source, mode=mode)
                self.assertEqual([row[4] for row in model.records["MAP"]], ["1", "0"])
                roots = [row for row in model.records["PPB"] if row[9] == self.compiler_path(source)]
                self.assertEqual(len(roots), 2)
                self.assertTrue(all(row[8] == "0" for row in roots))
                included = [row for row in model.records["PPB"] if row[9] == self.compiler_path(header)]
                self.assertEqual(len(included), 1)
                self.assertEqual(included[0][8], "1")
                self.assertEqual(model.records["INC"][0][6], "0")
                include_context = next(row for row in model.records["SRC"] if row[5] == "include")
                self.assertEqual(include_context[8], "0")
                if mode != "expressions":
                    identities = [row[1] for row in model.records["S"] if row[2].lower() == "remapped_value"]
                    self.assertEqual(len(identities), 1)
                    bindings = [row for row in model.records["B"] if row[1] == identities[0]]
                    self.assertTrue(bindings)
                    self.assertTrue(all(row[3] == "0" for row in bindings))

    def test_inactive_conditional_syntax_is_observed_without_validation(self) -> None:
        source = self.source("""#if 0
#if invalid expression
#else
#elseif this is not a valid condition
#else
#endif
#endif
dim accepted_value as long
""")
        for mode in ("full", "bindings", "expressions"):
            with self.subTest(mode=mode):
                model = self.compile(source, mode=mode)
                nested = [row for row in model.records["PPB"] if row[2] != "0"]
                self.assertEqual([row[7] for row in nested], ["if", "else", "elseif", "else"])
                self.assertTrue(all(model.conditional_decisions[int(row[1])][2:5] ==
                                    ["parent-inactive", "", "0"] for row in nested))
                self.assertFalse(model.records["PPT"])

    def test_macro_expansions_arguments_substitutions_and_origins(self) -> None:
        text = """#define SEM_VALUE 3
#define SEM_TWICE(x) ((x)*2)
#define SEM_INDIRECT SEM_TWICE
#define SEM_STRINGIFY(x) #x
#define SEM_JOIN(a,b) a##b
#define SEM_EMPTY
#define SEM_COUNT(args...) __FB_ARG_COUNT__(args)
dim SEM_JOIN(joined,SEM_VALUE) as long = SEM_INDIRECT(SEM_VALUE)
dim nested_value as long = SEM_TWICE(SEM_TWICE(2))
dim text_value as string = SEM_STRINGIFY(hello)
dim count_value as long = SEM_COUNT(1,2,3)
print SEM_EMPTY joined3
"""
        for encoding in ("ascii", "utf-16le"):
            source = self.working / (encoding + ".bas")
            source.write_bytes(text.encode("ascii") if encoding == "ascii" else codecs.BOM_UTF16_LE + text.encode("utf-16le"))
            for backend in self.backends:
                emissions = []
                for mode in ("off", "full", "bindings", "expressions"):
                    with self.subTest(encoding=encoding, backend=backend, mode=mode):
                        _, path = self.invoke([source], backend=backend, mode=mode)
                        suffix = ".ll" if backend == "llvm" else ".asm" if backend in ("gas", "gas64") else ".c"
                        emissions.append(source.with_suffix(suffix).read_bytes())
                        if mode == "off":
                            continue
                        model = Model.read(path, expressions_only=mode == "expressions", bindings_only=mode == "bindings")
                        outcomes: dict[str, list[str]] = {}
                        for identity, result in model.macro_results.items():
                            invocation = model.macro_invocations[identity]
                            name = model.macro_definitions[int(invocation[3])][5]
                            units = model.macro_units(result[3], result[5])
                            value = units.decode() if isinstance(units, bytes) else "".join(chr(unit) for unit in units)
                            outcomes.setdefault(name, []).append(value)
                        self.assertEqual(outcomes["SEM_JOIN"], ["joined3"])
                        self.assertEqual(outcomes["SEM_STRINGIFY"], ['$"hello"'])
                        self.assertEqual(outcomes["SEM_EMPTY"], [""])
                        self.assertEqual(outcomes["SEM_COUNT"], ["__FB_ARG_COUNT__(1,2,3)"])
                        self.assertEqual(outcomes["__FB_ARG_COUNT__"], ["3"])
                        self.assertEqual(sorted(outcomes["SEM_TWICE"]), sorted(["((3)*2)", "((2)*2)", "((((2)*2))*2)"]))
                        self.assertTrue({"root", "argument", "replacement"} <= {row[8] for row in model.records["MI"]})
                        join_definition = next(identity for identity, row in model.macro_definitions.items() if row[5] == "SEM_JOIN")
                        self.assertEqual([row[5] for row in model.macro_tokens[join_definition] if row[3] != "parameter"], ["0", "1"])
                        self.assertTrue({"parameter", "stringify", "callback", "text", "definition-text"} <=
                                        {row[5] for row in model.records["MS"]})
                        self.assertEqual(len(model.records["MC"]), 1)
                        if mode == "expressions":
                            self.assertTrue(all(row[2] == "0" for row in model.records["MD"]))
                        else:
                            joined = next(row[1] for row in model.records["S"] if row[2].lower() == "joined3")
                            bindings = {str(index) for index, row in enumerate(model.records["B"], 1) if row[1] == joined}
                            self.assertTrue(any(row[1] == "binding" and row[2] in bindings or
                                                row[1] == "symbol" and row[2] == joined and row[4].startswith("declaration-")
                                                for row in model.records["MR"]))
                            self.assertTrue(all(row[3] == "0" for row in model.records["B"] if row[1] == joined and row[2] == "declaration"))
            self.assertTrue(all(item == emissions[0] for item in emissions[1:]))

    def test_macro_reference_origins_keep_selected_fields_and_overloads(self) -> None:
        source = self.fixture("macro-reference-origins.bas")
        for backend in self.backends:
            with self.subTest(backend=backend):
                emissions = []
                for mode in ("off", "full", "bindings", "expressions"):
                    _, path = self.invoke([source], backend=backend, mode=mode)
                    suffix = ".ll" if backend == "llvm" else ".asm" if backend in ("gas", "gas64") else ".c"
                    emissions.append(source.with_suffix(suffix).read_bytes())
                    if mode == "off":
                        continue
                    model = Model.read(path, expressions_only=mode == "expressions", bindings_only=mode == "bindings")
                    coverage = [row[3] for row in model.records["CAP"] if row[2] == "macro-reference-origins"]
                    self.assertEqual(coverage, ["available" if mode == "full" else "unavailable"])
                    references = [row for row in model.records["MR"]
                                  if row[1] == "symbol" and row[4].startswith("reference-")]
                    if mode != "full":
                        self.assertFalse(references)
                        continue
                    self.assertEqual(len({row[4] for row in references}), len(references))
                    field_ids = {str(identity) for identity, row in model.types.items() if row[3] == "field"}
                    field_uses = [row for row in references if row[2] in field_ids]
                    self.assertEqual(len(field_uses), 4)
                    selected = [row for row in references if model.symbol_name(int(row[2])) == "PICK"]
                    self.assertEqual(len(selected), 2)
                    self.assertEqual(len({row[2] for row in selected}), 2)
                    for reference in field_uses + selected:
                        invocation = model.macro_invocations[int(reference[3])]
                        self.assertEqual(invocation[7], "normal")
                        self.assertIn(int(reference[3]), model.macro_results)
                    # Physical references retain their existing B ranges. The
                    # new symbol-level observations are not editable bindings.
                    self.assertTrue(any(row[2] == "reference" and row[3] == "1" and row[1] in field_ids
                                        for row in model.records["B"]))
                self.assertTrue(all(emission == emissions[0] for emission in emissions[1:]))

    def test_implicit_calls_keep_optional_physical_coordinate_receipts(self) -> None:
        text = (self.root / "tests/semantic-sidecar/implicit-call-coordinates.bas").read_text(encoding="utf-8")
        for encoding in ("utf-8", "utf-16le", "utf-16be", "utf-32le", "utf-32be"):
            source = self.working / (encoding + ".bas")
            prefix = {"utf-8": codecs.BOM_UTF8, "utf-16le": codecs.BOM_UTF16_LE,
                      "utf-16be": codecs.BOM_UTF16_BE, "utf-32le": codecs.BOM_UTF32_LE,
                      "utf-32be": codecs.BOM_UTF32_BE}[encoding]
            source.write_bytes(prefix + text.encode(encoding))
            for backend in self.backends:
                with self.subTest(encoding=encoding, backend=backend):
                    _, path = self.invoke([source], backend=backend)
                    model = Model.read(path)
                    self.assertEqual(model.capabilities[1]["implicit-call-coordinates"], "available")
                    properties = {(row[2], row[3]): row[4] for row in model.records["K"] if row[1] == "symbol"}
                    remapped = False
                    for ordinal, call in enumerate(model.records["I"], 1):
                        coordinate = properties[(call[2], "implicit-call-coordinate-" + str(ordinal))].split("\t")
                        self.assertEqual(len(coordinate), 8)
                        self.assertIn(int(coordinate[0]), model.source_contexts)
                        self.assertEqual(coordinate[7], "mapped")
                        self.assertLess(int(coordinate[5]), int(coordinate[6]))
                        self.assertIn((call[2], "implicit-call-origin-" + str(ordinal)), properties)
                        if call[6] == "virtual-construction.bas":
                            remapped = True
                            self.assertEqual(call[7], "801")
                            self.assertEqual(coordinate[1], "19")
                    self.assertTrue(remapped)
                    self.assertTrue(any(row[1] == "symbol" and row[4].startswith("construction-")
                                        for row in model.records["MR"]))

    def test_macro_reference_capability_is_unavailable_without_provenance(self) -> None:
        source = self.fixture("macro-reference-origins.bas")
        _, path = self.invoke([source], extra=("-semantic-model-compact",))
        model = Model.read(path)
        self.assertEqual([row[3] for row in model.records["CAP"] if row[2] == "macro-reference-origins"],
                         ["unavailable"])
        self.assertFalse(model.records["MR"])

    def test_compact_mode_omits_macro_graph_without_dropping_requested_semantics(self) -> None:
        source = self.source("#define SEM_COMPACT(x) ((x) + 1)\n"
                             "dim compact_value as long = SEM_COMPACT(4)\n"
                             "print compact_value + 2\n")
        macro_tags = ("MD", "MT", "MI", "MA", "MS", "MC", "ME", "ML", "MR")
        for mode in ("full", "bindings", "expressions"):
            with self.subTest(mode=mode):
                _, path = self.invoke([source], mode=mode, extra=("-semantic-model-compact",))
                model = Model.read(path, expressions_only=mode == "expressions",
                                   bindings_only=mode == "bindings")
                self.assertFalse(any(model.records[tag] for tag in macro_tags))
                self.assertTrue(model.records["D"])
                if mode == "full":
                    self.assertTrue(model.records["N"])
                elif mode == "bindings":
                    self.assertTrue(model.records["B"])
                else:
                    self.assertTrue(model.records["E"])

    def test_compact_full_model_keeps_generated_array_expression_anchor(self) -> None:
        call = "SEM_RGB(digits(0) * 17, digits(1) * 17, digits(2) * 17)"
        source = self.source("#define SEM_RGB(red, green, blue) (((red) * 65536) + ((green) * 256) + (blue))\n"
                             "sub ObserveColor()\n"
                             "    dim digits(0 to 2) as integer\n"
                             "    dim color_value as long\n"
                             "    color_value = " + call + "\n"
                             "end sub\n")
        invocation = self.span(source, call, "SEM_RGB")
        macro_tags = ("MD", "MT", "MI", "MA", "MS", "MC", "ME", "ML", "MR")
        complete = self.compile(source)
        compact = self.compile(source, extra=("-semantic-model-compact",))

        for model in (complete, compact):
            anchored = self.expressions(model, invocation)
            self.assertTrue(anchored, f"Missing generated expression anchor at {invocation}")
            self.assertTrue(all(row[2] == "0" for row in anchored), anchored)
        self.assertFalse(any(compact.records[tag] for tag in macro_tags))
        self.assertEqual(compact.capabilities[1]["macro-expansions"], "unavailable")

    def test_macro_lifetimes_preserve_retired_definitions_and_missing_undef(self) -> None:
        source = self.source("#define SEM_REVISED 3\n#define SEM_REVISED 3\ndim first_value as long = SEM_REVISED\n"
                             "#undef SEM_REVISED\n#undef SEM_ABSENT\n#define SEM_REVISED 4\ndim second_value as long = SEM_REVISED\n")
        for mode in ("full", "bindings", "expressions"):
            model = self.compile(source, mode=mode)
            self.assertEqual([row[3] for row in model.records["ML"]], ["define", "identical", "undef", "undef-missing", "define"])
            definitions = [row for row in model.records["MD"] if row[5] == "SEM_REVISED"]
            self.assertEqual(len(definitions), 2)
            self.assertNotEqual(definitions[0][1], definitions[1][1])
            self.assertEqual([row[5] for row in model.records["ME"]], ["3", "4"])
            self.assertEqual(model.records["ML"][3][2], "0")
            self.assertEqual([row[4] for row in model.macro_tokens[int(definitions[0][1])]], ["3"])
            self.assertEqual([row[4] for row in model.macro_tokens[int(definitions[1][1])]], ["4"])

    def test_macro_name_only_and_empty_arguments_do_not_fabricate_expansions(self) -> None:
        source = self.source("#define SEM_ARGLESS() 11\n#define SEM_OPTIONAL(x,args...) x\n"
                             "dim count_value as long = __FB_ARG_COUNT__(SEM_ARGLESS)\n"
                             "dim actual_value as long = SEM_ARGLESS()\ndim optional_value as long = SEM_OPTIONAL(5)\n")
        model = self.compile(source)
        attempts = [identity for identity, row in model.macro_invocations.items()
                    if model.macro_definitions[int(row[3])][5] == "SEM_ARGLESS"]
        self.assertEqual([model.macro_results[identity][2] for identity in attempts], ["not-invoked", "expanded"])
        self.assertEqual(model.macro_results[attempts[0]][4:7], ["0", "", "0"])
        self.assertFalse(model.macro_segments[attempts[0]])
        optional = next(identity for identity, row in model.macro_invocations.items()
                        if model.macro_definitions[int(row[3])][5] == "SEM_OPTIONAL")
        empty = model.macro_arguments[optional][1]
        self.assertEqual(empty[4:8], ["", "0", "0", ""])
        self.assertEqual(empty[8:], ["0", "0", "0", "0"])

    def test_macro_operator_origins_follow_consumption_inside_physical_operands(self) -> None:
        source = self.source("#define SEM_PLUS +\ndim value as long = 1 SEM_PLUS 2\n")
        for mode in ("full", "expressions"):
            model = self.compile(source, mode=mode)
            plus = next(identity for identity, row in model.macro_invocations.items()
                        if model.macro_definitions[int(row[3])][5] == "SEM_PLUS")
            expressions = [row for row in model.records["E"] if row[2] == "0" and row[4] == "2"]
            self.assertTrue(expressions)
            self.assertTrue(any(domain == "expression" and expansion == plus and role.startswith("token-")
                                for (domain, subject, role), expansion in model.macro_origins.items()
                                if subject in {int(row[1]) for row in expressions}))

    def test_macro_failed_and_recursive_attempts_remain_recovery(self) -> None:
        fixtures = (("#define SEM_RECURSE(x) SEM_RECURSE(x)\nprint SEM_RECURSE(1)\n", "recursive"),
                    ("print __FB_ARG_EXTRACT__(not_a_number,1)\n", "failed"))
        for text, expected in fixtures:
            with self.subTest(outcome=expected):
                source = self.source(text, expected + ".bas")
                _, path = self.invoke([source], mode="expressions", success=False)
                model = Model.read(path, expressions_only=True, allow_recovery=True)
                failures = [identity for identity, row in model.macro_results.items() if row[2] == expected]
                self.assertTrue(failures)
                if expected == "recursive":
                    self.assertTrue(all(model.macro_results[identity][4] == "0" for identity in failures))
                    self.assertTrue(all(not model.macro_segments[identity] for identity in failures))
        rejected = self.source("#define DOUBLE(x) x\nprint 1\n", "keyword.bas")
        _, path = self.invoke([rejected], mode="expressions", success=False)
        model = Model.read(path, expressions_only=True, allow_recovery=True)
        self.assertTrue(any(row[3] == "definition-rejected" for row in model.records["ML"]))
        self.assertFalse(any(row[5] == "DOUBLE" for row in model.records["MD"]))

    def test_macro_reader_rejects_fabricated_provenance_and_substitutions(self) -> None:
        source = self.source("#define SEM_TEXT(x) #x\n#define SEM_VALUE 2\ndim text_value as string = SEM_TEXT(SEM_VALUE)\n#undef SEM_VALUE\n")
        _, path = self.invoke([source])
        rows = [line.split("\t") for line in path.read_text().splitlines()]
        modifications = {
            "MD": ((1, "0"), (2, "999999"), (4, "999999"), (6, "unknown"), (7, "33"), (8, "16")),
            "MT": ((1, "999999"), (3, "unknown"), (5, "2")),
            "MI": ((2, "999999"), (3, "999999"), (4, "999999"), (7, "unknown"), (8, "callback")),
            "MA": ((2, "99999"), (3, "unknown"), (5, "0")),
            "MS": ((2, "99999"), (3, "99999"), (4, "99999"), (5, "unknown"), (6, "1"), (7, "99999")),
            "ME": ((2, "unknown"), (3, "unknown"), (4, "99999"), (5, "different")),
            "ML": ((2, "999999"), (3, "unknown"), (4, "999999")),
            "MR": ((1, "unknown"), (2, "999999"), (3, "999999")),
        }
        for tag, mutations in modifications.items():
            index = next(index for index, row in enumerate(rows) if row[0] == tag)
            for field, value in mutations:
                with self.subTest(tag=tag, field=field), self.assertRaises(ValueError):
                    changed = [row.copy() for row in rows]
                    changed[index][field] = value
                    Model("\n".join("\t".join(row) for row in changed) + "\n")
        with self.assertRaisesRegex(ValueError, "Macro expansion graph is incomplete"):
            changed = [row.copy() for row in rows if row[0] != "ME"]
            changed[-1][12] = str(int(changed[-1][12]) - sum(row[0] == "ME" for row in rows))
            Model("\n".join("\t".join(row) for row in changed) + "\n")

    def test_macro_optional_parentheses_restore_caller_delimiters(self) -> None:
        text = "#macro SEM_OPTION ?(x)\nx\n#endmacro\nprint SEM_OPTION 7: print 8\n"
        for wide in (False, True):
            source = self.working / ("optional-wide.bas" if wide else "optional.bas")
            source.write_bytes(codecs.BOM_UTF16_LE + text.encode("utf-16le") if wide else text.encode())
            model = self.compile(source)
            self.assertEqual([row[5] for row in model.records["MS"]], ["parameter", "restored-delimiter"])
            result = model.records["ME"][0]
            units = model.macro_units(result[3], result[5])
            self.assertEqual(units, (55, 58) if wide else b"7:")
            self.assertEqual([row[6:8] for row in model.records["MS"]], [["0", "1"], ["1", "1"]])

    def test_macro_arguments_with_invalid_extents_are_nonphysical_points(self) -> None:
        source = self.working / "macro_no_parentheses.bas"
        shutil.copyfile(self.root / "tests/pp/macro_no_parentheses.bas", source)
        model = self.compile(source, mode="expressions",
                             extra=("-i", str(self.root / "tests/fbcunit/inc")))
        arguments = model.records["MA"]
        self.assertTrue(arguments)
        points = [row for row in arguments if row[5] == "1" and row[6] == "0" and
                  row[8] == row[10] and row[9] == row[11]]
        self.assertTrue(points, "invalid macro argument extents should retain only a nonphysical start point")

    def test_macro_callback_outputs_and_argument_reuse_preserve_execution(self) -> None:
        source = self.source("__FB_UNIQUEID_PUSH__(SEM_STACK)\n"
                             "dim __FB_UNIQUEID__(SEM_STACK) as long = 17\n"
                             "dim evaluated_value as long = __FB_EVAL__(1 + 2)\n"
                             "dim source_line as long = __LINE__\n"
                             "print __FB_UNIQUEID__(SEM_STACK) + evaluated_value\n"
                             "__FB_UNIQUEID_POP__(SEM_STACK)\n" +
                             "#assert __FB_ARG_COUNT__(1,2,3) = 3\n" * 100)
        outcomes = []
        for mode in ("off", "full", "bindings", "expressions"):
            with self.subTest(mode=mode):
                executable = self.working / (mode + (".exe" if self.native_windows else ""))
                _, path = self.invoke([source], mode=mode, emit=False, extra=("-x", str(executable)))
                result = subprocess.run([str(executable)], capture_output=True, timeout=20)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout.strip(), b"20")
                outcomes.append(result.stdout)
                if mode == "off":
                    continue
                model = Model.read(path, expressions_only=mode == "expressions", bindings_only=mode == "bindings")
                callbacks = {}
                for identity, row in model.macro_callbacks.items():
                    name = model.macro_definitions[int(model.macro_invocations[identity][3])][5]
                    callbacks.setdefault(name, []).append(row[4])
                self.assertEqual(callbacks["__FB_EVAL__"], ["3"])
                self.assertEqual(callbacks["__LINE__"], ["4"])
                self.assertEqual(len(callbacks["__FB_ARG_COUNT__"]), 100)
                self.assertEqual(len(callbacks["__FB_UNIQUEID_PUSH__"]), 1)
                self.assertEqual(len(callbacks["__FB_UNIQUEID_POP__"]), 1)
                self.assertEqual(len(set(callbacks["__FB_UNIQUEID__"])), 1)
                self.assertTrue(any(row[6] == "define-callback" for row in model.records["MD"]))
        self.assertTrue(all(item == outcomes[0] for item in outcomes[1:]))

    def test_physical_locations_map_encodings_boms_and_line_endings(self) -> None:
        text = 'dim root_value as long\nprint len("😀") + root_value\n#line 700 "logical.bas"\nprint root_value\n'
        encodings = (("utf-8", b""), ("utf-8", codecs.BOM_UTF8),
                     ("utf-16-le", codecs.BOM_UTF16_LE), ("utf-16-be", codecs.BOM_UTF16_BE),
                     ("utf-32-le", codecs.BOM_UTF32_LE), ("utf-32-be", codecs.BOM_UTF32_BE))
        for index, (encoding, bom) in enumerate(encodings):
            for line_ending in ("\n", "\r\n", "\r"):
                with self.subTest(encoding=encoding, bom=bool(bom), ending=repr(line_ending)):
                    source = self.working / f"coordinates-{index}.bas"
                    original = bom + text.replace("\n", line_ending).encode(encoding)
                    source.write_bytes(original)
                    model = self.compile(source)
                    model.validate_physical_locations()
                    value = self.one(model, "root_value", "variable")
                    references = {str(ordinal) for ordinal, row in enumerate(model.records["B"], 1)
                                  if row[1] == str(value) and row[2] == "reference"}
                    locations = [row for row in model.records["LOC"] if row[1] == "binding" and row[2] in references]
                    self.assertEqual(len(locations), 2)
                    self.assertEqual([row[5:9] for row in locations], [["2", "18", "2", "28"], ["4", "6", "4", "16"]])
                    needle = "root_value".encode(encoding)
                    offsets = [original.index(needle, len(bom)), original.index(needle, original.index(needle, len(bom)) + len(needle))]
                    final = original.rindex(needle)
                    self.assertEqual([int(row[9]) for row in locations], [offsets[1], final])
                    self.assertTrue(all(row[11] == "mapped" for row in locations))
                    self.assertTrue(all(row[3] == "0" for row in model.records["B"] if row[4] == "logical.bas"))

    def test_empty_colon_statements_do_not_export_reversed_physical_ranges(self) -> None:
        source = self.source("sub A overload(byval p as any ptr) :            : end sub\n")
        model = self.compile(source, mode="bindings")
        model.validate_physical_locations()
        statement_ids = {int(row[1]) for row in model.records["ST"]}
        located_statements = {int(row[2]) for row in model.records["LOC"] if row[1] == "statement"}
        self.assertTrue(statement_ids)
        self.assertLess(len(located_statements), len(statement_ids))

    def test_physical_locations_preserve_repeated_include_and_remap_origins(self) -> None:
        header = self.working / "physical-header.bi"
        header.write_text("#line 500\ndim repeated_value as long\n")
        source = self.source("namespace First\n#include \"physical-header.bi\"\nend namespace\n"
                             "#line 80 \"mapped-root.bas\"\nnamespace Second\n#include \"physical-header.bi\"\nend namespace\n")
        for mode in ("full", "bindings", "expressions"):
            with self.subTest(mode=mode):
                model = self.compile(source, mode=mode)
                model.validate_physical_locations()
                included = [row for row in model.records["SRC"] if row[5] == "include"]
                self.assertEqual(len(included), 2)
                self.assertNotEqual(included[0][1], included[1][1])
                include_locations = [row for row in model.records["LOC"] if row[1] == "include"]
                self.assertEqual([row[5] for row in include_locations], ["2", "6"])
                self.assertTrue(all(row[11] == "mapped" for row in include_locations))
                if mode != "expressions":
                    declarations = {str(index) for index, row in enumerate(model.records["B"], 1)
                                    if row[2] == "declaration" and row[5] == "501"}
                    locations = [row for row in model.records["LOC"] if row[1] == "binding" and row[2] in declarations]
                    self.assertEqual(len(locations), 2)
                    self.assertEqual({row[4] for row in locations}, {row[1] for row in included})
                    self.assertTrue(all(row[5] == "2" and row[11] == "mapped" for row in locations))

    def test_physical_location_reader_rejects_false_bytes_and_forged_origins(self) -> None:
        source = self.source('dim value as long\nprint len("😀") + value\n')
        _, path = self.invoke([source])
        model = Model.read(path)
        model.validate_physical_locations()
        rows = [line.split("\t") for line in path.read_text().splitlines()]
        index = next(index for index, row in enumerate(rows) if row[0] == "LOC")
        for field, value in ((1, "unknown"), (2, "999999"), (4, "999999"), (5, "0"),
                             (9, "-2"), (10, "999999"), (11, "unknown")):
            with self.subTest(field=field), self.assertRaises(ValueError):
                changed = [row.copy() for row in rows]
                changed[index][field] = value
                Model("\n".join("\t".join(row) for row in changed) + "\n")
        changed = [row.copy() for row in rows]
        changed[index][9] = str(int(changed[index][9]) + 1)
        forged = Model("\n".join("\t".join(row) for row in changed) + "\n")
        with self.assertRaisesRegex(ValueError, "Physical bytes disagree"):
            forged.validate_physical_locations()
        source.write_text("dim changed_value as long\n")
        with self.assertRaisesRegex(ValueError, "stale"):
            model.validate_physical_locations()

    def test_generated_and_malformed_locations_do_not_claim_byte_mappings(self) -> None:
        source = self.source("#define SEM_GENERATED(x) x\n#define SEM_INDIRECT SEM_GENERATED\ndim value as long = SEM_INDIRECT(7)\n")
        model = self.compile(source)
        generated = {row[1] for row in model.records["MI"] if row[9] == "0"}
        self.assertTrue(generated)
        self.assertFalse(any(row[1] == "macro-attempt" and row[2] in generated for row in model.records["LOC"]))
        malformed = self.working / "malformed-coordinates.bas"
        malformed.write_bytes(b'dim value as string = "\xe9" : print value\n')
        model = self.compile(malformed)
        self.assertTrue(model.records["LOC"])
        self.assertTrue(all(row[11] == "unverified" and row[9:11] == ["-1", "-1"] for row in model.records["LOC"]))

    def test_source_calls_formals_and_lowered_calls_keep_distinct_meanings(self) -> None:
        for backend in self.backends:
            with self.subTest(backend=backend):
                model = self.compile(self.fixture("sufficiency.bas"), backend=backend)
                target = self.one(model, "OptionalValue", "procedure")
                formal = int(model.parameters[target][0][2])
                self.assertEqual(model.signatures[target][5], "1")
                calls = [row for identity, row in model.nodes.items()
                         if model.properties["node", identity]["kind"] == "call"
                         and row[9] == str(target)]
                self.assertEqual(len(calls), 2)
                for call in calls:
                    argument = next(row for row in model.nodes.values()
                                    if row[2] == call[1] and row[3] == "right")
                    self.assertEqual(int(argument[9]), formal)
                    # "default" describes the passing mode selected from the
                    # formal, not whether this source omitted the argument.
                    self.assertEqual(model.properties["node", int(argument[1])]["passing-mode"], "default")
                    value = next(row for row in model.nodes.values()
                                 if row[2] == argument[1] and row[3] == "left")
                    self.assertEqual(model.constants["node", int(value[1])], ("signed", "7"))
                unevaluated = self.one(model, "UnevaluatedValue", "procedure")
                typed_calls = [row for row in model.records["E"] if row[13] == str(unevaluated)]
                self.assertEqual(len(typed_calls), 2)
                runtime_calls = [row for identity, row in model.nodes.items()
                                 if model.properties["node", identity]["kind"] == "call"
                                 and row[9] == str(unevaluated)]
                self.assertFalse(runtime_calls)

    def test_source_expression_links_access_roles_and_argument_defaults(self) -> None:
        for backend in self.backends:
            with self.subTest(backend=backend):
                source = self.fixture("sufficiency.bas")
                model = self.compile(source, backend=backend)
                target = self.one(model, "OptionalValue", "procedure")
                calls = [row for identity, row in model.nodes.items()
                         if model.properties["node", identity]["kind"] == "call" and row[9] == str(target)]
                defaults = []
                for call in calls:
                    argument = next(row for row in model.nodes.values() if row[2] == call[1] and row[3] == "right")
                    defaults.append(model.properties["node", int(argument[1])]["default-argument"])
                self.assertEqual(sorted(defaults), ["0", "1"])
                links = model.relations("source-expression")
                self.assertTrue(links)
                expression_ids = {row[1] for row in model.records["E"]}
                self.assertTrue(all(row[3] == "expression" and row[4] in expression_ids for row in links))
                omitted = self.one(model, "UnevaluatedValue", "procedure")
                source_calls = {row[1] for row in model.records["E"] if row[13] == str(omitted)}
                self.assertFalse(source_calls & {row[4] for row in links})
                for text, role in (("value = 1", "write"), ("value += 2", "read-write"),
                                   ("print value", "read"), ("ChangeValue(value)", "byref")):
                    span = self.span(source, text, "value")
                    binding = next(index for index, row in enumerate(model.records["B"], 1)
                                   if source_range(row, 4) == span and row[2] == "reference")
                    self.assertEqual(model.access_roles[binding], role)
                self.assertEqual(model.capabilities[1]["argument-default-origin"], "available")
                self.assertEqual(model.capabilities[1]["external-call-effects"], "unavailable")
                self.assertEqual(model.capabilities[1]["expression-node-links"], "partial")

    def test_binding_access_roles_work_without_ast_export(self) -> None:
        source = self.fixture("sufficiency.bas")
        model = self.compile(source, mode="bindings")
        self.assertFalse(model.nodes)
        self.assertTrue(model.access_roles)
        self.assertEqual(model.capabilities[1]["lowered-ast"], "unavailable")
        self.assertEqual(model.capabilities[1]["source-access-roles"], "partial")
        changed = model.rows.copy()
        row = next(index for index, row in enumerate(changed) if row[0] == "ACC")
        changed[row] = ["ACC", "999999999", "write"]
        with self.assertRaises(ValueError):
            Model("\n".join("\t".join(row) for row in changed) + "\n", bindings_only=True)

    def test_file_statements_have_source_operation_owners(self) -> None:
        source = self.source('sub FileOperations()\n'
                             'dim number as long = freefile()\n'
                             'open "input.txt" for input as #number\n'
                             'dim text as string\n'
                             'line input #number, text\n'
                             'close #number\nend sub\n')
        for mode in ("full", "bindings", "expressions"):
            with self.subTest(mode=mode):
                model = self.compile(source, mode=mode)
                self.assertEqual([row[2] for row in model.records["SOP"]], ["file-open", "line-input", "file-close"])
                for row in model.records["SOP"]:
                    identity = int(row[1])
                    self.assertIn(identity, model.statements)
                    self.assertIn(identity, model.statement_endings)
                    self.assertIn(("statement", identity, "range"), model.physical_locations)

    def test_source_construct_families_and_routes_preserve_emission(self) -> None:
        source = self.fixture("constructs.bas")
        kinds = {"if", "for", "do", "while", "select", "with", "scope", "namespace",
                 "extern", "procedure", "type", "union", "enum", "assembly"}
        routes = {"declaration", "compound", "call-or-assignment", "intrinsic", "assembly",
                  "pointer-or-assignment", "label", "aggregate-member", "enumerator", "assembly-line"}
        for backend in self.backends:
            outputs = []
            for mode in ("off", "full", "bindings", "expressions"):
                with self.subTest(backend=backend, mode=mode):
                    result, path = self.invoke([source], backend=backend, mode=mode)
                    suffix = ".ll" if backend == "llvm" else ".asm" if backend in ("gas", "gas64") else ".c"
                    outputs.append((result.stdout, result.stderr, source.with_suffix(suffix).read_bytes()))
                    if mode == "off":
                        continue
                    model = Model.read(path, expressions_only=mode == "expressions", bindings_only=mode == "bindings")
                    self.assertEqual({row[5] for row in model.constructs.values()}, kinds)
                    self.assertEqual({row[2] for row in model.statement_endings.values()}, routes)
                    self.assertEqual(set(model.statements), set(model.statement_endings))
                    self.assertEqual(set(model.constructs), set(model.construct_endings))
                    self.assertEqual({row[3] for row in model.statement_endings.values()}, {"parsed"})
                    self.assertTrue(all(row[7] == "1" for row in model.statements.values()))
                    model.validate_physical_locations()
            self.assertTrue(all(output == outputs[0] for output in outputs[1:]), backend)

    def test_statement_extents_nesting_and_fact_owners(self) -> None:
        source = self.fixture("constructs.bas")
        model = self.compile(source)
        lines = source.read_text().splitlines()

        def statement(anchor: str) -> list[str]:
            line = next(index for index, text in enumerate(lines, 1) if anchor in text)
            matches = [row for row in model.statements.values() if int(row[13]) == line]
            self.assertTrue(matches, anchor)
            return min(matches, key=lambda row: int(row[14]))

        first = statement("dim value as long = 1:")
        second = next(row for row in model.statements.values() if row[13] == first[13] and row[1] != first[1])
        first_span = model.physical_locations["statement", int(first[1]), "range"]
        second_span = model.physical_locations["statement", int(second[1]), "range"]
        colon = lines[int(first[13]) - 1].index(":")
        self.assertEqual(first_span[5:9], [first[13], "1", first[13], str(colon)])
        self.assertEqual(second_span[5:9], [first[13], str(colon + 2), first[13], str(len(lines[int(first[13]) - 1]))])
        self.assertNotEqual(first[1], second[1])
        self.assertEqual(first[2:4], second[2:4])
        self.assertEqual(first[2], "0")
        self.assertEqual(model.symbols[int(first[4])][2].lower(), "observe")

        inline = statement("if value then")
        children = [row for row in model.statements.values() if row[2] == inline[1]]
        self.assertEqual(len(children), 2)
        self.assertEqual({row[13] for row in children}, {inline[13]})
        self.assertEqual({model.statement_endings[int(row[1])][2] for row in children}, {"call-or-assignment"})
        self.assertFalse(any(lines[int(row[13]) - 1][int(row[14]):].startswith("else") for row in children))

        continued = statement("dim nested as long = value + _")
        span = model.physical_locations["statement", int(continued[1]), "range"]
        self.assertEqual(span[5:9], [continued[13], "2", str(int(continued[13]) + 1), "4"])
        scope = model.constructs[int(continued[3])]
        self.assertEqual(scope[5], "scope")
        self.assertEqual(model.constructs[int(scope[2])][5], "procedure")
        loops = [row for row in model.constructs.values() if row[5] == "for"]
        self.assertEqual(len(loops), 2)
        self.assertEqual(len({model.construct_endings[int(row[1])][2] for row in loops}), 1)

        span = self.span(source, "value += 2", "value")
        binding = next(index for index, row in enumerate(model.records["B"], 1) if source_range(row, 4) == span)
        self.assertEqual(model.statement_owners["binding", binding], int(second[1]))
        owned_nodes = [identity for domain, identity in model.statement_owners if domain == "node"]
        self.assertTrue(owned_nodes)
        self.assertTrue(any(model.nodes[identity][11] == "0" for identity in owned_nodes))
        self.assertFalse(any(row[1] == "node" for row in model.records["LOC"]))

    def test_construct_include_occurrences_generated_and_inactive_exclusions(self) -> None:
        header = self.source("sub Included()\nscope\ndim included_value as long = 1\nend scope\nend sub\n", "constructs.bi")
        source = self.source('#define GENERATED_STMT dim generated_value as long = 7\n'
                             'GENERATED_STMT\n#if 0\nscope\nthis is invalid BASIC\nend scope\n#endif\n'
                             'namespace First\n#include "constructs.bi"\nend namespace\n'
                             'namespace Second\n#include "constructs.bi"\nend namespace\n'
                             '#line 500 "logical-constructs.bas"\nscope\nprint generated_value\nend scope\n')
        extra_source = self.source("scope\ndim second_module_value as long\nend scope\n", "second-module.bas")
        for mode in ("full", "bindings", "expressions"):
            with self.subTest(mode=mode):
                _, path = self.invoke([source, extra_source], mode=mode)
                model = Model.read(path, expressions_only=mode == "expressions", bindings_only=mode == "bindings")
                model.validate_physical_locations()
                included = [row for row in model.statements.values() if row[12] == self.compiler_path(header)]
                self.assertEqual(len({row[5] for row in included}), 2)
                self.assertEqual({row[7] for row in model.statements.values()}, {"1", "2"})
                self.assertFalse(any(row[12] == self.compiler_path(source) and 3 <= int(row[13]) <= 7
                                     for row in model.statements.values()))
                generated = [row for row in model.statements.values()
                             if row[12] == self.compiler_path(source) and row[13] == "2"]
                self.assertTrue(generated)
                self.assertTrue(all(row[11] == "0" for row in generated))
                self.assertFalse(any(("statement", int(row[1]), "range") in model.physical_locations for row in generated))
                logical = [row for row in model.statements.values() if row[12] == "logical-constructs.bas"]
                self.assertTrue(logical)
                self.assertTrue(all(row[11] == "0" for row in logical))
                self.assertTrue(all(("statement", int(row[1]), "range") in model.physical_locations for row in logical))

    def test_construct_reader_rejects_corrupt_ownership_and_closure(self) -> None:
        model = self.compile(self.fixture("constructs.bas"))
        rows = model.rows
        indexes = {tag: next(index for index, row in enumerate(rows) if row[0] == tag)
                   for tag in ("ST", "STE", "BLK", "BEND", "OWN")}
        for tag, field, value in (("ST", 1, "0"), ("ST", 2, "999999999"), ("ST", 3, "999999999"),
                                  ("ST", 5, "999999999"), ("ST", 6, "999999999"), ("ST", 7, "2"),
                                  ("STE", 1, "999999999"), ("STE", 2, "guessed-route"), ("STE", 3, "unmatched"),
                                  ("BLK", 2, "999999999"), ("BLK", 3, "0"), ("BLK", 5, "guessed-kind"),
                                  ("BEND", 1, "999999999"), ("BEND", 2, "999999999"),
                                  ("OWN", 1, "guessed-domain"), ("OWN", 2, "999999999"), ("OWN", 3, "999999999")):
            with self.subTest(tag=tag, field=field):
                changed = [row.copy() for row in rows]
                changed[indexes[tag]][field] = value
                with self.assertRaises(ValueError):
                    Model("\n".join("\t".join(row) for row in changed) + "\n")
        identity = model.records["BEND"][-1][1]
        changed = [row.copy() for row in rows if not (row[0] == "BEND" and row[1] == identity
                   or row[0] == "LOC" and row[1:3] == ["construct", identity])]
        changed[-1][12] = str(sum(row[0] in DETAIL_TAGS for row in changed))
        with self.assertRaises(ValueError):
            Model("\n".join("\t".join(row) for row in changed) + "\n")

    def test_statement_recovery_and_unfinished_constructs_remain_provisional(self) -> None:
        for text in ("dim good as long\n@\nprint good\n", "scope\ndim good as long\n"):
            source = self.source(text)
            with self.subTest(text=text):
                disabled, _ = self.invoke([source], mode="off", success=False)
                observed, path = self.invoke([source], mode="expressions", success=False)
                self.assertEqual(observed.stdout, disabled.stdout)
                self.assertEqual(observed.stderr, disabled.stderr)
                model = Model.read(path, expressions_only=True, allow_recovery=True)
                self.assertEqual(model.footer[0], "RECOVERY")
                self.assertTrue(model.statements)
                if text.startswith("scope"):
                    self.assertTrue(model.constructs)
                    self.assertNotEqual(set(model.constructs), set(model.construct_endings))
                else:
                    self.assertTrue(any(row[3] == "unmatched" for row in model.statement_endings.values()))
                _, full_path = self.invoke([source], success=False)
                if full_path.exists():
                    self.assertNotIn("END\t", full_path.read_text())

    def test_legacy_numeric_labels_retain_statement_boundaries(self) -> None:
        source = self.source("10 dim value as integer\n20 value = 1: ? value\n30 end\n")
        for language in ("qb", "deprecated", "fblite"):
            with self.subTest(language=language):
                model = self.compile(source, mode="bindings", extra=("-lang", language))
                labels = [row for row in model.statements.values() if model.statement_endings[int(row[1])][2] == "label"]
                self.assertEqual([row[13] for row in labels], ["1", "2", "3"])
                self.assertTrue(all(row[14] == "0" and row[16] == "2" for row in labels))
                self.assertEqual(len([row for row in model.statements.values() if row[13] == "2"]), 3)
                self.assertEqual(set(model.statements), set(model.statement_endings))
                model.validate_physical_locations()

    def test_procedure_linkage_is_not_calling_convention(self) -> None:
        source = self.fixture("procedure-linkage.bas")
        expected = {"BOUNDC": "c", "BOUNDCPP": "c++", "BASICVARIADIC": "basic",
                    "ALIASEDBASIC": "basic", "PROTOTYPEONLY": "basic", "FIXEDCDECL": "basic"}
        for backend in self.backends:
            with self.subTest(backend=backend):
                model = self.compile(source, backend=backend)
                procedures = {row[2].upper(): identity for identity, row in model.types.items()
                              if row[2].upper() in expected}
                self.assertEqual(set(procedures), set(expected))
                self.assertEqual(model.capabilities[1]["procedure-linkage"], "available")
                for name, identity in procedures.items():
                    self.assertEqual(model.properties["symbol", identity]["procedure-linkage"], expected[name])
                    self.assertEqual(model.signatures[identity][3], "cdecl")
                bodies = {int(row[1]) for row in model.records["P"]}
                self.assertNotIn(procedures["PROTOTYPEONLY"], bodies)
                self.assertTrue(all(identity in bodies for name, identity in procedures.items()
                                    if name != "PROTOTYPEONLY"))
        for mode in ("bindings", "expressions"):
            model = self.compile(source, mode=mode)
            self.assertEqual(model.capabilities[1]["procedure-linkage"], "unavailable")
            self.assertFalse(any(row[3] == "procedure-linkage" for row in model.records["K"]))

    def test_declared_field_counts_use_completed_source_members(self) -> None:
        expected = {"FieldGroup": 4, "OuterFields": 4, "BaseFields": 1,
                    "DerivedFields": 3, "DynamicFields": 3, "NestedOwner": 1,
                    "InnerGroup": 3, "MethodOnly": 0}
        for backend in self.backends:
            with self.subTest(backend=backend):
                source = self.fixture("field-groups.bas")
                model = self.compile(source, backend=backend)
                for name, count in expected.items():
                    identity = self.one(model, name, "type")
                    properties = model.properties["symbol", identity]
                    self.assertEqual(properties["layout-finalized"], "1", name)
                    self.assertEqual(properties["declared-field-count"], str(count), name)
                    fields = [row for row in model.types.values()
                              if row[3] == "field" and row[18] == "source"
                              and int(row[8]) == identity]
                    self.assertEqual(len(fields), count, name)
                    for field in fields:
                        rank = "1" if name == "DynamicFields" or field[2].upper() == "FIELDGROUPNUMBERS" else "0"
                        self.assertEqual(model.properties["symbol", int(field[1])]["field-array-rank"], rank)
                dynamic = self.one(model, "DynamicFields", "type")
                self.assertEqual(sum(row[3] == "field" and int(row[8]) == dynamic
                                     for row in model.types.values()), 6)
                derived = self.one(model, "DerivedFields", "type")
                self.assertEqual(sum(row[3] == "field" and row[18] == "compiler"
                                     and int(row[8]) == derived for row in model.types.values()), 1)
                counter = self.one(model, "FieldGroupCounter", "variable")
                # A Static member declaration is an external VAR until its
                # out-of-type definition. It is not an instance FIELD.
                self.assertEqual(model.types[counter][17:19], ["external", "source"])

    def test_declared_field_counts_do_not_change_emission(self) -> None:
        for backend in self.backends:
            with self.subTest(backend=backend):
                source = self.fixture("field-groups.bas")
                suffix = {"gcc": ".c", "clang": ".c", "llvm": ".ll", "gas": ".asm", "gas64": ".asm"}[backend]
                output = self.working / ("field-emission" + suffix)
                extra = ("-o", str(output))
                self.invoke([source], mode="off", backend=backend, extra=extra)
                baseline = output.read_bytes()
                self.assertTrue(baseline)
                for mode in ("full", "bindings", "expressions"):
                    self.invoke([source], mode=mode, backend=backend, extra=extra)
                    self.assertEqual(output.read_bytes(), baseline, (backend, mode))

    def test_declared_field_counts_are_optional_but_checked(self) -> None:
        source = self.fixture("field-groups.bas")
        _, path = self.invoke([source])
        text = path.read_text()
        model = Model(text)
        identity = self.one(model, "FieldGroup", "type")
        marker = f"K\tsymbol\t{identity}\tdeclared-field-count\t"
        self.assertTrue(any(line.startswith(marker) for line in text.splitlines()))
        for value in ("-1", "x", "1.5", "1000001", "999"):
            corrupted = "\n".join(marker + value if line.startswith(marker) else line
                                  for line in text.splitlines()) + "\n"
            with self.subTest(value=value), self.assertRaises(ValueError):
                Model(corrupted)
        lines = [line for line in text.splitlines()
                 if not (line.startswith("K\tsymbol\t") and
                         any("\t" + key + "\t" in line for key in ("declared-field-count", "field-array-rank")))]
        removed = len(text.splitlines()) - len(lines)
        self.assertGreater(removed, 0)
        footer = lines[-1].split("\t")
        footer[12] = str(int(footer[12]) - removed)
        lines[-1] = "\t".join(footer)
        older = Model("\n".join(lines) + "\n")
        self.assertFalse(any("declared-field-count" in properties
                             for properties in older.properties.values()))

    def test_option_occurrences_use_committed_values_and_complete_membership(self) -> None:
        for backend in self.backends:
            with self.subTest(backend=backend):
                source = self.fixture("option-defaults.bas")
                self.fixture("option-defaults.bi")
                model = self.compile(source, backend=backend)
                self.assertEqual(model.capabilities[1]["option-base-occurrences"], "available")
                kinds = {}
                bases = {}
                for context, options in model.options.items():
                    for (domain, key), value in options.items():
                        if domain != "language-default":
                            continue
                        if key.startswith("option-statement-"):
                            identity = int(key.removeprefix("option-statement-"))
                            self.assertNotIn(identity, kinds)
                            kinds[identity] = (context, value)
                        if key.startswith("base-statement-"):
                            identity = int(key.removeprefix("base-statement-"))
                            self.assertNotIn(identity, bases)
                            bases[identity] = (context, value)
                keyword_tokens = {int(properties["keyword-token"]): model.types[identity][2].upper()
                                  for (domain, identity), properties in model.properties.items()
                                  if domain == "symbol" and "keyword-token" in properties}
                statements = {identity: row for identity, row in model.statements.items()
                              if keyword_tokens.get(int(row[9])) == "OPTION"}
                self.assertEqual(set(kinds), set(statements))
                self.assertEqual(len(kinds), 18)
                self.assertEqual(sum(value == 0 for _, value in kinds.values()), 7)
                self.assertEqual(set(bases), {identity for identity, (_, value) in kinds.items() if value == 1})
                self.assertEqual([value for _, value in bases.values()], [1, 0, 1, 2, 2, 1, 2, 3, 5, 1, 0])
                for identity, (context, value) in bases.items():
                    self.assertEqual(kinds[identity], (context, 1))
                    self.assertEqual(model.options[context]["language-default", "base"], value)
                    self.assertEqual(model.configurations[context], int(statements[identity][7]))
                    self.assertEqual(model.statement_endings[identity][3], "parsed")
                first = next(identity for identity, row in statements.items()
                             if row[12] == self.compiler_path(source) and row[13] == "20")
                entry_context = int(statements[first][6])
                self.assertEqual(model.options[entry_context]["language-default", "base"], 0)
                self.assertEqual(bases[first][1], 1)
        for mode in ("bindings", "expressions"):
            model = self.compile(source, mode=mode)
            self.assertEqual(model.capabilities[1]["option-base-occurrences"], "unavailable")
            self.assertFalse(any(key.startswith(("option-statement-", "base-statement-"))
                                 for options in model.options.values() for _, key in options))

    def test_opening_tokens_survive_operand_macros_and_encodings(self) -> None:
        text = '#lang "fblite"\n#define value_one 1\ndim item as long\nlet item = value_one\nlet item = _\n    value_one\n'
        for encoding in ("utf-8-sig", "utf-16-le", "utf-16-be", "utf-32-le", "utf-32-be"):
            with self.subTest(encoding=encoding):
                source = self.working / "opening-tokens.bas"
                marker = {"utf-8-sig": b"", "utf-16-le": codecs.BOM_UTF16_LE,
                          "utf-16-be": codecs.BOM_UTF16_BE, "utf-32-le": codecs.BOM_UTF32_LE,
                          "utf-32-be": codecs.BOM_UTF32_BE}[encoding]
                source.write_bytes(marker + text.encode(encoding))
                model = self.compile(source)
                self.assertEqual(model.capabilities[1]["statement-opening-tokens"], "available")
                self.assertEqual({identity for domain, identity, role in model.physical_locations
                                  if domain == "statement" and role == "opening-token"}, set(model.statements))
                statements = [row for row in model.statements.values() if row[13] in ("4", "5")]
                self.assertEqual(len(statements), 2)
                for statement in statements:
                    identity = int(statement[1])
                    opening = model.physical_locations["statement", identity, "opening-token"]
                    whole = model.physical_locations.get(("statement", identity, "range"))
                    self.assertEqual(opening[11], "mapped")
                    self.assertEqual(opening[5:9], [statement[13], "0", statement[13], "3"])
                    self.assertEqual(int(opening[10]) - int(opening[9]), 3 * (1 if encoding == "utf-8-sig" else 2 if "16" in encoding else 4))
                    self.assertTrue(whole is None or whole[11] != "mapped")
                model.validate_physical_locations()

    def test_option_observations_do_not_change_emission(self) -> None:
        for backend in self.backends:
            with self.subTest(backend=backend):
                source = self.fixture("option-defaults.bas")
                self.fixture("option-defaults.bi")
                suffix = {"gcc": ".c", "clang": ".c", "llvm": ".ll", "gas": ".asm", "gas64": ".asm"}[backend]
                output = self.working / ("option-emission" + suffix)
                extra = ("-o", str(output))
                self.invoke([source], mode="off", backend=backend, extra=extra)
                baseline = output.read_bytes()
                self.assertTrue(baseline)
                for mode in ("full", "bindings", "expressions"):
                    self.invoke([source], mode=mode, backend=backend, extra=extra)
                    self.assertEqual(output.read_bytes(), baseline, (backend, mode))

    def test_original_formal_modes_survive_merging_and_generated_names(self) -> None:
        text = ('declare sub Forwarded(byval proto_value as long)\n'
                'sub Forwarded(body_value as long)\nend sub\n'
                '#define NAME_WORD generated_value\n'
                'sub NameExpanded(NAME_WORD as long)\nend sub\n'
                '#define MODE_WORD byref\n'
                'sub ExplicitExpanded(MODE_WORD text_value as string)\nend sub\n'
                '#macro BODY_WORD()\nsub GeneratedBody(macro_value as long)\nend sub\n#endmacro\n'
                'BODY_WORD()\n'
                'sub Descriptor(values() as long)\nend sub\n'
                'type CallbackShape as sub(nested_value as long)\n')
        for backend in self.backends:
            with self.subTest(backend=backend):
                source = self.source(text)
                model = self.compile(source, backend=backend)
                self.assertEqual(model.capabilities[1]['formal-passing-modes'], 'available')
                formals = {properties.get('declaration-name', ''): (identity, properties)
                           for (domain, identity), properties in model.properties.items()
                           if domain == 'symbol' and 'formal-role' in properties}
                expected = {'proto_value': ('1', '1', 'prototype'),
                            'body_value': ('1', '0', 'definition'),
                            'generated_value': ('1', '0', 'definition'),
                            'text_value': ('2', '2', 'definition'),
                            'macro_value': ('1', '0', 'definition'),
                            'values': ('3', '0', 'definition'),
                            'nested_value': ('1', '0', 'prototype')}
                self.assertEqual(set(formals), set(expected))
                for name, values in expected.items():
                    identity, properties = formals[name]
                    self.assertEqual(tuple(properties[key] for key in
                                           ('formal-accepted-mode', 'formal-written-mode', 'formal-role')), values)
                    self.assertIn(identity, model.symbols)
                for name in ('generated_value', 'macro_value'):
                    identity, properties = formals[name]
                    self.assertEqual(properties['formal-declaration'], '0')
                    self.assertEqual(properties['formal-span-kind'], 'generated')
                    origins = [row for row in model.records['MR']
                               if row[1:3] == ['symbol', str(identity)] and row[4] == 'formal-name']
                    self.assertEqual(len(origins), 1)
                    self.assertGreater(int(origins[0][3]), 0)
                identity, properties = formals['text_value']
                self.assertEqual(properties['formal-span-kind'], 'generated')
                self.assertGreater(int(properties['formal-declaration']), 0)
                self.assertTrue(any(row[1:3] == ['symbol', str(identity)] and row[4] == 'formal-start'
                                    for row in model.records['MR']))
                model.validate_physical_locations()
        for mode in ('bindings', 'expressions'):
            model = self.compile(source, mode=mode)
            self.assertEqual(model.capabilities[1]['formal-passing-modes'], 'unavailable')
            self.assertFalse(any('formal-accepted-mode' in properties for properties in model.properties.values()))

    def test_formal_modes_follow_legacy_option_and_descriptor_grammar(self) -> None:
        text = ('#lang "fblite"\n'
                'sub RefDefaults(number_value as long, text_value as string)\nend sub\n'
                'option byval\n'
                'sub ValueDefaults(later_number as long, later_text as string)\nend sub\n'
                'declare sub Variadic cdecl(byval first_value as long, ...)\n')
        for dialect in ('fblite', 'deprecated', 'qb'):
            with self.subTest(dialect=dialect):
                source = self.source(text.replace('"fblite"', '"' + dialect + '"'))
                model = self.compile(source)
                modes = {properties.get('declaration-name'): properties['formal-accepted-mode']
                         for properties in model.properties.values() if 'formal-accepted-mode' in properties}
                self.assertEqual(modes['number_value'], '2')
                self.assertEqual(modes['text_value'], '2')
                self.assertEqual(modes['later_number'], '1')
                self.assertEqual(modes['later_text'], '1')
                self.assertTrue(any(properties.get('formal-accepted-mode') == '4' and
                                    properties['formal-written-mode'] == '0'
                                    for properties in model.properties.values()))

    def test_formal_mode_receipts_do_not_change_emission(self) -> None:
        source = self.source('sub Modes(scalar_value as long, text_value as string, values() as long)\n'
                             'scalar_value += len(text_value)\nend sub\n')
        for backend in self.backends:
            with self.subTest(backend=backend):
                suffix = {'gcc': '.c', 'clang': '.c', 'llvm': '.ll', 'gas': '.asm', 'gas64': '.asm'}[backend]
                output = self.working / ('formal-modes' + suffix)
                extra = ('-o', str(output))
                self.invoke([source], mode='off', backend=backend, extra=extra)
                baseline = output.read_bytes()
                self.assertTrue(baseline)
                for mode in ('full', 'bindings', 'expressions'):
                    self.invoke([source], mode=mode, backend=backend, extra=extra)
                    self.assertEqual(output.read_bytes(), baseline, (backend, mode))

    def test_for_counter_bindings_record_initialization_writes(self) -> None:
        source = self.source('sub CountValue(byval counter_value as long, byval bound_value as long)\n'
                             'for counter_value = 0 to bound_value step bound_value\n'
                             'print counter_value\nnext\nend sub\n'
                             'sub ShadowValue(byval shadow_value as long)\n'
                             'scope\nfor shadow_value as long = 0 to 1\n'
                             'print shadow_value\nnext\nend scope\n'
                             'print shadow_value\nend sub\n'
                             '#macro COUNT_MACRO()\nfor macro_counter = 0 to 1\nnext\n#endmacro\n'
                             'sub CountMacro(byval macro_counter as long)\nCOUNT_MACRO()\nend sub\n')
        for backend in self.backends:
            for mode in ('full', 'bindings'):
                with self.subTest(backend=backend, mode=mode):
                    model = self.compile(source, mode=mode, backend=backend)
                    self.assertEqual(model.capabilities[1]['for-counter-writes'],
                                     'available' if mode == 'full' else 'unavailable')
                    for anchor, token, role in (
                            ('for counter_value =', 'counter_value', 'write'),
                            ('to bound_value', 'bound_value', 'read'),
                            ('step bound_value', 'bound_value', 'read'),
                            ('print counter_value', 'counter_value', 'read')):
                        span = self.span(source, anchor, token)
                        binding = next(index for index, row in enumerate(model.records['B'], 1)
                                       if row[2] == 'reference' and source_range(row, 4) == span)
                        self.assertEqual(model.access_roles[binding], role)
                    formals = {int(row[2]): int(row[7]) for row in model.records['G'] if int(row[7])}
                    # Use the accepted source parameter names, not a fixture
                    # naming convention, to identify the body-variable slot.
                    if mode == 'full':
                        for name, expected_write in (('shadow_value', False), ('macro_counter', True)):
                            formal = next(identity for (domain, identity), properties in model.properties.items()
                                          if domain == 'symbol' and properties.get('formal-role') == 'definition'
                                          and properties.get('declaration-name') == name)
                            variable = formals[formal]
                            self.assertEqual(model.properties.get(('symbol', variable), {}).get(
                                             'direct-source-write', '0'), '1' if expected_write else '0')
        compact = self.compile(source, mode='expressions')
        self.assertEqual(compact.capabilities[1]['for-counter-writes'], 'unavailable')

    def test_for_counter_access_does_not_change_emission(self) -> None:
        source = self.source('sub Count(byval counter_value as long, byval bound_value as long)\n'
                             'for counter_value = 0 to bound_value\nprint counter_value\nnext\nend sub\n')
        for backend in self.backends:
            with self.subTest(backend=backend):
                suffix = {'gcc': '.c', 'clang': '.c', 'llvm': '.ll', 'gas': '.asm', 'gas64': '.asm'}[backend]
                output = self.working / ('for-access' + suffix)
                extra = ('-o', str(output))
                self.invoke([source], mode='off', backend=backend, extra=extra)
                baseline = output.read_bytes()
                self.assertTrue(baseline)
                for mode in ('full', 'bindings', 'expressions'):
                    self.invoke([source], mode=mode, backend=backend, extra=extra)
                    self.assertEqual(output.read_bytes(), baseline, (backend, mode))

    def test_for_byref_counter_rejection_remains_unchanged(self) -> None:
        source = self.source('sub CountReference(byref counter_ref as long)\n'
                             'for counter_ref = 1 to 0\nnext\nend sub\n')
        for backend in self.backends:
            for mode in ('off', 'full', 'bindings', 'expressions'):
                with self.subTest(backend=backend, mode=mode):
                    result, _ = self.invoke([source], mode=mode, backend=backend, success=False)
                    # BYREF is a dereference, not the scalar variable node
                    # required by the existing FOR grammar. Do not broaden
                    # grammar or fabricate a loop write for rejected code.
                    self.assertIn('error 52: Expected scalar counter', result.stdout + result.stderr)

    def test_direct_source_writes_survive_runtime_and_macro_lowering(self) -> None:
        source = self.source('type Box\nnumber as long\nend type\n'
                             'declare sub MayChange(byref value as long)\n'
                             '#define WRITE_WORD macro_value\n'
                             'sub Writes(byval direct_value as long, byval swap_value as long, _\n'
                             'byval read_value as long, byval input_value as long, byval get_value as long, _\n'
                             'byval bytes_value as integer, byval mid_value as string, byval set_value as string, _\n'
                             'byval text_value as string, byval line_value as string, byval pointer_value as long ptr, _\n'
                             'byval member_value as Box, byval whole_value as Box, byval quiet_value as long, byval macro_value as long)\n'
                             'dim other_value as long\ndirect_value = 3\nswap swap_value, other_value\n'
                             'read read_value\ninput #1, input_value\nget #1,, get_value,, bytes_value\n'
                             'mid(mid_value, 1, 1) = "x"\nlset set_value = "x"\ntext_value = "changed"\n'
                             'line input #1, line_value\n*pointer_value = 1\nmember_value.number = 1\nwhole_value = type<Box>(2)\n'
                             'MayChange(quiet_value)\n#if 0\nquiet_value = 4\n#endif\nWRITE_WORD = 5\nend sub\n')
        written = {'direct_value', 'swap_value', 'read_value', 'input_value', 'get_value',
                   'bytes_value', 'mid_value', 'set_value', 'text_value', 'line_value', 'macro_value', 'whole_value'}
        for backend in self.backends:
            with self.subTest(backend=backend):
                model = self.compile(source, backend=backend)
                self.assertEqual(model.capabilities[1]['direct-source-writes'], 'available')
                variables = {int(row[2]): int(row[7]) for row in model.records['G'] if int(row[7])}
                observed = {}
                for (domain, identity), properties in model.properties.items():
                    if domain == 'symbol' and properties.get('formal-role') == 'definition':
                        observed[properties['declaration-name']] = model.properties[
                            'symbol', variables[identity]]['direct-source-write']
                self.assertEqual(set(observed), written | {'pointer_value', 'member_value', 'quiet_value'})
                self.assertEqual({name for name, value in observed.items() if value == '1'}, written)
        for mode in ('bindings', 'expressions'):
            compact = self.compile(source, mode=mode)
            self.assertEqual(compact.capabilities[1]['direct-source-writes'], 'unavailable')
            self.assertFalse(any('direct-source-write' in properties for properties in compact.properties.values()))

    def test_direct_source_write_observations_do_not_change_emission(self) -> None:
        source = self.source('sub Writes(byval number_value as long, byval text_value as string)\n'
                             'dim other_value as long\nswap number_value, other_value\n'
                             'read number_value\ninput #1, number_value\nget #1,, number_value\n'
                             'mid(text_value, 1, 1) = "x"\nrset text_value = "x"\n'
                             'line input #1, text_value\nend sub\n')
        for backend in self.backends:
            with self.subTest(backend=backend):
                suffix = {'gcc': '.c', 'clang': '.c', 'llvm': '.ll', 'gas': '.asm', 'gas64': '.asm'}[backend]
                output = self.working / ('direct-writes' + suffix)
                extra = ('-o', str(output))
                self.invoke([source], mode='off', backend=backend, extra=extra)
                baseline = output.read_bytes()
                self.assertTrue(baseline)
                for mode in ('full', 'bindings', 'expressions'):
                    self.invoke([source], mode=mode, backend=backend, extra=extra)
                    self.assertEqual(output.read_bytes(), baseline, (backend, mode))

    def test_for_counter_identities_keep_each_native_loop_and_scope(self) -> None:
        source = self.source('#macro CountMacro(item)\nfor item = 1 to 0\nnext\n#endmacro\n'
                             'sub Counters(byval input_value as long, byref reference_value as long)\n'
                             'for input_value = 1 to 0\nnext\n'
                             'for input_value = 2 to 1\nnext\n'
                             'for input_value as long = 1 to 0\nnext\n'
                             'for reference_value as long = 1 to 0\nnext\n'
                             'CountMacro(input_value)\nend sub\n')
        for backend in self.backends:
            with self.subTest(backend=backend):
                model = self.compile(source, backend=backend)
                self.assertEqual(model.capabilities[1]['for-counter-identities'], 'available')
                observations = [(identity, int(key.split(':', 1)[1]), value)
                                for (domain, identity), properties in model.properties.items()
                                if domain == 'symbol' for key, value in properties.items()
                                if key.startswith('for-counter:')]
                self.assertEqual(len(observations), 5)
                self.assertEqual(Counter(value for _, _, value in observations), {'existing': 3, 'local': 2})
                bodies = {int(row[7]) for row in model.records['G'] if int(row[7])}
                direct_variables = {variable for variable, _, kind in observations if kind == 'existing'}
                self.assertEqual(len(direct_variables), 1)
                self.assertTrue(direct_variables <= bodies)
                self.assertTrue(all(variable not in bodies for variable, _, kind in observations if kind == 'local'))
                statements = {int(row[1]): row for row in model.records['ST']}
                for variable, statement, _ in observations:
                    # FB_TK_FOR is the grammar token in ST, not a source-prefix guess.
                    self.assertEqual(int(statements[statement][9]), 281)
                    self.assertEqual(int(model.symbols[variable][3]), 1)
                    self.assertTrue(('statement', statement, 'for-counter') in model.physical_locations or
                                    any(row[1] == 'statement' and int(row[2]) == statement and row[4] == 'for-counter'
                                        for row in model.records['MR']))
        for mode in ('bindings', 'expressions'):
            compact = self.compile(source, mode=mode)
            self.assertEqual(compact.capabilities[1]['for-counter-identities'], 'unavailable')
            self.assertFalse(any(key.startswith('for-counter:') for properties in compact.properties.values() for key in properties))

    def test_for_counter_identity_observations_do_not_change_emission(self) -> None:
        source = self.source('#macro CountMacro(item)\nfor item = 1 to 0\nnext\n#endmacro\n'
                             'sub Counters(byval input_value as long, byref reference_value as long)\n'
                             'for input_value = 1 to 0\nnext\n'
                             'for reference_value as long = 1 to 0\nnext\n'
                             'CountMacro(input_value)\nend sub\n')
        for backend in self.backends:
            with self.subTest(backend=backend):
                suffix = {'gcc': '.c', 'clang': '.c', 'llvm': '.ll', 'gas': '.asm', 'gas64': '.asm'}[backend]
                output = self.working / ('for-identities' + suffix)
                extra = ('-o', str(output))
                self.invoke([source], mode='off', backend=backend, extra=extra)
                baseline = output.read_bytes()
                self.assertTrue(baseline)
                for mode in ('full', 'bindings', 'expressions'):
                    self.invoke([source], mode=mode, backend=backend, extra=extra)
                    self.assertEqual(output.read_bytes(), baseline, (backend, mode))

    def numeric_suffixes(self, model: Model) -> list[tuple[int, str, list[str]]]:
        return [(owner, name, value.split('\t'))
                for (domain, owner), properties in model.properties.items()
                if domain == 'symbol' for name, value in properties.items()
                if name.startswith('parsed-numeric-suffix-')]

    def test_parsed_numeric_suffixes_keep_native_spelling_and_coordinates(self) -> None:
        source = self.source('Print 1u, 2Ul, 3uLl, 4l, 5Ll, .5f, &B1d, &O7f, &H8d, 1d, 1e, 2%, 3&\n')
        expected = ['u', 'Ul', 'uLl', 'l', 'Ll', 'f', 'd', 'f']
        for backend in self.backends:
            with self.subTest(backend=backend):
                model = self.compile(source, backend=backend)
                self.assertEqual(model.capabilities[1]['parsed-numeric-suffixes'], 'available')
                observations = self.numeric_suffixes(model)
                self.assertEqual([fields[4] for _, _, fields in observations], expected)
                for owner, name, fields in observations:
                    self.assertEqual(len(fields), 5)
                    self.assertEqual(int(model.symbols[owner][3]), 8)
                    self.assertIn(int(fields[0]), model.source_contexts)
                    self.assertEqual(fields[2], '0')
                    location = model.physical_locations['source-context', int(fields[0]), name]
                    self.assertEqual(location[11], 'mapped')
                    self.assertEqual(location[4], fields[0])
                    spelling = source.read_bytes()[int(location[9]):int(location[10])].decode('utf-8')
                    self.assertTrue(spelling.endswith(fields[4]), (spelling, fields))

    def test_parsed_numeric_suffixes_exclude_unparsed_macro_output(self) -> None:
        source = self.source('#define inner_suffix 1ul\n'
                             '#define stringify_suffix(value) #value\n'
                             '#define drop_suffix(value) 1\n'
                             '#define forward_suffix(value) value\n'
                             'Print stringify_suffix(inner_suffix), drop_suffix(inner_suffix)\n'
                             'Print forward_suffix(inner_suffix)\n'
                             '#macro mixed_suffix\nPrint 1ul\nAsm\nmov eax, 2ul\nEnd Asm\n'
                             '#if 0\nPrint 4ul\n#endif\nPrint 3ll\n#endmacro\nmixed_suffix\n')
        for backend in self.backends:
            with self.subTest(backend=backend):
                model = self.compile(source, backend=backend)
                observations = self.numeric_suffixes(model)
                self.assertEqual(Counter(fields[4] for _, _, fields in observations), {'ul': 2, 'll': 1})
                for owner, name, fields in observations:
                    expansion = int(fields[2])
                    self.assertGreater(expansion, 0)
                    self.assertEqual(model.macro_origins['symbol', owner, name], expansion)
                    self.assertEqual(model.macro_results[expansion][2], 'expanded')

    def test_parsed_numeric_suffixes_keep_physical_line_under_all_encodings(self) -> None:
        text = '#line 400 "logical.bas"\nPrint 1ul, .5f\n'
        for encoding, marker in [('utf-8', b''), ('utf-8', codecs.BOM_UTF8),
                                 ('utf-16-le', codecs.BOM_UTF16_LE), ('utf-16-be', codecs.BOM_UTF16_BE),
                                 ('utf-32-le', codecs.BOM_UTF32_LE), ('utf-32-be', codecs.BOM_UTF32_BE)]:
            source = self.working / 'encoded.bas'
            source.write_bytes(marker + text.encode(encoding))
            for backend in self.backends:
                with self.subTest(encoding=encoding, marker=marker, backend=backend):
                    model = self.compile(source, backend=backend)
                    observations = self.numeric_suffixes(model)
                    self.assertEqual([fields[4] for _, _, fields in observations], ['ul', 'f'])
                    for owner, name, fields in observations:
                        location = model.physical_locations['source-context', int(fields[0]), name]
                        self.assertEqual((location[5], location[7], location[11]), ('2', '2', 'mapped'))

    def test_parsed_numeric_suffixes_do_not_change_emission_or_compact_modes(self) -> None:
        source = self.source('#define suffix_value 3ul\nPrint 1l + 2ll, .5f, &B1d, suffix_value\n')
        for backend in self.backends:
            with self.subTest(backend=backend):
                suffix = {'gcc': '.c', 'clang': '.c', 'llvm': '.ll', 'gas': '.asm', 'gas64': '.asm'}[backend]
                output = self.working / ('suffix-observations' + suffix)
                extra = ('-o', str(output))
                self.invoke([source], mode='off', backend=backend, extra=extra)
                baseline = output.read_bytes()
                self.assertTrue(baseline)
                for mode in ('full', 'bindings', 'expressions'):
                    _, path = self.invoke([source], mode=mode, backend=backend, extra=extra)
                    self.assertEqual(output.read_bytes(), baseline, (backend, mode))
                    model = Model.read(path, expressions_only=mode == 'expressions',
                                       bindings_only=mode == 'bindings')
                    self.assertEqual(model.capabilities[1]['parsed-numeric-suffixes'],
                                     'available' if mode == 'full' else 'unavailable')
                    self.assertEqual(len(self.numeric_suffixes(model)), 5 if mode == 'full' else 0)

    def test_unevaluated_queries_retain_discarded_cast_inputs(self) -> None:
        source = self.fixture("unevaluated-query-inputs.bas")
        for backend in self.backends:
            with self.subTest(backend=backend):
                model = self.compile(source, backend=backend)
                self.assertEqual(model.capabilities[1]["unevaluated-query-inputs"], "available")
                marked = {identity for (domain, identity), properties in model.properties.items()
                          if domain == "expression" and properties.get("unevaluated-query-input") == "1"}
                expressions = {int(row[1]): row for row in model.records["E"]}
                casts = {int(row[1]): expressions[int(row[1])] for row in model.records["EX"]
                         if row[2:4] == ["cast", "cast"]}
                self.assertTrue(casts)
                observed_lines = set()
                for identity, expression in casts.items():
                    line = int(expression[4])
                    observed_lines.add(line)
                    self.assertEqual(identity in marked, 11 <= line <= 16, expression)
                self.assertTrue(set(range(10, 18)).issubset(observed_lines), observed_lines)
                kinds = {value.split("\t")[2] for properties in model.properties.values()
                         for key, value in properties.items() if key.startswith("unevaluated-query-range-")}
                self.assertEqual(kinds, {"typeof", "sizeof"})

    def test_unevaluated_queries_preserve_emission_and_compact_modes(self) -> None:
        source = self.fixture("unevaluated-query-inputs.bas")
        for backend in self.backends:
            with self.subTest(backend=backend):
                suffix = {"gcc": ".c", "clang": ".c", "llvm": ".ll", "gas": ".asm", "gas64": ".asm"}[backend]
                output = self.working / ("query-observations" + suffix)
                extra = ("-o", str(output))
                self.invoke([source], mode="off", backend=backend, extra=extra)
                baseline = output.read_bytes()
                self.assertTrue(baseline)
                for mode in ("full", "bindings", "expressions"):
                    _, path = self.invoke([source], mode=mode, backend=backend, extra=extra)
                    self.assertEqual(output.read_bytes(), baseline, (backend, mode))
                    model = Model.read(path, expressions_only=mode == "expressions", bindings_only=mode == "bindings")
                    self.assertEqual(model.capabilities[1]["unevaluated-query-inputs"],
                                     "available" if mode == "full" else "unavailable")
                    observed = any(key.startswith("unevaluated-query-") for properties in model.properties.values()
                                   for key in properties)
                    self.assertEqual(observed, mode == "full")

    def test_unevaluated_query_reader_rejects_incomplete_groups(self) -> None:
        source = self.fixture("unevaluated-query-inputs.bas")
        _, path = self.invoke([source])
        rows = [line.split("\t") for line in path.read_text(encoding="ascii").splitlines()]
        input_index = next(index for index, row in enumerate(rows)
                           if row[0] == "K" and row[3] == "unevaluated-query-input")
        range_index = next(index for index, row in enumerate(rows)
                           if row[0] == "K" and row[3].startswith("unevaluated-query-range-"))
        first, last, kind = unescape(rows[range_index][4]).split("\t")
        changed_groups = []
        for index, field, value in (
                (input_index, 4, "0"),
                (input_index, 3, "unevaluated-query-unknown"),
                (range_index, 3, "unevaluated-query-range-0"),
                (range_index, 3, "unevaluated-query-range-4294967296"),
                (range_index, 4, f"0%09{last}%09{kind}"),
                (range_index, 4, f"{first}%091000001%09{kind}"),
                (range_index, 4, f"{first}%09{last}%09len")):
            changed = [row.copy() for row in rows]
            changed[index][field] = value
            changed_groups.append(changed)
        changed_groups.append([row.copy() for row in rows if not (row[0] == "K" and
                              row[3].startswith("unevaluated-query-range-"))])
        changed_groups.append([row.copy() for row in rows if not (row[0] == "K" and
                              row[2] == rows[input_index][2] and row[3] == "unevaluated-query-input")])
        duplicate = [row.copy() for row in rows]
        duplicate.insert(-1, rows[range_index].copy())
        changed_groups.append(duplicate)
        for index, changed in enumerate(changed_groups):
            with self.subTest(mutation=index):
                changed[-1][12] = str(sum(row[0] in DETAIL_TAGS for row in changed))
                with self.assertRaises(ValueError):
                    Model("\n".join("\t".join(row) for row in changed) + "\n")

    def test_select_case_inputs_keep_parser_owned_alternatives(self) -> None:
        source = self.fixture("select-case-inputs.bas")
        for backend in self.backends:
            with self.subTest(backend=backend):
                model = self.compile(source, backend=backend)
                observations = {key: value for properties in model.properties.values()
                                for key, value in properties.items() if key.startswith("select-case-")}
                self.assertEqual(model.capabilities[1]["select-case-inputs"], "available")
                for prefix, expected in (("input", 2), ("clause", 6), ("alternative", 8), ("end", 2)):
                    self.assertEqual(sum(key.startswith("select-case-" + prefix + ":")
                                         for key in observations), expected)
                alternatives = [value.split("\t") for key, value in observations.items()
                                if key.startswith("select-case-alternative:")]
                self.assertEqual(Counter(fields[1] for fields in alternatives),
                                 {"value": 5, "range": 2, "is": 1})

    def test_select_case_inputs_preserve_emission_and_compact_modes(self) -> None:
        source = self.fixture("select-case-inputs.bas")
        for backend in self.backends:
            with self.subTest(backend=backend):
                suffix = {"gcc": ".c", "clang": ".c", "llvm": ".ll", "gas": ".asm", "gas64": ".asm"}[backend]
                output = self.working / ("select-observations" + suffix)
                extra = ("-o", str(output))
                self.invoke([source], mode="off", backend=backend, extra=extra)
                baseline = output.read_bytes()
                for mode in ("full", "bindings", "expressions"):
                    _, path = self.invoke([source], mode=mode, backend=backend, extra=extra)
                    self.assertEqual(output.read_bytes(), baseline, (backend, mode))
                    model = Model.read(path, expressions_only=mode == "expressions", bindings_only=mode == "bindings")
                    self.assertEqual(model.capabilities[1]["select-case-inputs"],
                                     "available" if mode == "full" else "unavailable")
                    self.assertEqual(any(key.startswith("select-case-") for properties in model.properties.values()
                                         for key in properties), mode == "full")

    def test_select_case_reader_rejects_incomplete_groups(self) -> None:
        source = self.fixture("select-case-inputs.bas")
        _, path = self.invoke([source])
        rows = [line.split("\t") for line in path.read_text(encoding="ascii").splitlines()]
        indexes = {kind: next(index for index, row in enumerate(rows)
                             if row[0] == "K" and row[3].startswith("select-case-" + kind + ":"))
                   for kind in ("input", "clause", "alternative", "end")}
        changed_groups = []
        owner = rows[indexes["input"]][2]
        foreign_owner = next(row[1] for row in rows if row[0] == "S" and row[3] == "3" and row[1] != owner)
        for kind, field, value in (("input", 4, "0%091%091%09normal"),
                                   ("input", 3, "select-case-input:0"),
                                   ("alternative", 3, "select-case-unknown:1")):
            changed = [row.copy() for row in rows]
            changed[indexes[kind]][field] = value
            changed_groups.append(changed)
        for kind in indexes:
            changed_groups.append([row.copy() for index, row in enumerate(rows) if index != indexes[kind]])
            duplicate = [row.copy() for row in rows]
            duplicate.insert(-1, rows[indexes[kind]].copy())
            changed_groups.append(duplicate)
            foreign_duplicate = [row.copy() for row in rows]
            item = rows[indexes[kind]].copy()
            item[2] = foreign_owner
            foreign_duplicate.insert(-1, item)
            changed_groups.append(foreign_duplicate)
        header = unescape(rows[indexes["input"]][4]).split("\t")[0]
        clause = rows[indexes["clause"]][3].split(":")[1]
        for statement in (header, clause):
            changed = [row.copy() for row in rows]
            statement_index = next(index for index, row in enumerate(changed) if row[0] == "ST" and row[1] == statement)
            changed[statement_index][4] = foreign_owner
            changed_groups.append(changed)
        for index, changed in enumerate(changed_groups):
            with self.subTest(mutation=index):
                changed[-1][12] = str(sum(row[0] in DETAIL_TAGS for row in changed))
                with self.assertRaises(ValueError):
                    Model("\n".join("\t".join(row) for row in changed) + "\n")

    def test_select_case_alternatives_respect_the_parser_table_limit(self) -> None:
        for count in (1024, 1025):
            header = ("'' Project: FreeBASIC semantic sidecar tests\n'' File: case-limit.bas\n"
                      "'' Purpose: Verify the parser table boundary.\n"
                      "'' Responsibilities: Accepted and excessive alternatives.\n"
                      "'' This file intentionally does NOT execute CASE bodies.\n")
            source = self.source(header + "sub CheckLimit(byval value as long)\nselect case value\ncase " +
                                 ", ".join(str(index) for index in range(count)) +
                                 "\nprint value\nend select\nend sub\n'' end of case-limit.bas\n", "case-limit.bas")
            with self.subTest(alternatives=count):
                if count == 1024:
                    model = self.compile(source)
                    alternatives = [key for properties in model.properties.values() for key in properties
                                    if key.startswith("select-case-alternative:")]
                    self.assertEqual(len(alternatives), count)
                else:
                    rejected, _ = self.invoke([source], success=False)
                    self.assertIn("too many labels", (rejected.stdout + rejected.stderr).lower())

# end of test_sidecar.py
