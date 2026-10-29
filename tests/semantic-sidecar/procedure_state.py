"""Project: FreeBASIC semantic sidecar tests
File: procedure_state.py
Purpose: Exercise actual hidden error-handler and GOSUB state relationships.
Responsibilities: Native storage/action ownership, mode exclusions and corruption.
This file intentionally does NOT infer runtime execution from generated actions.
"""

import subprocess

from driver_inputs import text_rows
from sidecar import Model
from semantic_procedure_state import FEATURE

HANDLERS = '''#lang "fblite"
sub Work(byval enabled as long)
    if enabled then
        on local error goto Handler
    end if
    exit sub
Handler:
    resume next
end sub
Work(0)
'''
GOSUB = '''#lang "fblite"
option gosub
sub Work()
    dim selected as long = 1
    gosub FirstTarget
    on selected gosub FirstTarget, SecondTarget
    exit sub
FirstTarget:
    return
SecondTarget:
    return
end sub
Work()
'''


def check_guards(test):
    source = test.working / "procedure-state-guards.bas"
    source.write_bytes((test.root / "tests/semantic-sidecar/procedure-state-guards.bas").read_bytes())
    collector = test.working / "semantic-procedure-state.bas"
    collector.write_bytes((test.root / "src/compiler/tooling/semantic-procedure-state.bas").read_bytes())
    executable = test.working / ("procedure-state-guards.exe" if test.native_windows else "procedure-state-guards")
    result = subprocess.run([str(test.compiler), "-prefix", str(test.toolchain_prefix), "-e",
                             "-i", str(test.root / "inc"), "-i", str(test.root / "src/compiler"),
                             "-m", source.stem, "-x", str(executable), str(source),
                             str(collector)],
                            cwd=test.working, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], cwd=test.working, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertIn("PROCEDURE_STATE_GUARDS_PASS", result.stdout)


def selected(model, name):
    return [state for (module, identity), state in model.procedure_states.items()
            if model.symbol_name(identity).upper() == name.upper()]


def check_handlers(test):
    source = test.source(HANDLERS, "procedure-state-handlers.bas")
    for backend in test.backends:
        baseline, _ = test.invoke([source], mode="off", backend=backend, extra=("-exx",))
        emission = test.emission_path(source.stem, backend).read_bytes()
        result, path = test.invoke([source], backend=backend, extra=("-exx",))
        test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
        test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
        model = Model.read(path)
        states = selected(model, "Work")
        test.assertEqual(len(states), 1)
        events = states[0]["events"]
        test.assertTrue({"begin", "handler-allocated", "handler-save", "module-name-save", "function-name-save",
                         "before-error-exit", "after-error-exit"} <= {row[1] for row in events})
        saves = [row for row in events if row[1] == "handler-save"]
        test.assertTrue(all(row[5] != "0" and row[8] != "0" and row[11] != "0" and row[12] != "0" for row in saves))
        before = next(row for row in events if row[1] == "before-error-exit")
        after = next(row for row in events if row[1] == "after-error-exit")
        test.assertTrue(all(value != "0" for value in before[5:9]))
        test.assertEqual(after[5:9], ["0"] * 4)


def check_gosub(test):
    source = test.source(GOSUB, "procedure-state-gosub.bas")
    for backend in test.backends:
        baseline, _ = test.invoke([source], mode="off", backend=backend)
        emission = test.emission_path(source.stem, backend).read_bytes()
        result, path = test.invoke([source], backend=backend)
        test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
        test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
        model = Model.read(path)
        state = selected(model, "Work")[0]
        events = [row for row in state["events"] if row[1].startswith("gosub-")]
        test.assertTrue({"gosub-allocated", "gosub-direct", "gosub-computed", "gosub-return", "gosub-exit"} <= {row[1] for row in events})
        test.assertEqual({row[9] for row in events}, {events[0][9]})
        test.assertEqual({row[10] for row in events}, {"setjmp"})


def check_exclusions(test):
    source = test.source('sub Plain()\nend sub\nPlain()\n', "procedure-state-plain.bas")
    model = test.compile(source)
    for state in model.procedure_states.values():
        test.assertTrue(all(row[5:10] == ["0"] * 5 for row in state["events"]))
    for mode in ("bindings", "expressions"):
        model = test.compile(source, mode=mode)
        test.assertEqual(model.capabilities[1][FEATURE], "unavailable")
        test.assertFalse(model.procedure_state_events)


def check_assembly(test):
    source = test.source(GOSUB, "procedure-state-gosub-assembly.bas")
    # This is emission-only: select the required target instead of assuming
    # that the host name includes its CPU or that a target SDK is installed.
    for options, expected in (((), "call-ret"), (("-z", "gosub-setjmp"), "setjmp")):
        extra = ("-target", "linux-x86_64", *options)
        baseline, _ = test.invoke([source], mode="off", backend="gas64", extra=extra)
        emission = test.emission_path(source.stem, "gas64").read_bytes()
        result, path = test.invoke([source], backend="gas64", extra=extra)
        test.assertEqual((result.stdout, result.stderr), (baseline.stdout, baseline.stderr))
        test.assertEqual(test.emission_path(source.stem, "gas64").read_bytes(), emission)
        model = Model.read(path)
        events = [row for row in selected(model, "Work")[0]["events"] if row[1].startswith("gosub-")]
        test.assertTrue(events)
        test.assertEqual({row[10] for row in events}, {expected})


def check_transport(test):
    source = test.source(HANDLERS, "procedure-state-transport.bas")
    for abi in ("native", "stable-v1"):
        extra = ("-exx", "-semantic-type-abi", abi)
        model = test.compile(source, extra=extra)
        path = test.working / ("procedure-state-" + abi + ".fbsem")
        test.invoke([source], mode="off", extra=(*extra, "-semantic-model", str(path)))
        binary = Model.read(path)
        test.assertEqual(binary.procedure_state_events, model.procedure_state_events)
    _, path = test.invoke([source, source], extra=("-exx",))
    model = Model.read(path)
    test.assertEqual(set(model.procedure_state_events), {1, 2})


def check_corruption(test):
    model = test.compile(test.source(HANDLERS, "procedure-state-corruption.bas"), extra=("-exx",))
    for mutation in ("missing", "summary", "ordinal", "owner", "kind", "context", "storage", "saved-pair", "temporary", "mode", "duplicate"):
        with test.subTest(procedure_state_mutation=mutation):
            rows = [row.copy() for row in model.rows]
            row = next(row for row in rows if row[0] == "K" and row[3].startswith("native-procedure-state:")
                       and row[4].split("\t")[1] == "handler-save")
            summary = next(row for row in rows if row[3:4] == ["native-procedure-state-count"])
            if mutation == "missing":
                rows.remove(row)
            elif mutation == "summary":
                rows.remove(summary)
            elif mutation == "ordinal":
                row[3] = "native-procedure-state:999999"
            elif mutation == "duplicate":
                rows.insert(-1, row.copy())
            else:
                index, value = {"owner": (0, "999999"), "kind": (1, "guessed"), "context": (3, "999999"),
                                "storage": (5, "999999"), "saved-pair": (8, "0"), "temporary": (11, "0"),
                                "mode": (10, "guessed")}[mutation]
                fields = row[4].split("\t")
                fields[index] = value
                row[4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(text_rows(rows))

# end of procedure_state.py
