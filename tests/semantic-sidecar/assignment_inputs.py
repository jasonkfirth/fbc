"""Project: FreeBASIC semantic sidecar tests
File: assignment_inputs.py
Purpose: Exercise original assignment operands and complete statement counts.
Responsibilities: Selected operations, compact modes and malformed observations.
This file intentionally does NOT implement linter storage equivalence.
"""
from sidecar import Model
from semantic_assignment_inputs import PREFIXES

FEATURE = 'source-assignment-inputs'
BODY = '''Type Slot
    value As Integer
    Declare Operator Let(ByRef other As Slot)
End Type
Operator Slot.Let(ByRef other As Slot)
    value = other.value
End Operator
Dim leftValue As Integer, rightValue As Integer = 2
Dim text As String, suffix As String
Dim item As Slot, values(0 To 8) As Integer
Dim valuePointer As Integer Ptr = @leftValue
leftValue = rightValue
leftValue += rightValue
leftValue -= rightValue
leftValue *= rightValue
leftValue /= rightValue
leftValue \\= rightValue
leftValue Mod= rightValue
leftValue And= rightValue
leftValue Or= rightValue
leftValue Xor= rightValue
leftValue Eqv= rightValue
leftValue Imp= rightValue
leftValue Shl= rightValue
leftValue Shr= rightValue
leftValue ^= rightValue
text &= suffix
text = text
item = item
values(leftValue) = values(rightValue)
*valuePointer = *valuePointer
#Macro COPY_VALUES()
    leftValue = rightValue
#EndMacro
COPY_VALUES()
If leftValue Then leftValue = rightValue Else rightValue = leftValue
#Include "assignment-input-header.bi"
#If 0
missingValue = missingOther
#EndIf
#Line 700 "assignment-input-remap.bas"
leftValue = rightValue
'''
EXPECTED = [('copy', 'assign', 'builtin')] * 2 + [
    ('compound', code, 'builtin') for code in (
        'add', 'subtract', 'multiply', 'divide', 'integer-divide', 'modulo',
        'and', 'or', 'xor', 'equivalence', 'implication', 'shift-left',
        'shift-right', 'power', 'concatenate')] + [
    ('copy', 'assign', 'builtin'), ('copy', 'assign', 'overloaded')
] + [('copy', 'assign', 'builtin')] * 7


def source_text(body, filename='assignment-inputs.bas', dialect='fb'):
    return ("' Project: FreeBASIC semantic sidecar tests\n' File: " + filename + "\n"
            "' Purpose: Exercise original accepted assignment inputs.\n"
            "' Responsibilities: Operand ownership, counts and operator origins.\n"
            "' This file intentionally does NOT run the source program.\n"
            '#Lang "' + dialect + '"\n' + body + "' end of " + filename + '\n')


def fixture(test, filename='assignment-inputs.bas'):
    test.source(source_text('rightValue = leftValue\n', 'assignment-input-header.bi'),
                'assignment-input-header.bi')
    return test.source(source_text(BODY, filename), filename)


def reviewed(model):
    return [tuple(row[4].split('\t')[2:5]) for row in model.records['K']
            if row[3].startswith('assignment-input:') and row[4].split('\t')[2] != 'initializer']


def check_inputs(test):
    for backend in test.backends:
        with test.subTest(backend=backend):
            source = fixture(test)
            output = test.working / ('assignments.c' if backend == 'gcc' else 'assignments.asm')
            extra = ('-o', str(output))
            test.invoke([source], mode='off', backend=backend, extra=extra)
            plain = output.read_bytes()
            model = test.compile(source, backend=backend, extra=extra)
            test.assertEqual(output.read_bytes(), plain)
            test.assertEqual(reviewed(model), EXPECTED)
            test.assertEqual(sum(row[3].startswith('assignment-count:') for row in model.records['K']),
                             len(model.statements))
            test.assertTrue(any(row[4].startswith('assignment-operator:') for row in model.records['MR']))
            for mode in ('bindings', 'expressions'):
                compact = test.compile(source, mode=mode, backend=backend, extra=extra)
                test.assertEqual(output.read_bytes(), plain)
                test.assertEqual(compact.capabilities[1][FEATURE], 'unavailable')
                test.assertFalse(any(row[3].startswith(PREFIXES) for row in compact.records['K']))


