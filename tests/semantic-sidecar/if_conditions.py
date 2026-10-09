"""Project: FreeBASIC semantic sidecar tests
File: if_conditions.py
Purpose: Exercise original IF and ELSEIF inputs before branch lowering.
Responsibilities: Folded inputs, complete grammar and malformed receipt cases.
This file intentionally does NOT derive expected roles from source spans.
"""
from sidecar import Model

FEATURE = 'if-condition-inputs'
PREFIX = 'if-condition-expression:'
BODY = '''Enum State
    Ready = 3
    Busy = 6
End Enum
Dim stateValue As State = Ready
Dim flag As Boolean
Dim total As Integer
If stateValue Then
    total += 1
ElseIf (flag) Then
    total += 2
ElseIf 1 Then
    total += 3
Else
    total += 4
End If
If flag Then total += 1 Else total += 2
If flag Then If stateValue Then total += 1
If False Then
    total += 1
ElseIf True Then
    total += 2
End If
If (flag AndAlso stateValue) Then total += 1
If stateValue Goto acceptedLabel
total += 1
acceptedLabel:
Sub Choose(ByRef predicate As Boolean, ByVal stateArgument As State)
    If predicate Then
        Exit Sub
    ElseIf stateArgument Then
        Exit Sub
    End If
End Sub
Type TruthValue
    Value As Integer
    Declare Operator Cast() As Integer
End Type
Operator TruthValue.Cast() As Integer
    Return Value
End Operator
Dim truth As TruthValue
If truth Then total += 1
#Macro CHOOSE_TWICE()
    If stateValue Then If flag Then total += 1
#EndMacro
CHOOSE_TWICE()
#Define CHOOSE_HEADER If
#Define NEXT_HEADER ElseIf
CHOOSE_HEADER (flag) Then
    total += 1
NEXT_HEADER stateValue Then
    total += 2
End If
If flag _
    AndAlso stateValue Then total += 1
#Line 800 "original-if.bas"
If flag Then total += 1
#Define REMAPPED_CONDITION Not flag
If REMAPPED_CONDITION Then total += 1
#Include "if-conditions.bi"
#If 0
If stateValue Then total += 100
#EndIf
'''
INCLUDED = 'If flag Then total += 1\n'
EXPECTED_COUNT = 21
EXPECTED_ELSEIF_COUNT = 5


def source_text(body, filename='if-conditions.bas', dialect='fb'):
    return ("' Project: FreeBASIC semantic sidecar tests\n' File: " + filename + "\n"
            "' Purpose: Exercise original compiler-owned IF conditions.\n"
            "' Responsibilities: Native grammar roles and folded predicates.\n"
            "' This file intentionally does NOT validate linter truth advice.\n"
            '#Lang "' + dialect + '"\n' + body + "' end of " + filename + '\n')


def fixture(test):
    test.source(source_text(INCLUDED, 'if-conditions.bi'), 'if-conditions.bi')
    return test.source(source_text(BODY))


def check_inputs(test):
    for backend in test.backends:
        with test.subTest(backend=backend):
            source = fixture(test)
            emitted = test.emission_path('if-conditions', backend)
            extra = ('-o', str(emitted))
            test.invoke([source], mode='off', backend=backend, extra=extra)
            plain = emitted.read_bytes()
            model = test.compile(source, backend=backend, extra=extra)
            test.assertEqual(emitted.read_bytes(), plain)
            receipts = [(owner, key, value) for (domain, owner), properties in model.properties.items()
                        if domain == 'symbol' for key, value in properties.items() if key.startswith(PREFIX)]
            test.assertEqual(len(receipts), EXPECTED_COUNT)
            statements = [model.statements[int(key[len(PREFIX):])] for _, key, _ in receipts]
            test.assertEqual(sum(start[9] == '269' for start in statements), EXPECTED_ELSEIF_COUNT)
            test.assertEqual(sum(('expression', int(value)) in model.constants for _, _, value in receipts), 3)
            for mode in ('bindings', 'expressions'):
                compact = test.compile(source, mode=mode, backend=backend, extra=extra)
                test.assertEqual(emitted.read_bytes(), plain)
                test.assertTrue(all(features[FEATURE] == 'unavailable' for features in compact.capabilities.values()))
                test.assertFalse(any(key.startswith(PREFIX) for properties in compact.properties.values() for key in properties))


