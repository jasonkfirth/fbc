"""Project: FreeBASIC semantic sidecar tests
File: call_atoms.py
Purpose: Verify original constant and bound inputs at selected calls.
Responsibilities: Literal identity, virtual targets, module closure and readers.
This file intentionally does NOT recover call semantics from source spelling.
"""
from sidecar import Model

FEATURES = ('parsed-constant-symbols', 'source-call-bindings', 'bound-expression-symbols')
BODY = '''Declare Sub PlainSink(ByVal EnabledFlag As Boolean)
Sub PairSink(ByVal FirstValue As Long, ByVal SecondValue As Long)
End Sub
Sub TextSink(ByRef FirstText As String, ByRef SecondText As String)
End Sub
Const NamedFlag As Boolean = True
Const FirstValue As Long = 1
Const SecondValue As Long = 2
Const FirstText = "first"
Const SecondText = "second"
Dim FlagValue As Boolean
PlainSink(True)
PlainSink(NamedFlag)
PlainSink(CBool(True))
PlainSink(True And False)
PlainSink((False))
PlainSink(FlagValue)
PairSink(SecondValue, FirstValue)
TextSink(SecondText, FirstText)
Type Consumer Extends Object
Declare Virtual Sub MethodSink(ByVal EnabledFlag As Boolean)
End Type
Sub Consumer.MethodSink(ByVal EnabledFlag As Boolean)
End Sub
Dim ConsumerValue As Consumer
ConsumerValue.MethodSink(True)
Sub FromReference(ByRef ReferenceFlag As Boolean)
PlainSink(ReferenceFlag)
End Sub
#Define MACRO_SINK PlainSink(False)
MACRO_SINK
#If 0
PlainSink(True)
#EndIf
'''


def source_text(body):
    return ("' Project: FreeBASIC semantic sidecar tests\n' File: call-atoms.bas\n"
            "' Purpose: Exercise original selected call inputs.\n"
            "' Responsibilities: Constant, variable, reference and virtual inputs.\n"
            "' This file intentionally does NOT execute its calls.\n#Lang \"fb\"\n"
            + body + "' end of call-atoms.bas\n")


def check_atoms(test):
    for backend in test.backends:
        with test.subTest(backend=backend):
            source = test.source(source_text(BODY))
            output = test.working / ('call-atoms.c' if backend == 'gcc' else 'call-atoms.asm')
            extra = ('-o', str(output))
            test.invoke([source], mode='off', backend=backend, extra=extra)
            original = output.read_bytes()
            model = test.compile(source, backend=backend, extra=extra)
            test.assertEqual(output.read_bytes(), original)
            test.assertTrue(all(model.capabilities[1][feature] == 'available' for feature in FEATURES))
            constants = {model.symbols[int(properties['constant-symbol'])][2].upper(): properties['constant-atom-kind']
                         for (domain, identity), properties in model.properties.items() if 'constant-symbol' in properties}
            test.assertEqual(constants['TRUE'], 'boolean-literal')
            test.assertEqual(constants['FALSE'], 'boolean-literal')
            for spelling in ('NAMEDFLAG', 'FIRSTVALUE', 'SECONDVALUE', 'FIRSTTEXT', 'SECONDTEXT'):
                test.assertEqual(constants[spelling], 'named-constant')
            bound = [model.symbols[int(properties['bound-value-symbol'])]
                     for properties in model.properties.values() if 'bound-value-symbol' in properties]
            test.assertTrue(any(symbol[2].upper() == 'REFERENCEFLAG' for symbol in bound))
            test.assertFalse(any(int(symbol[7]) & 0x800 for symbol in bound))
            targets = {int(row[2]): row[4] for row in model.records['H']
                       if row[1] == 'node' and row[3] == 'symbol' and row[5] == 'static-target'}
            observed = set()
            for row in model.records['H']:
                if row[1] != 'node' or row[3] != 'binding' or row[5] != 'source-binding':
                    continue
                node_id = int(row[2])
                node = model.nodes[node_id]
                if node[4] != '9':
                    continue
                kind = model.properties['node', node_id]['call-kind']
                if kind not in ('direct', 'virtual'):
                    continue
                binding = model.records['B'][int(row[4]) - 1]
                test.assertEqual(binding[1], targets[node_id] if kind == 'virtual' else node[9])
                observed.add(kind)
            test.assertEqual(observed, {'direct', 'virtual'})
            for mode in ('bindings', 'expressions'):
                compact = test.compile(source, mode=mode, backend=backend, extra=extra)
                test.assertEqual(output.read_bytes(), original)
                test.assertTrue(all(compact.capabilities[1][feature] == 'unavailable' for feature in FEATURES))
                test.assertFalse(any(key in properties for properties in compact.properties.values()
                                     for key in ('constant-symbol', 'constant-atom-kind', 'bound-value-symbol')))


