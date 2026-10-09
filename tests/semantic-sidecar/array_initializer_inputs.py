"""Project: FreeBASIC semantic sidecar tests
File: array_initializer_inputs.py
Purpose: Verify original array elements before assignment or constructor lowering.
Responsibilities: Array ownership, input grouping and unchanged backend output.
This file intentionally does NOT infer initializer roles from source delimiters.
"""
from sidecar import Model, source_range

BODY = '''Type Item
    text As String
    Declare Constructor()
    Declare Constructor(ByRef value As Const String)
End Type
Constructor Item()
End Constructor
Constructor Item(ByRef value As Const String)
    text = value
End Constructor
Type Holder
    texts(0 To 1) As ZString * 16
    text As ZString * 16
End Type
Dim plain(0 To 1) As String = {"left" "right", "tail"} ' plain
Dim grouped(0 To 1) As String = {("left" "right"), "tail"} ' grouped
Dim numbers(0 To 1) As Byte = {300, 2} ' numbers
Dim grid(0 To 0, 0 To 1) As String = {{"left" "right", "tail"}} ' grid
Dim inferred(0 To ...) As String = {"left" "right", "tail"} ' inferred
Dim items(0 To 1) As Item = {"left" "right", "tail"} ' items
Dim record As Holder = ({"left" "right", "tail"}, "scalar" "join") ' field
Sub local_arrays()
    Static local_values(0 To 1) As ZString * 16 = {"left" "right", "tail"} ' local
End Sub
#Define ARRAY_WORD "macro" "word"
Dim macro_values(0 To 1) As String = {ARRAY_WORD, "tail"} ' macro
Dim scalar As String = "left" "right"
Dim scalar_object As Item = "left" "right"
Const joined = "left" "right"
Sub optional_scalar(ByRef value As Const String = "left" "right")
End Sub
'''
EXPECTED = {
    'plain': ('PLAIN', 1, 'assignment', 4),
    'grouped': ('GROUPED', 1, 'assignment', 4),
    'numbers': ('NUMBERS', 1, 'assignment', 8),
    'grid': ('GRID', 2, 'assignment', 4),
    'inferred': ('INFERRED', 1, 'assignment', 4),
    'items': ('ITEMS', 1, 'constructor', 4),
    'field': ('TEXTS', 1, 'assignment', 4),
    'local': ('LOCAL_VALUES', 1, 'assignment', 4),
    'macro': ('MACRO_VALUES', 1, 'assignment', 4),
}


def fixture(test):
    filename = 'array-initializer-inputs.bas'
    text = ("' Project: FreeBASIC semantic sidecar tests\n' File: " + filename + '\n'
            "' Purpose: Exercise original initializer element identities.\n"
            "' Responsibilities: Arrays, constructors and scalar controls.\n"
            "' This file intentionally does NOT execute narrowed values.\n"
            '#Lang "fb"\n' + BODY + "' end of " + filename + '\n')
    return test.source(text, filename)


