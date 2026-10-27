"""Project: FreeBASIC semantic array mutation inputs
File: array_mutation_inputs.py
Purpose: Qualify direct selected targets and independent module closure.
Responsibilities: Native identity, emission parity, transport and reduced captures.
This file intentionally does NOT infer operations from lowered runtime names.
"""
from collections import Counter

from sidecar import Model

FEATURE = "native-array-variable-mutation-inputs"
PREFIX = "native-array-mutation:"
COUNT = "native-array-mutation-count:"


def operations(model):
    return [row for row in model.records["K"] if row[3].startswith(PREFIX)]


def check_inputs(test):
    source = test.fixture("array-mutation-inputs.bas")
    for backend in test.backends:
        with test.subTest(backend=backend):
            test.invoke([source], mode="off", backend=backend)
            emission = test.emission_path(source.stem, backend).read_bytes()
            _, path = test.invoke([source], backend=backend, extra=("-semantic-type-abi", "native"))
            model = Model.read(path)
            model.validate_physical_locations()
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
            test.assertEqual(model.capabilities[1][FEATURE], "available")
            inputs = operations(model)
            test.assertEqual(len(inputs), 5)
            observed = Counter()
            for row in inputs:
                kind, statement, context, place, macro = row[4].split("\t")
                test.assertIn(int(statement), model.statements)
                test.assertIn(int(context), model.source_contexts)
                test.assertIn(place, ("variable", "expanded"))
                observed[model.symbols[int(row[2])][2].upper(), kind] += 1
                test.assertTrue(any(link[1:6] == ["symbol", row[2], "symbol", row[2], row[3]]
                                    for link in model.records["H"]))
            test.assertEqual(observed, Counter({("FIXEDITEMS", "erase"): 1,
                                                ("DYNAMICITEMS", "redim"): 1,
                                                ("DYNAMICITEMS", "redim-preserve"): 1,
                                                ("DYNAMICITEMS", "erase"): 1,
                                                ("MACROITEMS", "erase"): 1}))
            test.assertEqual([row[4] for row in model.records["K"] if row[3].startswith(COUNT)], ["5"])
            binary = test.working / ("array-mutation-" + backend + ".fbcsem")
            # Keep the selected object name identical across transports now
            # that the model also records the driver's effective inputs.
            test.invoke([source], mode="off", backend=backend,
                        extra=("-semantic-type-abi", "native", "-semantic-model", str(binary)))
            test.assertEqual(Model.read(binary).rows, model.rows)
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
            for mode, extra in (("bindings", ()), ("expressions", ()),
                                ("full", ("-semantic-model-compact",))):
                _, reduced_path = test.invoke([source], backend=backend, mode=mode,
                                              extra=("-semantic-type-abi", "native", *extra))
                rows = [line.split("\t") for line in reduced_path.read_text(encoding="ascii").splitlines()]
                test.assertEqual([row[3] for row in rows if row[:3] == ["CAP", "1", FEATURE]], ["unavailable"])
                test.assertFalse(any(row[0] == "K" and row[3].startswith((PREFIX, COUNT)) for row in rows))
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)
            empty = test.compile(test.source('#Lang "fb"\n', "empty-array-mutations.bas"), backend=backend,
                                 extra=("-semantic-type-abi", "native"))
            test.assertEqual([row[4] for row in empty.records["K"] if row[3].startswith(COUNT)], ["0"])
            second = test.source('Dim Values(0 To 1) As Long\nErase Values\n', "second-array-mutations.bas")
            _, path = test.invoke([source, second], backend=backend, extra=("-semantic-type-abi", "native"))
            combined = Model.read(path)
            test.assertEqual([row[4] for row in combined.records["K"] if row[3].startswith(COUNT)], ["5", "1"])

# end of array_mutation_inputs.py
