"""Project: FreeBASIC compiler observations
File: callback_conventions.py
Purpose: Exercise original anonymous callback conventions at native boundaries.
Responsibilities: Interned types, nested signatures, imports and reporting sites.
This file intentionally does NOT infer a callback convention from BASIC text.
"""
CASES = (
    ('library', 'fb', 'Declare Sub SetCallback Lib "callback_fixture"(ByVal cb As Function() As Long)', ('Function()',)),
    ('alias', 'fb', 'Declare Sub SetCallback Alias "foreign_set"(ByVal cb As Sub())', ('Sub()',)),
    ('outer-cdecl', 'fb', 'Declare Sub SetCallback Cdecl Alias "foreign_set"(ByVal cb As Function() As Long)', ('Function()',)),
    ('explicit-cdecl', 'fb', 'Declare Sub SetCallback Alias "foreign_set"(ByVal cb As Function Cdecl() As Long)', ()),
    ('explicit-stdcall', 'fb', 'Declare Sub SetCallback Alias "foreign_set"(ByVal cb As Function Stdcall() As Long)', ()),
    ('explicit-pascal', 'fb', 'Declare Sub SetCallback Alias "foreign_set"(ByVal cb As Function Pascal() As Long)', ()),
    ('explicit-fastcall', 'fb', 'Declare Sub SetCallback Alias "foreign_set"(ByVal cb As Function __fastcall(ByVal value As Long) As Long)', ()),
    ('plain', 'fb', 'Declare Sub SetCallback(ByVal cb As Function() As Long)', ()),
    ('plain-cdecl', 'fb', 'Declare Sub SetCallback Cdecl(ByVal cb As Function() As Long)', ()),
    ('ordinary-pointer', 'fb', 'Type Record\nvalue As Long\nEnd Type\nDeclare Sub SetCallback Lib "callback_fixture"(ByVal cb As Record Ptr)', ()),
    ('named-callback', 'fb', 'Type Handler As Function() As Long\nDeclare Sub SetCallback Lib "callback_fixture"(ByVal cb As Handler)', ()),
    ('named-explicit', 'fb', 'Type Handler As Function Cdecl() As Long\nDeclare Sub SetCallback Lib "callback_fixture"(ByVal cb As Handler)', ()),
    ('interned-explicit-first', 'fb', 'Type Handler As Function Stdcall() As Long\nDeclare Sub SetCallback Alias "foreign_set"(ByVal cb As Function() As Long)', ('cb As Function()',)),
    ('interned-explicit-cdecl-first', 'fb', 'Type Handler As Function Cdecl() As Long\nDeclare Sub SetCallback Alias "foreign_set"(ByVal cb As Function() As Long)', ('cb As Function()',)),
    ('interned-implicit-first', 'fb', 'Type Handler As Function() As Long\nDeclare Sub SetCallback Alias "foreign_set"(ByVal cb As Function Stdcall() As Long)', ()),
    ('nested', 'fb', 'Declare Sub SetCallback Alias "foreign_set"(ByVal cb As Function(ByVal inner As Sub()) As Long)', ('cb As Function(', 'inner As Sub()')),
    ('nested-inner', 'fb', 'Declare Sub SetCallback Alias "foreign_set"(ByVal cb As Function Cdecl(ByVal inner As Sub()) As Long)', ('inner As Sub()',)),
    ('nested-outer', 'fb', 'Declare Sub SetCallback Alias "foreign_set"(ByVal cb As Function(ByVal inner As Sub Cdecl()) As Long)', ('cb As Function(',)),
    ('result', 'fb', 'Declare Function GetCallback Alias "foreign_get"() As Function() As Long', ('Function()',)),
    ('result-explicit', 'fb', 'Declare Function GetCallback Alias "foreign_get"() As Function Cdecl() As Long', ()),
    ('extern-c', 'fb', 'Extern "C"\nDeclare Sub SetCallback(ByVal cb As Function() As Long)\nEnd Extern', ('Function()',)),
    ('extern-c-explicit', 'fb', 'Extern "C"\nDeclare Sub SetCallback(ByVal cb As Function Cdecl() As Long)\nEnd Extern', ()),
    ('keyword-name', 'fb', 'Declare Sub SetCallback Lib "callback_fixture"(ByVal cdeclValue As Long, ByVal cb As Function() As Long)', ('Function()',)),
    ('local-storage', 'fb', 'Dim cb As Function() As Long', ()),
    ('field-storage', 'fb', 'Type Record\ncb As Function() As Long\nEnd Type', ()),
    ('definition', 'fb', 'Sub SetCallback Alias "foreign_set"(ByVal cb As Function() As Long)\nEnd Sub', ()),
    ('inactive', 'fb', '#if 0\nDeclare Sub SetCallback Alias "foreign_set"(ByVal cb As Function() As Long)\n#endif', ()),
    ('continued', 'fb', 'Declare Sub SetCallback Alias "foreign_set"( _\nByVal cb As _\nFunction() As Long)', ('Function()',)),
    ('macro-callback', 'fb', '#define CallbackType Function() As Long\nDeclare Sub SetCallback Alias "foreign_set"(ByVal cb As CallbackType)', ('cb As CallbackType',)),
    ('macro-header', 'fb', '#define NativeHeader Declare Sub SetCallback Alias "foreign_set"(ByVal cb As Function() As Long)\nNativeHeader', ('NativeHeader',)),
    ('remapped', 'fb', '#line 900\nDeclare Sub SetCallback Alias "foreign_set"(ByVal cb As Function() As Long)', ('Function()',)),
    ('legacy-repeat', 'fblite', 'Declare Sub SetCallback Alias "foreign_set"(ByVal cb As Function() As Long)\nDeclare Sub SetCallback Alias "foreign_set"(ByVal cb As Function() As Long)', ('Function()', 'Function()')),
)