def check_rejection(test):
    for backend in test.backends:
        source = test.source(source_text(BODY))
        _, artifact = test.invoke([source], backend=backend)
        rows = [line.split('\t') for line in artifact.read_text().splitlines()]
        variable = next(row[1] for row in rows if row[0] == 'S' and row[2].upper() == 'FLAGVALUE')
        for mode in ('missing-kind', 'missing-symbol', 'invalid-symbol', 'wrong-class',
                     'invalid-kind', 'unavailable-constants', 'unavailable-bound', 'wrong-virtual-binding'):
            with test.subTest(backend=backend, mutation=mode):
                changed = [row[:] for row in rows]
                if mode.startswith('unavailable-'):
                    feature = 'parsed-constant-symbols' if mode.endswith('constants') else 'bound-expression-symbols'
                    next(row for row in changed if row[0] == 'CAP' and row[2] == feature)[3] = 'unavailable'
                elif mode == 'wrong-virtual-binding':
                    node_id = next(row[2] for row in changed if row[0] == 'H' and row[5] == 'static-target')
                    relation = next(row for row in changed if row[0] == 'H' and row[1] == 'node' and row[2] == node_id
                                    and row[3] == 'binding' and row[5] == 'source-binding')
                    signature = next(row[9] for row in changed if row[0] == 'N' and row[1] == node_id)
                    [row for row in changed if row[0] == 'B'][int(relation[4]) - 1][1] = signature
                else:
                    key = 'constant-atom-kind' if mode in ('missing-kind', 'invalid-kind') else 'constant-symbol'
                    row = next(row for row in changed if row[0] == 'K' and row[3] == key)
                    if mode.startswith('missing-'):
                        row[3] = 'fixture-unknown-property'
                    else:
                        row[4] = variable if mode == 'wrong-class' else '999999' if mode == 'invalid-symbol' else 'unknown-kind'
                malformed = test.working / 'call-atoms-malformed.sem'
                malformed.write_text('\n'.join('\t'.join(row) for row in changed) + '\n')
                with test.assertRaises(ValueError):
                    Model.read(malformed)
        # Both endpoints can be valid CONST symbols while belonging to different
        # modules. Matching values and spellings must not bridge that boundary.
        second = test.source(source_text('Declare Sub PlainSinkOther(ByVal EnabledFlag As Boolean)\n'
            'Const OtherConstant As Boolean = True\nPlainSinkOther(OtherConstant)\n'), 'other.bas')
        _, artifact = test.invoke([source, second], backend=backend)
        rows = [line.split('\t') for line in artifact.read_text().splitlines()]
        foreign = next(row[1] for row in rows if row[0] == 'S' and row[2].upper() == 'OTHERCONSTANT')
        next(row for row in rows if row[0] == 'K' and row[3] == 'constant-symbol')[4] = foreign
        malformed = test.working / 'call-atoms-foreign.sem'
        malformed.write_text('\n'.join('\t'.join(row) for row in rows) + '\n')
        with test.assertRaises(ValueError):
            Model.read(malformed)

# end of call_atoms.py
