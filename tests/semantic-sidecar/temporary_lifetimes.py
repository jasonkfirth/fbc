"""Project: FreeBASIC semantic sidecar tests
File: temporary_lifetimes.py
Purpose: Prove native temporary state transitions and conditional flush ownership.
Responsibilities: Actual actions/cancellations, cookie reuse, runtime and corruption.
This file intentionally does NOT infer branch execution from retained temporary actions.
"""

from collections import Counter
import subprocess

from driver_inputs import text_rows
from semantic_temporaries import FEATURE, PREFIXES, NODE_FEATURE
from sidecar import Model


def check_events(test):
    source = test.fixture("temporary-lifetimes.bas")
    choices = [(backend, ()) for backend in test.backends]
    choices += [("gas", ("-target", "linux-x86")), ("gas64", ())]
    for backend, extra in choices:
        with test.subTest(backend=backend):
            test.invoke([source], mode="off", backend=backend, extra=extra)
            baseline = test.emission_path(source.stem, backend).read_bytes()
            model = test.compile(source, backend=backend, extra=extra)
            test.assertEqual(model.capabilities[1][FEATURE], "available")
            facts = model.temporary_lifetimes[1]
            events = facts["events"]
            kinds = Counter(fields[0] for fields in events.values())
            for kind in ("add", "remove", "scope-begin", "scope-end", "flush-begin", "flush-action", "flush-end", "conditional", "rescope"):
                test.assertGreater(kinds[kind], 0, kind)
            test.assertTrue(any(fields[0] == "rescope" and fields[-1] == "-1" for fields in events.values()))
            test.assertTrue(any(fields[:1] == ("remove",) and fields[-1] == "discard" for fields in events.values()))
            cookie_epochs = {}
            for epoch, cookie, _ in facts["scope_events"].values():
                cookie_epochs.setdefault(cookie, set()).add(epoch)
            test.assertTrue(any(len(epochs) > 1 for epochs in cookie_epochs.values()))
            test.assertTrue(any(left and right for _, left, right in facts["conditionals"].values()))
            for _, left, right in facts["conditionals"].values():
                if left and right:
                    first = {generation for generation, _ in facts["flushes"][left]["actions"]}
                    second = {generation for generation, _ in facts["flushes"][right]["actions"]}
                    test.assertFalse(first & second)
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)


def check_modes(test):
    source = test.fixture("temporary-lifetimes.bas")
    for backend in test.backends:
        for mode in ("bindings", "expressions"):
            with test.subTest(backend=backend, mode=mode):
                model = test.compile(source, backend=backend, mode=mode)
                test.assertEqual(model.capabilities[1][FEATURE], "unavailable")
                test.assertEqual(model.capabilities[1][NODE_FEATURE], "unavailable")
                test.assertFalse([row for row in model.records["K"] if row[3].startswith(PREFIXES)])
        for abi in ("native", "stable-v1"):
            with test.subTest(backend=backend, abi=abi):
                extra = ("-semantic-type-abi", abi)
                model = test.compile(source, backend=backend, extra=extra)
                path = test.working / (backend + "-temporary-" + abi + ".bin")
                test.invoke([source], backend=backend, mode="off", extra=(*extra, "-semantic-model", str(path)))
                test.assertEqual(Model.read(path).rows, model.rows)
    first = test.source('dim first_temporary as string = "first"\n', "first-temporary.bas")
    second = test.source('dim second_temporary as string = "second"\n', "second-temporary.bas")
    _, path = test.invoke([first, second])
    model = Model.read(path)
    test.assertEqual(set(model.temporary_lifetimes), {1, 2})


