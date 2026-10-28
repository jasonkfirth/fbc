"""Project: FreeBASIC semantic sidecar tests
File: data_streams.py
Purpose: Prove DATA source ownership, lowering selections and deliberate omissions.
Responsibilities: FB/QB values, ranges, native chains, transport and corrupt closure.
This file intentionally does NOT predict a runtime cursor through control flow.
"""

import subprocess

from driver_inputs import text_rows
from semantic_data import FEATURE
from sidecar import Model


def check_native_nodes(test):
    native = (test.root / "src/compiler/ast/nodes/ast-node-data.bas").read_text()
    bodies = []
    for name in ("astDataStmtBegin", "astDataStmtStore"):
        start = native.index("function " + name)
        end = native.index("\nend function", start) + len("\nend function")
        bodies.append(native[start:end])
    guard = (test.root / "tests/semantic-sidecar/data-node-guards.bas").read_text()
    source = test.working / "data-node-guards.bas"
    source.write_text(guard.replace("'' Native constructors are inserted here.", "\n".join(bodies)))
    executable = test.working / ("data-node-guards.exe" if test.native_windows else "data-node-guards")
    result = subprocess.run([str(test.compiler), "-prefix", str(test.toolchain_prefix), "-e", "-i", str(test.root / "inc"),
                             "-i", str(test.root / "src/compiler"), "-x", str(executable), str(source)],
                            cwd=test.working, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], cwd=test.working, capture_output=True, text=True, timeout=30)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertIn("DATA_NODE_GUARDS_PASS", result.stdout)


SOURCE = '''dim shared target(0 to 2) as long
restore
restore later
first:
data -1, 18446744073709551615ULL, 1.5, "hi", !"a\\0b", wstr("wide"), @target(1)
data 8
later:
data 9
type RecordValue
    value as long
end type
dim number as long, text as string, item as RecordValue
dim cursor as long ptr = @number
read number, text, target(1), *cursor, item.value
#define ValueData (6 + 7)
data ValueData, 99
#if 0
data 12345
restore inactive_label
read absent
#endif
'''


def check_streams(test):
    source = test.source(SOURCE)
    for backend in test.backends:
        with test.subTest(backend=backend):
            test.invoke([source], mode="off", backend=backend)
            baseline = test.emission_path(source.stem, backend).read_bytes()
            model = test.compile(source, backend=backend)
            facts = model.data_streams[1]
            test.assertEqual(facts["counts"][1], ["4", "11", "5", "5", "2"])
            items = facts["item"]
            test.assertEqual([items[1, ordinal][0] for ordinal in range(7)],
                             ["number", "number", "number", "string", "string", "wide-string", "address"])
            test.assertEqual(model.data_ast_payloads[1, 1]["item_ids"], (2, 2, 2, 1, 1, 32768, 65534))
            for group, payload in model.data_ast_payloads.items():
                test.assertEqual(payload["trailer_id"], 65535)
                test.assertEqual(payload["container_elements"], len(payload["item_ids"]) + 1)
            test.assertEqual([items[1, ordinal][4:6] for ordinal in range(6)], [
                ["signed", "-1"], ["unsigned", "18446744073709551615"],
                ["float64-bits", "0x3FF8000000000000"], ["bytes", "6869"],
                ["bytes", "610062"], ["wide-units", "00000077000000690000006400000065"]])
            test.assertEqual(items[1, 6][6:], [str(test.one(model, "TARGET", "variable")), "4"])
            test.assertEqual(items[4, 0][4:6], ["signed", "13"])
            descriptors = facts["descriptor"]
            first_descriptor = next(iter(descriptors))
            test.assertEqual(descriptors[first_descriptor], ["0", "0", "0", "1"])
            test.assertEqual(facts["restore"][1][2:], ["0", str(first_descriptor)])
            later = test.one(model, "LATER", "label")
            later_array = int(facts["group"][3][2])
            test.assertEqual(facts["restore"][2][2:], [str(later), str(later_array)])
            test.assertEqual(descriptors[later_array][1:3], [str(later), "3"])
            test.assertEqual([int(fields[2]) for fields in facts["read"].values()], list(range(5)))
            targets = [model.symbols[int(fields[0])][2] for fields in facts["read-runtime"].values()]
            test.assertEqual(targets, ["FB_DATAREADINT", "FB_DATAREADSTR", "FB_DATAREADINT", "FB_DATAREADINT", "FB_DATAREADINT"])
            context = int(facts["group"][1][1])
            literal_range = model.physical_locations["source-context", context, "data-item:1:4"]
            test.assertEqual(source.read_bytes()[int(literal_range[9]):int(literal_range[10])], b'!"a\\0b"')
            test.assertNotIn(("source-context", context, "data-item:4:0"), model.physical_locations)
            expanded_expression = items[4, 0][3]
            test.assertTrue(any(row[1:3] == ["expression", expanded_expression] for row in model.records["MR"]))
            test.assertFalse(model.named("INACTIVE_LABEL", "label"))
            test.assertFalse(any(properties.get("kind") == "data"
                                 for (domain, _), properties in model.properties.items() if domain == "node"))
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)
            for mode in ("bindings", "expressions"):
                reduced = test.compile(source, mode=mode, backend=backend)
                test.assertEqual(reduced.capabilities[1][FEATURE], "unavailable")
                test.assertFalse(reduced.data_streams)
                test.assertFalse(any(row[3].startswith("data-") for row in reduced.records["K"]))
                if mode == "expressions":
                    test.assertEqual(reduced.parser_observations, model.parser_observations)
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)


