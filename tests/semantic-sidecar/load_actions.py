"""Project: FreeBASIC semantic sidecar tests
File: load_actions.py
Purpose: Prove actual loader intervals, late calls and guarded handler transfers.
Responsibilities: Origins, omissions, exact emission, runtime and malformed actions.
This file intentionally does NOT treat lowering traversal as dynamic execution.
"""

import subprocess

from driver_inputs import text_rows
from semantic_loads import FEATURE, PREFIXES
from sidecar import Model


def check_actions(test):
    source = test.fixture("load-actions.bas")
    choices = [(backend, ()) for backend in test.backends]
    choices += [("gas", ("-target", "linux-x86")), ("gas64", ())]
    for backend, target in choices:
        with test.subTest(backend=backend):
            extra = (*target, "-exx")
            test.invoke([source], mode="off", backend=backend, extra=extra)
            baseline = test.emission_path(source.stem, backend).read_bytes()
            model = test.compile(source, backend=backend, extra=extra)
            test.assertEqual(model.capabilities[1][FEATURE], "available")
            facts = model.native_loads[1]
            test.assertEqual(set(facts["enters"]), set(facts["ends"]))
            unused = test.one(model, "UnusedLoadBody", "procedure")
            unused_entries = {identity: fields for identity, fields in facts["enters"].items() if fields[1] == str(unused)}
            test.assertTrue(unused_entries)
            test.assertTrue(all(fields[8] == "0" and facts["ends"][identity] == "0" for identity, fields in unused_entries.items()))
            test.assertFalse([fields for fields in facts["calls"].values() if int(fields[0]) in unused_entries])
            test.assertTrue(any(row[2] != "0" for row in facts["enters"].values()))
            test.assertTrue(any(row[2] == "0" and row[3] != "0" for row in facts["enters"].values()))
            checks = list(facts["checks"].values())
            test.assertTrue(any(row[1] == "bounds-or-dimensions" for row in checks))
            test.assertTrue(any(row[1] == "null-pointer" for row in checks))
            test.assertTrue(all(row[3:] == ("nonzero-handler", "unknown-indirect") for row in checks))
            calls = list(facts["calls"].values())
            unicode_calls = [row for row in calls if model.types[int(row[1])][2].casefold() == "fb_ustrindex"]
            test.assertTrue(unicode_calls)
            test.assertTrue(any(facts["enters"][int(row[0])][2] == "0" for row in unicode_calls))
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)
            unchecked = test.compile(source, backend=backend, extra=target)
            test.assertFalse(unchecked.native_loads[1]["checks"])


def check_modes(test):
    source = test.fixture("load-actions.bas")
    for backend in test.backends:
        for mode in ("bindings", "expressions"):
            with test.subTest(backend=backend, mode=mode):
                model = test.compile(source, backend=backend, mode=mode, extra=("-exx",))
                test.assertEqual(model.capabilities[1][FEATURE], "unavailable")
                test.assertFalse([row for row in model.records["K"] if row[3].startswith(PREFIXES)])
        for abi in ("native", "stable-v1"):
            with test.subTest(backend=backend, abi=abi):
                extra = ("-exx", "-semantic-type-abi", abi)
                model = test.compile(source, backend=backend, extra=extra)
                path = test.working / (backend + "-load-" + abi + ".bin")
                test.invoke([source], backend=backend, mode="off", extra=(*extra, "-semantic-model", str(path)))
                test.assertEqual(Model.read(path).rows, model.rows)
    first = test.source("print 1\n", "load-one.bas")
    second = test.source("print 2\n", "load-two.bas")
    _, path = test.invoke([first, second])
    model = Model.read(path)
    test.assertEqual(set(model.native_loads), {1, 2})
    test.assertTrue(all(next(iter(facts["enters"])) == 1 for facts in model.native_loads.values()))


def check_rejection(test):
    model = test.compile(test.fixture("load-actions.bas"), extra=("-exx",))
    cases = (("native-load-enter:", 0, "1000000"), ("native-load-enter:", 1, "4294967295"),
             ("native-load-enter:", 2, "4294967295"), ("native-load-enter:", 3, "4294967295"),
             ("native-load-enter:", 4, "1000000"), ("native-load-enter:", 5, "4294967295"),
             ("native-load-enter:", 8, "2"), ("native-load-enter:", 9, "0"),
             ("native-load-end:", 0, "2"), ("native-load-call:", 0, "1000000"),
             ("native-load-call:", 2, "2"), ("native-load-call:", 3, "guessed"),
             ("native-load-check:", 1, "guessed"), ("native-load-check:", 3, "always"),
             ("native-load-check:", 4, "known"), ("native-load-counts:", 0, "0"))
    for prefix, slot, value in cases:
        with test.subTest(prefix=prefix, slot=slot):
            rows = [row.copy() for row in model.rows]
            row = next(row for row in rows if row[0] == "K" and row[3].startswith(prefix))
            fields = row[4].split("\t")
            fields[slot] = value
            row[4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(text_rows(rows))
    for prefix, slot in (("native-load-enter:", 0), ("native-load-end:", 1)):
        with test.subTest(recounted_missing=prefix):
            rows = [row.copy() for row in model.rows]
            packet = next(row for row in rows if row[0] == "K" and row[3].startswith(prefix))
            rows.remove(packet)
            counts = next(row for row in rows if row[0] == "K" and row[3].startswith("native-load-counts:"))
            fields = counts[4].split("\t")
            fields[slot] = str(int(fields[slot]) - 1)
            counts[4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(text_rows(rows))
    rows = [row.copy() for row in model.rows]
    unused = next(row for row in rows if row[0] == "K" and row[3].startswith("native-load-enter:") and row[4].split("\t")[8] == "0")
    identity = unused[3].split(":")[1]
    next(row for row in rows if row[0] == "K" and row[3] == "native-load-end:" + identity)[4] = "1"
    with test.assertRaises(ValueError):
        Model(text_rows(rows))


def check_execution(test):
    baseline = None
    for mode in ("off", "full", "bindings", "expressions"):
        executable = test.working / ("loads-" + mode)
        test.invoke([test.fixture("load-actions.bas")], mode=mode, emit=False, extra=("-exx", "-x", str(executable)))
        result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
        test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        test.assertEqual(result.stdout.strip(), "111")
        if baseline is None:
            baseline = result.stdout
        else:
            test.assertEqual(result.stdout, baseline)


def check_guards(test):
    executable = test.working / "load-guards"
    command = [str(test.compiler), "-prefix", str(test.toolchain_prefix), "-i", str(test.root / "src/compiler"),
               "-i", str(test.root / "inc"), str(test.fixture("load-action-guards.bas")),
               str(test.root / "src/compiler/tooling/semantic-loads.bas"), "-x", str(executable)]
    result = subprocess.run(command, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertEqual(result.stdout.strip(), "LOAD_GUARDS_PASS")

# end of load_actions.py