def check_inputs(test):
    source = fixture(test)
    text = source.read_text()
    labels = {number: line.rsplit("' ", 1)[1] for number, line in enumerate(text.splitlines(), 1)
              if "' " in line and not line.startswith("'")}
    for backend in test.backends:
        with test.subTest(backend=backend):
            emitted = test.emission_path('initializer', backend)
            extra = ('-o', str(emitted))
            plain_result, _ = test.invoke([source], mode='off', backend=backend, extra=extra)
            plain = emitted.read_bytes()
            full_result, path = test.invoke([source], backend=backend, extra=extra)
            model = Model.read(path)
            test.assertEqual(emitted.read_bytes(), plain)
            test.assertEqual(full_result.stdout + full_result.stderr,
                             plain_result.stdout + plain_result.stderr)
            test.assertTrue(all(features['original-array-initializer-inputs'] == 'available'
                                for features in model.capabilities.values()))
            expressions = {int(row[1]): row for row in model.records['E']}
            receipts = [(identity, properties) for (domain, identity), properties in model.properties.items()
                        if domain == 'expression' and 'array-initializer-symbol' in properties]
            test.assertEqual(len(receipts), 2 * len(EXPECTED))
            counts = {label: 0 for label in EXPECTED}
            grouped_roots = {int(row[1]) for row in model.records['EX'] if row[2] == 'group'}
            observed_groups = 0
            nonphysical = 0
            for identity, properties in receipts:
                expression = expressions[identity]
                label = labels[source_range(expression, 3)[1]]
                target = model.symbols[int(properties['array-initializer-symbol'])]
                observed = (target[2], int(properties['array-initializer-dimension']),
                            properties['array-initializer-kind'], int(expression[12]) & 511)
                test.assertEqual(observed, EXPECTED[label])
                counts[label] += 1
                if label == 'grouped' and identity in grouped_roots:
                    observed_groups += 1
                if expression[2] == '0':
                    nonphysical += 1
            test.assertEqual(counts, {label: 2 for label in EXPECTED})
            test.assertEqual(observed_groups, 1)
            test.assertEqual(nonphysical, 1)
            for mode in ('bindings', 'expressions'):
                compact = test.compile(source, mode=mode, backend=backend, extra=extra)
                test.assertEqual(emitted.read_bytes(), plain)
                test.assertFalse(any('array-initializer-symbol' in properties
                                     for properties in compact.properties.values()))
                test.assertTrue(all(features['original-array-initializer-inputs'] == 'unavailable'
                                    for features in compact.capabilities.values()))


MUTATIONS = ('missing-kind', 'unknown-property', 'wrong-domain', 'wrong-kind',
             'zero-dimension', 'wrong-dimension', 'negative-dimension',
             'oversized-dimension', 'zero-target', 'unknown-target',
             'scalar-target', 'procedure-target', 'unavailable', 'partial',
             'missing-capability')


def malformed_rows(rows, mutation):
    changed = [row[:] for row in rows]
    target = next(row for row in changed if row[0] == 'K' and row[3] == 'array-initializer-symbol')
    dimension = next(row for row in changed if row[:4] == ['K', 'expression', target[2], 'array-initializer-dimension'])
    kind = next(row for row in changed if row[:4] == ['K', 'expression', target[2], 'array-initializer-kind'])
    if mutation == 'missing-kind':
        kind[3] = 'fixture-missing-array-kind'
    elif mutation == 'unknown-property':
        kind[3] = 'array-initializer-unknown'
    elif mutation == 'wrong-domain':
        target[1:3] = ['symbol', target[4]]
    elif mutation == 'wrong-kind':
        kind[4] = 'default'
    elif mutation in ('zero-dimension', 'wrong-dimension', 'negative-dimension', 'oversized-dimension'):
        dimension[4] = {'zero-dimension': '0', 'wrong-dimension': '2',
                        'negative-dimension': '-1', 'oversized-dimension': '999999999999999999999'}[mutation]
    elif mutation == 'zero-target':
        target[4] = '0'
    elif mutation == 'unknown-target':
        target[4] = str(1 + max(int(row[1]) for row in changed if row[0] == 'S'))
    elif mutation == 'scalar-target':
        target[4] = next(row[1] for row in changed if row[0] == 'S' and row[2] == 'SCALAR')
    elif mutation == 'procedure-target':
        target[4] = next(row[1] for row in changed if row[0] == 'S' and row[3] == '3')
    elif mutation in ('unavailable', 'partial', 'missing-capability'):
        capability = next(row for row in changed if row[0] == 'CAP' and row[2] == 'original-array-initializer-inputs')
        if mutation == 'missing-capability':
            capability[2] = 'fixture-missing-array-capability'
        else:
            capability[3] = mutation
    else:
        raise AssertionError(mutation)
    return changed


def check_rejection(test):
    for backend in test.backends:
        _, artifact = test.invoke([fixture(test)], backend=backend)
        rows = [line.split('\t') for line in artifact.read_text().splitlines()]
        for mutation in MUTATIONS:
            with test.subTest(backend=backend, mutation=mutation):
                changed = malformed_rows(rows, mutation)
                malformed = test.working / 'array-initializer-malformed.sem'
                malformed.write_text('\n'.join('\t'.join(row) for row in changed) + '\n')
                with test.assertRaises(ValueError):
                    Model.read(malformed)

# end of array_initializer_inputs.py
