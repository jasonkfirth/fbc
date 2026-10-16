"""Project: FreeBASIC semantic call provenance
File: macro_call_inputs.py
Purpose: Qualify selected macro calls, compatibility and malformed observations.
Responsibilities: Backend/transport parity, modes, exact targets and refusals.
This file intentionally does NOT execute recursive or virtual subjects.
"""
from collections import Counter
import subprocess

from sidecar import Model
from parsed_expression_coverage import write_rows
from semantic_macro_calls import FEATURE, LINK, COUNT

MUTATIONS = ("missing", "duplicate", "wrong-reference", "zero", "noncanonical", "count", "closure", "domain", "unavailable")


def damaged_rows(base, mutation):
    rows = [row[:] for row in base]
    selected = next(row for row in rows if row[0] == "K" and row[3] == LINK)
    closure = next(row for row in rows if row[0] == "K" and row[3].startswith(COUNT))
    if mutation in ("missing", "closure"):
        rows.remove(selected if mutation == "missing" else closure)
    elif mutation == "duplicate":
        rows.insert(len(rows) - 1, selected[:])
    elif mutation == "wrong-reference":
        selected[4] = next(row[4] for row in rows if row[0] == "K" and row[3] == LINK and row != selected)
    elif mutation == "zero":
        selected[4] = "0"
    elif mutation == "noncanonical":
        selected[4] = "0" + selected[4]
    elif mutation == "count":
        closure[4] = str(int(closure[4]) + 1)
    elif mutation == "domain":
        selected[1] = "expression"
    elif mutation == "unavailable":
        next(row for row in rows if row[0] == "CAP" and row[2] == FEATURE)[3] = "unavailable"
    else:
        raise ValueError("Unknown macro call mutation")
    return rows


def check_inputs(test):
    source = test.fixture("macro-calls.bas")
    for backend in test.backends:
        with test.subTest(backend=backend):
            output = test.emission_path(source.stem, backend)
            extra = ("-o", str(output))
            test.invoke([source], mode="off", backend=backend, extra=extra)
            plain = output.read_bytes()
            _, path = test.invoke([source], backend=backend, extra=extra)
            model = Model.read(path)
            model.validate_physical_locations()
            test.assertEqual(model.capabilities[1][FEATURE], "available")
            test.assertEqual(output.read_bytes(), plain)
            links = [row for row in model.records["K"] if row[3] == LINK]
            test.assertEqual(len(links), 4)
            static = {int(row[2]): row[4] for row in model.relations("static-target")}
            targets = [static[int(row[2])] if model.properties["node", int(row[2])]["call-kind"] == "virtual"
                       else model.nodes[int(row[2])][9] for row in links]
            names = [model.symbols[int(target)][2].upper() for target in targets]
            test.assertEqual(Counter(names), Counter({"MACROSELF": 1, "CHOOSE": 2, "PROBE": 1}))
            test.assertEqual(len({target for target, name in zip(targets, names) if name == "CHOOSE"}), 2)
            test.assertTrue(any(row[1] == "node" and row[3] == "binding"
                                for row in model.relations("source-binding")))
            binary = test.working / "macro-calls-binary.fbcsem"
            command = [str(test.compiler), "-prefix", str(test.toolchain_prefix), "-i", str(test.root / "inc"),
                       "-gen", backend, "-maxerr", "20", "-r", "-semantic-model", str(binary),
                       "-o", str(output), str(source)]
            result = subprocess.run(command, capture_output=True, timeout=120, check=False)
            test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            transported = Model.read(binary)
            transported.validate_physical_locations()
            test.assertEqual(transported.rows, model.rows)
            test.assertEqual(output.read_bytes(), plain)
            for mode in ("bindings", "expressions", "macro-compact"):
                if mode == "macro-compact":
                    _, path = test.invoke([source], backend=backend, extra=(*extra, "-semantic-model-compact"))
                    reduced = Model.read(path)
                else:
                    reduced = test.compile(source, backend=backend, mode=mode, extra=extra)
                test.assertEqual(reduced.capabilities[1][FEATURE], "unavailable")
                test.assertFalse(any(row[3].startswith("source-call-macro-") for row in reduced.records["K"]))
                test.assertEqual(output.read_bytes(), plain)
            empty = test.compile(test.source('#lang "fb"\n', "empty-macro-calls.bas"), backend=backend)
            test.assertEqual([row[4] for row in empty.records["K"] if row[3].startswith(COUNT)], ["0"])


