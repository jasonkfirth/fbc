"""Project: FreeBASIC semantic sidecar tests
File: keyword_contexts.py
Purpose: Verify native REM keyword state after actual hash removal.
Responsibilities: Committed contexts, variable bindings and per-module reset.
This file intentionally does NOT infer keyword state from source spelling.
"""
from sidecar import Model


def check_keyword_contexts(test):
    removed = test.source('''\
' Project: FreeBASIC semantic context tests
' File: removed-rem.bas
' Purpose: Preserve actual keyword hash removal in option contexts.
' Responsibilities: Active comment, removed keyword and native variable use.
' This file intentionally does NOT define alternate lexical grammar.
#Lang "fblite"
Rem native comment before removal
Option Nokeyword Rem
Dim Rem As Integer
Rem = 9
' end of removed-rem.bas
''', 'removed-rem.bas')
    active = test.source('''\
' Project: FreeBASIC semantic context tests
' File: active-rem.bas
' Purpose: Check that the next module restores the native keyword table.
' Responsibilities: Native REM comment and ordinary selected variable.
' This file intentionally does NOT inherit the preceding module's options.
#Lang "qb"
Rem native comment after module reset
Dim value As Integer
value = 7
' end of active-rem.bas
''', 'active-rem.bas')
    for backend in test.backends:
        with test.subTest(backend=backend):
            _, path = test.invoke([removed, active], backend=backend)
            model = Model.read(path)
            key = ('language-policy', 'remkeyword-active')
            first = [model.options[context][key] for context, module in model.configurations.items() if module == 1]
            second = [model.options[context][key] for context, module in model.configurations.items() if module == 2]
            test.assertEqual(first[0], 1)
            test.assertIn(0, first)
            test.assertEqual(first[first.index(0):], [0] * (len(first) - first.index(0)))
            test.assertTrue(second)
            test.assertEqual(set(second), {1})
            variables = set(model.named('REM', 'variable'))
            test.assertEqual(len(variables), 1)
            observed = 0
            for row in model.records['N']:
                if int(row[9]) in variables:
                    context = model.context_uses.get(('node', int(row[1])))
                    if context:
                        test.assertEqual(model.options[context][key], 0)
                        observed += 1
            test.assertGreater(observed, 0)
# end of keyword_contexts.py
