"""Project: FreeBASIC semantic sidecar tests
File: evaluations.py
Purpose: Exercise actual preprocessor evaluation contexts and exclusions.
Responsibilities: Copied inputs/results, retries, errors, source and transport.
This file intentionally does NOT implement an evaluator or query resolver.
"""

import codecs
import subprocess

from driver_inputs import text_rows
from semantic_evaluations import BUFFER_FEATURE, FEATURE, PREFIX
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


def check_wide_text(test):
    text = 'print __FB_EVAL__("h\u00e9\u03bb")\n'
    encodings = (("utf-16-le", codecs.BOM_UTF16_LE), ("utf-16-be", codecs.BOM_UTF16_BE),
                 ("utf-32-le", codecs.BOM_UTF32_LE), ("utf-32-be", codecs.BOM_UTF32_BE))
    for encoding, bom in encodings:
        source = test.working / ("evaluation-text-" + encoding + ".bas")
        source.write_bytes(bom + text.encode(encoding))
        for backend in test.backends:
            model = test.compile(source, backend=backend)
            test.assertEqual(len(model.evaluation_contexts), 1)
            scope = next(iter(model.evaluation_contexts.values()))
            test.assertEqual(scope["begin"][9], "wide")
            test.assertIn(0x00E9, scope["input"])
            test.assertIn(0x03BB, scope["input"])
            callback = model.macro_callbacks[int(scope["begin"][19])]
            test.assertEqual(scope["result"], model.macro_units(callback[2], callback[4]))


def check_long_input(test):
    expression = " + ".join(["1"] * 351)
    source = test.source("const LongEvaluation = __FB_EVAL__(" + expression + ")\nprint LongEvaluation\n", "evaluation-long.bas")
    model = test.compile(source)
    scope = next(iter(model.evaluation_contexts.values()))
    test.assertGreater(len(scope["input"]), 1024)
    test.assertEqual(ascii_value(scope["input"]), expression)
    test.assertEqual(ascii_value(scope["result"]), "351")


def check_preprocessor(test):
    source = test.source('''#if __FB_EVAL__(1)
print 1
#elseif __FB_EVAL__(0)
print 2
#else
print 3
#endif
#if 0
print __FB_EVAL__(SkippedEvaluation)
#endif
''', "evaluation-preprocessor.bas")
    for mode in ("full", "expressions"):
        for extra in ((), ("-pp",)):
            baseline, _ = test.invoke([source], mode="off", extra=extra)
            result, path = test.invoke([source], mode=mode, extra=extra)
            test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
            model = Model.read(path, expressions_only=mode == "expressions")
            scopes = list(model.evaluation_contexts.values())
            test.assertEqual([ascii_value(scope["input"]) for scope in scopes], ["1", "0"])
            test.assertEqual([ascii_value(scope["result"]) for scope in scopes], ["1", "0"])
            test.assertTrue(all(scope["end"][3] == "0" for scope in scopes))


def nested_input(depth):
    expression = "1"
    for ordinal in range(depth):
        expression = "__FB_EVAL__(" + expression + " + 1)"
    return "print " + expression + "\n"


def check_nesting(test):
    model = test.compile(test.source(nested_input(6), "evaluation-nested.bas"))
    scopes = list(model.evaluation_contexts.values())
    test.assertEqual(len(scopes), 6)
    test.assertEqual([scope["begin"][2] for scope in scopes], ["0"] * 6)
    test.assertEqual([scope["begin"][8] for scope in scopes], ["0"] * 6)
    test.assertEqual([ascii_value(scope["result"]) for scope in scopes], [str(value) for value in range(2, 8)])
    test.assertTrue(any(model.macro_invocations[int(scope["begin"][19])][2] != "0" for scope in scopes))
    rows = [row.copy() for row in model.rows]
    events = [row for row in rows if row[0] == "K" and row[3].startswith(PREFIX + ":")]
    # Forge two overlapping lifetimes while preserving every event, ordinal
    # and terminal total. The declared capacity must constrain peak depth,
    # even when both contexts are closed again before the summary.
    first = [row[4] for row in events if row[4].split("\t")[1] == "1"]
    second = [row[4] for row in events if row[4].split("\t")[1] == "2"]
    fields = second[0].split("\t");fields[2] = "1";fields[8] = "2";fields[18] = "0"
    reordered = first[:-1] + ["\t".join(fields)] + second[1:] + first[-1:]
    for row, value in zip(events, reordered):row[4] = value
    summary = next(row for row in rows if row[0] == "K" and row[3] == PREFIX + "-counts")
    fields = summary[4].split("\t");fields[4] = "1";summary[4] = "\t".join(fields)
    with test.assertRaisesRegex(ValueError, "depth"):Model(text_rows(rows))