def check_rejection(test):
    _, path = test.invoke([test.fixture("macro-calls.bas")])
    model = Model.read(path)
    # write_rows consumes wire fields. Decoded TAB payloads would otherwise
    # damage unrelated records and conceal whether the intended check fired.
    base = [line.split('\t') for line in path.read_text(encoding='ascii').splitlines()]
    roundtrip = test.working / 'macro-call-roundtrip.fbcsem'
    write_rows(roundtrip, [row[:] for row in base])
    test.assertEqual(Model.read(roundtrip).rows, model.rows)
    for mutation in MUTATIONS:
        with test.subTest(mutation=mutation):
            rows = damaged_rows(base, mutation)
            damaged = test.working / ("macro-call-" + mutation + ".fbcsem")
            write_rows(damaged, rows)
            with test.assertRaises(ValueError):
                Model.read(damaged).validate_physical_locations()


def check_modules(test):
    first = test.fixture("macro-calls.bas")
    second = test.source('#lang "fb"\n#Define UseOther() Other()\n'
                         'Private Sub Other()\n    UseOther()\nEnd Sub\n', "other-calls.bas")
    _, path = test.invoke([first, second])
    model = Model.read(path)
    model.validate_physical_locations()
    counts = {row[3][len(COUNT):]: row[4] for row in model.records["K"] if row[3].startswith(COUNT)}
    test.assertEqual(counts, {"1": "4", "2": "1"})
    rows = [line.split('\t') for line in path.read_text(encoding='ascii').splitlines()]
    closure = next(row for row in rows if row[0] == "K" and row[3] == COUNT + "2")
    closure[3] = COUNT + "1"
    damaged = test.working / "foreign-macro-closure.fbcsem"
    write_rows(damaged, rows)
    with test.assertRaises(ValueError):
        Model.read(damaged).validate_physical_locations()


def check_runtime_inputs(test):
    source = test.fixture("runtime-macro-calls.bas")
    for backend in test.backends:
        with test.subTest(backend=backend):
            output = test.emission_path(source.stem, backend)
            extra = ("-eassert", "-o", str(output))
            test.invoke([source], mode="off", backend=backend, extra=extra)
            plain = output.read_bytes()
            _, path = test.invoke([source], backend=backend, extra=extra)
            model = Model.read(path)
            model.validate_physical_locations()
            links = [row for row in model.records["K"] if row[3] == LINK]
            test.assertEqual(len(links), 3)
            for row in links:
                node = int(row[2])
                test.assertEqual(model.properties["node", node]["call-kind"], "runtime")
                test.assertEqual(model.types[int(model.nodes[node][9])][18], "runtime")
            test.assertEqual(output.read_bytes(), plain)
            rows = [line.split('\t') for line in path.read_text(encoding='ascii').splitlines()]
            selected = links[0][2]
            target = model.nodes[int(selected)][9]
            for mutation in ("indirect", "source-target", "missing-reference"):
                damaged = [row[:] for row in rows]
                if mutation == "indirect":
                    next(row for row in damaged if row[:4] == ["K", "node", selected, "call-kind"])[4] = "indirect"
                elif mutation == "source-target":
                    for row in damaged:
                        if row[:2] == ["T", target]:
                            row[18] = "source"
                else:
                    reference = links[0][4]
                    damaged = [row for row in damaged if not
                               (row[0] == "MR" and row[1] == "symbol" and row[4] == "reference-" + reference)]
                destination = test.working / ("runtime-call-" + mutation + ".fbcsem")
                write_rows(destination, damaged)
                with test.assertRaises(ValueError):
                    Model.read(destination).validate_physical_locations()


# end of macro_call_inputs.py
