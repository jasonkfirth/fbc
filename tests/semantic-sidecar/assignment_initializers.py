"""Project: FreeBASIC semantic sidecar tests
File: assignment_initializers.py
Purpose: Verify original scalar initializer inputs before assignment conversion.
Responsibilities: DIM/VAR, typed constants, deliberate exclusions and emission.
This file intentionally does NOT infer initialization from source spelling.
"""
from sidecar import Model
from assignment_inputs import source_text

BODY = '''Type Item
    value As Integer
End Type
Sub ScalarInitializers()
    Dim original As Integer
    Dim integerValue As Long = 7
    Dim floatingValue As Single = 0.5
    Dim textValue As String = "value"
    Dim pointerValue As Integer Ptr = @original
    Var inferredValue = original
    Static staticValue As Integer = 3
    Var ByRef aliasValue = original
    Dim values(0 To 1) As Integer = {1, 2}
    Dim arrayPointer As Integer Ptr = @values(0)
    Dim first As Item
    Dim second As Item = first
    Dim allocated As Integer Ptr
    allocated = New Integer(original)
    Dim declaredAllocation As Integer Ptr = New Integer(original)
    Var inferredAllocation = New Integer(original)
    Delete allocated
    Delete declaredAllocation
    Delete inferredAllocation
End Sub
'''


def check_initializers(test):
    for backend in test.backends:
        with test.subTest(backend=backend):
            source = test.source(source_text(BODY, 'scalar-initializers.bas'), 'scalar-initializers.bas')
            output = test.emission_path('initializers', backend)
            # Model validates native datatype slots; dedicated linter producers
            # can otherwise select their stable client ABI by default.
            extra = ('-o', str(output), '-semantic-type-abi', 'native')
            test.invoke([source], mode='off', backend=backend, extra=extra)
            plain = output.read_bytes()
            _, artifact = test.invoke([source], backend=backend, extra=extra)
            model = Model.read(artifact)
            test.assertEqual(output.read_bytes(), plain)
            test.assertEqual(model.capabilities[1]['assignment-initializers'], 'available')
            entries = [row for row in model.records['K'] if row[3].startswith('assignment-input:')
                       and row[4].split('\t')[2] == 'initializer']
            names = set()
            expressions = {int(row[1]): row for row in model.records['E']}
            for entry in entries:
                fields = entry[4].split('\t')
                left = expressions[int(fields[0])]
                names.add(model.symbols[int(left[13])][2].lower())
                test.assertEqual(model.types[int(left[13])][18], 'source')
                test.assertEqual(fields[3:6], ['assign', 'builtin', '0'])
            test.assertEqual(names, {'integervalue', 'floatingvalue', 'textvalue', 'pointervalue',
                                   'inferredvalue', 'staticvalue', 'arraypointer',
                                   'declaredallocation', 'inferredallocation'})
            test.assertTrue(all(model.statement_endings[int(row[3].split(':')[1])][2] == 'declaration'
                                for row in entries))
            for mode in ('bindings', 'expressions'):
                compact = test.compile(source, mode=mode, backend=backend, extra=extra)
                test.assertEqual(output.read_bytes(), plain)
                test.assertEqual(compact.capabilities[1]['assignment-initializers'], 'unavailable')
                test.assertFalse(any(row[3].startswith('assignment-input:') for row in compact.records['K']))
            rows = [line.split('\t') for line in artifact.read_text().splitlines()]
            changed = [row[:] for row in rows]
            next(row for row in changed if row[0] == 'CAP' and row[2] == 'assignment-initializers')[3] = 'unavailable'
            malformed = test.working / 'initializer-unavailable.semantic'
            malformed.write_text('\n'.join('\t'.join(row) for row in changed) + '\n')
            with test.assertRaises(ValueError):
                Model.read(malformed)
# end of assignment_initializers.py
