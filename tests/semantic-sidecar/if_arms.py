"""Project: FreeBASIC semantic sidecar tests
File: if_arms.py
Purpose: Exercise accepted IF arm roles independently of source spans.
Responsibilities: Exact counts, nested grammar, encodings and malformed facts.
This file intentionally does NOT establish linter statement equivalence.
"""
from sidecar import Model, unescape
from semantic_if_arms import PREFIXES

FEATURE = 'if-arm-inputs'
BODY = '''Dim flag As Integer
Dim total As Integer
If flag Then
    total += 1
    total += 2
ElseIf Not flag Then
    total += 3
Else
    total += 4
End If
If flag Then total = 1 Else total = 1
If flag Then If flag Then total = 2 Else total = 3
If flag Then Else total = 4
If flag Then total = 5 : total += 1 Else total = 5 : total += 1
If flag Then
#Include "if-arm-header.bi"
    total = 6
End If
If flag Goto acceptedLabel
acceptedLabel:
#Macro ARM_HEADER()
    If flag Then total = 7 Else total = 8
#EndMacro
ARM_HEADER()
#Define FIRST_HEADER If
#Define NEXT_HEADER ElseIf
FIRST_HEADER flag Then
    total = 9
NEXT_HEADER total Then
    total = 10
Else
    total = 11
End If
If flag _
    Then total = 12 Else total = 13
If False Then
    total = 14
Else
    total = 15
End If
#If 0
If flag Then total = 16 Else total = 17
#EndIf
#Line 800 "if-arm-remap.bas"
If flag Then total = 18 Else total = 19
'''
# This is parser order, reviewed from BODY. The outer nested IF owns one
# statement; the dangling ELSE belongs to the inner IF only.
EXPECTED = [('block', [('then', 2), ('elseif', 1), ('else', 1)]),
            ('inline', [('then', 1), ('else', 1)]),
            ('inline', [('then', 1)]), ('inline', [('then', 1), ('else', 1)]),
            ('inline', [('then', 0), ('else', 1)]),
            ('inline', [('then', 2), ('else', 2)]),
            ('block', [('then', 0), ('else', 1)]),
            ('inline', [('then', 1)]),
            ('inline', [('then', 1), ('else', 1)]),
            ('block', [('then', 1), ('elseif', 1), ('else', 1)]),
            ('inline', [('then', 1), ('else', 1)]),
            ('block', [('then', 1), ('else', 1)]),
            ('inline', [('then', 1), ('else', 1)])]


def source_text(body, filename='if-arms.bas', dialect='fb'):
    return ("' Project: FreeBASIC semantic sidecar tests\n' File: " + filename + "\n"
            "' Purpose: Exercise accepted IF arm ownership.\n"
            "' Responsibilities: Roles, members and original opening tokens.\n"
            "' This file intentionally does NOT run the source program.\n"
            '#Lang "' + dialect + '"\n' + body + "' end of " + filename + '\n')


def fixture(test, filename='if-arms.bas'):
    test.source(source_text('Else\n', 'if-arm-header.bi'), 'if-arm-header.bi')
    return test.source(source_text(BODY, filename), filename)


def reviewed(model):
    result = []
    for row in model.records['K']:
        if not row[3].startswith('if-construct:'):
            continue
        identity = row[3].split(':')[1]
        properties = model.properties['symbol', int(row[2])]
        count = int(properties['if-end:' + identity].split('\t')[0])
        result.append((row[4].split('\t')[1], [
            (properties['if-arm:' + identity + ':' + str(ordinal)].split('\t')[0],
             int(properties['if-arm-end:' + identity + ':' + str(ordinal)]))
            for ordinal in range(1, count + 1)]))
    return result


def check_inputs(test):
    for backend in test.backends:
        with test.subTest(backend=backend):
            source = fixture(test)
            output = test.working / ('if-arms.c' if backend == 'gcc' else 'if-arms.asm')
            extra = ('-o', str(output))
            test.invoke([source], mode='off', backend=backend, extra=extra)
            plain = output.read_bytes()
            model = test.compile(source, backend=backend, extra=extra)
            test.assertEqual(output.read_bytes(), plain)
            test.assertEqual(reviewed(model), EXPECTED)
            test.assertEqual(sum(row[3].startswith('if-arm-transfer:') for row in model.records['K']), 0)
            test.assertTrue(any(row[1] == 'construct' and row[4].startswith('if-arm:') for row in model.records['MR']))
            for mode in ('bindings', 'expressions'):
                compact = test.compile(source, mode=mode, backend=backend, extra=extra)
                test.assertEqual(output.read_bytes(), plain)
                test.assertTrue(all(features[FEATURE] == 'unavailable' for features in compact.capabilities.values()))
                test.assertFalse(any(row[3].startswith(PREFIXES) for row in compact.records['K']))


