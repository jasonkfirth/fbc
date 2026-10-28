"""Project: FreeBASIC semantic sidecar tests
File: default_types.py
Purpose: Verify actual DEF tables and their committed source history.
Responsibilities: Every directive, native range rules, cache ownership and omissions.
This file intentionally does NOT infer defaults from emitted variable names.
"""

import subprocess

from driver_inputs import text_rows
from semantic_contexts import DEFAULT_FEATURE, DEFAULT_KEYS
from sidecar import Model

DIRECTIVES = (("defbyte", 2), ("defubyte", 3), ("defshort", 5), ("defushort", 6),
              ("defint", 8), ("defuint", 9), ("deflng", 11), ("defulng", 12),
              ("deflongint", 13), ("defulongint", 14), ("defsng", 17), ("defdbl", 18), ("defstr", 19))
RANGES = '''#lang "fblite"
defstr "z"-"a"
dim aText
defstr "z"-"a"
defdbl "@"-"b"
dim aDouble
defstr "["-"_"
dim _Tail
option base 1
dim _Array(2)
'''


def declaration_contexts(model, name):
    symbols = set(model.named(name))
    return [int(model.statements[model.statement_owners["binding", ordinal]][6])
            for ordinal, row in enumerate(model.records["B"], 1)
            if int(row[1]) in symbols and row[2] == "declaration"]


def check_directives(test):
    text = '#lang "fblite"\n' + "\n".join(directive + " " + chr(97 + index) + "\ndim " + chr(97 + index) + "Value"
                                                   for index, (directive, dtype) in enumerate(DIRECTIVES)) + "\n"
    source = test.source(text, "default-all-directives.bas")
    for backend in test.backends:
        baseline, _ = test.invoke([source], mode="off", backend=backend)
        emission = test.emission_path(source.stem, backend).read_bytes()
        result, path = test.invoke([source], backend=backend)
        test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
        test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
        model = Model.read(path)
        changes = model.default_type_changes[1]
        test.assertEqual(len(changes), 13)
        for index, ((directive, dtype), change) in enumerate(zip(DIRECTIVES, changes)):
            test.assertEqual(list(map(int, change[:4])), [65 + index] * 4, directive)
            test.assertEqual(int(change[4]), dtype, directive)
            contexts = declaration_contexts(model, chr(97 + index) + "Value")
            test.assertTrue(contexts, directive)
            test.assertTrue(all(model.options[context]["language-default", "def-type-" + str(65 + index)] == dtype for context in contexts))


def check_ranges(test):
    source = test.source(RANGES, "default-native-ranges.bas")
    for backend in test.backends:
        model = test.compile(source, backend=backend)
        changes = model.default_type_changes[1]
        test.assertEqual([list(map(int, row[:4])) for row in changes], [[90, 65, 65, 90], [90, 65, 65, 90], [64, 66, 65, 66], [91, 95, 91, 95]])
        test.assertEqual([row[6] for row in changes], ["1", "0", "1", "1"])
        test.assertEqual(changes[1][7], changes[1][8])
        for name, code, dtype in (("aText", 65, 19), ("aDouble", 65, 18), ("_Tail", 95, 19), ("_Array", 95, 19)):
            contexts = declaration_contexts(model, name)
            test.assertTrue(contexts, name)
            test.assertTrue(all(model.options[context]["language-default", "def-type-" + str(code)] == dtype for context in contexts))
        test.assertEqual(len(model.default_type_initial[1]["raw"]), 32)
        test.assertEqual(model.default_type_initial[1]["raw"], [8] * 32)


def check_modes(test):
    source = test.source(RANGES, "default-modes.bas")
    for abi, double in (("native", "18"), ("stable-v1", "16")):
        extra = ("-semantic-type-abi", abi)
        model = test.compile(source, extra=extra)
        test.assertEqual(model.default_type_changes[1][2][4:6], ["18", double])
        binary = test.working / ("default-" + abi + ".fbsem")
        test.invoke([source], mode="off", extra=(*extra, "-semantic-model", str(binary)))
        test.assertEqual(Model.read(binary).default_type_changes, model.default_type_changes)
        expression = test.compile(source, mode="expressions", extra=extra)
        test.assertEqual(len(expression.default_type_changes[1]), 4)
        test.assertTrue(all(row[7:9] == ["0", "0"] for row in expression.default_type_changes[1]))
    bindings = test.compile(source, mode="bindings")
    test.assertEqual(bindings.capabilities[1][DEFAULT_FEATURE], "unavailable")
    test.assertFalse(bindings.default_type_initial)
    plain = test.source('dim freshValue as long\nprint freshValue\n', "default-reset.bas")
    _, path = test.invoke([source, plain, source])
    model = Model.read(path)
    test.assertEqual([len(model.default_type_changes[module]) for module in (1, 2, 3)], [4, 0, 4])
    test.assertEqual(model.default_type_initial[2]["raw"], [8] * 32)


def check_dialects_and_recovery(test):
    for dialect, initial, integer in (("fblite", 8, 8), ("deprecated", 8, 8), ("qb", 17, 5)):
        source = test.source('#lang "' + dialect + '"\ndefint a\ndim aValue\n', "default-dialect-" + dialect + ".bas")
        model = test.compile(source)
        test.assertEqual(model.default_type_initial[1]["raw"], [initial] * 32)
        test.assertEqual(model.default_type_changes[1][0][4], str(integer))
    cases = (("fb", 'defstr a\n', 0), ("fblite", 'defstr abc\ndim aValue\n', 1),
             ("fblite", 'sub Blocked()\ndefstr a\nend sub\n', 0))
    for index, (dialect, text, count) in enumerate(cases):
        source = test.source('#lang "' + dialect + '"\n' + text, "default-recovery-" + str(index) + ".bas")
        baseline, _ = test.invoke([source], mode="off", success=False)
        result, path = test.invoke([source], mode="expressions", success=False)
        test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
        model = Model.read(path, expressions_only=True, allow_recovery=True)
        test.assertEqual(len(model.default_type_changes[1]), count)
    # QB applies DEF during parsing even in a procedure that never executes.
    source = test.source('#lang "qb"\nsub Unused()\ndefstr a\nend sub\ndim aValue\naValue = "text"\n', "default-qb-procedure.bas")
    model = test.compile(source)
    test.assertEqual(model.default_type_changes[1][0][4], "19")


