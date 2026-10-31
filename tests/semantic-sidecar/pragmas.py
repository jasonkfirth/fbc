"""Project: FreeBASIC semantic sidecar tests
File: pragmas.py
Purpose: Exercise native pragma history and its exclusions.
Responsibilities: Stack failures, actual setters/callbacks, source identity and corruption.
This file intentionally does NOT implement pragma evaluation or reservation policy.
"""

import subprocess

from driver_inputs import text_rows
from semantic_pragmas import FEATURE, PREFIX
from sidecar import Model

SCALARS = '''#pragma msbitfields = 1
#pragma push(msbitfields, 0)
#pragma push(profile, 0)
#pragma pop(profile)
#pragma pop(msbitfields)
#pragma constness = 0
#pragma push(constness, 1)
#pragma pop(constness)
#pragma private = 1
sub Hidden()
end sub
#pragma private = 0
#pragma gui = 1
#pragma gui = 0
print 1
'''
CALLBACKS = '''#pragma reserve BasicReserved
#pragma reserve(asm) AsmReserved
#pragma reserve(extern) GlobalReserved
#pragma reserve(asm,extern) BothReserved
print 1
'''


def check_scalars(test):
    source = test.source(SCALARS, "pragma-scalars.bas")
    for backend in test.backends:
        baseline, _ = test.invoke([source], mode="off", backend=backend)
        emission = test.emission_path(source.stem, backend).read_bytes()
        result, path = test.invoke([source], backend=backend)
        test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
        test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
        model = Model.read(path)
        scopes = list(model.pragma_scopes.values())
        test.assertEqual(len(scopes), 12)
        test.assertEqual({int(scope["selection"][2]) for scope in scopes}, {0, 2, 4, 5, 6})
        test.assertTrue(all(scope["apply"] is not None and scope["end"][2] == "completed" for scope in scopes))
        test.assertEqual(sum(scope["stack"] is not None for scope in scopes), 6)
        test.assertTrue(any(scope["apply"][5] != scope["apply"][6] for scope in scopes))


def check_callbacks(test):
    source = test.source(CALLBACKS, "pragma-callbacks.bas")
    for backend in test.backends:
        model = test.compile(source, backend=backend)
        scopes = list(model.pragma_scopes.values())
        test.assertEqual([row[2] for scope in scopes for row in scope["reservations"]], ["symbol", "asm", "extern", "asm", "extern"])
        test.assertTrue(all(row[4] == "1" for scope in scopes for row in scope["reservations"]))
        symbol = scopes[0]["reservations"][0][5]
        test.assertEqual(model.symbols[int(symbol)][3], "16")
    include = test.source('#pragma once\nconst IncludedValue = 1\n', "pragma-once.bi")
    source = test.source('#include "pragma-once.bi"\n#include "pragma-once.bi"\nprint IncludedValue\n', "pragma-once.bas")
    model = test.compile(source)
    once = [scope for scope in model.pragma_scopes.values() if scope["callback"] and scope["callback"][3] == "once"]
    test.assertEqual(len(once), 1)
    test.assertEqual(once[0]["begin"][7], str(include))