def check_deep_macros(test):
    source = test.source(nested_input(18), "evaluation-deep-macros.bas")
    baseline, _ = test.invoke([source], mode="off")
    result, path = test.invoke([source], mode="expressions")
    test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
    model = Model.read(path, expressions_only=True)
    scopes = list(model.evaluation_contexts.values())
    test.assertEqual(len(scopes), 18)
    test.assertTrue(all(scope["begin"][2] == "0" for scope in scopes))
    test.assertTrue(all(scope["end"] is not None for scope in scopes))
    test.assertTrue(all(scope["end"][3] == "0" for scope in scopes))
    test.assertEqual(ascii_value(scopes[-1]["result"]), "19")


def check_buffers(test):
    for encoding, bom in (("utf-8", b""), ("utf-16-le", codecs.BOM_UTF16_LE)):
        source = test.working / ("evaluation-buffers-" + encoding + ".bas")
        source.write_bytes(bom + (CONSTANTS + QUERIES).encode(encoding))
        for backend in test.backends:
            model = test.compile(source, backend=backend)
            test.assertEqual(model.capabilities[1][BUFFER_FEATURE], "available")
            scopes = list(model.evaluation_contexts.values())
            for scope in scopes:
                test.assertIsNotNone(scope["buffer_event"])
                test.assertEqual(list(scope["buffer"]), list(scope["input"]) + [10])
                test.assertEqual(scope["buffer_event"][2], "bytes" if encoding == "utf-8" else "wide")
                if scope["begin"][3] != "0":
                    previous = model.evaluation_contexts[1, int(scope["begin"][3])]
                    test.assertEqual(previous["buffer"], scope["buffer"])
            if encoding != "utf-8":
                mixed = [scope for scope in scopes if scope["begin"][9] == "bytes"]
                test.assertTrue(mixed, "Z query callbacks must retain their conversion to wide lexer buffers")
                test.assertTrue(all(scope["end"][6] == "bytes" or scope["end"][2] == "replay" for scope in mixed))


def check_cross_target_buffers(test):
    source = test.working / "evaluation-cross-target.bas"
    text = 'print __FB_EVAL__("\u03bb")\n' + QUERIES
    source.write_bytes(codecs.BOM_UTF16_LE + text.encode("utf-16-le"))
    native = test.compile(source)
    target = test.compile(source, extra=("-target", "win64"))
    native_buffers = [(scope["buffer_event"][4], tuple(scope["buffer"])) for scope in native.evaluation_contexts.values()]
    target_buffers = [(scope["buffer_event"][4], tuple(scope["buffer"])) for scope in target.evaluation_contexts.values()]
    test.assertTrue(native_buffers)
    test.assertEqual(target_buffers, native_buffers)
    test.assertTrue(all(scope["buffer_event"][2] == "wide" for scope in target.evaluation_contexts.values()))


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
    parser_error_seen, deferred_error_seen = False, False
    for encoding, bom in (("utf-8", b""), ("utf-16-le", codecs.BOM_UTF16_LE)):
        for text in ('print __FB_EVAL__(1 +)\n', 'dim value as long\nprint __FB_EVAL__(value)\n',
                     'print __FB_EVAL__(1 2)\n'):
            for backend in test.backends:
                source = test.working / ("evaluation-recovery-" + encoding + ".bas")
                source.write_bytes(bom + text.encode(encoding))
                baseline, _ = test.invoke([source], mode="off", backend=backend, success=False)
                result, path = test.invoke([source], mode="expressions", backend=backend, success=False)
                test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
                model = Model.read(path, expressions_only=True, allow_recovery=True)
                scopes = list(model.evaluation_contexts.values())
                test.assertTrue(scopes)
                test.assertTrue(any(scope["native_error_observed"] for scope in scopes))
                test.assertTrue(all(scope["end"] is not None for scope in scopes))
                for scope in scopes:
                    end = scope["end"]
                    parser_error_seen |= end[3] == "0" and int(end[5]) > int(end[4])
                    deferred_error_seen |= int(end[3]) > 0 and end[4] == end[5]
    test.assertTrue(parser_error_seen, "A zero callback error must not erase the parser's diagnostic")
    test.assertTrue(deferred_error_seen, "An unreported callback error must not be treated as a successful value")


