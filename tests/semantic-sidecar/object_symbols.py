"""Project: FreeBASIC compiler observations
File: object_symbols.py
Purpose: Verify native emission-owned procedure object symbol observations.
Responsibilities: Source identity, unknown states, module isolation and emission.
This file intentionally does NOT infer whether a procedure resolves at link time.
"""
from sidecar import Model


def source_text(body, name):
    return ("' Project: FreeBASIC semantic sidecar tests\n"
            "' File: " + name + "\n"
            "' Purpose: Exercise native object names and deliberately unknown states.\n"
            "' Responsibilities: Source procedures and procedure-pointer controls.\n"
            "' This file intentionally does NOT execute linked code.\n"
            '#lang "fb"\n\n' + body + "\n' end of " + name + '\n')


def check_observations(test):
    body = ('Declare Function ExternalNative Cdecl Alias "native_external_symbol"() As Long\n'
            'Declare Function UnusedNative() As Long\n'
            'Type Callback As Function() As Long\n'
            '#if 0\nDeclare Function InactiveNative() As Long\n#endif\n'
            'Function InternalNative() As Long\nReturn 23\nEnd Function\n'
            'Print ExternalNative(), InternalNative()\n')
    for backend in test.backends:
        with test.subTest(backend=backend):
            source = test.source(source_text(body, 'object-symbols.bas'), 'object-symbols.bas')
            model = test.compile(source, backend=backend)
            test.assertEqual(model.capabilities[1]['procedure-object-symbol-observations'], 'available')
            active = [test.one(model, name, 'procedure') for name in ('ExternalNative', 'UnusedNative', 'InternalNative')]
            for identity in active:
                properties = model.properties['symbol', identity]
                if backend in ('gas', 'gas64'):
                    expected = 'unobserved' if identity == active[1] else 'observed'
                    test.assertEqual(properties['procedure-object-symbol-state'], expected)
                    test.assertEqual(bool(properties['procedure-object-symbol']), expected == 'observed')
                else:
                    test.assertEqual(properties['procedure-object-symbol-state'], 'unsupported-backend')
                    test.assertEqual(properties['procedure-object-symbol'], '')
            test.assertEqual(model.named('InactiveNative'), [])
            for identity, signature in model.signatures.items():
                if signature[2] == 'procedure-pointer':
                    test.assertNotIn('procedure-object-symbol-state', model.properties['symbol', identity])
            for identity, detail in model.types.items():
                if detail[18] == 'compiler':
                    test.assertNotIn('procedure-object-symbol-state', model.properties['symbol', identity])
            # The export must not allocate backend identifiers or otherwise
            # affect code generation. Reuse one output across all four modes.
            suffix = {'gcc': '.c', 'clang': '.c', 'llvm': '.ll', 'gas': '.asm', 'gas64': '.asm'}[backend]
            output = test.working / ('object-symbol-emission' + suffix)
            extra = ('-o', str(output))
            test.invoke([source], mode='off', backend=backend, extra=extra)
            baseline = output.read_bytes()
            for mode in ('full', 'bindings', 'expressions'):
                _, artifact = test.invoke([source], mode=mode, backend=backend, extra=extra)
                test.assertEqual(output.read_bytes(), baseline, (backend, mode))
                if mode != 'full':
                    compact = Model.read(artifact, bindings_only=mode == 'bindings', expressions_only=mode == 'expressions')
                    test.assertEqual(compact.capabilities[1]['procedure-object-symbol-observations'], 'unavailable')
                    test.assertFalse(any(row[0] == 'K' and row[3].startswith('procedure-object-symbol') for row in compact.rows))


def check_modules(test):
    for backend in test.backends:
        with test.subTest(backend=backend):
            sources = [test.source(source_text('Declare Function ' + name +
                       ' Cdecl Alias "native_shared_symbol"() As Long\nPrint ' + name + '()', filename), filename)
                       for name, filename in (('FirstNative', 'first.bas'), ('SecondNative', 'second.bas'))]
            _, artifact = test.invoke(sources, backend=backend)
            model = Model.read(artifact)
            test.assertEqual(len(model.capabilities), 2)
            test.assertTrue(all(features['procedure-object-symbol-observations'] == 'available'
                                for features in model.capabilities.values()))
            first = test.one(model, 'FirstNative', 'procedure')
            second = test.one(model, 'SecondNative', 'procedure')
            test.assertNotEqual(first, second)
            expected = 'observed' if backend in ('gas', 'gas64') else 'unsupported-backend'
            test.assertEqual(model.properties['symbol', first]['procedure-object-symbol-state'], expected)
            test.assertEqual(model.properties['symbol', second]['procedure-object-symbol-state'], expected)
            test.assertEqual(model.properties['symbol', first]['procedure-object-symbol'],
                             model.properties['symbol', second]['procedure-object-symbol'])

# end of object_symbols.py
