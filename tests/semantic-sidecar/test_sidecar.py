"""Project: FreeBASIC semantic sidecar tests
File: test_sidecar.py
Purpose: Verify exported compiler facts and their deliberate exclusions.
Responsibilities: Exercise every record family, source provenance, and transactions.
This file intentionally does NOT contain: alternate BASIC parsing or name resolution.
"""

from __future__ import annotations

from collections import Counter
import codecs
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile
import time
import unittest

from sidecar import DETAIL_TAGS, IMPLICIT_KINDS, PRIMITIVE_NAMES, Model, source_range, unescape


class SidecarTests(unittest.TestCase):
    root: Path
    compiler: Path
    backends: list[str]

    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory(prefix="fbc-semantic-")
        self.addCleanup(self.temporary.cleanup)
        self.working = Path(self.temporary.name)
        self.sequence = 0

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
        command = [str(self.compiler), "-prefix", str(self.root), "-i", str(self.root / "inc"),
                   "-gen", backend, "-maxerr", "20"]
        if backend == "gas" and "-target" not in extra:
            command += ["-target", "linux-x86"]
        if emit:
            command.append("-r")
        if mode != "off":
            command += ["-semantic-model-expressions" if mode == "expressions" else "-semantic-model",
                        str(model)]
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
        return Model.read(path, expressions_only=mode == "expressions")

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
        return str(path), line_number, start_column, line_number, end_column

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
                self.assertFalse(any(model.records[tag] for tag in DETAIL_TAGS))
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
                             [str(source), str(preinclude), str(first), str(second)])
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
        source = self.source("".join(f'#include "{path}"\n' for path in paths))
        model = self.compile(source, mode="expressions")
        self.assertEqual(len(model.records["D"]), 5000)
        self.assertEqual(model.footer[11], "0")
        self.assertEqual(model.records["D"][0][1], str(source))

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
        if not hasattr(os, "mkfifo"):
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
                            destination.symlink_to(protected.name)
                    original = protected.read_bytes()
                    extra = ("-include", str(included)) if alias == "preinclude" else ()
                    option = "-semantic-model-expressions" if mode == "expressions" else "-semantic-model"
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
        if os.name != "posix":
            self.skipTest("Replacement failure fixture uses POSIX rename")
        compiler = shutil.which(os.environ.get("GCC") or "gcc")
        if compiler is None:
            self.skipTest("C compiler is unavailable")
        executable = self.working / "publication-test"
        command = [compiler, "-Wall", "-Wextra", "-Werror",
                   "-I", str(self.root / "src/compiler/tooling"),
                   str(self.root / "tests/semantic-sidecar/semantic-output-test.c"),
                   "-o", str(executable)]
        built = subprocess.run(command, cwd=self.working, text=True, capture_output=True, timeout=60)
        self.assertEqual(built.returncode, 0, built.stdout + built.stderr)
        tested = subprocess.run([str(executable)], cwd=self.working, text=True, capture_output=True, timeout=30)
        self.assertEqual(tested.returncode, 0, tested.stdout + tested.stderr)
        self.assertIn("semantic publication failures passed", tested.stdout)
        self.assertFalse(list(self.working.glob(".fb-semantic-*")))

    def test_unopened_sources_are_not_dependencies(self) -> None:
        source = self.working / "absent.bas"
        _, path = self.invoke([source], success=False)
        self.assertNotIn(str(source), [line.split("\t")[1] for line in path.read_text().splitlines()
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
            "unsupported-schema": text.replace("FBCSEM\t20", "FBCSEM\t999", 1),
            "invalid-escape": text.replace("M\t", "M\t%ZZ", 1),
            "short-record": text.replace("M\t" + str(source), "M", 1),
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
        self.assertEqual([row[1] for row in model.records["D"]], [str(source), str(header)])
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
                self.assertEqual(context[1], str(source))
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

# end of test_sidecar.py
