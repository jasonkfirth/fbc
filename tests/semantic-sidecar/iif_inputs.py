"""Project: FreeBASIC semantic sidecar tests
File: iif_inputs.py
Purpose: Exercise original IIf operands before folding and branch lowering.
Responsibilities: Complete roles, macros, discarded arms and damaged receipts.
This file intentionally does NOT infer expected operands from source spans.
"""
from sidecar import Model

FEATURE = 'original-iif-inputs'
BODY = '''Dim flag As Boolean
Dim booleanResult As Boolean = IIf(flag, True, False)
Dim integerResult As Integer = IIf(flag, 1, 0)
Dim foldedResult As Integer = IIf(True, 123, 456)
Dim nestedResult As Integer = IIf(flag, IIf(flag, 11, 12), IIf(False, 21, 22))
Dim nestedCondition As Integer = IIf(IIf(flag, True, False), 1, 0)
Dim convertedResult As Integer = IIf(flag, CInt((IIf(flag, 1, 0))), 0)
Dim queryResult As Integer = SizeOf(IIf(flag, 1ll, 0ll))
Dim stringResult As String = IIf(flag, "yes", "no")
Type Box
    Value As Integer
End Type
Dim recordResult As Box = IIf(flag, Type<Box>(1), Type<Box>(2))
Type TruthValue
    Value As Integer
    Declare Operator Cast() As Integer
End Type
Operator TruthValue.Cast() As Integer
    Return Value
End Operator
Dim truth As TruthValue
Dim overloadedCondition As Boolean = IIf(truth, True, False)
#Macro CONDITIONAL_VALUE()
    IIf(flag, 3, 4)
#EndMacro
Dim macroResult As Integer = CONDITIONAL_VALUE()
#Include "iif-inputs.bi"
#If 0
Dim inactiveResult As Integer = IIf(True, 1, 0)
#EndIf
'''
INCLUDED = 'Dim includedResult As Integer = IIf(flag, -5, 8)\n'
EXPECTED_COUNT = 16


def source_text(body, filename='iif-inputs.bas', dialect='fb'):
    return ("' Project: FreeBASIC semantic sidecar tests\n' File: " + filename + "\n"
            "' Purpose: Exercise original compiler-owned conditional operands.\n"
            "' Responsibilities: Native IIf roles and folded-result identities.\n"
            "' This file intentionally does NOT execute the conditional arms.\n"
            '#Lang "' + dialect + '"\n' + body + "' end of " + filename + '\n')


def fixture(test):
    test.source(source_text(INCLUDED, 'iif-inputs.bi'), 'iif-inputs.bi')
    return test.source(source_text(BODY))


def check_inputs(test):
    for backend in test.backends:
        with test.subTest(backend=backend):
            source = fixture(test)
            emitted = test.emission_path('iif-inputs', backend)
            extra = ('-o', str(emitted))
            test.invoke([source], mode='off', backend=backend, extra=extra)
            plain = emitted.read_bytes()
            model = test.compile(source, backend=backend, extra=extra)
            test.assertEqual(emitted.read_bytes(), plain)
            properties = [(identity, values[FEATURE].split('\t')) for (domain, identity), values in model.properties.items()
                          if domain == 'expression' and FEATURE in values]
            test.assertEqual(len(properties), EXPECTED_COUNT)
            expressions = {int(row[1]): row for row in model.records['E']}
            test.assertTrue(any(expressions[identity][8] == '16' for identity, _ in properties))
            test.assertTrue(any(expressions[identity][8] == '26' for identity, _ in properties))
            for identity, arguments in properties:
                test.assertEqual(len(arguments), 3)
                condition, true_arm, false_arm = map(int, arguments)
                test.assertLess(condition, true_arm)
                test.assertLess(true_arm, false_arm)
                test.assertLess(false_arm, identity)
            # These literal arms have different values. Their original C
            # receipts must survive even though the condition folds to true.
            folded = [(identity, arguments) for identity, arguments in properties
                      if model.constants.get(('expression', int(arguments[1])), ('', ''))[1] == '123']
            test.assertEqual(len(folded), 1)
            identity, arguments = folded[0]
            test.assertEqual(model.constants['expression', int(arguments[2])][1], '456')
            test.assertEqual(model.constants['expression', identity][1], '123')
            for mode in ('bindings', 'expressions'):
                compact = test.compile(source, mode=mode, backend=backend, extra=extra)
                test.assertEqual(emitted.read_bytes(), plain)
                test.assertEqual(compact.capabilities[1][FEATURE], 'unavailable')
                test.assertFalse(any(FEATURE in values for values in compact.properties.values()))