def check_qb_and_modules(test):
    qb = test.source('#define RawValue 321\ndata RawValue, 1+2, "a,b", , "", "hello there"\n'
                     'dim value as string\nrestore\nread value\n', "qb.bas")
    for backend in test.backends:
        with test.subTest(backend=backend):
            test.invoke([qb], mode="off", backend=backend, extra=("-lang", "qb"))
            baseline = test.emission_path(qb.stem, backend).read_bytes()
            model = test.compile(qb, backend=backend, extra=("-lang", "qb"))
            facts = model.data_streams[1]
            test.assertEqual(facts["counts"][1], ["1", "6", "1", "1", "1"])
            expected = [b"RawValue", b"1+2", b"a,b", b"", b"", b"hello there"]
            test.assertEqual([bytes.fromhex(fields[5]) for fields in facts["item"].values()], expected)
            test.assertTrue(all(fields[0] == "qb-raw" and fields[3] == "0" for fields in facts["item"].values()))
            test.assertEqual(model.data_ast_payloads[1, 1], {"item_ids": (1,) * 6, "trailer_id": 65535, "container_elements": 7})
            context = int(facts["group"][1][1])
            test.assertNotIn(("source-context", context, "data-item:1:3"), model.physical_locations)
            test.assertIn(("source-context", context, "data-item:1:4"), model.physical_locations)
            test.assertFalse(any(row[4].startswith("data-") for row in model.records["MR"]))
            test.assertEqual(test.emission_path(qb.stem, backend).read_bytes(), baseline)
    primary = test.source("restore later\nlater:\ndata 1\n", "primary.bas")
    secondary = test.source("data 2, 3\n", "secondary.bas")
    _, artifact = test.invoke([primary, secondary])
    model = Model.read(artifact)
    test.assertEqual(model.data_streams[1]["counts"][1], ["1", "1", "1", "0", "1"])
    test.assertEqual(model.data_streams[2]["counts"][2], ["1", "2", "1", "0", "0"])
    test.assertNotEqual(model.data_streams[1]["owner"], model.data_streams[2]["owner"])
    empty = test.compile(test.source("print 1\n", "empty.bas"))
    test.assertEqual(empty.data_streams[1]["counts"][1], ["0"] * 5)
    bad = test.source("dim value as long\ndata value\n", "nonconstant.bas")
    _, artifact = test.invoke([bad], success=False, mode="expressions")
    recovery = Model.read(artifact, expressions_only=True, allow_recovery=True)
    test.assertFalse(recovery.data_streams)


def check_transport(test):
    source = test.source(SOURCE)
    text = test.compile(source)
    binary = test.working / "data.bin"
    result = subprocess.run([str(test.compiler), "-prefix", str(test.toolchain_prefix), "-i", str(test.root / "inc"),
                             "-gen", "gcc", "-maxerr", "20", "-r", "-semantic-model", str(binary), str(source),
                             "-o", str(source.with_suffix(".o"))], cwd=test.working,
                            capture_output=True, text=True)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertEqual(Model.read(binary).rows, text.rows)


