"""Project: FreeBASIC semantic sidecar tests
File: control_dispatch.py
Purpose: Verify accepted native control-statement dispatch observations.
Responsibilities: Parser selections, language changes, macros and export modes.
This file intentionally does NOT classify statements from source spelling.
"""
from collections import Counter

from sidecar import Model


def source_text(filename, body, language='fblite'):
    return ("' Project: FreeBASIC semantic sidecar tests\n"
        "' File: " + filename + "\n"
        "' Purpose: Observe accepted control-statement dispatches.\n"
        "' Responsibilities: A bounded parser route and its native statement owner.\n"
        "' This file intentionally does NOT execute its legacy control flow.\n\n"
        '#Lang "' + language + '"\n\n' + body + "\n\n' end of " + filename + '\n')


def dispatch_cases():
    return {
        'all-routes': ('fblite', '''Option NoGosub
Function Ordinary() As Integer
Return 7
End Function
Option Gosub
Sub Legacy(ByVal ChoiceValue As Integer)
On Local Error Goto Trouble
On Local Error Goto 0
Gosub BodyLabel
On ChoiceValue Goto BodyLabel, OtherLabel
On ChoiceValue Gosub BodyLabel, OtherLabel
Goto Finish
BodyLabel:
Return
OtherLabel:
Return Finish
Trouble:
Resume Next
Finish:
End Sub''', ('procedure-return', 'on-error-set', 'on-error-clear', 'gosub',
             'on-goto', 'on-gosub', 'goto', 'gosub-return', 'gosub-return-label')),
        'numeric-labels': ('qb', '''On 1 Gosub 100, 200
Goto 300
100 Return
200 Return 300
300''', ('on-gosub', 'goto', 'gosub-return', 'gosub-return-label')),
        'macro-dispatch': ('fblite', '''Option Gosub
#Macro EnterBody()
Gosub BodyLabel
#EndMacro
#Macro ReturnBody()
Return
#EndMacro
EnterBody()
Goto Finish
BodyLabel:
ReturnBody()
Finish:''', ('gosub', 'goto', 'gosub-return')),
        'changed-options': ('fblite', '''Option Gosub
Sub Legacy()
Return
End Sub
Option NoGosub
Sub Ordinary()
Return
End Sub''', ('gosub-return', 'procedure-return')),
        'inactive-routes': ('fblite', '''#If 0
On Error Goto Missing
Gosub Missing
Return
#EndIf
Print 1''', ()),
        'removed-keyword': ('fblite', '''Option NoKeyword Goto
Dim As Integer Goto = 7
Print Goto''', ()),
    }


def check_dispatches(test):
    for case, (language, body, expected) in dispatch_cases().items():
        source = test.source(source_text(case + '.bas', body, language), case + '.bas')
        for backend in test.backends:
            output = test.emission_path('dispatch', backend)
            extra = ('-o', str(output))
            with test.subTest(case=case, backend=backend, mode='off'):
                test.invoke([source], backend=backend, mode='off', extra=extra)
                baseline = output.read_bytes()
            for mode in ('full', 'bindings', 'expressions'):
                with test.subTest(case=case, backend=backend, mode=mode):
                    _, path = test.invoke([source], backend=backend, mode=mode, extra=extra)
                    model = Model.read(path, bindings_only=mode == 'bindings', expressions_only=mode == 'expressions')
                    test.assertEqual(output.read_bytes(), baseline)
                    actual = [operation for values in model.statement_operations.values() for operation in values]
                    test.assertEqual(Counter(actual), Counter(expected))
                    test.assertEqual(model.capabilities[1]['statement-control-operations'], 'available')
                    endings = {int(row[1]): row for row in model.records['STE']}
                    for statement in model.statement_operations:
                        test.assertIn(statement, model.statements)
                        test.assertEqual(endings[statement][3], 'parsed')
# end of control_dispatch.py
