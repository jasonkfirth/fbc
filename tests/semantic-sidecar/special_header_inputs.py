"""Project: FreeBASIC semantic sidecar tests
File: special_header_inputs.py
Purpose: Check original ABI sites for members without identifier tokens.
Responsibilities: Exact physical/macro origins and unchanged named bindings/output.
This file intentionally does NOT reconstruct production header locations.
"""
from sidecar import Model

PREFIX = 'procedure-abi-input:'


def check_headers(test):
    fixture = test.fixture('special-headers.bas')
    original = fixture.read_text(encoding='utf-8')
    expected = set()
    for line, text in enumerate(original.splitlines(), 1):
        if "' HEADER: " in text:
            token, role = text.split("' HEADER: ")[1].split()
            start = text.index(token)
            expected.add((line, start, line, start + len(token), role))
    test.assertEqual(len(expected), 17)
    for backend in test.backends:
        for case in ('physical', 'remapped', 'macro'):
            with test.subTest(backend=backend, case=case):
                text = original
                points = expected
                if case == 'remapped':
                    text = '#line 700 "logical-special-headers.bas"\n' + text
                    points = {(a + 1, b, c + 1, d, e) for a, b, c, d, e in expected}
                elif case == 'macro':
                    body = '\n'.join(line for line in original.splitlines() if not line.startswith('#lang'))
                    text = '#lang "fb"\n#macro MakeHeaders()\n' + body + '\n#endmacro\n'
                    text += '#line 700 "logical-special-headers.bas"\nMakeHeaders()\n'
                source = test.source(text, 'special-' + case + '.bas')
                output = test.emission_path(source.stem, backend)
                extra = ('-o', str(output))
                test.invoke([source], mode='off', backend=backend, extra=extra)
                baseline = output.read_bytes()
                _, path = test.invoke([source], backend=backend, extra=extra)
                model = Model.read(path)
                model.validate_physical_locations()
                test.assertEqual(output.read_bytes(), baseline)
                headers = [row for row in model.records['K'] if row[3].startswith(PREFIX)]
                test.assertEqual(len(headers), 17)
                actual, origins, special = set(), set(), set()
                for row in headers:
                    symbol, role = int(row[2]), row[4].split('\t')[1]
                    spelling = model.properties['symbol', symbol]['procedure-declaration-spelling:' + row[3][len(PREFIX):]]
                    if spelling == '':
                        special.add(symbol)
                    if case == 'macro':
                        expansion = model.macro_origins['symbol', symbol, row[3]]
                        origins.add(expansion)
                        location = model.physical_locations['macro-attempt', expansion, 'invocation']
                        test.assertEqual(int(location[5]), len(text.splitlines()))
                    else:
                        source_id = int(row[4].split('\t')[4])
                        location = model.physical_locations['source-context', source_id, row[3]]
                        test.assertEqual(location[11], 'mapped')
                        actual.add((*map(int, location[5:9]), role))
                test.assertEqual(len(special), 7)
                if case == 'macro':
                    test.assertEqual(len(origins), 1)
                else:
                    test.assertEqual(actual, points)
                # A special-member keyword is an ABI header site, not a new
                # identifier declaration that could shift ordinary B ordinals.
                test.assertFalse(any(int(row[1]) in special and row[2] == 'declaration'
                                     for row in model.records['B']))


# end of special_header_inputs.py
