"""Project: FreeBASIC semantic sidecar tests
File: evaluations.py
Purpose: Exercise actual preprocessor evaluation contexts and exclusions.
Responsibilities: Copied inputs/results, retries, errors, source and transport.
This file intentionally does NOT implement an evaluator or query resolver.
"""

import codecs
import subprocess

from driver_inputs import text_rows
from semantic_evaluations import FEATURE, PREFIX
from sidecar import Model

CONSTANTS = '''const Evaluated = __FB_EVAL__(1 + 2)
const Selected = __FB_IIF__(1, 7, 8)
print Evaluated; Selected
'''
QUERIES = '''dim QueryValue as long
const PointerType = __FB_QUERY_SYMBOL__(1, long ptr)
const KnownValue = __FB_QUERY_SYMBOL__(7, QueryValue)
print PointerType; KnownValue
'''


def ascii_value(value):
    return bytes(value).decode("ascii")


def check_constants(test):
    for encoding, bom in (("utf-8", b""), ("utf-16-le", codecs.BOM_UTF16_LE)):
        source = test.working / ("evaluation-" + encoding + ".bas")
        source.write_bytes(bom + CONSTANTS.encode(encoding))
        for backend in test.backends:
            baseline, _ = test.invoke([source], mode="off", backend=backend)
            emission = test.emission_path(source.stem, backend).read_bytes()
            result, path = test.invoke([source], backend=backend)
            test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
            model = Model.read(path)
            scopes = list(model.evaluation_contexts.values())
            test.assertEqual(len(scopes), 2)
            test.assertEqual([ascii_value(scope["input"]) for scope in scopes], ["1 + 2", "1"])
            test.assertEqual([ascii_value(scope["result"]) for scope in scopes], ["3", "1"])
            test.assertTrue(all(scope["begin"][4] == "constant-expression" for scope in scopes))
            test.assertTrue(all(scope["end"][2:4] == ["completed", "0"] for scope in scopes))
            expected = "bytes" if encoding == "utf-8" else "wide"
            test.assertTrue(all(scope["begin"][9] == expected for scope in scopes))
            test.assertTrue(all(int(scope["begin"][19]) > 0 for scope in scopes))


def check_queries(test):
    source = test.source(QUERIES, "evaluation-queries.bas")
    for backend in test.backends:
        model = test.compile(source, backend=backend)
        scopes = list(model.evaluation_contexts.values())
        retries = [scope for scope in scopes if scope["begin"][3] != "0"]
        test.assertEqual(len(retries), 1)
        replacement = retries[0]
        previous = model.evaluation_contexts[1, int(replacement["begin"][3])]
        test.assertEqual(previous["end"][2], "replay")
        test.assertIsNone(previous["result"])
        test.assertEqual(previous["input"], replacement["input"])
        test.assertEqual(ascii_value(replacement["input"]), "long ptr")
        test.assertEqual(replacement["end"][2:4], ["completed", "0"])
        known = next(scope for scope in scopes if ascii_value(scope["input"]) == "QueryValue")
        test.assertEqual(ascii_value(known["result"]), "-1")


def check_modes(test):
    source = test.source(CONSTANTS + QUERIES, "evaluation-modes.bas")
    for abi in ("native", "stable-v1"):
        extra = ("-semantic-type-abi", abi)
        full = test.compile(source, extra=extra)
        binary = test.working / ("evaluations-" + abi + ".fbsem")
        test.invoke([source], mode="off", extra=(*extra, "-semantic-model", str(binary)))
        test.assertEqual(Model.read(binary).evaluation_events, full.evaluation_events)
        reduced = test.compile(source, mode="expressions", extra=extra)
        test.assertEqual(len(reduced.evaluation_contexts), len(full.evaluation_contexts))
        test.assertTrue(all(scope["begin"][6] == "0" for scope in reduced.evaluation_contexts.values()))
    binding = test.compile(source, mode="bindings")
    test.assertEqual(binding.capabilities[1][FEATURE], "unavailable")
    test.assertFalse(binding.evaluation_events)
    for mode in ("full", "expressions"):
        hidden = test.compile(source, mode=mode, extra=("-semantic-model-compact",))
        test.assertTrue(hidden.evaluation_contexts)
        test.assertFalse(hidden.macro_invocations)
        test.assertTrue(all(scope["begin"][19] == "0" for scope in hidden.evaluation_contexts.values()))
    plain = test.source("print 1\n", "evaluation-empty.bas")
    _, path = test.invoke([source, plain, source])
    reset = Model.read(path)
    test.assertEqual(len(reset.evaluation_events[1]), len(reset.evaluation_events[3]))
    test.assertFalse(reset.evaluation_events[2])
    test.assertIn((3, 1), reset.evaluation_contexts)


