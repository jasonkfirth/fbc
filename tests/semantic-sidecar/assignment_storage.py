"""Project: FreeBASIC semantic sidecar tests
File: assignment_storage.py
Purpose: Exercise original assignment storage independently of executable ASTs.
Responsibilities: Typed forests, exact destination origins and malformed inputs.
This file intentionally does NOT infer storage equality from source spelling.
"""
from sidecar import Model, unescape
from assignment_inputs import BODY, fixture, source_text

FEATURE = 'assignment-storage-trees'
MUTATIONS = (
    'missing-group', 'missing-node', 'missing-group-marker', 'missing-node-marker',
    'missing-destination', 'wrong-destination', 'noncanonical-destination',
    'wrong-left-root', 'wrong-right-root', 'wrong-count', 'oversized-count',
    'noncanonical-key', 'zero-node-id', 'oversized-node-id', 'invalid-class',
    'invalid-dtype', 'invalid-pointer-depth', 'foreign-subtype', 'foreign-symbol',
    'invalid-options', 'invalid-offset', 'invalid-scale', 'invalid-conversion',
    'invalid-coverage', 'opaque-children', 'missing-left-child', 'backward-child',
    'oversized-child', 'repeated-child', 'unknown-operation', 'nonoperator-code',
    'invalid-constant-kind', 'overflow-signed', 'underflow-signed', 'overflow-unsigned',
    'noncanonical-signed', 'invalid-float', 'wrong-root-type', 'unavailable-capability',
    'wrong-marker-owner', 'wrong-marker-node', 'noncanonical-marker',
)


def malformed_rows(rows, mutation):
    changed = [row[:] for row in rows]
    group = next(row for row in changed if row[0] == 'K' and row[3].startswith('assignment-storage:'))
    _, statement, ordinal = group[3].split(':')
    group_marker = next(row for row in changed if row[0] == 'H' and row[5] == 'assignment-storage:' + ordinal and row[6] == statement)
    node = next(row for row in changed if row[0] == 'K' and row[3] == 'assignment-tree:' + statement + ':' + ordinal + ':1')
    node_marker = next(row for row in changed if row[0] == 'H' and row[5] == 'assignment-tree:' + ordinal + ':1' and row[6] == statement)
    destination = next(row for row in changed if row[0] == 'LOC' and row[1:4] == ['statement', statement, 'assignment-destination:' + ordinal])
    binary = next(row for row in changed if row[0] == 'K' and row[3].startswith('assignment-tree:') and unescape(row[4]).split('\t')[0] == '3')
    constant = next(row for row in changed if row[0] == 'K' and row[3].startswith('assignment-tree:') and unescape(row[4]).split('\t')[0] == '16')

    def replace(row, field, value):
        fields = unescape(row[4]).split('\t')
        fields[field] = value
        row[4] = '\t'.join(fields).replace('%', '%25').replace('\t', '%09')

    if mutation == 'missing-group': group[3] = 'fixture-removed-group'
    if mutation == 'missing-node': node[3] = 'fixture-removed-node'
    if mutation == 'missing-group-marker': group_marker[5] = 'fixture-removed-group-marker'
    if mutation == 'missing-node-marker': node_marker[5] = 'fixture-removed-node-marker'
    if mutation == 'missing-destination': destination[3] = 'fixture-removed-destination'
    if mutation == 'wrong-destination': destination[3] = 'assignment-destination:999999'
    if mutation == 'noncanonical-destination': destination[3] = 'assignment-destination:01'
    if mutation == 'wrong-left-root': replace(group, 0, '2')
    if mutation == 'wrong-right-root': replace(group, 1, '1')
    if mutation == 'wrong-count': replace(group, 2, '1')
    if mutation == 'oversized-count': replace(group, 2, '65537')
    if mutation == 'noncanonical-key': group[3] = 'assignment-storage:0' + statement + ':' + ordinal
    if mutation == 'zero-node-id': node[3] = 'assignment-tree:' + statement + ':' + ordinal + ':0'
    if mutation == 'oversized-node-id': node[3] = 'assignment-tree:' + statement + ':' + ordinal + ':65537'
    if mutation == 'invalid-class': replace(node, 0, '46')
    if mutation == 'invalid-dtype': replace(node, 1, '31')
    if mutation == 'invalid-pointer-depth': replace(node, 1, '288')
    if mutation == 'foreign-subtype': replace(node, 2, '999999')
    if mutation == 'foreign-symbol': replace(node, 3, '999999')
    if mutation == 'invalid-options': replace(binary, 5, '256')
    if mutation == 'invalid-offset': replace(node, 6, '9223372036854775808')
    if mutation == 'invalid-scale': replace(node, 7, '9223372036854775808')
    if mutation == 'invalid-conversion': replace(node, 8, '2')
    if mutation == 'invalid-coverage': replace(node, 15, 'maybe')
    if mutation == 'opaque-children': replace(binary, 15, 'opaque')
    if mutation == 'missing-left-child': replace(binary, 13, '0')
    if mutation == 'backward-child': replace(binary, 13, '1')
    if mutation == 'oversized-child': replace(binary, 13, '65537')
    if mutation == 'repeated-child': replace(binary, 14, unescape(binary[4]).split('\t')[13])
    if mutation == 'unknown-operation': replace(binary, 4, 'fixture-unknown')
    if mutation == 'nonoperator-code': replace(node, 4, 'add')
    if mutation == 'invalid-constant-kind': replace(constant, 11, 'bytes')
    if mutation in ('overflow-signed', 'underflow-signed', 'noncanonical-signed'):
        replace(constant, 11, 'signed')
        replace(constant, 12, {'overflow-signed': '9223372036854775808',
            'underflow-signed': '-9223372036854775809', 'noncanonical-signed': '-0'}[mutation])
    if mutation == 'overflow-unsigned':
        replace(constant, 11, 'unsigned'); replace(constant, 12, '18446744073709551616')
    if mutation == 'invalid-float':
        replace(constant, 11, 'float64-bits'); replace(constant, 12, '0xDEADBEEF')
    if mutation == 'wrong-root-type': replace(node, 1, '16' if unescape(node[4]).split('\t')[1] != '16' else '8')
    if mutation == 'unavailable-capability':
        next(row for row in changed if row[0] == 'CAP' and row[2] == FEATURE)[3] = 'unavailable'
    if mutation == 'wrong-marker-owner': node_marker[4] = '999999'
    if mutation == 'wrong-marker-node': node_marker[5] = 'assignment-tree:' + ordinal + ':65537'
    if mutation == 'noncanonical-marker': node_marker[5] = 'assignment-tree:0' + ordinal + ':1'
    return changed