def check_read_types_and_runtime(test):
    declarations = [("flag", "boolean", "FB_DATAREADBOOL"), ("i8", "byte", "FB_DATAREADBYTE"),
                    ("u8", "ubyte", "FB_DATAREADUBYTE"), ("i16", "short", "FB_DATAREADSHORT"),
                    ("u16", "ushort", "FB_DATAREADUSHORT"), ("i32", "long", "FB_DATAREADINT"),
                    ("u32", "ulong", "FB_DATAREADUINT"), ("i64", "longint", "FB_DATAREADLONGINT"),
                    ("u64", "ulongint", "FB_DATAREADULONGINT"), ("r4", "single", "FB_DATAREADSINGLE"),
                    ("r8", "double", "FB_DATAREADDOUBLE"), ("text", "string", "FB_DATAREADSTR"),
                    ("fixed", "string * 8", "FB_DATAREADSTR"), ("narrow", "zstring * 8", "FB_DATAREADSTR"),
                    ("wide", "wstring * 8", "FB_DATAREADWSTR"), ("unicode_text", "ustring", "FB_USTRDATAREAD")]
    source = test.source("\n".join(f"dim {name} as {dtype}" for name, dtype, _ in declarations) +
                         "\nread " + ", ".join(name for name, _, _ in declarations) + "\n", "read-types.bas")
    for backend in test.backends:
        with test.subTest(backend=backend):
            model = test.compile(source, backend=backend)
            facts = model.data_streams[1]
            test.assertEqual(facts["counts"][1], ["0", "0", "0", str(len(declarations)), "0"])
            selected = [model.symbols[int(fields[0])][2] for fields in facts["read-runtime"].values()]
            test.assertEqual(selected, [proc for _, _, proc in declarations])
    runnable = test.source('restore values\nvalues:\ndata -1, "hello", wstr("wide"), 9\n'
                           'dim n as long, text as string, wide_text as wstring * 10\n'
                           'read n, text, wide_text\n'
                           'if n <> -1 or text <> "hello" or wide_text <> wstr("wide") then end 1\n'
                           'read n\nif n <> 9 then end 2\n'
                           'restore values\nread n\nif n <> -1 then end 3\nend 0\n', "runtime.bas")
    for mode in ("off", "full"):
        executable = test.working / ("runtime-" + mode)
        _, artifact = test.invoke([runnable], mode=mode, emit=False, extra=("-x", str(executable)))
        result = subprocess.run([str(executable)], capture_output=True, timeout=30)
        test.assertEqual((result.returncode, result.stdout, result.stderr), (0, b"", b""))
        if mode == "full":
            test.assertEqual(Model.read(artifact).data_streams[1]["counts"][1], ["1", "4", "1", "5", "2"])


def check_rejection(test):
    model = test.compile(test.source(SOURCE))
    mutations = ("count", "missing-item", "missing-payload", "missing-next", "reverse-next", "missing-runtime",
                 "restore-target", "restore-label", "item-source", "original-value", "runtime-value", "tag",
                 "group-count", "group-owner", "huge-count", "synthetic-items", "read-order", "read-target",
                 "reorder-items", "foreign-range", "capability")
    for mutation in mutations:
        with test.subTest(mutation=mutation), test.assertRaises(ValueError):
            rows = [row.copy() for row in model.rows]
            def prop(key):
                return next(row for row in rows if row[0] == "K" and row[3] == key)
            def change(key, index, value):
                row = prop(key)
                fields = row[4].split("\t")
                fields[index] = value
                row[4] = "\t".join(fields)
            if mutation in ("count", "huge-count"):
                change("data-counts:1", 1, "12" if mutation == "count" else "1000001")
            elif mutation.startswith("missing-"):
                if mutation == "missing-next":
                    rows.remove(next(row for row in rows if row[0] == "H" and row[5] == "data-next"))
                else:
                    rows.remove(prop({"missing-item": "data-item:1:0", "missing-payload": "data-payload:1:0",
                                      "missing-runtime": "data-read-runtime:1"}[mutation]))
            elif mutation == "reverse-next":
                row = next(row for row in rows if row[0] == "H" and row[5] == "data-next")
                row[2], row[4] = row[4], row[2]
            elif mutation == "restore-target":
                change("data-restore:1", 3, model.data_streams[1]["group"][2][2])
            elif mutation == "restore-label":
                change("data-restore:2", 2, str(test.one(model, "FIRST", "label")))
            elif mutation in ("item-source", "original-value"):
                change("data-item:1:0", 3 if mutation == "item-source" else 5,
                       model.data_streams[1]["read"][1][3] if mutation == "item-source" else "-2")
            elif mutation in ("runtime-value", "tag"):
                change("data-payload:1:0", 1 if mutation == "runtime-value" else 0, "-2" if mutation == "runtime-value" else "1")
            elif mutation in ("group-count", "group-owner"):
                change("data-group:1", 3 if mutation == "group-count" else 0, "1000000" if mutation == "group-count" else "999999")
            elif mutation == "synthetic-items":
                synthetic = next(iter(model.data_streams[1]["descriptor"]))
                change(f"data-descriptor:{synthetic}", 3, "2")
            elif mutation == "read-order":
                change("data-read:2", 2, "0")
            elif mutation == "read-target":
                change("data-read-runtime:1", 0, str(test.one(model, "TARGET", "variable")))
            elif mutation == "reorder-items":
                first, second = rows.index(prop("data-item:1:0")), rows.index(prop("data-item:1:1"))
                rows[first], rows[second] = rows[second], rows[first]
            elif mutation == "foreign-range":
                row = next(row for row in rows if row[0] == "LOC" and row[3] == "data-item:1:0")
                row[3] = "data-item:99:0"
            else:
                next(row for row in rows if row[:3] == ["CAP", "1", FEATURE])[3] = "unavailable"
            Model(text_rows(rows))

# end of data_streams.py
