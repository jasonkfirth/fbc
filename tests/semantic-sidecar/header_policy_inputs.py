"""Project: FreeBASIC semantic sidecar tests
File: header_policy_inputs.py
Purpose: Verify accepted per-source policies without changing compiler output.
Responsibilities: Three backends, nested strings, closure and malformed facts.
This file intentionally does NOT decide which public header advice to publish.
"""
from sidecar import Model, unescape

FEATURE = 'source-header-policy-inputs'
MUTATIONS = ('missing-policy', 'invalid-once', 'noncanonical-count',
             'library-count', 'import-count', 'missing-library',
             'duplicate-library', 'invalid-kind', 'noncanonical-key',
             'missing-capability', 'missing-import', 'duplicate-import')
HEADER = '''#Pragma Once
#Inclib "fixture"
#Libpath "percent% and\t tab"
Namespace PolicyScope
    Const Value = 1
End Namespace
Using PolicyScope
'''


def malformed_rows(rows, mutation):
    changed = [row.copy() for row in rows]
    library = next(row for row in changed if row[0] == 'K' and row[3].startswith('header-library:'))
    source_id = unescape(library[4]).split('\t')[0]
    policy = next(row for row in changed if row[0] == 'K' and row[3] == 'header-policy:' + source_id)
    operation = next(row for row in changed if row[0] == 'SOP' and row[2] == 'namespace-import')
    fields = unescape(policy[4]).split('\t')
    if mutation == 'missing-policy': policy[3] = 'fixture-removed-policy'
    elif mutation == 'invalid-once': fields[0] = '2'
    elif mutation == 'noncanonical-count': fields[1] = '0' + fields[1]
    elif mutation == 'library-count': fields[1] = '0'
    elif mutation == 'import-count': fields[2] = '0'
    elif mutation == 'missing-library': library[3] = 'fixture-removed-library'
    elif mutation == 'duplicate-library': changed.insert(-1, library.copy())
    elif mutation == 'invalid-kind': library[4] = library[4].replace('inclib', 'unknown')
    elif mutation == 'noncanonical-key': library[3] = 'header-library:0' + library[3].split(':')[1]
    elif mutation == 'missing-capability':
        capability = next(row for row in changed if row[:3] == ['CAP', '1', FEATURE])
        capability[3] = 'unavailable'
    elif mutation == 'missing-import': changed.remove(operation)
    elif mutation == 'duplicate-import': changed.insert(-1, operation.copy())
    else: raise AssertionError(mutation)
    if mutation in ('invalid-once', 'noncanonical-count', 'library-count', 'import-count'):
        policy[4] = '%09'.join(fields)
    counts = {tag: sum(row[0] == tag for row in changed[1:-1]) for tag in ('M', 'P', 'S', 'V', 'N', 'E', 'B', 'I', 'D')}
    for position, tag in enumerate(counts, 2):
        changed[-1][position] = str(counts[tag])
    changed[-1][12] = str(len(changed) - 2 - sum(counts.values()))
    return changed


def check_header_policy(test):
    header = test.source(HEADER, 'policy.bi')
    source = test.source('#Include "policy.bi"\nPrint PolicyScope.Value\n', 'header-policy.bas')
    macro_source = test.source('#define POLICY_LIBRARY "fixture"\n#inclib POLICY_LIBRARY\n'
                               '#define POLICY_DIRECTORY "fixture%path"\n#libpath POLICY_DIRECTORY\n'
                               'Print 1\n', 'macro-library.bas')
    for backend in test.backends:
        with test.subTest(backend=backend):
            emitted = test.emission_path('header-policy', backend)
            extra = ('-o', str(emitted))
            test.invoke([source], mode='off', backend=backend, extra=extra)
            baseline = emitted.read_bytes()
            _, artifact = test.invoke([source], backend=backend, extra=extra)
            model = Model.read(artifact)
            test.assertEqual(emitted.read_bytes(), baseline)
            policy_source = next(identity for identity, row in model.source_contexts.items() if row[5] == 'include')
            policy = next(row for row in model.records['K'] if row[3] == 'header-policy:' + str(policy_source))
            test.assertEqual(policy[4], '1\t2\t1')
            library_values = [row[4].split('\t') for row in model.records['K'] if row[3].startswith('header-library:')]
            test.assertEqual([unescape(fields[2]) for fields in library_values], ['fixture', 'percent% and\t tab'])
            for mode in ('bindings', 'expressions'):
                compact = test.compile(source, mode=mode, backend=backend, extra=extra)
                test.assertEqual(emitted.read_bytes(), baseline)
                test.assertEqual(compact.capabilities[1][FEATURE], 'unavailable')
                test.assertFalse(any(row[3].startswith('header-policy:') for row in compact.records['K']))
            rows = [line.split('\t') for line in artifact.read_text().splitlines()]
            for mutation in MUTATIONS:
                with test.subTest(mutation=mutation), test.assertRaises(ValueError):
                    Model('\n'.join('\t'.join(row) for row in malformed_rows(rows, mutation)) + '\n')
            test.invoke([macro_source], mode='off', backend=backend, extra=extra)
            macro_baseline = emitted.read_bytes()
            macro_model = test.compile(macro_source, backend=backend, extra=extra)
            test.assertEqual(emitted.read_bytes(), macro_baseline)
            origins = [key for key in macro_model.macro_origins if key[2].startswith('header-library:')]
            test.assertEqual(len(origins), 2)
            test.assertTrue(all(key[0] == 'symbol' for key in origins))
            macro_values = [row[4].split('\t') for row in macro_model.records['K']
                            if row[3].startswith('header-library:')]
            test.assertEqual([unescape(fields[2]) for fields in macro_values], ['fixture', 'fixture%path'])
# end of header_policy_inputs.py