def check_storage(test):
    for backend in test.backends:
        with test.subTest(backend=backend):
            source = fixture(test)
            source.write_text(source_text(BODY + 'leftValue = CInt(Rnd)\n'), encoding='utf-8')
            model = test.compile(source, backend=backend)
            groups = [row for row in model.records['K'] if row[3].startswith('assignment-storage:')]
            inputs = [row for row in model.records['K'] if row[3].startswith('assignment-input:')]
            test.assertEqual(len(groups), len(inputs))
            test.assertTrue(any(row[4].startswith('assignment-destination:') for row in model.records['MR']))
            test.assertTrue(any(row[3].startswith('assignment-tree:') and row[4].split('\t')[15] == 'opaque'
                                for row in model.records['K']))
            for mode in ('bindings', 'expressions'):
                compact = test.compile(source, mode=mode, backend=backend)
                test.assertEqual(compact.capabilities[1][FEATURE], 'unavailable')
                test.assertFalse(any(row[3].startswith(('assignment-storage:', 'assignment-tree:')) for row in compact.records['K']))
    check_index_binding(test)


def check_rejection(test):
    for backend in test.backends:
        _, artifact = test.invoke([fixture(test)], backend=backend)
        rows = [line.split('\t') for line in artifact.read_text().splitlines()]
        for mutation in MUTATIONS:
            with test.subTest(backend=backend, mutation=mutation), test.assertRaises(ValueError):
                Model('\n'.join('\t'.join(row) for row in malformed_rows(rows, mutation)) + '\n')


def check_index_binding(test):
    source = test.source('''\
' Project: FreeBASIC semantic sidecar tests
' File: overloaded-index.bas
' Purpose: Preserve the selected BYREF operator in original storage snapshots.
' Responsibilities: Opaque accessors and matching declared/native return types.
' This file intentionally does NOT assume that repeated access is side-effect free.
Type IndexedValue
    Values(0 To 0) As Integer
    Declare Operator [](ByVal Item As Integer) ByRef As Integer
    Declare Operator Cast() As Integer
End Type
Operator IndexedValue.[](ByVal Item As Integer) ByRef As Integer
    Return Values(Item)
End Operator
Operator IndexedValue.Cast() As Integer
    Return Values(0)
End Operator
Sub CopyIndexed(ByRef Value As IndexedValue)
    Value[0] = Value[0]
End Sub
' end of overloaded-index.bas
''', 'overloaded-index.bas')
    for backend in test.backends:
        with test.subTest(backend=backend, case='overloaded-index'):
            _, artifact = test.invoke([source], backend=backend)
            model = Model.read(artifact)
            expression = next(row for row in model.records['E'] if row[10:12] == ['index', 'overloaded'])
            selected = expression[13]
            signature = model.signatures[int(selected)]
            test.assertEqual(signature[6], '1')
            rows = [line.split('\t') for line in artifact.read_text().splitlines()]
            call = next(row for row in rows if row[0] == 'K' and row[3].startswith('assignment-tree:')
                        and unescape(row[4]).split('\t')[0] == '9'
                        and unescape(row[4]).split('\t')[3] == selected)
            test.assertEqual(unescape(call[4]).split('\t')[15], 'opaque')
            other = next(row for row in rows if row[0] == 'F' and row[10] == 'cast')
            for fault in ('wrong-call', 'wrong-operator', 'non-reference-return', 'wrong-native-return', 'transparent-call'):
                damaged = [row[:] for row in rows]
                for row in damaged:
                    if row == call and fault in ('wrong-call', 'transparent-call'):
                        fields = unescape(row[4]).split('\t')
                        fields[3 if fault == 'wrong-call' else 15] = other[1] if fault == 'wrong-call' else 'complete'
                        row[4] = '\t'.join(fields).replace('%', '%25').replace('\t', '%09')
                    if row[0:2] == ['F', selected]:
                        if fault == 'wrong-operator': row[10] = 'cast'
                        if fault == 'non-reference-return': row[6] = '0'
                        if fault == 'wrong-native-return': row[7] = expression[12]
                with test.subTest(fault=fault), test.assertRaises(ValueError):
                    Model('\n'.join('\t'.join(row) for row in damaged) + '\n')

# end of assignment_storage.py
