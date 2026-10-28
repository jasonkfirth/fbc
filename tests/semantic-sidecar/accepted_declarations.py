"""Project: FreeBASIC semantic sidecar tests
File: accepted_declarations.py
Purpose: Prove order and coverage across published source declaration classes.
Responsibilities: Repetitions, macros, missing names, transport and corruptions.
This file intentionally does NOT treat acceptance order as lexical token order.
"""

import subprocess

from driver_inputs import text_rows
from semantic_accepted_declarations import FEATURE, PREFIX, CLOSURE
from sidecar import Model


def check_routes(test):
    classes, routes = set(), set()
    for backend in test.backends:
        for filename in ("declaration-repetitions.bas", "declaration-repetitions-legacy.bas",
                         "declaration_observations_probe.bas", "object-model.bas", "call-abi.bas", "accepted-declarations.bas"):
            with test.subTest(backend=backend, fixture=filename):
                source = test.fixture(filename)
                test.invoke([source], backend=backend, mode="off")
                emission = test.emission_path(source.stem, backend).read_bytes()
                model = test.compile(source, backend=backend)
                test.assertEqual(model.capabilities[1][FEATURE], "available")
                events = model.accepted_declarations[1]
                test.assertTrue(events)
                routes.update(row[0] for row in events)
                classes.update(int(row[3]) for row in events)
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
                # Every observed generated parameter still lacks a fabricated
                # source declaration. A real source route owns every receipt.
                for row in events:
                    symbol = int(row[2])
                    test.assertNotEqual(model.types[symbol][18], "compiler")
    test.assertTrue({"binding", "declaration"} <= routes, routes)
    test.assertTrue({1, 2, 3, 4, 7, 8, 9, 10, 12, 13} <= classes, classes)


def check_macros_and_unnamed(test):
    source = test.source('#define DECLARED_NAME expanded_declaration\n'
                         'dim DECLARED_NAME as long\n'
                         'declare sub UnnamedDeclaration cdecl(byval as long, ...)\n'
                         'sub DefinedDeclaration(byval source_name as long)\nend sub\n',
                         "accepted-macro-declarations.bas")
    for backend in test.backends:
        with test.subTest(backend=backend):
            model = test.compile(source, backend=backend)
            events = model.accepted_declarations[1]
            target = test.one(model, "expanded_declaration", "variable")
            test.assertTrue(any(row[0] == "macro-point" and row[2] == str(target) for row in events))
            for mutation in ("macro-identity", "macro-capability", "recounted-macro-removal"):
                with test.subTest(mutation=mutation):
                    rows = [row.copy() for row in model.rows]
                    point = next(row for row in rows if row[0] == "K" and row[3].startswith(PREFIX)
                                 and row[4].startswith("macro-point\t"))
                    if mutation == "macro-identity":
                        fields = point[4].split("\t")
                        fields[1] = "4294967295"
                        point[4] = "\t".join(fields)
                    elif mutation == "macro-capability":
                        next(row for row in rows if row[:3] == ["CAP", "1", "accepted-macro-declaration-order"])[3] = "unavailable"
                    else:
                        rows.remove(point)
                        events_left = [row for row in rows if row[0] == "K" and row[3].startswith(PREFIX)]
                        for ordinal, event in enumerate(events_left, 1):
                            event[3] = PREFIX + str(ordinal)
                        next(row for row in rows if row[0] == "K" and row[3] == CLOSURE + "1")[4] = str(len(events_left))
                    with test.assertRaises(ValueError):
                        Model(text_rows(rows))
            compact = test.compile(source, backend=backend, extra=("-semantic-model-compact",))
            test.assertEqual(compact.capabilities[1]["accepted-macro-declaration-order"], "unavailable")
            test.assertFalse(any(row[0] == "macro-point" for row in compact.accepted_declarations[1]))
            unnamed = [row for row in model.records["DCL"] if row[3] == "parameter-prototype"]
            test.assertEqual(len(unnamed), 2)
            test.assertTrue(all(row[4] == "" for row in unnamed))
            test.assertTrue(all(any(event[0:2] == ["declaration", row[1]] for event in events) for row in unnamed))


