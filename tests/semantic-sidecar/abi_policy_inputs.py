"""Project: FreeBASIC semantic sidecar tests
File: abi_policy_inputs.py
Purpose: Verify original convention and managed-string header choices.
Responsibilities: Target-independent receipts, compact exclusions and corruption.
This file intentionally does NOT infer interoperability policy from ABI defaults.
"""
from sidecar import Model

FEATURE = 'procedure-abi-policy-inputs'
MUTATIONS = ('missing-policy', 'missing-marker', 'noncanonical-key',
             'zero-identity', 'foreign-header', 'invalid-convention',
             'invalid-string', 'extra-flag', 'unavailable-capability',
             'wrong-marker-owner', 'wrong-marker-target', 'wrong-marker-statement',
             'noncanonical-marker', 'orphan-marker')


def malformed_rows(rows, mutation):
    changed = [row.copy() for row in rows]
    policy = next(row for row in changed if row[0] == 'K' and row[3].startswith('abi-policy-input:'))
    marker = next(row for row in changed if row[0] == 'H' and row[5] == policy[3])
    identity = policy[3].split(':')[1]
    if mutation == 'missing-policy': policy[3] = 'fixture-removed-policy'
    if mutation == 'missing-marker': marker[5] = 'fixture-removed-marker'
    if mutation == 'noncanonical-key': policy[3] = 'abi-policy-input:0' + identity
    if mutation == 'zero-identity': policy[3] = 'abi-policy-input:0'
    if mutation == 'foreign-header': policy[3] = 'abi-policy-input:999999'
    if mutation == 'invalid-convention': policy[4] = '2%090'
    if mutation == 'invalid-string': policy[4] = '0%09-1'
    if mutation == 'extra-flag': policy[4] = '0%090%090'
    if mutation == 'unavailable-capability': next(row for row in changed if row[0] == 'CAP' and row[2] == FEATURE)[3] = 'unavailable'
    if mutation == 'wrong-marker-owner': marker[2] = '999999'
    if mutation == 'wrong-marker-target': marker[4] = '999999'
    if mutation == 'wrong-marker-statement': marker[6] = '0'
    if mutation == 'noncanonical-marker': marker[5] = 'abi-policy-input:0' + identity
    if mutation == 'orphan-marker': marker[5] = 'abi-policy-input:999999'
    return changed


def check_policy_inputs(test):
    source = test.source('''Type ManagedAlias As String
Declare Sub ImplicitScalar(ByVal value As Long)
Declare Sub ExplicitScalar Cdecl(ByVal value As Long)
Declare Sub ManagedParameter(ByRef value As ManagedAlias)
Declare Function ManagedResult() As String
Declare Sub ManagedPointer(ByVal value As ManagedAlias Ptr)
Declare Sub CallbackHeader(ByVal callback As Sub Cdecl())
''', 'abi-policy.bas')
    expected = {'implicitscalar': ['0', '0'], 'explicitscalar': ['1', '0'],
                'managedparameter': ['0', '1'], 'managedresult': ['0', '1'],
                'managedpointer': ['0', '1'], 'callbackheader': ['0', '0']}
    for backend in test.backends:
        with test.subTest(backend=backend):
            emitted = test.emission_path('abi-policy', backend)
            extra = ('-o', str(emitted))
            test.invoke([source], mode='off', backend=backend, extra=extra)
            baseline = emitted.read_bytes()
            _, artifact = test.invoke([source], backend=backend, extra=extra)
            model = Model.read(artifact)
            test.assertEqual(emitted.read_bytes(), baseline)
            test.assertEqual(model.capabilities[1]['procedure-abi-policy-inputs'], 'available')
            policies = [row for row in model.records['K'] if row[3].startswith('abi-policy-input:')]
            actual = {model.types[int(row[2])][2].lower(): row[4].split('\t') for row in policies}
            test.assertEqual(actual, expected)
            for mode in ('bindings', 'expressions'):
                compact = test.compile(source, mode=mode, backend=backend, extra=extra)
                test.assertEqual(emitted.read_bytes(), baseline)
                test.assertEqual(compact.capabilities[1]['procedure-abi-policy-inputs'], 'unavailable')
                test.assertFalse(any(row[3].startswith('abi-policy-input:') for row in compact.records['K']))
            rows = [line.split('\t') for line in artifact.read_text().splitlines()]
            for mutation in MUTATIONS:
                with test.subTest(mutation=mutation), test.assertRaises(ValueError):
                    Model('\n'.join('\t'.join(row) for row in malformed_rows(rows, mutation)) + '\n')
            # FBLITE's default convention depends on the target. Fix the
            # Windows target so the implicit and written signatures agree.
            target = 'win64' if backend == 'gas64' else 'win32'
            repeated = test.source('#lang "fblite"\nDeclare Sub Repeated(ByVal value As Long)\n'
                                   'Declare Sub Repeated Stdcall(ByVal value As Long)\n', 'abi-repeat.bas')
            repeated_model = test.compile(repeated, backend=backend, extra=('-target', target))
            policies = [row for row in repeated_model.records['K'] if row[3].startswith('abi-policy-input:')]
            test.assertEqual([row[4].split('\t') for row in policies], [['0', '0'], ['1', '0']])
            test.assertEqual(len({row[2] for row in policies}), 1)
            for option, convention in (('no-fastcall', '__fastcall'), ('no-thiscall', '__thiscall')):
                ignored = test.source('Declare Sub ConventionProbe ' + convention + '(ByVal value As Long)\n', option + '.bas')
                ignored_model = test.compile(ignored, backend=backend, extra=('-target', target, '-z', option))
                policy = next(row for row in ignored_model.records['K'] if row[3].startswith('abi-policy-input:'))
                test.assertEqual(policy[4].split('\t'), ['1', '0'])
# end of abi_policy_inputs.py