def check_selected_operations(test):
    body = '''Type Box
    value As Integer
    Declare Operator Let(ByRef other As Box)
End Type
Operator Box.Let(ByRef other As Box)
    value = other.value
End Operator
Operator +(ByRef first As Box, ByRef second As Box) As Box
    Dim result As Box
    result.value = first.value + second.value
    Return result
End Operator
Type Counter
    value As Integer
    Declare Operator +=(ByRef other As Counter)
End Type
Operator Counter.+=(ByRef other As Counter)
    value += other.value
End Operator
Function IndexValue() As Integer
    Return 0
End Function
Dim first As Box, second As Box
Dim receiver As Box Ptr = @first
Dim counterValue As Counter, otherCounter As Counter
first += second
receiver[IndexValue()] += second
first = second
counterValue += otherCounter
'''
    for backend in test.backends:
        with test.subTest(backend=backend):
            source = test.source(source_text(body, 'assignment-selections.bas'), 'assignment-selections.bas')
            output = test.working / ('assignment-selections.c' if backend == 'gcc' else 'assignment-selections.asm')
            extra = ('-o', str(output))
            test.invoke([source], mode='off', backend=backend, extra=extra)
            plain = output.read_bytes()
            model = test.compile(source, backend=backend, extra=extra)
            test.assertEqual(output.read_bytes(), plain)
            selected_add = {int(row[13]) for row in model.records['E']
                            if row[10:12] == ['add', 'overloaded']}
            test.assertEqual(len(selected_add), 1)
            entries = [row[4].split('\t') for row in model.records['K']
                       if row[3].startswith('assignment-input:')]
            compound = [row for row in entries if row[2] == 'compound' and row[4] == 'overloaded']
            test.assertEqual(len(compound), 3)
            test.assertEqual([int(row[5]) for row in compound[:2]], list(selected_add) * 2)
            copy = [row for row in entries if row[2:5] == ['copy', 'assign', 'overloaded']]
            test.assertEqual(len(copy), 1)
            test.assertNotIn(int(copy[0][5]), selected_add)
            test.assertNotIn(int(compound[-1][5]), selected_add)
            test.assertNotEqual(compound[-1][5], copy[0][5])


MUTATIONS = ('missing-input', 'missing-count', 'missing-zero-count', 'missing-marker',
             'missing-input-pair', 'wrong-count', 'negative-count', 'huge-count',
             'wrong-owner', 'wrong-statement', 'wrong-ordinal', 'noncanonical-key',
             'wrong-left', 'wrong-right', 'foreign-expression-owner', 'wrong-kind',
             'wrong-code', 'wrong-selection', 'builtin-target', 'missing-selected-target',
             'wrong-selected-target', 'wrong-marker-domain', 'wrong-marker-statement',
             'duplicate-input', 'duplicate-marker', 'missing-origin', 'wrong-origin-ordinal',
             'unavailable-capability')


