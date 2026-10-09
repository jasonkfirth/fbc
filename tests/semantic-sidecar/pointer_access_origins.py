"""Project: FreeBASIC semantic sidecar tests
File: pointer_access_origins.py
Purpose: Verify pointer inputs retained before dereference and index lowering.
Responsibilities: Exact access origins, complete groups and unchanged code output.
This file intentionally does NOT execute potentially invalid pointer accesses.
"""
from sidecar import Model, source_range

BODY = '''Type Holder
    value As Long
    pointer As Long Ptr
End Type
Sub access_inputs(ByVal p As Long Ptr, ByVal pp As Long Ptr Ptr, ByVal index As Long, ByRef item As Holder)
    Dim value As Long
    value = *p ' single
    value = **pp ' multiple
    value = *(*pp) ' nested
    value = p[index] ' index
    value = p[CByte(index)] ' cast-index
    value = *item.pointer ' field
    value = *(@value) ' canceled
    Dim address As Long Ptr = @*p ' address
    Dim bytes As Long = SizeOf(*p) ' unevaluated
    #Define INPUT_ACCESS *p
    value = INPUT_ACCESS ' macro
End Sub
'''


def fixture(test, filename='pointer-access-origins.bas'):
    return test.source("' Project: FreeBASIC semantic sidecar tests\n' File: " + filename + '\n'
                       "' Purpose: Exercise original pointer access identities.\n"
                       "' Responsibilities: Dereferences, indices and lowering controls.\n"
                       "' This file intentionally does NOT execute pointer reads.\n"
                       '#Lang "fb"\n' + BODY + "' end of " + filename + '\n', filename)


def check_inputs(test):
    source = fixture(test)
    labels = {line: text.rsplit("' ", 1)[1] for line, text in enumerate(source.read_text().splitlines(), 1)
              if "' " in text and not text.startswith("'")}
    expected = {'single': ('pointer-dereference-input', 1),
                'multiple': ('pointer-dereference-input', 2),
                'nested': ('pointer-dereference-input', 1),
                'index': ('pointer-index-input', None),
                'cast-index': ('pointer-index-input', None),
                'field': ('pointer-dereference-input', 1),
                'canceled': ('pointer-dereference-input', 1),
                'address': ('pointer-dereference-input', 1),
                'unevaluated': ('pointer-dereference-input', 1),
                'macro': ('pointer-dereference-input', 1)}
    for backend in test.backends:
        with test.subTest(backend=backend):
            emitted = test.working / ('pointer.c' if backend == 'gcc' else 'pointer.asm')
            extra = ('-o', str(emitted))
            plain_result, _ = test.invoke([source], mode='off', backend=backend, extra=extra)
            plain = emitted.read_bytes()
            full_result, artifact = test.invoke([source], backend=backend, extra=extra)
            model = Model.read(artifact)
            test.assertEqual(emitted.read_bytes(), plain)
            test.assertEqual(full_result.stdout + full_result.stderr, plain_result.stdout + plain_result.stderr)
            test.assertTrue(all(features['pointer-access-origins'] == 'available'
                                for features in model.capabilities.values()))
            expressions = {int(row[1]): row for row in model.records['E']}
            observations = [row for row in model.records['H'] if row[5] in
                            ('pointer-dereference-input', 'pointer-index-input')]
            counts = {label: 0 for label in expected}
            for row in observations:
                identity = int(row[2])
                label = labels[source_range(expressions[identity], 3)[1]]
                test.assertEqual(row[5], expected[label][0])
                if expected[label][1] is not None:
                    test.assertEqual(int(row[6]), expected[label][1])
                elif label == 'cast-index':
                    test.assertEqual(int(expressions[int(row[6])][12]) & 511, 2)
                counts[label] += 1
            test.assertEqual(counts, {label: 2 if label == 'nested' else 1 for label in expected})
            for mode in ('bindings', 'expressions'):
                compact = test.compile(source, mode=mode, backend=backend, extra=extra)
                test.assertEqual(emitted.read_bytes(), plain)
                test.assertFalse(any(row[5] in ('pointer-dereference-input', 'pointer-index-input')
                                     for row in compact.records['H']))
                test.assertTrue(all(features['pointer-access-origins'] == 'unavailable'
                                    for features in compact.capabilities.values()))


MUTATIONS = ('missing-origin', 'missing-group', 'missing-detail', 'wrong-input',
             'cyclic-input', 'wrong-detail', 'wrong-domain', 'duplicate-origin',
             'unavailable', 'partial', 'missing-capability', 'wrong-owner')