def check_legacy(test):
    for dialect in ('deprecated', 'fblite', 'qb'):
        for backend in test.backends:
            with test.subTest(dialect=dialect, backend=backend):
                body = ('Dim flag As Integer\nDim result As Integer = IIf(flag, IIf(-1, 3, 4), 0)\n'
                        if dialect != 'qb' else 'flag% = 0\nresult% = __IIf(flag%, __IIf(-1, 3, 4), 0)\n')
                source = test.source(source_text(body, dialect=dialect))
                model = test.compile(source, backend=backend)
                test.assertEqual(sum(FEATURE in values for values in model.properties.values()), 2)
                if dialect != 'qb':
                    ordinary = ('Option NoKeyword IIf\n'
                                'Function IIf(ByVal flag As Integer, ByVal first As Integer, '
                                'ByVal second As Integer) As Integer\n'
                                '    Return first + second\nEnd Function\n'
                                'Dim result As Integer = IIf(1, 2, 3)\n')
                    model = test.compile(test.source(source_text(ordinary, dialect=dialect)), backend=backend)
                    test.assertFalse(any(FEATURE in values for values in model.properties.values()))


MUTATIONS = ('missing-input', 'missing-marker', 'unavailable', 'wrong-domain', 'short-payload',
             'long-payload', 'zero-condition', 'negative-condition', 'oversized-condition',
             'swapped-arms', 'repeated-arm', 'result-as-arm', 'foreign-statement', 'wrong-marker-owner',
             'duplicate-input', 'duplicate-marker')


def malformed_rows(rows, mutation):
    changed = [row[:] for row in rows]
    receipt = next(row for row in changed if row[0] == 'K' and row[3] == FEATURE)
    marker = next(row for row in changed if row[0] == 'H' and row[5] == 'parsed-iif' and row[4] == receipt[2])
    if mutation == 'missing-input': receipt[3] = 'fixture-unknown-iif-input'
    elif mutation == 'missing-marker': marker[5] = 'fixture-unknown-iif-marker'
    elif mutation == 'unavailable': next(row for row in changed if row[0] == 'CAP' and row[2] == FEATURE)[3] = 'unavailable'
    elif mutation == 'wrong-domain': receipt[1], receipt[2] = 'symbol', marker[2]
    elif mutation == 'wrong-marker-owner': marker[2] = next(row[1] for row in changed if row[0] == 'S' and row[3] != '8')
    elif mutation == 'duplicate-input':
        duplicate = next(row for row in changed if row[0] == 'K' and row[3] != FEATURE)
        duplicate[:] = receipt
    elif mutation == 'duplicate-marker':
        duplicate = next(row for row in changed if row[0] == 'H' and row[5] != 'parsed-iif')
        duplicate[:] = marker
    else:
        fields = receipt[4].split('%09')
        if mutation == 'short-payload': fields.pop()
        elif mutation == 'long-payload': fields.append(fields[-1])
        elif mutation == 'zero-condition': fields[0] = '0'
        elif mutation == 'negative-condition': fields[0] = '-1'
        elif mutation == 'oversized-condition': fields[0] = '999999999999999999999'
        elif mutation == 'swapped-arms': fields[1], fields[2] = fields[2], fields[1]
        elif mutation == 'repeated-arm': fields[2] = fields[1]
        elif mutation == 'result-as-arm': fields[2] = receipt[2]
        elif mutation == 'foreign-statement':
            first_statement = next(row[3] for row in changed if row[0] == 'OWN' and row[1:3] == ['expression', fields[0]])
            foreign = next(row for row in changed if row[0] == 'OWN' and row[1:3] == ['expression', fields[0]])
            foreign[3] = next(row[1] for row in changed if row[0] == 'ST' and row[1] != first_statement)
        else: raise AssertionError(mutation)
        receipt[4] = '%09'.join(fields)
    return changed


def check_rejection(test):
    for backend in test.backends:
        source = fixture(test)
        _, artifact = test.invoke([source], backend=backend)
        rows = [line.split('\t') for line in artifact.read_text().splitlines()]
        for mutation in MUTATIONS:
            with test.subTest(backend=backend, mutation=mutation):
                changed = malformed_rows(rows, mutation)
                malformed = test.working / 'iif-malformed.sem'
                malformed.write_text('\n'.join('\t'.join(row) for row in changed) + '\n')
                with test.assertRaises(ValueError):
                    Model.read(malformed)

# end of iif_inputs.py