def check_origins(test):
    test.source('defdbl b\n', "default-include.bi")
    source = test.source('''#lang "fblite"
#define LETTERS a-c
defstr LETTERS
#include "default-include.bi"
#if 0
defint a-z
#endif
#line 80 "logical-default.bas"
defsng z
''', "default-origins.bas")
    model = test.compile(source)
    changes = model.default_type_changes[1]
    test.assertEqual(len(changes), 3)
    test.assertNotEqual(changes[0][17], "0")
    test.assertEqual(model.source_contexts[int(changes[1][10])][5], "include")
    test.assertEqual(changes[2][11], "logical-default.bas")
    test.assertEqual(changes[2][16], "0")
    for mode in ("full", "expressions"):
        hidden = test.compile(source, mode=mode, extra=("-semantic-model-compact",))
        test.assertTrue(all(row[17:20] == ["0", "0", "0"] for row in hidden.default_type_changes[1]))


def check_corruption(test):
    model = test.compile(test.source(RANGES, "default-corruption.bas"))
    for mutation in ("initial", "count", "missing", "initial-abi", "ordinal", "range", "raw-range", "type", "changed", "before", "after", "statement", "source", "macro", "restore-table", "missing-key", "unknown-key", "removed-all"):
        with test.subTest(default_type_mutation=mutation):
            rows = [row.copy() for row in model.rows]
            event = next(row for row in rows if row[3:4] and row[3].startswith("native-default-type:"))
            initial = next(row for row in rows if row[3:4] == ["native-default-type-initial"])
            count = next(row for row in rows if row[3:4] == ["native-default-type-count"])
            if mutation in ("initial", "count", "missing"):
                rows.remove({"initial": initial, "count": count, "missing": event}[mutation])
            elif mutation == "removed-all":
                rows = [row for row in rows if not (row[0] == "K" and row[3].startswith("native-default-type:"))]
                count[4] = "0"
            elif mutation == "initial-abi":
                fields = initial[4].split("\t");fields[34] = "19";initial[4] = "\t".join(fields)
            elif mutation == "ordinal":
                event[3] = "native-default-type:999999"
            elif mutation in ("restore-table", "missing-key", "unknown-key"):
                option = next(row for row in rows if row[0] == "OPT" and row[2:4] == ["language-default", DEFAULT_KEYS[0]])
                if mutation == "missing-key":rows.remove(option)
                elif mutation == "unknown-key":option[3] = "def-type-96"
                else:option[4] = "19"
            else:
                field, value = {"range": (2, "64"), "raw-range": (0, "65"), "type": (5, "18"), "changed": (6, "0"),
                                "before": (7, "999999"), "after": (8, "999999"), "statement": (9, "999999"),
                                "source": (10, "999999"), "macro": (17, "999999")}[mutation]
                fields = event[4].split("\t");fields[field] = value;event[4] = "\t".join(fields)
            with test.assertRaises(ValueError):Model(text_rows(rows))


def check_runtime(test):
    source = test.source('#lang "fblite"\ndefint a-z\naValue = 2\ndefsng b\nbValue = 3.5\ndefstr c\ncValue = "ok"\nprint aValue; bValue; cValue\n', "default-runtime.bas")
    for backend in test.backends:
        outputs = []
        for mode in ("off", "full", "expressions", "bindings"):
            executable = test.working / ("default-" + backend + "-" + mode + (".exe" if test.native_windows else ""))
            test.invoke([source], mode=mode, backend=backend, emit=False, extra=("-x", str(executable)))
            result = subprocess.run([str(executable)], cwd=test.working, capture_output=True, text=True, timeout=30)
            test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            outputs.append(result.stdout)
        test.assertEqual(outputs, [outputs[0]] * 4)
        test.assertIn("ok", outputs[0])


def check_guards(test):
    native = (test.root / "src/compiler/symbols/symb.bas").read_text()
    bodies = []
    for kind, name in (("function", "symbGetDefType"), ("sub", "symbSetDefType")):
        start = native.index(kind + " " + name + " _")
        end = native.index("\nend " + kind, start) + len("\nend " + kind)
        bodies.append(native[start:end])
    guard = (test.root / "tests/semantic-sidecar/default-type-guards.bas").read_text()
    marker = "'' Native bodies are inserted here."
    source = test.working / "default-type-guards.bas"
    source.write_text(guard.replace(marker, "\n".join(bodies)))
    collector = test.working / "semantic-default-types.bas"
    collector.write_bytes((test.root / "src/compiler/tooling/semantic-default-types.bas").read_bytes())
    executable = test.working / ("default-type-guards.exe" if test.native_windows else "default-type-guards")
    result = subprocess.run([str(test.compiler), "-prefix", str(test.toolchain_prefix), "-e", "-i", str(test.root / "inc"),
                             "-i", str(test.root / "src/compiler"), "-m", source.stem, "-x", str(executable), str(source), str(collector)],
                            cwd=test.working, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], cwd=test.working, capture_output=True, text=True, timeout=300)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertIn("DEFAULT_TYPE_GUARDS_PASS", result.stdout)

# end of default_types.py