def check_sources(test):
    include = test.source(CONSTANTS, "evaluation-include.bi")
    source = test.source('''#include "evaluation-include.bi"
#if 0
print __FB_EVAL__(NoSuchSkippedValue)
#endif
#line 200 "logical-evaluation.bas"
print __FB_EVAL__(4 + 5)
''', "evaluation-origins.bas")
    model = test.compile(source)
    scopes = list(model.evaluation_contexts.values())
    test.assertEqual(len(scopes), 3)
    test.assertTrue(all(scope["begin"][8] == "1" for scope in scopes[:2]))
    test.assertTrue(all(scope["begin"][13] == str(include) for scope in scopes[:2]))
    test.assertEqual(scopes[-1]["begin"][13], "logical-evaluation.bas")
    test.assertEqual(scopes[-1]["begin"][18], "0")
    test.assertFalse(any(b"NoSuchSkippedValue" in bytes(scope["input"]) for scope in scopes))


def check_recovery(test):
    for text in ('print __FB_EVAL__(1 +)\n', 'dim value as long\nprint __FB_EVAL__(value)\n',
                 'print __FB_EVAL__(1 2)\n'):
        for backend in test.backends:
            source = test.source(text, "evaluation-recovery.bas")
            baseline, _ = test.invoke([source], mode="off", backend=backend, success=False)
            result, path = test.invoke([source], mode="expressions", backend=backend, success=False)
            test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
            model = Model.read(path, expressions_only=True, allow_recovery=True)
            scopes = list(model.evaluation_contexts.values())
            test.assertTrue(scopes)
            test.assertTrue(any(scope["end"] and int(scope["end"][3]) > 0 for scope in scopes))
            test.assertTrue(all(scope["end"] is not None for scope in scopes))


def check_corruption(test):
    model = test.compile(test.source(CONSTANTS + QUERIES, "evaluation-integrity.bas"))
    for mutation in ("count", "ordinal", "kind", "parent", "replay", "purpose", "source", "context", "loader",
                     "parent-kind", "input-length", "input-width", "result-width", "result-error", "end-id", "missing-end"):
        with test.subTest(evaluation_mutation=mutation):
            rows = [row.copy() for row in model.rows]
            def event(kind):
                return next(row for row in rows if row[0] == "K" and row[3].startswith(PREFIX + ":") and row[4].split("\t")[0] == kind)
            summary = next(row for row in rows if row[0] == "K" and row[3] == PREFIX + "-counts")
            if mutation == "count":
                rows.remove(summary)
            elif mutation == "ordinal":
                event("begin")[3] = PREFIX + ":999999"
            elif mutation == "missing-end":
                rows.remove(event("end"))
                events = [row for row in rows if row[0] == "K" and row[3].startswith(PREFIX + ":")]
                for ordinal, row in enumerate(events, 1):row[3] = PREFIX + ":" + str(ordinal)
                fields = summary[4].split("\t");fields[0] = str(len(events));fields[2] = str(int(fields[2]) - 1);fields[3] = "1";summary[4] = "\t".join(fields)
            else:
                kind, index, value = {"kind": ("begin", 0, "inferred"), "parent": ("begin", 2, "999999"),
                                      "replay": ("begin", 3, "1"), "purpose": ("begin", 4, "execute"),
                                      "source": ("begin", 7, "999999"), "context": ("begin", 6, "999999"),
                                      "loader": ("begin", 19, "999999"), "parent-kind": ("begin", 8, "2"),
                                      "input-length": ("begin", 10, "99999"), "input-width": ("begin", 11, "4"),
                                      "result-width": ("end", 8, "4"), "result-error": ("end", 3, "-1"),
                                      "end-id": ("end", 1, "999999")}[mutation]
                row = event(kind);fields = row[4].split("\t");fields[index] = value;row[4] = "\t".join(fields)
            with test.assertRaises(ValueError):Model(text_rows(rows))


def check_runtime(test):
    source = test.source(CONSTANTS + QUERIES, "evaluation-runtime.bas")
    for backend in test.backends:
        outputs = []
        for mode in ("off", "full", "expressions", "bindings"):
            executable = test.working / ("evaluation-" + backend + "-" + mode + (".exe" if test.native_windows else ""))
            test.invoke([source], mode=mode, backend=backend, emit=False, extra=("-x", str(executable)))
            result = subprocess.run([str(executable)], cwd=test.working, capture_output=True, text=True, timeout=30)
            test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            outputs.append(result.stdout)
        test.assertEqual(outputs, [outputs[0]] * 4)


def check_capacity(test):
    native = (test.root / "src/compiler/symbols/symb-define.bas").read_text()
    start = native.index("private function hCanPushEvalCtx(")
    end = native.index("\nend function", start) + len("\nend function")
    guard = (test.root / "tests/semantic-sidecar/evaluation-capacity-guards.bas").read_text()
    source = test.working / "evaluation-capacity-guards.bas"
    source.write_text(guard.replace("'' Native guard is inserted here.", native[start:end]))
    executable = test.working / ("evaluation-capacity.exe" if test.native_windows else "evaluation-capacity")
    result = subprocess.run([str(test.compiler), "-prefix", str(test.toolchain_prefix), "-x", str(executable), str(source)],
                            cwd=test.working, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], cwd=test.working, capture_output=True, text=True, timeout=30)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertIn("EVALUATION_CAPACITY_PASS", result.stdout)

# end of evaluations.py