def check_modes_transport_and_reset(test):
    first = test.source("dim accepted_first as long\n", "accepted-first.bas")
    second = test.source("sub AcceptedSecond()\nend sub\n", "accepted-second.bas")
    for mode in ("bindings", "expressions"):
        model = test.compile(first, mode=mode)
        test.assertEqual(model.capabilities[1][FEATURE], "unavailable")
        test.assertFalse(model.accepted_declarations)
    text = test.compile(first)
    binary = test.working / "accepted-declarations.bin"
    test.invoke([first], mode="off", extra=("-semantic-model", str(binary)))
    test.assertEqual(Model.read(binary).rows, text.rows)
    _, path = test.invoke([first, second])
    model = Model.read(path)
    test.assertEqual(set(model.accepted_declarations), {1, 2})
    for module in (1, 2):
        test.assertTrue(model.accepted_declarations[module])
    empty = test.source("print 1\n", "accepted-empty.bas")
    model = test.compile(empty)
    test.assertFalse(model.accepted_declarations.get(1))
    test.assertEqual(next(row[4] for row in model.records["K"] if row[3] == CLOSURE + "1"), "0")


def check_rejection(test):
    model = test.compile(test.fixture("declaration-repetitions-legacy.bas"))
    for mutation in ("receipt", "symbol", "class", "counter", "statement", "source", "configuration",
                     "kind", "order", "recounted-removal", "unavailable"):
        with test.subTest(mutation=mutation):
            rows = [row.copy() for row in model.rows]
            events = [row for row in rows if row[0] == "K" and row[3].startswith(PREFIX)]
            event = next(row for row in events if row[4].startswith("declaration\t"))
            fields = event[4].split("\t")
            slots = {"receipt": 1, "symbol": 2, "class": 3, "counter": 4,
                     "statement": 5, "source": 6, "configuration": 7}
            if mutation in slots:
                fields[slots[mutation]] = "4294967295"
                event[4] = "\t".join(fields)
            elif mutation == "kind":
                fields[0] = "generated"
                event[4] = "\t".join(fields)
            elif mutation == "order":
                events[0][4], events[1][4] = events[1][4], events[0][4]
            elif mutation == "recounted-removal":
                rows.remove(events[-1])
                closure = next(row for row in rows if row[0] == "K" and row[3] == CLOSURE + "1")
                closure[4] = str(int(closure[4]) - 1)
            else:
                next(row for row in rows if row[:3] == ["CAP", "1", FEATURE])[3] = "unavailable"
            with test.assertRaises(ValueError):
                Model(text_rows(rows))


def check_guards(test):
    executable = test.working / "accepted-declaration-guards"
    command = [str(test.compiler), "-prefix", str(test.toolchain_prefix), "-i", str(test.root / "src/compiler"),
               str(test.fixture("accepted-declaration-guards.bas")),
               str(test.root / "src/compiler/tooling/semantic-declarations.bas"), "-x", str(executable)]
    result = subprocess.run(command, capture_output=True, text=True, timeout=120)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
    test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
    test.assertEqual(result.stdout.strip(), "DECLARATION_GUARDS_PASS")


def check_literal_snapshots(test):
    for filename in ("parsed-string-intrinsics.bas", "select-case-lowering.bas", "target-wide-prefixes.bas"):
        source = test.fixture(filename)
        for backend in ("gcc", "gas64"):
            with test.subTest(fixture=filename, backend=backend):
                test.invoke([source], mode="off", backend=backend)
                baseline = test.emission_path(source.stem, backend).read_bytes()
                model = test.compile(source, backend=backend)
                prefixes = [row for row in model.records["K"] if row[3] == "literal-target-wide-prefix"]
                test.assertTrue(prefixes)
                test.assertEqual(len({row[2] for row in prefixes}), len(prefixes))
                test.assertTrue(any(sum(item[1] == row[2] for item in model.records["T"]) > 1 for row in prefixes))
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)
                # Mutable symbol snapshots remain legal; repeating an immutable
                # literal prefix must still fail the independent reader.
                rows = [row.copy() for row in model.rows]
                rows.insert(-1, prefixes[0].copy())
                with test.assertRaises(ValueError):
                    Model(text_rows(rows))
    first = test.source('dim as wstring * 4 FirstWideLiteral = wchr(&h3A9)\nprint len(FirstWideLiteral)\n',
                        "first-wide-literal.bas")
    second = test.source('dim as wstring * 4 SecondWideLiteral = wchr(&h3A9)\nprint len(SecondWideLiteral)\n',
                         "second-wide-literal.bas")
    _, artifact = test.invoke([first, second])
    model = Model.read(artifact)
    prefixes = [row for row in model.records["K"] if row[3] == "literal-target-wide-prefix"]
    test.assertEqual(len(prefixes), 2)
    test.assertEqual([row[4] for row in prefixes], ["000003A9", "000003A9"])
    test.assertEqual(len({row[2] for row in prefixes}), 2)

# end of accepted_declarations.py
