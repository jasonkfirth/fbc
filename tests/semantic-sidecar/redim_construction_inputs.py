"""Project: FreeBASIC semantic sidecar tests
File: redim_construction_inputs.py
Purpose: Preserve compiler-selected REDIM object-array constructors.
Responsibilities: Native ownership, callbacks, origins and unchanged emission.
This file intentionally does NOT infer execution counts or run constructors.
"""
from collections import Counter
import subprocess

from sidecar import Model

BODY = '''Type Element
    value As Integer
    Declare Constructor()
End Type
Constructor Element()
    value = 1
End Constructor
Type DestructorOnly
    value As Integer
    Declare Destructor()
End Type
Destructor DestructorOnly()
End Destructor
Type Holder
    fieldItems(Any) As Element
End Type
Type GeneratedHolder
    Redim generatedItems(0 To 1) As Element
End Type
Dim generatedOwner As GeneratedHolder
Dim Shared controlledGlobal As Element
Redim globalItems(0 To 1) As Element
Sub ResizeLocal(ByVal extent As Integer)
    Dim declItems(0 To extent) As Element
    Static startupItems(0 To extent) As Element
    Dim runItems() As Element
    Static persistentItems() As Element
    Dim fixedItems(0 To 1) As Element
    Static fixedStaticItems(0 To 1) As Element
    Dim plainItems() As Integer, dtorItems() As DestructorOnly
    Redim runItems(0 To 1)
    Redim Preserve runItems(0 To 2)
    Redim runItems(2 To 1)
    Redim persistentItems(0 To 1)
    Redim plainItems(0 To 1)
    Redim dtorItems(0 To 1)
#Define GROW_ITEMS(items) Redim items(0 To 3)
    GROW_ITEMS(runItems)
#Line 700 "redim-construction-remap.bas"
    Redim runItems(0 To 4): Redim Preserve runItems(0 To 5)
End Sub
Sub ResizeField(ByRef instance As Holder)
    Redim instance.fieldItems(0 To 1)
End Sub
Sub ResizeBorrowed(borrowedItems() As Element)
    Redim borrowedItems(0 To 1)
End Sub
'''
EXPECTED = Counter({'globalitems': 1, 'declitems': 1, 'startupitems': 1,
                    'runitems': 6, 'persistentitems': 1, 'fielditems': 1,
                    'borroweditems': 1})


def fixture(test):
    from assignment_inputs import source_text
    return test.source(source_text(BODY, 'redim-constructions.bas'), 'redim-constructions.bas')


def observations(model):
    return [row for row in model.records['K'] if row[3].startswith('construction-observation:')
            and row[4].split('\t')[2] == 'redim-constructor']