def check_legacy(test):
    for dialect in ('deprecated', 'fblite', 'qb'):
        for backend in test.backends:
            with test.subTest(dialect=dialect, backend=backend):
                body = ('Dim flag As Integer\nIf flag Then flag = 1 Else flag = 2\n'
                        'If flag Then\nflag = 3\nElseIf 1 Then\nflag = 4\nEnd If\n'
                        'If flag Then 100 Else 200\n100:\n200:\n')
                model = test.compile(test.source(source_text(body, dialect=dialect)), backend=backend)
                test.assertEqual(reviewed(model), [('inline', [('then', 1), ('else', 1)]),
                                                  ('block', [('then', 1), ('elseif', 1)]),
                                                  ('inline', [('then', 1), ('else', 1)])])


def check_encodings(test):
    encodings = [('utf8', 'utf-8', b''), ('utf8bom', 'utf-8', b'\xef\xbb\xbf'),
                 ('utf16le', 'utf-16-le', b'\xff\xfe'), ('utf16be', 'utf-16-be', b'\xfe\xff'),
                 ('utf32le', 'utf-32-le', b'\xff\xfe\0\0'), ('utf32be', 'utf-32-be', b'\0\0\xfe\xff')]
    line = 'Print "λ🙂": If flag Then total = 1 Else total = 1'
    text = source_text('Dim flag As Integer\nDim total As Integer\n' + line + '\n')
    for target, backend in (('win64', 'gas64'), ('linux-x86', 'gas')):
        for name, codec, bom in encodings:
            with test.subTest(target=target, encoding=name):
                source = test.working / 'if-arms.bas'
                source.write_bytes(bom + text.encode(codec))
                model = test.compile(source, backend=backend, extra=('-target', target))
                test.assertEqual(reviewed(model), [('inline', [('then', 1), ('else', 1)])])
                site = next(row for row in model.records['LOC'] if row[1] == 'construct' and row[3] == 'if-arm:2')
                test.assertEqual(site[11], 'mapped')
                test.assertEqual(int(site[5]), 9)
                test.assertEqual(int(site[6]), len(line[:line.index('Else')].encode('utf-16-le')) // 2)
                test.assertEqual(source.read_bytes()[int(site[9]):int(site[10])], 'Else'.encode(codec))


MUTATIONS = ('missing-mode', 'missing-mode-marker', 'missing-mode-pair', 'missing-arm', 'missing-arm-marker',
             'missing-close', 'missing-end', 'missing-member', 'missing-member-marker', 'missing-member-pair',
             'missing-location', 'unavailable', 'partial', 'wrong-mode', 'wrong-header', 'wrong-role',
             'wrong-member-owner', 'wrong-member-construct', 'wrong-member-arm', 'wrong-member-ordinal',
             'wrong-arm-count', 'negative-count', 'huge-count', 'wrong-else-count', 'duplicate-mode',
             'duplicate-marker', 'wrong-transfer-target', 'wrong-marker-domain', 'noncanonical-key')


def malformed_rows(rows, mutation):
    changed = [row[:] for row in rows]
    mode = next(row for row in changed if row[0] == 'K' and row[3].startswith('if-construct:'))
    arm = next(row for row in changed if row[0] == 'K' and row[3].startswith('if-arm:'))
    close = next(row for row in changed if row[0] == 'K' and row[3].startswith('if-arm-end:'))
    end = next(row for row in changed if row[0] == 'K' and row[3].startswith('if-end:'))
    member = next(row for row in changed if row[0] == 'K' and row[3].startswith('if-arm-statement:'))
    marker = next(row for row in changed if row[0] == 'H' and row[5] == 'if-construct')
    member_marker = next(row for row in changed if row[0] == 'H' and row[5] == 'if-arm-statement')
    if mutation in ('missing-mode', 'missing-mode-pair'): mode[3] = 'fixture-missing-mode'
    if mutation in ('missing-mode-marker', 'missing-mode-pair'): marker[5] = 'fixture-missing-mode-marker'
    if mutation == 'missing-arm': arm[3] = 'fixture-missing-arm'
    if mutation == 'missing-arm-marker': next(row for row in changed if row[0] == 'H' and row[5] == 'if-arm:1')[5] = 'fixture-missing-arm-marker'
    if mutation == 'missing-close': close[3] = 'fixture-missing-close'
    if mutation == 'missing-end': end[3] = 'fixture-missing-end'
    if mutation in ('missing-member', 'missing-member-pair'): member[3] = 'fixture-missing-member'
    if mutation in ('missing-member-marker', 'missing-member-pair'): member_marker[5] = 'fixture-missing-member-marker'
    if mutation == 'missing-location': next(row for row in changed if row[0] == 'LOC' and row[1] == 'construct' and row[3] == 'if-arm:1')[3] = 'fixture-missing-location'
    if mutation in ('unavailable', 'partial'): next(row for row in changed if row[0] == 'CAP' and row[2] == FEATURE)[3] = mutation
    if mutation == 'wrong-mode': mode[4] = mode[4].split('%09')[0] + '%09unknown'
    if mutation == 'wrong-header': mode[4] = '0%09block'
    if mutation == 'wrong-role': arm[4] = 'else%090'
    if mutation == 'wrong-member-owner': member[2] = next(row[1] for row in changed if row[0] == 'S' and row[3] != '3')
    if mutation in ('wrong-member-construct', 'wrong-member-arm', 'wrong-member-ordinal'):
        fields = member[4].split('%09')
        fields[{'wrong-member-construct': 0, 'wrong-member-arm': 1, 'wrong-member-ordinal': 2}[mutation]] = '999999'
        member[4] = '%09'.join(fields)
    if mutation == 'wrong-arm-count': close[4] = '0'
    if mutation == 'negative-count': close[4] = '-1'
    if mutation == 'huge-count': end[4] = '999999999999%090'
    if mutation == 'wrong-else-count': end[4] = end[4].split('%09')[0] + '%090'
    if mutation == 'duplicate-mode': next(row for row in changed if row[0] == 'K' and not row[3].startswith(PREFIXES))[:] = mode
    if mutation == 'duplicate-marker': next(row for row in changed if row[0] == 'H' and row[5] != 'if-construct')[:] = marker
    if mutation == 'wrong-transfer-target': next(row for row in changed if row[0] == 'K' and row[3].startswith('if-arm-transfer:'))[4] = mode[2]
    if mutation == 'wrong-marker-domain': marker[3] = 'expression'
    if mutation == 'noncanonical-key': mode[3] = 'if-construct:0' + mode[3].split(':')[1]
    return changed


def check_rejection(test):
    for backend in test.backends:
        _, artifact = test.invoke([fixture(test)], backend=backend)
        rows = [line.split('\t') for line in artifact.read_text().splitlines()]
        numeric = test.source(source_text('Dim flag As Integer\nIf flag Then flag = 1 Else flag = 2\n'
                                          'If flag Then 100 Else 200\n100:\n200:\n',
                                          'if-arm-numeric.bas', 'fblite'), 'if-arm-numeric.bas')
        _, numeric_artifact = test.invoke([numeric], backend=backend)
        numeric_rows = [line.split('\t') for line in numeric_artifact.read_text().splitlines()]
        for mutation in MUTATIONS:
            with test.subTest(backend=backend, mutation=mutation):
                changed = malformed_rows(numeric_rows if mutation == 'wrong-transfer-target' else rows, mutation)
                with test.assertRaises(ValueError):
                    Model('\n'.join('\t'.join(row) for row in changed) + '\n')


def check_module_ownership(test):
    for backend in test.backends:
        first, second = fixture(test, 'first.bas'), fixture(test, 'second.bas')
        _, artifact = test.invoke([first, second], backend=backend)
        Model.read(artifact)
        rows = [line.split('\t') for line in artifact.read_text().splitlines()]
        second_module = next(index for index, row in enumerate(rows) if row[0] == 'M' and index > 5)
        member = next(row for row in rows[:second_module] if row[0] == 'K' and row[3].startswith('if-arm-statement:'))
        transplanted = member[:]
        member[3] = 'fixture-removed-arm-member'
        next(row for row in rows[second_module:] if row[0] == 'K' and not row[3].startswith(PREFIXES))[:] = transplanted
        with test.assertRaises(ValueError):
            Model('\n'.join('\t'.join(row) for row in rows) + '\n')

# end of if_arms.py