def check_legacy(test):
    for dialect in ('deprecated', 'fblite', 'qb'):
        for backend in test.backends:
            with test.subTest(dialect=dialect, backend=backend):
                body = ('Dim flag As Integer\nIf flag Then flag = 1 Else flag = 2\nIf flag Then\n'
                        'flag = 3\nElseIf 1 Then\nflag = 4\nEnd If\nIf flag Goto 100\n100:\n')
                model = test.compile(test.source(source_text(body, dialect=dialect)), backend=backend)
                test.assertEqual(sum(key.startswith(PREFIX) for properties in model.properties.values() for key in properties), 4)


MUTATIONS = ('missing-input', 'missing-marker', 'missing-both', 'unavailable', 'partial',
             'wrong-domain', 'unknown-property', 'zero-input', 'negative-input', 'oversized-input',
             'missing-expression', 'foreign-statement-input', 'wrong-statement-token',
             'wrong-owner', 'wrong-marker-owner', 'wrong-marker-input', 'wrong-marker-statement',
             'wrong-marker-domain', 'duplicate-input', 'duplicate-marker')


def malformed_rows(rows, mutation):
    changed = [row[:] for row in rows]
    receipt = next(row for row in changed if row[0] == 'K' and row[3].startswith(PREFIX))
    statement = receipt[3][len(PREFIX):]
    marker = next(row for row in changed if row[0] == 'H' and row[5] == 'if-condition' and row[6] == statement)
    if mutation in ('missing-input', 'missing-both'): receipt[3] = 'fixture-unknown-if-input'
    if mutation in ('missing-marker', 'missing-both'): marker[5] = 'fixture-unknown-if-marker'
    if mutation in ('missing-input', 'missing-marker', 'missing-both'): return changed
    if mutation in ('unavailable', 'partial'):
        next(row for row in changed if row[0] == 'CAP' and row[2] == FEATURE)[3] = mutation
    elif mutation == 'wrong-domain': receipt[1:3] = ['expression', receipt[4]]
    elif mutation == 'unknown-property': receipt[3] = 'if-condition-unknown:' + statement
    elif mutation == 'zero-input': receipt[4] = '0'
    elif mutation == 'negative-input': receipt[4] = '-1'
    elif mutation == 'oversized-input': receipt[4] = '999999999999999999999'
    elif mutation == 'missing-expression': receipt[4] = str(1 + max(int(row[1]) for row in changed if row[0] == 'E'))
    elif mutation == 'foreign-statement-input':
        receipt[4] = next(row[2] for row in changed if row[0] == 'OWN' and row[1] == 'expression' and row[3] != statement)
        marker[4] = receipt[4]
    elif mutation == 'wrong-statement-token':
        target = next(row[1] for row in changed if row[0] == 'ST' and row[9] not in ('266', '269'))
        receipt[3] = PREFIX + target
        marker[6] = target
    elif mutation in ('wrong-owner', 'wrong-marker-owner'):
        target = next(row[1] for row in changed if row[0] == 'S' and row[3] != '3')
        (receipt if mutation == 'wrong-owner' else marker)[2] = target
    elif mutation == 'wrong-marker-input': marker[4] = next(row[1] for row in changed if row[0] == 'E' and row[1] != receipt[4])
    elif mutation == 'wrong-marker-statement': marker[6] = next(row[1] for row in changed if row[0] == 'ST' and row[1] != statement)
    elif mutation == 'wrong-marker-domain': marker[1:3] = ['expression', receipt[4]]
    elif mutation == 'duplicate-input': next(row for row in changed if row[0] == 'K' and not row[3].startswith(PREFIX))[:] = receipt
    elif mutation == 'duplicate-marker': next(row for row in changed if row[0] == 'H' and row[5] != 'if-condition')[:] = marker
    else: raise AssertionError(mutation)
    return changed


def check_rejection(test):
    for backend in test.backends:
        _, artifact = test.invoke([fixture(test)], backend=backend)
        rows = [line.split('\t') for line in artifact.read_text().splitlines()]
        for mutation in MUTATIONS:
            with test.subTest(backend=backend, mutation=mutation):
                changed = malformed_rows(rows, mutation)
                malformed = test.working / 'if-malformed.sem'
                malformed.write_text('\n'.join('\t'.join(row) for row in changed) + '\n')
                with test.assertRaises(ValueError):
                    Model.read(malformed)

# end of if_conditions.py
