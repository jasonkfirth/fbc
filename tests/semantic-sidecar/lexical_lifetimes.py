"""Project: FreeBASIC semantic sidecar tests
File: lexical_lifetimes.py
Purpose: Prove normal lexical teardown ownership, order and exclusions.
Responsibilities: Native selections, roots, empty groups, transport and corruption.
This file intentionally does NOT infer execution of unreachable teardown paths.
"""

import subprocess

from driver_inputs import text_rows
from semantic_lifetimes import FEATURE, PREFIXES
from sidecar import Model


def check_selections(test):
    source = test.fixture("lexical-lifetimes.bas")
    choices = [(backend, ()) for backend in test.backends]
    choices += [("gas", ("-target", "linux-x86")), ("gas64", ())]
    for backend, extra in choices:
        with test.subTest(backend=backend):
            test.invoke([source], mode="off", backend=backend, extra=extra)
            baseline = test.emission_path(source.stem, backend).read_bytes()
            model = test.compile(source, backend=backend, extra=extra)
            facts = model.lexical_lifetimes[1]
            proc = test.one(model, "NormalLexicalTeardown", "procedure")
            selected = {int(row[2]) for row in facts["actions"].values()}
            root = [test.one(model, name, "variable") for name in ("root_lexical_two", "root_lexical_one")]
            inner = [test.one(model, name, "variable") for name in ("inner_lexical_two", "inner_lexical_one")]
            by_owner = {}
            for fields in facts["actions"].values():
                header = facts["groups"][int(fields[0])]
                by_owner.setdefault(int(header[0]), []).append(int(fields[2]))
            test.assertEqual([identity for identity in by_owner[proc] if identity in root], root)
            test.assertTrue(any([identity for identity in values if identity in inner] == inner for values in by_owner.values()))
            test.assertIn(test.one(model, "lexical_text", "variable"), selected)
            arrays = {int(row[1]) for row in model.records["A"]}
            array = next(identity for identity in model.named("lexical_array", "variable") if identity in arrays)
            test.assertIn(array, selected)
            for name in ("plain_lexical", "fixed_lexical", "retained_lexical", "borrowed_lexical", "borrowed_pointer"):
                test.assertTrue(model.named(name, "variable"), name)
                test.assertFalse(selected.intersection(model.named(name, "variable")), name)
            test.assertTrue(any(value == 0 for value in facts["ends"].values()))
            test.assertTrue(facts["nodes"])
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)


def check_modes(test):
    source = test.fixture("lexical-lifetimes.bas")
    for backend in test.backends:
        for mode in ("bindings", "expressions"):
            with test.subTest(backend=backend, mode=mode):
                model = test.compile(source, backend=backend, mode=mode)
                test.assertEqual(model.capabilities[1][FEATURE], "unavailable")
                test.assertFalse([row for row in model.records["K"] if row[3].startswith(PREFIXES) or row[3] == "native-lexical-cleanup"])
        for abi in ("native", "stable-v1"):
            with test.subTest(backend=backend, abi=abi):
                extra = ("-semantic-type-abi", abi)
                model = test.compile(source, backend=backend, extra=extra)
                path = test.working / (backend + "-lexical-" + abi + ".bin")
                test.invoke([source], backend=backend, mode="off", extra=(*extra, "-semantic-model", str(path)))
                test.assertEqual(Model.read(path).rows, model.rows)
    first = test.source("sub FirstLexical()\nscope\nend scope\nend sub\n", "first-lexical.bas")
    second = test.source("sub SecondLexical()\nscope\nend scope\nend sub\n", "second-lexical.bas")
    _, path = test.invoke([first, second])
    model = Model.read(path)
    test.assertEqual(set(model.lexical_lifetimes), {1, 2})
    test.assertTrue(all(next(iter(facts["groups"])) == 1 for facts in model.lexical_lifetimes.values()))


def check_rejection(test):
    model = test.compile(test.fixture("lexical-lifetimes.bas"))
    plain = test.one(model, "plain_lexical", "variable")
    cases = (("native-lexical-cleanup-group:", 0, str(plain)), ("native-lexical-cleanup-group:", 1, str(plain)),
             ("native-lexical-cleanup-group:", 2, "4294967295"), ("native-lexical-cleanup-group:", 3, "4294967295"),
             ("native-lexical-cleanup-group:", 4, "4294967295"), ("native-lexical-cleanup:", 0, "1000000"),
             ("native-lexical-cleanup:", 1, "0"), ("native-lexical-cleanup:", 2, str(plain)),
             ("native-lexical-cleanup:", 3, "2"), ("native-lexical-cleanup-end:", 0, "1000000"),
             ("native-lexical-cleanup-counts:", 0, "0"))
    for prefix, slot, value in cases:
        with test.subTest(prefix=prefix, slot=slot):
            rows = [row.copy() for row in model.rows]
            packet = next(row for row in rows if row[0] == "K" and row[3].startswith(prefix))
            fields = packet[4].split("\t")
            fields[slot] = value
            packet[4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(text_rows(rows))
    for prefix, domain, count_slot in (("native-lexical-cleanup-group:", "symbol", 0),
                                       ("native-lexical-cleanup:", "symbol", 1), ("native-lexical-cleanup", "node", 2)):
        with test.subTest(recounted=prefix, domain=domain):
            rows = [row.copy() for row in model.rows]
            packets = [row for row in rows if row[0] == "K" and row[1] == domain and row[3].startswith(prefix)]
            rows.remove(packets[-1])
            closure = next(row for row in rows if row[0] == "K" and row[3].startswith("native-lexical-cleanup-counts:"))
            fields = closure[4].split("\t")
            fields[count_slot] = str(int(fields[count_slot]) - 1)
            closure[4] = "\t".join(fields)
            with test.assertRaises(ValueError):
                Model(text_rows(rows))
    rows = [row.copy() for row in model.rows]
    next(row for row in rows if row[:3] == ["CAP", "1", FEATURE])[3] = "unavailable"
    with test.assertRaises(ValueError):
        Model(text_rows(rows))


def check_execution(test):
    baseline = None
    for mode in ("off", "full", "bindings", "expressions"):
        executable = test.working / ("lexical-" + mode)
        test.invoke([test.fixture("lexical-lifetimes.bas")], mode=mode, emit=False, extra=("-x", str(executable)))
        result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
        test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        test.assertEqual([int(value) for value in result.stdout.split()], [4, 3, 2, 1, 99])
        if baseline is None:
            baseline = result.stdout
        else:
            test.assertEqual(result.stdout, baseline)


def check_guards(test):
    executable = test.working / "lexical-guards"
    command = [str(test.compiler), "-prefix", str(test.toolchain_prefix), "-i", str(test.root / "src/compiler"),
               str(test.fixture("lexical-lifetime-guards.bas")), str(test.root / "src/compiler/tooling/semantic-lifetimes.bas"),
               "-x", str(executable)]
    result = subprocess.run(command, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertEqual(result.stdout.strip(), "LEXICAL_GUARDS_PASS")

# end of lexical_lifetimes.py
