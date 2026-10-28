"""Project: FreeBASIC semantic sidecar tests
File: array_forward_types.py
Purpose: Validate array element types at each use across forward resolution.
Responsibilities: Nominal and alias transitions, emission parity and false-type refusal.
This file intentionally does NOT equate unrelated nominal types by their spelling.
"""

from driver_inputs import text_rows
from sidecar import Model


def check_forward_arrays(test):
    cases = [
        'type LaterValue as LaterDefinition\n'
        'type Holder\nitems(0 to 1) as LaterValue ptr\nend type\n'
        'type LaterDefinition\nvalue as long\nend type\n'
        'dim item as Holder\nprint item.items(0)\n',
        'type LaterValue as LaterDefinition\n'
        'type Holder\nitems(0 to 1) as LaterValue ptr\nend type\n'
        'dim item as Holder\nitem.items(0) = 0\n'
        'type LaterDefinition\nvalue as long\nend type\nitem.items(1) = 0\n',
        'type LaterAlias as MiddleAlias\n'
        'type Holder\nitems(0 to 1) as LaterAlias ptr\nend type\n'
        'dim item as Holder\nitem.items(0) = 0\n'
        'type MiddleAlias as FinalAlias\nitem.items(0) = 0\n'
        'type FinalAlias as long ptr\nitem.items(1) = 0\n',
    ]
    for case, text in enumerate(cases):
        source = test.source(text + 'type OtherValue\nvalue as long\nend type\n', f"forward-{case}.bas")
        for backend in test.backends:
            with test.subTest(case=case, backend=backend):
                test.invoke([source], mode="off", backend=backend)
                baseline = test.emission_path(source.stem, backend).read_bytes()
                model = test.compile(source, backend=backend)
                expressions = {int(row[1]): row for row in model.records["E"]}
                arrays = [(identity, int(properties["array-subscript-symbol"]))
                          for (domain, identity), properties in model.properties.items()
                          if domain == "expression" and "array-subscript-symbol" in properties]
                test.assertEqual(len(arrays), [1, 2, 3][case])
                # At least one use must differ from the immutable first S
                # record, otherwise the fixture would miss the original bug.
                changed = [(identity, symbol) for identity, symbol in arrays
                           if expressions[identity][12] != model.symbols[symbol][4]
                           or expressions[identity][14] != model.symbols[symbol][5]]
                test.assertTrue(changed)
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)
                if case < 2:
                    identity, symbol = changed[-1]
                    rows = [row.copy() for row in model.rows]
                    receipt = next(index for index, row in enumerate(rows)
                                   if row[:4] == ["K", "expression", str(identity), "array-subscript-symbol"])
                    snapshot = next(index for index in range(receipt - 1, -1, -1)
                                    if rows[index][:2] == ["T", str(symbol)])
                    other = str(test.one(model, "OtherValue", "type"))
                    rows[snapshot][7] = other
                    # Keep the native/normalized pair internally consistent.
                    # This control exercises the array's selected element type,
                    # independently of the lower-level NT integrity check.
                    normalized = next(index for index in range(snapshot - 1, -1, -1)
                                      if rows[index][:3] == ["NT", "symbol", str(symbol)])
                    rows[normalized][5] = other
                    with test.assertRaisesRegex(ValueError, "Array subscript selects another element type"):
                        Model(text_rows(rows))

# end of array_forward_types.py