def check_corruption(test):
    model = test.compile(test.source(CONSTANTS + QUERIES, "evaluation-integrity.bas"))
    mutations = ["count", "ordinal", "kind", "parent", "replay", "purpose", "source", "context", "loader",
                 "parent-kind", "input-length", "input-width", "result-width", "result-error", "end-id", "missing-end"]
    if model.capabilities[1].get(BUFFER_FEATURE) == "available":
        mutations += ["missing-buffer", "buffer-id", "buffer-marker"]
    for mutation in mutations:
        with test.subTest(evaluation_mutation=mutation):
            rows = [row.copy() for row in model.rows]
            def event(kind):
                return next(row for row in rows if row[0] == "K" and row[3].startswith(PREFIX + ":") and row[4].split("\t")[0] == kind)
            summary = next(row for row in rows if row[0] == "K" and row[3] == PREFIX + "-counts")
            if mutation == "count":
                rows.remove(summary)
            elif mutation == "ordinal":
                event("begin")[3] = PREFIX + ":999999"
            elif mutation in ("missing-end", "missing-buffer"):
                rows.remove(event("end" if mutation == "missing-end" else "buffer"))
                events = [row for row in rows if row[0] == "K" and row[3].startswith(PREFIX + ":")]
                for ordinal, row in enumerate(events, 1):row[3] = PREFIX + ":" + str(ordinal)
                fields = summary[4].split("\t");fields[0] = str(len(events))
                if mutation == "missing-end":fields[2] = str(int(fields[2]) - 1);fields[3] = "1"
                summary[4] = "\t".join(fields)
            elif mutation == "buffer-marker":
                row = event("buffer");fields = row[4].split("\t")
                test.assertTrue(fields[5].endswith("%0A"))
                fields[5] = fields[5][:-3] + "X";row[4] = "\t".join(fields)
            else:
                kind, index, value = {"kind": ("begin", 0, "inferred"), "parent": ("begin", 2, "999999"),
                                      "replay": ("begin", 3, "1"), "purpose": ("begin", 4, "execute"),
                                      "source": ("begin", 7, "999999"), "context": ("begin", 6, "999999"),
                                      "loader": ("begin", 19, "999999"), "parent-kind": ("begin", 8, "2"),
                                      "input-length": ("begin", 10, "1"), "input-width": ("begin", 11, "4"),
                                      "result-width": ("end", 8, "4"), "result-error": ("end", 3, "-1"),
                                      "end-id": ("end", 1, "999999"), "buffer-id": ("buffer", 1, "999999")}[mutation]
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


def check_observer_guards(test):
    source = test.fixture("evaluation-observer-guards.bas")
    collector = test.working / "semantic-evaluations.bas"
    collector.write_bytes((test.root / "src/compiler/tooling/semantic-evaluations.bas").read_bytes())
    executable = test.working / ("evaluation-observer.exe" if test.native_windows else "evaluation-observer")
    result = subprocess.run([str(test.compiler), "-prefix", str(test.toolchain_prefix), "-e", "-i", str(test.root / "inc"),
                             "-i", str(test.root / "src/compiler"), "-m", source.stem, "-x", str(executable), str(source), str(collector)],
                            cwd=test.working, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], cwd=test.working, capture_output=True, text=True, timeout=300)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertIn("EVALUATION_OBSERVER_PASS", result.stdout)

# end of evaluations.py