def check_inputs(test):
    for backend in test.backends:
        with test.subTest(backend=backend):
            source = fixture(test)
            output = test.emission_path('redim-constructions', backend)
            extra = ('-o', str(output))
            test.invoke([source], mode='off', backend=backend, extra=extra)
            plain = output.read_bytes()
            _, artifact = test.invoke([source], backend=backend, extra=extra)
            model = Model.read(artifact)
            test.assertEqual(model.capabilities[1]['redim-constructor-selections'], 'available')
            test.assertEqual(output.read_bytes(), plain)
            model.validate_physical_locations()
            rows = observations(model)
            actual = Counter(model.symbols[int(row[2])][2].casefold() for row in rows)
            test.assertEqual(actual, EXPECTED)
            targets = set()
            macros = []
            statements = set()
            for row in rows:
                target, type_id, kind, statement, context, macro = row[4].split('\t')
                targets.add(int(target))
                test.assertEqual(model.signatures[int(target)][2], 'constructor')
                test.assertEqual(model.symbols[int(type_id)][2].casefold(), 'element')
                test.assertGreater(int(statement), 0)
                test.assertEqual(model.statements[int(statement)][5], context)
                statements.add(int(statement))
                if macro != '0':
                    macros.append(int(macro))
                    test.assertIn(int(macro), model.macro_invocations)
            test.assertEqual(len(targets), 1)
            test.assertEqual(len(statements), 12)
            test.assertTrue(macros)
            selected = [row for row in model.records['I'] if row[4] == 'redim-constructor']
            test.assertTrue(selected)
            test.assertTrue(all(int(row[2]) in targets for row in selected))
            controls = [row for row in model.records['K'] if row[3].startswith('construction-observation:')
                        and model.symbols[int(row[2])][2].casefold() in ('fixeditems', 'fixedstaticitems')]
            test.assertEqual(len(controls), 2)
            test.assertTrue(all(row[4].split('\t')[2] == 'default-constructor' for row in controls))
            # The generated field initializer uses the same runtime lowering
            # helper, but is not another written REDIM occurrence.
            test.assertNotIn('generateditems', actual)
            # Binary and text carry exactly the same selections, including
            # origin records. Neither transport may alter backend emission.
            binary = test.working / ('redim-' + backend + '.fbcsem')
            reply = subprocess.run([str(test.compiler), '-prefix', str(test.toolchain_prefix),
                                    '-i', str(test.root / 'inc'), '-r', '-gen', backend, '-maxerr', '20',
                                    '-semantic-model', str(binary), *extra, str(source)],
                                   cwd=test.working, capture_output=True, text=True, timeout=120)
            test.assertEqual(reply.returncode, 0, reply.stdout + reply.stderr)
            test.assertTrue(Model.read(binary).rows == model.rows,
                            'Binary transport changed the decoded REDIM model')
            test.assertEqual(output.read_bytes(), plain)
            for mode in ('bindings', 'expressions'):
                compact = test.compile(source, mode=mode, backend=backend, extra=extra)
                test.assertEqual(output.read_bytes(), plain)
                test.assertEqual(compact.capabilities[1]['implicit-construction-inputs'], 'unavailable')
                test.assertEqual(compact.capabilities[1]['redim-constructor-selections'], 'unavailable')
                test.assertFalse(observations(compact))


MUTATIONS = ('owner', 'target', 'type', 'kind', 'statement', 'source', 'macro',
             'ordinal', 'closure', 'missing-selection')


def control_rows(rows, mutation):
    changed = [row[:] for row in rows]
    selected = next(row for row in changed if row[0] == 'K'
                    and row[3].startswith('construction-observation:')
                    and row[4].split('%09')[2] == 'redim-constructor')
    fields = selected[4].split('%09')
    if mutation == 'owner':
        selected[2] = fields[0]
    elif mutation in ('target', 'type'):
        fields[0 if mutation == 'target' else 1] = selected[2]
    elif mutation == 'kind':
        fields[2] = 'unknown-constructor'
    elif mutation in ('statement', 'source'):
        fields[3 if mutation == 'statement' else 4] = '0'
    elif mutation == 'macro':
        fields[5] = '1000000'
    elif mutation == 'ordinal':
        selected[3] = 'construction-observation:1000000'
    elif mutation == 'closure':
        closure = next(row for row in changed if row[0] == 'K'
                       and row[3].startswith('construction-observation-count:'))
        closure[4] = str(int(closure[4]) + 1)
    elif mutation == 'missing-selection':
        # Renumber and recount so the retained I selection, rather than a
        # generic footer or ordinal gap, proves that this observation is lost.
        changed.remove(selected)
        ordinal = 0
        for row in changed:
            if row[0] == 'K' and row[3].startswith('construction-observation:'):
                ordinal += 1
                row[3] = 'construction-observation:' + str(ordinal)
        closure = next(row for row in changed if row[0] == 'K'
                       and row[3].startswith('construction-observation-count:'))
        closure[4] = str(ordinal)
    else:
        raise ValueError('Unknown REDIM control: ' + mutation)
    selected[4] = '%09'.join(fields)
    return changed


def check_rejection(test):
    from parsed_expression_coverage import write_rows
    for backend in test.backends:
        source = fixture(test)
        _, artifact = test.invoke([source], backend=backend)
        rows = [line.split('\t') for line in artifact.read_text().splitlines()]
        damaged = test.working / 'damaged-redim.tsv'
        for mutation in MUTATIONS:
            with test.subTest(backend=backend, mutation=mutation):
                write_rows(damaged, control_rows(rows, mutation))
                with test.assertRaises(ValueError):
                    Model.read(damaged)

# end of redim_construction_inputs.py