def malformed_rows(rows, mutation):
    changed = [row[:] for row in rows]
    entry = next(row for row in changed if row[0] == 'K' and row[3].startswith('assignment-input:'))
    statement = entry[3].split(':')[1]
    count = next(row for row in changed if row[0] == 'K' and row[3] == 'assignment-count:' + statement)
    marker = next(row for row in changed if row[0] == 'H' and row[5] == 'assignment-input:1' and row[6] == statement)
    fields = entry[4].split('%09')
    if mutation in ('missing-input', 'missing-input-pair'): entry[3] = 'fixture-removed-input'
    if mutation in ('missing-marker', 'missing-input-pair'): marker[5] = 'fixture-removed-marker'
    if mutation == 'missing-count': count[3] = 'fixture-removed-count'
    if mutation == 'missing-zero-count':
        next(row for row in changed if row[0] == 'K' and row[3].startswith('assignment-count:') and row[4] == '0')[3] = 'fixture-removed-zero-count'
    if mutation in ('wrong-count', 'negative-count', 'huge-count'):
        count[4] = {'wrong-count': '0', 'negative-count': '-1', 'huge-count': '999999999999'}[mutation]
    if mutation == 'wrong-owner': entry[2] = next(row[1] for row in changed if row[0] == 'S' and row[3] != '3')
    if mutation == 'wrong-statement': entry[3] = 'assignment-input:999999:1'
    if mutation == 'wrong-ordinal': entry[3] = 'assignment-input:' + statement + ':2'
    if mutation == 'noncanonical-key': entry[3] = 'assignment-input:0' + statement + ':1'
    if mutation in ('wrong-left', 'wrong-right'): fields[mutation == 'wrong-right'] = '999999'
    if mutation == 'foreign-expression-owner':
        fields[1] = next(row[1] for row in changed if row[0] == 'E'
                         and any(own[0:3] == ['OWN', 'expression', row[1]] and own[3] != statement for own in changed))
    if mutation == 'wrong-kind': fields[2] = 'initializer'
    if mutation == 'wrong-code': fields[3] = 'add'
    if mutation == 'wrong-selection': fields[4] = 'unknown'
    if mutation == 'builtin-target': fields[5] = entry[2]
    if mutation in ('missing-selected-target', 'wrong-selected-target'):
        overloaded = next(row for row in changed if row[0] == 'K' and row[3].startswith('assignment-input:') and '%09overloaded%09' in row[4])
        selected = overloaded[4].split('%09')
        selected[5] = ('0' if mutation == 'missing-selected-target' else
                       next(row[1] for row in changed if row[0] == 'F' and row[2] != 'operator'))
        overloaded[4] = '%09'.join(selected)
    if mutation == 'wrong-marker-domain': marker[3] = 'expression'
    if mutation == 'wrong-marker-statement': marker[6] = '999999'
    if mutation == 'duplicate-input':
        next(row for row in changed if row[0] == 'K' and not row[3].startswith(PREFIXES))[:] = entry
    if mutation == 'duplicate-marker':
        next(row for row in changed if row[0] == 'H' and not row[5].startswith(('assignment-count', 'assignment-input:')))[:] = marker
    if mutation in ('missing-origin', 'wrong-origin-ordinal'):
        location = next(row for row in changed if row[0] == 'LOC' and row[1:4] == ['statement', statement, 'assignment-operator:1'])
        location[3] = 'fixture-removed-origin' if mutation == 'missing-origin' else 'assignment-operator:2'
    if mutation == 'unavailable-capability':
        next(row for row in changed if row[0] == 'CAP' and row[2] == FEATURE)[3] = 'unavailable'
    if mutation not in ('missing-input', 'missing-input-pair', 'wrong-owner', 'wrong-statement',
                        'wrong-ordinal', 'noncanonical-key'):
        entry[4] = '%09'.join(fields)
    return changed


def check_rejection(test):
    for backend in test.backends:
        _, artifact = test.invoke([fixture(test)], backend=backend)
        rows = [line.split('\t') for line in artifact.read_text().splitlines()]
        for mutation in MUTATIONS:
            with test.subTest(backend=backend, mutation=mutation):
                with test.assertRaises(ValueError):
                    Model('\n'.join('\t'.join(row) for row in malformed_rows(rows, mutation)) + '\n')


def check_module_ownership(test):
    for backend in test.backends:
        first, second = fixture(test, 'first.bas'), fixture(test, 'second.bas')
        _, artifact = test.invoke([first, second], backend=backend)
        rows = [line.split('\t') for line in artifact.read_text().splitlines()]
        boundary = next(index for index, row in enumerate(rows) if row[0] == 'M' and index > 5)
        for tag in ('K', 'H'):
            changed = [row[:] for row in rows]
            original = next(row for row in changed[:boundary] if row[0] == tag and
                            (row[3].startswith('assignment-input:') if tag == 'K' else row[5].startswith('assignment-input:')))
            transplanted = original[:]
            original[3 if tag == 'K' else 5] = 'fixture-removed-input'
            next(row for row in changed[boundary:] if row[0] == tag)[:] = transplanted
            with test.subTest(backend=backend, tag=tag), test.assertRaises(ValueError):
                Model('\n'.join('\t'.join(row) for row in changed) + '\n')

# end of assignment_inputs.py