def check_recovery(test):
    cases = (("underflow", '#pragma pop(profile)\n', "pop"),
             ("overflow", '#pragma push(profile, 0)\n' * 9, "push"),
             ("unknown", '#pragma NoSuchOption\n', "unknown-option"),
             ("callback-push", '#pragma push(once)\n', "disallowed-stack"),
             ("callback-assignment", '#pragma once = 1\n', "disallowed-assignment"),
             ("duplicate", '#pragma reserve UsedName\n#pragma reserve UsedName\n', "reserve"),
             ("partial-reservation", '#pragma reserve(extern) TakenName\n#pragma reserve(asm,extern) TakenName\n', "partial"),
             ("closing", '#pragma push(profile, 0\n', "closing"),
             ("scope-gui", 'sub Work()\n#pragma gui = 1\nend sub\n', "gui"))
    for backend in test.backends:
        for label, text, expected in cases:
            with test.subTest(backend=backend, case=label):
                source = test.source(text, "pragma-recovery-" + label + ".bas")
                baseline, _ = test.invoke([source], mode="off", backend=backend, success=False)
                result, path = test.invoke([source], mode="expressions", backend=backend, success=False)
                test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
                model = Model.read(path, expressions_only=True, allow_recovery=True)
                scopes = list(model.pragma_scopes.values())
                test.assertTrue(scopes)
                if expected in ("push", "pop"):
                    failure = next(scope for scope in scopes if scope["stack"] and scope["stack"][5] == "0")
                    test.assertEqual(failure["stack"][0], expected)
                    test.assertIsNotNone(failure["apply"], "A failed native stack action still reaches the setter")
                elif expected in ("unknown-option", "disallowed-stack", "disallowed-assignment"):
                    test.assertEqual(scopes[0]["end"][2], expected)
                    test.assertIsNone(scopes[0]["apply"])
                    test.assertIsNone(scopes[0]["callback"])
                elif expected in ("reserve", "partial"):
                    actions = scopes[-1]["reservations"]
                    test.assertEqual(actions[-1][4], "0")
                    if expected == "partial":
                        test.assertEqual([(row[2], row[4]) for row in actions], [("asm", "1"), ("extern", "0")])
                elif expected == "gui":
                    test.assertEqual(scopes[0]["apply"][5:7], ["0", "0"])
                else:
                    test.assertIsNotNone(scopes[0]["apply"])


def check_modes(test):
    source = test.source(SCALARS + CALLBACKS.replace("print 1\n", ""), "pragma-modes.bas")
    for abi in ("native", "stable-v1"):
        extra = ("-semantic-type-abi", abi)
        model = test.compile(source, extra=extra)
        binary = test.working / ("pragma-" + abi + ".fbsem")
        test.invoke([source], mode="off", extra=(*extra, "-semantic-model", str(binary)))
        test.assertEqual(Model.read(binary).pragma_events, model.pragma_events)
        reduced = test.compile(source, mode="expressions", extra=extra)
        test.assertTrue(reduced.pragma_events)
        test.assertTrue(all(row[5] == "0" for row in reduced.pragma_events[1] if row[0] == "begin"))
        test.assertTrue(all(row[5] == "0" for row in reduced.pragma_events[1] if row[0] == "reserve"))
    binding = test.compile(source, mode="bindings")
    test.assertEqual(binding.capabilities[1][FEATURE], "unavailable")
    test.assertFalse(binding.pragma_events)
    _, path = test.invoke([source, source])
    model = Model.read(path)
    test.assertEqual(len(model.pragma_events[1]), len(model.pragma_events[2]))


def check_origins(test):
    test.source('#pragma push(profile, 0)\n#pragma pop(profile)\n', "pragma-origin.bi")
    source = test.source('''#define OPTION_NAME profile
#pragma OPTION_NAME = 0
#include "pragma-origin.bi"
#if 0
#pragma pop(profile)
#endif
#line 80 "logical-pragma.bas"
#pragma profile = 1
print 1
''', "pragma-origins.bas")
    model = test.compile(source)
    scopes = list(model.pragma_scopes.values())
    test.assertEqual(len(scopes), 4)
    test.assertNotEqual(scopes[0]["begin"][13], "0")
    test.assertEqual(model.source_contexts[int(scopes[1]["begin"][6])][5], "include")
    test.assertEqual(scopes[-1]["begin"][7], "logical-pragma.bas")
    test.assertEqual(scopes[-1]["begin"][12], "0")
    for mode in ("full", "expressions"):
        hidden = test.compile(source, mode=mode, extra=("-semantic-model-compact",))
        test.assertTrue(all(row[13:16] == ["0", "0", "0"] for row in hidden.pragma_events[1] if row[0] == "begin"))


