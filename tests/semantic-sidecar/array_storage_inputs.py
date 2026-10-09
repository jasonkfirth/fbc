"""Project: FreeBASIC semantic sidecar tests
File: array_storage_inputs.py
Purpose: Verify original array storage across fields, indices and bound folding.
Responsibilities: Exact receivers, rejection cases and unchanged compiler output.
This file intentionally does NOT execute potentially invalid array accesses.
"""
from sidecar import Model, source_range

BODY = '''Type ArrayHolder
    fixed_values(0 To 3) As Long
    dynamic_values(Any) As Long
End Type
Type OuterHolder
    inner As ArrayHolder
End Type
Declare Function ChooseHolder(ByRef item As ArrayHolder) As ArrayHolder Ptr
Declare Sub ReceiveArray(values() As Long)
Sub storage_inputs(ByRef item As ArrayHolder, ByRef other As ArrayHolder, _
    ByRef outer As OuterHolder, ByVal holder_ptr As ArrayHolder Ptr, ByVal index As Long)
    Dim values(0 To 3) As Long, dynamic_values() As Long
    Dim holders(0 To 1) As ArrayHolder
    Dim value As Long
    value = UBound(values) ' plain-query
    value = values(index) ' plain-index
    value = UBound(dynamic_values) ' dynamic-query
    value = dynamic_values(index) ' dynamic-index
    value = UBound(item.fixed_values) ' fixed-query
    value = item.fixed_values(index) ' fixed-index
    value = UBound(other.fixed_values) ' other-query
    value = other.fixed_values(index) ' other-index
    value = UBound(item.dynamic_values) ' field-dynamic-query
    value = item.dynamic_values(index) ' field-dynamic-index
    value = UBound(holder_ptr->fixed_values) ' pointer-query
    value = holder_ptr->fixed_values(index) ' pointer-index
    value = UBound(outer.inner.fixed_values) ' nested-query
    value = outer.inner.fixed_values(index) ' nested-index
    value = UBound(holders(index).fixed_values) ' indexed-query
    value = holders(index).fixed_values(index) ' indexed-index
    value = UBound(ChooseHolder(item)->fixed_values()) ' call-query
    value = ChooseHolder(item)->fixed_values(index) ' call-index
    Dim address As Long Ptr = @item.fixed_values(index) ' address
    value = SizeOf(item.fixed_values(index)) ' unevaluated
    ReceiveArray(item.fixed_values()) ' unindexed
    #Define STORAGE_QUERY UBound(item.fixed_values)
    value = STORAGE_QUERY ' macro-query
End Sub
'''

EXPECTED = {
    'plain-query': ('array-bound-symbol', 'variable', set()),
    'plain-index': ('array-subscript-symbol', 'variable', set()),
    'dynamic-query': ('array-bound-symbol', 'variable', set()),
    'dynamic-index': ('array-subscript-symbol', 'variable', set()),
    'fixed-query': ('array-bound-symbol', 'field', {'item'}),
    'fixed-index': ('array-subscript-symbol', 'field', {'item'}),
    'other-query': ('array-bound-symbol', 'field', {'other'}),
    'other-index': ('array-subscript-symbol', 'field', {'other'}),
    'field-dynamic-query': ('array-bound-symbol', 'field', {'item'}),
    'field-dynamic-index': ('array-subscript-symbol', 'field', {'item'}),
    'pointer-query': ('array-bound-symbol', 'field', {'holder_ptr'}),
    'pointer-index': ('array-subscript-symbol', 'field', {'holder_ptr'}),
    'nested-query': ('array-bound-symbol', 'field', {'outer'}),
    'nested-index': ('array-subscript-symbol', 'field', {'outer'}),
    'indexed-query': ('array-bound-symbol', 'field', {'holders', 'index'}),
    'indexed-index': ('array-subscript-symbol', 'field', {'holders', 'index'}),
    'call-query': ('array-bound-symbol', 'field', {'item'}),
    'call-index': ('array-subscript-symbol', 'field', {'item'}),
    'address': ('array-subscript-symbol', 'field', {'item'}),
    'unevaluated': ('array-subscript-symbol', 'field', {'item'}),
    'macro-query': ('array-bound-symbol', 'field', {'item'}),
}


