"""Project: FreeBASIC semantic sidecar tests
File: abi_policy_inputs.py
Purpose: Verify original convention and managed-string header choices.
Responsibilities: Target-independent receipts, compact exclusions and corruption.
This file intentionally does NOT infer interoperability policy from ABI defaults.
"""
from sidecar import Model


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
            for flags in ('2%090', '0%09-1'):
                malformed = [row.copy() for row in rows]
                next(row for row in malformed if row[0] == 'K' and row[3].startswith('abi-policy-input:'))[4] = flags
                with test.assertRaises(ValueError):
                    Model('\n'.join('\t'.join(row) for row in malformed) + '\n')
            malformed = [row.copy() for row in rows]
            next(row for row in malformed if row[0] == 'H' and row[5].startswith('abi-policy-input:'))[6] = '0'
            with test.assertRaises(ValueError):
                Model('\n'.join('\t'.join(row) for row in malformed) + '\n')
# end of abi_policy_inputs.py