def check_corruption(test):
    model = test.compile(test.source(SCALARS + CALLBACKS, "pragma-corruption.bas"))
    mutations = ("count", "ordinal", "kind", "selection", "entry-selection", "stack-depth", "stack-result", "pop-value", "apply-value", "context", "source", "callback", "reservation", "missing-apply", "missing-reservation")
    for mutation in mutations:
        with test.subTest(pragma_mutation=mutation):
            rows = [row.copy() for row in model.rows]
            def event(kind):
                return next(row for row in rows if row[0] == "K" and row[3].startswith(PREFIX + ":") and row[4].split("\t")[0] == kind)
            summary = next(row for row in rows if row[3:4] == [PREFIX + "-counts"])
            if mutation == "count":rows.remove(summary)
            elif mutation == "ordinal":event("begin")[3] = PREFIX + ":999999"
            elif mutation.startswith("missing-"):
                rows.remove(event("apply" if mutation == "missing-apply" else "reserve"))
                events = [row for row in rows if row[0] == "K" and row[3].startswith(PREFIX + ":")]
                for ordinal, row in enumerate(events, 1):row[3] = PREFIX + ":" + str(ordinal)
                fields = summary[4].split("\t");fields[0] = str(len(events));summary[4] = "\t".join(fields)
            else:
                kind, index, value = {"kind": ("begin", 0, "guessed"), "selection": ("select", 5, "0"), "entry-selection": ("begin", 3, "profile"),
                                      "stack-depth": ("push", 3, "8"), "stack-result": ("push", 5, "0"),
                                      "pop-value": ("pop", 6, "99"), "apply-value": ("apply", 4, "99"),
                                      "context": ("apply", 7, "999999"), "source": ("begin", 6, "999999"),
                                      "callback": ("callback", 3, "guessed"), "reservation": ("reserve", 5, "999999")}[mutation]
                row = event(kind);fields = row[4].split("\t");fields[index] = value;row[4] = "\t".join(fields)
            with test.assertRaises(ValueError):Model(text_rows(rows))


def check_runtime(test):
    source = test.source(SCALARS, "pragma-runtime.bas")
    for backend in test.backends:
        outputs = []
        for mode in ("off", "full", "expressions", "bindings"):
            executable = test.working / ("pragma-" + backend + "-" + mode + (".exe" if test.native_windows else ""))
            test.invoke([source], mode=mode, backend=backend, emit=False, extra=("-x", str(executable)))
            result = subprocess.run([str(executable)], cwd=test.working, capture_output=True, text=True, timeout=30)
            test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            outputs.append(result.stdout)
        test.assertEqual(outputs, [outputs[0]] * 4)


def check_guards(test):
    native = (test.root / "src/compiler/preprocessor/pp-pragma.bas").read_text()
    prefix = native[native.index("enum LEXPP_PRAGMAFLAG_ENUM"):native.index("sub ppPragmaInit")]
    bodies = []
    for name in ("pragmaPush", "pragmaPop"):
        start = native.index("private sub " + name + "(")
        end = native.index("\nend sub", start) + len("\nend sub")
        bodies.append(native[start:end])
    source = test.working / "pragma-guards.bas"
    guard = (test.root / "tests/semantic-sidecar/pragma-guards.bas").read_text()
    source.write_text(guard.replace("'' Native definitions are inserted here.", prefix + "\n".join(bodies)))
    collector = test.working / "semantic-pragmas.bas"
    collector.write_bytes((test.root / "src/compiler/tooling/semantic-pragmas.bas").read_bytes())
    executable = test.working / ("pragma-guards.exe" if test.native_windows else "pragma-guards")
    result = subprocess.run([str(test.compiler), "-prefix", str(test.toolchain_prefix), "-e", "-i", str(test.root / "inc"), "-i", str(test.root / "src/compiler"),
                             "-m", source.stem, "-x", str(executable), str(source), str(collector)], cwd=test.working, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], cwd=test.working, capture_output=True, text=True, timeout=300)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertIn("PRAGMA_GUARDS_PASS", result.stdout)

# end of pragmas.py