def fixture(test, filename='array-storage-inputs.bas'):
    return test.source("' Project: FreeBASIC semantic sidecar tests\n' File: " + filename + '\n'
                       "' Purpose: Exercise array storage before descriptor lowering.\n"
                       "' Responsibilities: Bound folding, receivers and control inputs.\n"
                       "' This file intentionally does NOT execute array accesses.\n"
                       '#Lang "fb"\n' + BODY + "' end of " + filename + '\n', filename)


def receiver_variables(model, root):
    children = {}
    for identity, node in model.nodes.items():
        if node[3] != 'root':
            children.setdefault(int(node[2]), []).append(identity)
    names = set()
    pending = [root]
    while pending:
        identity = pending.pop()
        node = model.nodes[identity]
        if node[4] == '17':
            names.add(model.symbol_name(int(node[9])).casefold())
        pending.extend(children.get(identity, ()))
    return names


def check_inputs(test):
    source = fixture(test)
    labels = {line: text.rsplit("' ", 1)[1] for line, text in enumerate(source.read_text().splitlines(), 1)
              if "' " in text and not text.startswith("'")}
    for backend in test.backends:
        with test.subTest(backend=backend):
            emitted = test.working / ('storage.c' if backend == 'gcc' else 'storage.asm')
            extra = ('-o', str(emitted))
            plain_result, _ = test.invoke([source], mode='off', backend=backend, extra=extra)
            plain = emitted.read_bytes()
            full_result, artifact = test.invoke([source], backend=backend, extra=extra)
            model = Model.read(artifact)
            test.assertEqual(emitted.read_bytes(), plain)
            test.assertEqual(full_result.stdout + full_result.stderr, plain_result.stdout + plain_result.stderr)
            test.assertTrue(all(features['array-storage-inputs'] == 'available'
                                for features in model.capabilities.values()))
            expressions = {int(row[1]): row for row in model.records['E']}
            counts = {label: 0 for label in EXPECTED}
            roots = set()
            field_symbols = {}
            for (domain, identity), properties in model.properties.items():
                if 'array-storage-symbol' not in properties:
                    continue
                label = labels[source_range(expressions[identity], 3)[1]]
                if (label not in EXPECTED or EXPECTED[label][0] not in properties
                        or EXPECTED[label][1] != properties['array-storage-kind']):
                    continue
                key, kind, names = EXPECTED[label]
                counts[label] += 1
                test.assertEqual(properties['array-storage-kind'], kind)
                test.assertEqual(properties['array-storage-symbol'], properties[key])
                root = int(properties['array-storage-receiver'])
                if kind == 'field':
                    test.assertGreater(root, 0)
                    test.assertEqual(receiver_variables(model, root), names)
                    roots.add(root)
                    if label in ('fixed-query', 'other-query'):
                        field_symbols[label] = properties['array-storage-symbol']
                else:
                    test.assertEqual(root, 0)
            test.assertEqual(counts, {label: 1 for label in EXPECTED})
            test.assertEqual(field_symbols['fixed-query'], field_symbols['other-query'])
            test.assertEqual(len(roots), sum(kind == 'field' for _, kind, _ in EXPECTED.values()))
            compact_result, compact_artifact = test.invoke([source], mode='expressions', backend=backend, extra=extra)
            compact = Model.read(compact_artifact, expressions_only=True)
            test.assertEqual(emitted.read_bytes(), plain)
            test.assertEqual(compact_result.stdout + compact_result.stderr, plain_result.stdout + plain_result.stderr)
            test.assertTrue(all(features['array-storage-inputs'] == 'unavailable'
                                for features in compact.capabilities.values()))
            test.assertFalse(any(row[5].startswith('array-storage-') for row in compact.records['H']))


MUTATIONS = ('missing-selection', 'missing-group', 'missing-input', 'missing-root',
             'wrong-receiver', 'wrong-root-owner', 'wrong-node-owner', 'wrong-kind',
             'wrong-symbol', 'unknown-key', 'wrong-domain', 'duplicate-selection',
             'duplicate-property', 'duplicate-root', 'phased-snapshot', 'partial',
             'unavailable', 'missing-capability')