EMPTY_CASES = (
    ('empty-fastcall', 'fb', 'Declare Sub SetCallback Alias "foreign_set"(ByVal cb As Function __fastcall() As Long)', ()),
    ('empty-thiscall', 'fb', 'Declare Sub SetCallback Alias "foreign_set"(ByVal cb As Function __thiscall() As Long)', ()),
)

INDIRECT_CASES = (
    ('indirect-sub', 'fb', 'Sub Sink()\nEnd Sub\nDim p As Any Ptr = @Sink\nCast(Sub(), p)()', ()),
    ('indirect-nested-sub', 'fb', 'Sub Sink(ByVal cb As Function() As Long)\nEnd Sub\n'
        'Function Callback() As Long\nReturn 1\nEnd Function\n'
        'Dim p As Any Ptr = @Sink\nCast(Sub(ByVal cb As Function() As Long), p)(@Callback)', ()),
)


def source_text(language, code):
    return ("' Project: FreeBASIC native callback conventions\n"
            "' File: generated callback convention case\n"
            "' Purpose: Exercise original callback type grammar at native boundaries.\n"
            "' Responsibilities: This case's explicit convention and ownership checks.\n"
            "' This file intentionally does NOT call imported procedures.\n"
            f'#lang "{language}"\n{code}\n'
            "' end of generated callback convention case\n")


def expected_points(text, anchors):
    # Markers locate sites only. Their associated native proof is asserted from
    # compiler K/F/G observations; the production linter never reads these markers.
    used, points = set(), []
    for anchor in anchors:
        candidates = [(line, source.index(anchor)) for line, source in enumerate(text.splitlines(), 1)
                      if anchor in source and line not in used and not source.startswith('#define')]
        assert candidates, anchor
        line, column = candidates[0]
        if anchors.count(anchor) > 1:
            used.add(line)
        if anchor.startswith(('cb As ', 'inner As ')):
            column += len(anchor.split(' As ')[0]) + 4
        points.append((line, column))
    return sorted(points)

def check_callbacks(test):
    for backend, target in (('gas', 'win32'), ('gas64', 'win64'), ('gcc', 'linux-x86_64')):
        for label, language, code, anchors in CASES + EMPTY_CASES + INDIRECT_CASES:
            with test.subTest(backend=backend, target=target, case=label):
                source = test.source(source_text(language, code), label + '.bas')
                output = test.working / ('callback.c' if backend == 'gcc' else 'callback.asm')
                extra = ('-target', target, '-o', str(output))
                test.invoke([source], mode='off', backend=backend, extra=extra)
                original = output.read_bytes()
                model = test.compile(source, backend=backend, extra=extra)
                test.assertEqual(output.read_bytes(), original)
                test.assertEqual(model.capabilities[1]['procedure-callback-inputs'], 'available')
                callbacks = [row for row in model.records['K'] if row[3].startswith('callback-convention-input:')]
                headers = [row for row in model.records['K'] if row[3].startswith('procedure-abi-input:')]
                for row in callbacks:
                    statement, explicit, context = row[4].split('\t')
                    test.assertEqual(model.signatures[int(row[2])][2], 'procedure-pointer')
                    test.assertIn(explicit, ('0', '1'))
                    test.assertEqual(model.statements[int(statement)][5], context)
                    test.assertTrue(any(location[3] == row[3] for location in model.records['LOC'])
                                    or ('symbol', int(row[2]), row[3]) in model.macro_origins)
                for row in headers:
                    fields = row[4].split('\t')
                    test.assertEqual(int(fields[5]), sum(entry[4].split('\t')[0] == fields[0] for entry in callbacks))
                if label in ('interned-explicit-first', 'interned-explicit-cdecl-first'):
                    test.assertEqual(len(callbacks), 2)
                    test.assertEqual([row[4].split('\t')[1] for row in callbacks], ['1', '0'])
                    same_mode = model.signatures[int(callbacks[0][2])][3] == model.signatures[int(callbacks[1][2])][3]
                    test.assertEqual(callbacks[0][2] == callbacks[1][2], same_mode)
                if label == 'nested':
                    test.assertEqual(len(callbacks), 2)
                    test.assertEqual([row[4].split('\t')[1] for row in callbacks], ['0', '0'])
                    test.assertEqual(headers[0][4].split('\t')[5], '2')
                if label == 'continued':
                    location = next(row for row in model.records['LOC'] if row[3] == callbacks[0][3])
                    test.assertEqual((int(location[5]), int(location[6])), expected_points(source_text(language, code), anchors)[0])
                    for mode in ('bindings', 'expressions'):
                        compact = test.compile(source, mode=mode, backend=backend, extra=extra)
                        test.assertEqual(output.read_bytes(), original)
                        test.assertEqual(compact.capabilities[1]['procedure-callback-inputs'], 'unavailable')
                        test.assertFalse(any(row[3].startswith(('callback-convention-', 'procedure-abi-'))
                                             for row in compact.records['K']))
        for option, convention in (('no-fastcall', '__fastcall'), ('no-thiscall', '__thiscall')):
            with test.subTest(backend=backend, target=target, disabled=option):
                code = ('Declare Sub SetCallback Alias "foreign_set"(ByVal cb As Function '
                        + convention + '(ByVal value As Long) As Long)')
                source = test.source(source_text('fb', code), option + '.bas')
                model = test.compile(source, backend=backend, extra=('-target', target, '-z', option))
                callbacks = [row for row in model.records['K'] if row[3].startswith('callback-convention-input:')]
                test.assertEqual(len(callbacks), 1)
                test.assertEqual(callbacks[0][4].split('\t')[1], '0')


# end of callback_conventions.py
