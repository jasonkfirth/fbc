"""Project: FreeBASIC semantic sidecar tests
File: let_destinations.py
Purpose: Verify original LET destinations and declaration-order field slots.
Responsibilities: Omitted slots, overloads, metadata closure and unchanged code.
This file intentionally does NOT parse destination lists from source text.
"""
from sidecar import Model, unescape
from assignment_inputs import source_text

FEATURE = 'let-destination-inputs'
BODY = '''Type Triple
    firstValue As Long
    secondValue As Long
    thirdValue As Long
End Type
Type Receiver
    stored As Long
    Declare Operator Let(ByVal value As Long)
End Type
Operator Receiver.Let(ByVal value As Long)
    stored = value
End Operator
Sub Destinations()
    Dim source As Triple = (1, 2, 3)
    Dim value As Long, values(0 To 2) As Long
    Dim received As Receiver
    Let(value, , value) = source
    Let(values(0), values(0)) = source
    Let(received, received) = source
End Sub
'''
MUTATIONS = ('missing-slot', 'missing-marker', 'noncanonical-key', 'noncanonical-slot',
             'zero-slot', 'oversized-slot', 'missing-field', 'wrong-field-class',
             'wrong-original-field', 'repeated-slot', 'wrong-input-kind',
             'unavailable-capability', 'wrong-marker-statement', 'wrong-marker-ordinal',
             'orphan-slot', 'wrong-statement-route')


def malformed_rows(rows, mutation):
    changed = [row[:] for row in rows]
    slots = [row for row in changed if row[0] == 'K' and row[3].startswith('let-slot:')]
    slot = slots[0]
    _, statement, ordinal = slot[3].split(':')
    marker = next(row for row in changed if row[0] == 'H' and row[5] == 'let-slot:' + ordinal and row[6] == statement)
    assignment = next(row for row in changed if row[0] == 'K' and row[3] == 'assignment-input:' + statement + ':' + ordinal)

    def replace(row, field, value):
        parts = unescape(row[4]).split('\t')
        parts[field] = value
        row[4] = '\t'.join(parts).replace('%', '%25').replace('\t', '%09')

    if mutation == 'missing-slot': slot[3] = 'fixture-removed-slot'
    if mutation == 'missing-marker': marker[5] = 'fixture-removed-marker'
    if mutation == 'noncanonical-key': slot[3] = 'let-slot:0' + statement + ':' + ordinal
    if mutation == 'noncanonical-slot': replace(slot, 0, '01')
    if mutation == 'zero-slot': replace(slot, 0, '0')
    if mutation == 'oversized-slot': replace(slot, 0, '65537')
    if mutation == 'missing-field': replace(slot, 1, '999999')
    if mutation == 'wrong-field-class': replace(slot, 1, slot[2])
    if mutation == 'wrong-original-field': replace(slot, 1, unescape(slots[1][4]).split('\t')[1])
    if mutation == 'repeated-slot': replace(slots[1], 0, '1')
    if mutation == 'wrong-input-kind': replace(assignment, 2, 'copy')
    if mutation == 'unavailable-capability': next(row for row in changed if row[0] == 'CAP' and row[2] == FEATURE)[3] = 'unavailable'
    if mutation == 'wrong-marker-statement': marker[6] = '999999'
    if mutation == 'wrong-marker-ordinal': marker[5] = 'let-slot:999999'
    if mutation == 'orphan-slot': slot[3] = 'let-slot:' + statement + ':999999'
    if mutation == 'wrong-statement-route': next(row for row in changed if row[0] == 'STE' and row[1] == statement)[2] = 'declaration'
    return changed


def check_destinations(test):
    for backend in test.backends:
        with test.subTest(backend=backend):
            source = test.source(source_text(BODY, 'let-destinations.bas'), 'let-destinations.bas')
            output = test.emission_path('let', backend)
            extra = ('-o', str(output))
            test.invoke([source], mode='off', backend=backend, extra=extra)
            plain = output.read_bytes()
            _, artifact = test.invoke([source], backend=backend, extra=extra)
            model = Model.read(artifact)
            test.assertEqual(output.read_bytes(), plain)
            test.assertEqual(model.capabilities[1][FEATURE], 'available')
            slots = [row[4].split('\t')[0] for row in model.records['K'] if row[3].startswith('let-slot:')]
            test.assertEqual(slots, ['1', '3', '1', '2', '1', '2'])
            inputs = [row[4].split('\t') for row in model.records['K']
                      if row[3].startswith('assignment-input:') and row[4].split('\t')[2] == 'let']
            test.assertEqual([fields[4] for fields in inputs], ['builtin'] * 4 + ['overloaded'] * 2)
            rows = [line.split('\t') for line in artifact.read_text().splitlines()]
            for mutation in MUTATIONS:
                with test.subTest(mutation=mutation), test.assertRaises(ValueError):
                    Model('\n'.join('\t'.join(row) for row in malformed_rows(rows, mutation)) + '\n')
            for mode in ('bindings', 'expressions'):
                compact = test.compile(source, mode=mode, backend=backend, extra=extra)
                test.assertEqual(output.read_bytes(), plain)
                test.assertEqual(compact.capabilities[1][FEATURE], 'unavailable')
                test.assertFalse(any(row[3].startswith('let-slot:') for row in compact.records['K']))
# end of let_destinations.py