def malformed_rows(rows, mutation):
    changed = [row[:] for row in rows]
    selection = next(row for row in changed if row[0] == 'H' and row[5] == 'array-storage-selection'
                     and any(prop[:4] == ['K', 'expression', row[4], 'array-storage-kind']
                             and prop[4] == 'field' for prop in changed))
    identity = selection[4]
    group = [row for row in changed if row[:3] == ['K', 'expression', identity] and row[3].startswith('array-storage-')]
    receiver = next(row for row in group if row[3] == 'array-storage-receiver')
    input_row = next(row for row in changed if row[:3] == ['H', 'expression', identity] and row[5] == 'array-storage-input')
    root = next(row for row in changed if row[0] == 'H' and row[4] == receiver[4] and row[5] == 'array-storage-root')
    if mutation == 'missing-selection':
        selection[5] = 'fixture-missing-selection'
    elif mutation == 'missing-group':
        for row in group:
            row[3] = 'fixture-' + row[3]
    elif mutation == 'missing-input':
        input_row[5] = 'fixture-missing-input'
    elif mutation == 'missing-root':
        root[5] = 'fixture-missing-root'
    elif mutation == 'wrong-receiver':
        input_row[4] = next(row[4] for row in changed if row[0] == 'H'
                            and row[5] == 'array-storage-root' and row[4] != input_row[4])
    elif mutation == 'wrong-root-owner':
        root[2] = next(row[1] for row in changed if row[0] == 'S' and row[3] == '12' and row[1] != root[2])
    elif mutation == 'wrong-node-owner':
        owner = next(row for row in changed if row[:3] == ['OWN', 'node', root[4]])
        owner[3] = next(row[1] for row in changed if row[0] == 'ST' and row[1] != owner[3])
    elif mutation == 'wrong-kind':
        next(row for row in group if row[3] == 'array-storage-kind')[4] = 'variable'
    elif mutation == 'wrong-symbol':
        next(row for row in group if row[3] == 'array-storage-symbol')[4] = root[2] = next(
            row[1] for row in changed if row[0] == 'A' and row[1] != selection[2])
    elif mutation == 'unknown-key':
        receiver[3] = 'array-storage-unknown'
    elif mutation == 'wrong-domain':
        receiver[1:3] = ['node', root[4]]
    elif mutation in ('duplicate-selection', 'duplicate-root'):
        replacement = selection if mutation == 'duplicate-selection' else root
        next(row for row in changed if row[0] == 'H' and row[5] == 'source-expression')[:] = replacement
    elif mutation == 'duplicate-property':
        next(row for row in changed if row[0] == 'K' and row[3] == 'array-storage-kind' and row[2] != identity)[:] = receiver
    elif mutation == 'phased-snapshot':
        next(row for row in changed if row[0] == 'NP')[1] = root[4]
    elif mutation in ('partial', 'unavailable', 'missing-capability'):
        capability = next(row for row in changed if row[0] == 'CAP' and row[2] == 'array-storage-inputs')
        if mutation == 'missing-capability':
            capability[2] = 'fixture-missing-capability'
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
                malformed = test.working / 'array-storage-malformed.sem'
                malformed.write_text('\n'.join('\t'.join(row) for row in malformed_rows(rows, mutation)) + '\n')
                with test.assertRaises(ValueError):
                    Model.read(malformed)


def check_module_ownership(test):
    for backend in test.backends:
        first, second = fixture(test, 'first.bas'), fixture(test, 'second.bas')
        _, artifact = test.invoke([first, second], backend=backend)
        model = Model.read(artifact)
        test.assertEqual(len(model.capabilities), 2)
        rows = [line.split('\t') for line in artifact.read_text().splitlines()]
        roots_by_module = {}
        module = 0
        for row in rows:
            if row[0] == 'M':
                module += 1
            elif row[0] == 'H' and row[5] == 'array-storage-root':
                roots_by_module.setdefault(module, []).append(row)
        first_root, foreign = roots_by_module[1][0], roots_by_module[2][0]
        changed = [row[:] for row in rows]
        input_row = next(row for row in changed if row[0] == 'H' and row[5] == 'array-storage-input'
                         and row[4] == first_root[4])
        input_row[4] = foreign[4]
        malformed = test.working / 'array-storage-foreign.sem'
        malformed.write_text('\n'.join('\t'.join(row) for row in changed) + '\n')
        with test.assertRaises(ValueError):
            Model.read(malformed)

# end of array_storage_inputs.py