def check_rejection(test):
    model = test.compile(test.fixture("temporary-lifetimes.bas"))
    cases = (("add", 1, "0"), ("add", 2, "4294967295"), ("add", 4, "1"), ("add", 5, "1000000"),
             ("add", 6, "0"), ("add", 7, "4294967295"), ("scope-begin", 2, "0"),
             ("scope-end", 1, "0"), ("flush-action", 1, "1000000"), ("flush-action", 2, "1000000"),
             ("flush-action", 3, "2"), ("flush-end", 1, "0"), ("flush-end", 2, "1000000"),
             ("remove", 2, "guessed"), ("rescope", 2, "1000000"), ("conditional", 2, "1000000"))
    for kind, slot, value in cases:
        with test.subTest(kind=kind, slot=slot):
            rows = [row.copy() for row in model.rows]
            row = next(row for row in rows if row[0] == "K" and row[3].startswith("native-temp-event:") and row[4].split("\t")[0] == kind)
            fields = row[4].split("\t")
            fields[slot] = value
            row[4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(text_rows(rows))
    for kind in ("add", "flush-action", "scope-end", "remove"):
        with test.subTest(recounted_missing=kind):
            rows = [row.copy() for row in model.rows]
            victim = next(row for row in rows if row[0] == "K" and row[3].startswith("native-temp-event:") and row[4].split("\t")[0] == kind)
            rows.remove(victim)
            events = [row for row in rows if row[0] == "K" and row[3].startswith("native-temp-event:")]
            for ordinal, row in enumerate(events, 1):
                row[3] = "native-temp-event:" + str(ordinal)
            closure = next(row for row in rows if row[0] == "K" and row[3].startswith("native-temp-counts:"))
            fields = closure[4].split("\t")
            fields[0] = str(int(fields[0]) - 1)
            if kind == "add":
                fields[1] = str(int(fields[1]) - 1)
            closure[4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(text_rows(rows))


def check_execution(test):
    baseline = None
    for mode in ("off", "full", "bindings", "expressions"):
        executable = test.working / ("temporary-" + mode)
        test.invoke([test.fixture("temporary-lifetimes.bas")], mode=mode, emit=False, extra=("-x", str(executable)))
        result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
        test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        test.assertEqual([int(value) for value in result.stdout.split()], [10, 20, 3, 1])
        if baseline is None:
            baseline = result.stdout
        else:
            test.assertEqual(result.stdout, baseline)


def check_guards(test):
    executable = test.working / "temporary-guards"
    command = [str(test.compiler), "-prefix", str(test.toolchain_prefix), "-i", str(test.root / "src/compiler"),
               "-i", str(test.root / "inc"),
               str(test.fixture("temporary-lifetime-guards.bas")), str(test.root / "src/compiler/tooling/semantic-temporaries.bas"),
               "-x", str(executable)]
    result = subprocess.run(command, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertEqual(result.stdout.strip(), "TEMPORARY_GUARDS_PASS")


def check_nodes(test):
    for backend in test.backends:
        for fixture in ("temporary-lifetimes.bas", "lifetimes.bas"):
            with test.subTest(backend=backend, fixture=fixture):
                model = test.compile(test.fixture(fixture), backend=backend)
                test.assertEqual(model.capabilities[1][NODE_FEATURE], "available")
                state = model.temporary_lifetimes[1]
                actions = {identity: fields for identity, fields in state["events"].items() if fields[0] == "flush-action"}
                test.assertEqual(set(state["projections"]), set(actions))
                test.assertTrue(state["nodes"])
                test.assertTrue(all(actions[action][3] == "1" for action in state["nodes"].values()))
                test.assertTrue(any(state["projections"][identity] > 0 for identity in actions))


def check_node_rejection(test):
    model = test.compile(test.fixture("temporary-lifetimes.bas"))
    for mutation in ("node-action", "missing-projection", "projection-count", "recounted-node", "swapped-arms", "unavailable"):
        with test.subTest(mutation=mutation):
            rows = [row.copy() for row in model.rows]
            def packet(prefix, domain="symbol"):
                return next(row for row in rows if row[0] == "K" and row[1] == domain and row[3].startswith(prefix))
            if mutation == "node-action":
                packet("native-temp-cleanup", "node")[4] = "1000000"
            elif mutation == "missing-projection":
                rows.remove(packet("native-temp-action-nodes:"))
            elif mutation == "projection-count":
                packet("native-temp-action-nodes:")[4] = "1000000"
            elif mutation == "recounted-node":
                victim = packet("native-temp-cleanup", "node")
                rows.remove(victim)
                closure = packet("native-temp-projection-counts:")
                fields = closure[4].split("\t")
                fields[1] = str(int(fields[1]) - 1)
                closure[4] = "\t".join(fields)
            elif mutation == "swapped-arms":
                row = next(row for row in rows if row[0] == "K" and row[3].startswith("native-temp-event:")
                           and row[4].split("\t")[0] == "conditional" and all(value != "0" for value in row[4].split("\t")[2:]))
                fields = row[4].split("\t")
                fields[2], fields[3] = fields[3], fields[2]
                row[4] = "\t".join(fields)
            else:
                next(row for row in rows if row[:3] == ["CAP", "1", NODE_FEATURE])[3] = "unavailable"
            with test.assertRaises(ValueError):
                Model(text_rows(rows))

# end of temporary_lifetimes.py