def malformed_rows(rows, role, mutation):
    changed = [row[:] for row in rows]
    origin = next(row for row in changed if row[0] == 'H' and row[5] == role)
    prefix = 'pointer-dereference-' if role == 'pointer-dereference-input' else 'pointer-index-'
    operand = next(row for row in changed if row[:4] == ['K', 'expression', origin[2], prefix + 'operand'])
    detail = next(row for row in changed if row[:4] == ['K', 'expression', origin[2], prefix +
                                                     ('count' if prefix == 'pointer-dereference-' else 'index')])
    if mutation == 'missing-origin':
        origin[5] = 'fixture-missing-origin'
    elif mutation == 'missing-group':
        operand[3] = 'fixture-missing-operand'
        detail[3] = 'fixture-missing-detail'
    elif mutation == 'missing-detail':
        detail[3] = 'fixture-missing-detail'
    elif mutation == 'wrong-input':
        origin[4] = next(row[1] for row in changed if row[0] == 'E' and row[1] != origin[4])
    elif mutation == 'cyclic-input':
        origin[4] = origin[2]
    elif mutation == 'wrong-detail':
        origin[6] = str(int(origin[6]) + 1)
    elif mutation == 'wrong-domain':
        origin[1:3] = ['symbol', next(row[1] for row in changed if row[0] == 'S')]
    elif mutation == 'duplicate-origin':
        next(row for row in changed if row[0] == 'H' and row[5] not in
             ('pointer-dereference-input', 'pointer-index-input'))[:] = origin
    elif mutation in ('unavailable', 'partial', 'missing-capability'):
        capability = next(row for row in changed if row[0] == 'CAP' and row[2] == 'pointer-access-origins')
        if mutation == 'missing-capability':
            capability[2] = 'fixture-missing-pointer-capability'
        else:
            capability[3] = mutation
    elif mutation == 'wrong-owner':
        owner = next(row for row in changed if row[:3] == ['OWN', 'expression', origin[4]])
        owner[3] = next(row[1] for row in changed if row[0] == 'ST' and row[1] != owner[3])
    else:
        raise AssertionError(mutation)
    return changed


def check_rejection(test):
    for backend in test.backends:
        _, artifact = test.invoke([fixture(test)], backend=backend)
        rows = [line.split('\t') for line in artifact.read_text().splitlines()]
        for role in ('pointer-dereference-input', 'pointer-index-input'):
            for mutation in MUTATIONS:
                with test.subTest(backend=backend, role=role, mutation=mutation):
                    changed = malformed_rows(rows, role, mutation)
                    malformed = test.working / 'pointer-malformed.sem'
                    malformed.write_text('\n'.join('\t'.join(row) for row in changed) + '\n')
                    with test.assertRaises(ValueError):
                        Model.read(malformed)


def check_compatibility(test):
    for backend in test.backends:
        _, artifact = test.invoke([fixture(test)], backend=backend)
        rows = [line.split('\t') for line in artifact.read_text().splitlines()]
        for row in rows:
            if row[0] == 'CAP' and row[2] == 'pointer-access-origins':
                row[2] = 'fixture-earlier-producer'
            if row[0] == 'H' and row[5] in ('pointer-dereference-input', 'pointer-index-input'):
                row[5] = 'fixture-earlier-producer'
        earlier = test.working / 'pointer-earlier.sem'
        earlier.write_text('\n'.join('\t'.join(row) for row in rows) + '\n')
        model = Model.read(earlier)
        test.assertTrue(any('pointer-dereference-operand' in properties
                            for properties in model.properties.values()))
        test.assertTrue(any('pointer-index-operand' in properties
                            for properties in model.properties.values()))


def check_module_ownership(test):
    for backend in test.backends:
        _, artifact = test.invoke([fixture(test), fixture(test, 'second-pointer-input.bas')], backend=backend)
        Model.read(artifact)
        rows = [line.split('\t') for line in artifact.read_text().splitlines()]
        module = 0
        origins = []
        for index, row in enumerate(rows):
            if row[0] == 'M':
                module += 1
            if row[0] == 'H' and row[5] == 'pointer-dereference-input' and len(origins) < module:
                origins.append(index)
        test.assertEqual(len(origins), 2)
        rows[origins[0]], rows[origins[1]] = rows[origins[1]], rows[origins[0]]
        malformed = test.working / 'pointer-foreign-module.sem'
        malformed.write_text('\n'.join('\t'.join(row) for row in rows) + '\n')
        with test.assertRaises(ValueError):
            Model.read(malformed)

# end of pointer_access_origins.py
