"""Project: FreeBASIC semantic sidecar tests
File: declaration_observations.py
Purpose: Preserve selected typedefs and exact source receiver observations.
Responsibilities: Independent identities, ABI conversion and native code parity.
This file intentionally does NOT execute negative-index or aliasing subjects.
"""
from sidecar import Model


def check_observations(test):
    for backend in test.backends:
        with test.subTest(backend=backend):
            source = test.fixture('declaration_observations_probe.bas')
            test.invoke([source], mode='off', backend=backend)
            native = test.emission_path(source.stem, backend).read_bytes()
            _, path = test.invoke([source], backend=backend,
                                  extra=('-semantic-type-abi', 'native'))
            model = Model.read(path)
            model.validate_physical_locations()
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), native)
            symbols = {row[1]: row for row in model.records['S']}
            types = {row[1]: row for row in model.records['T']}
            expressions = {row[1]: row for row in model.records['E']}
            properties = {(row[1], row[2], row[3]): row[4] for row in model.records['K']}
            cast_lines = {number for number, line in enumerate(source.read_text().splitlines(), 1)
                          if "' CAST: " in line}
            casts = [row for row in model.records['EX'] if row[2] == 'cast']
            test.assertEqual({int(row[9]) for row in casts}, cast_lines)
            test.assertEqual(len(cast_lines), 3)
            for row in casts:
                expression = expressions[row[1]]
                # GAS64 can lower a wide conversion to a helper result's
                # storage view before export. EX still retains the accepted
                # source CAST; the native E class must describe the real tree.
                if expression[8] == '5':
                    test.assertEqual(expression[9:12], ['-1', 'cast', 'builtin'])
                else:
                    test.assertEqual(backend, 'gas64')
                    test.assertEqual(expression[8:12], ['20', '-1', 'dereference', 'builtin'])
                    primitive = next(item for item in model.records['Y']
                                     if int(item[1]) == int(expression[12]) & 31)
                    test.assertIn(primitive[2], ('integer128', 'uinteger128'))
            initialized = set()
            for row in model.records['K']:
                if not row[3].startswith('assignment-input:'):
                    continue
                payload = row[4].split('\t')
                if payload[2] == 'initializer':
                    initialized.add(symbols[expressions[payload[0]][13]][2].upper())
            test.assertTrue({'WIDEFROMBYTE', 'UNSIGNEDFROMBYTE'} <= initialized)
            aliases = [row for row in model.records['H'] if row[5] == 'type-alias-input']
            test.assertEqual({symbols[row[2]][2].upper(): symbols[row[4]][2].upper()
                              for row in aliases if row[1] == 'symbol'},
                             {'INDEXVALUE': 'NUMERICBYTE', 'FIXEDPOINTER': 'POINTERALIAS'})
            test.assertEqual(len(aliases), 2)
            for row in aliases:
                test.assertEqual(properties[row[1], row[2], 'type-alias-input:' + row[6]], row[4])
                test.assertIn(('symbol', row[2], 'type-alias-declaration-site:' + row[6]), properties)
            strings = [row for row in model.records['H'] if row[5] == 'string-index-input']
            test.assertEqual(len(strings), 1)
            for row in strings:
                test.assertEqual(properties['expression', row[2], 'string-index-operand'], row[4])
                test.assertEqual(properties['expression', row[2], 'string-index-index'], row[6])
            receivers = [row for row in model.records['H'] if row[5] == 'member-receiver-type']
            test.assertTrue(receivers)
            for row in receivers:
                payload = properties['expression', row[2], 'member-receiver-input'].split('\t')
                test.assertEqual(payload[1:3], [row[4], row[6]])
                test.assertEqual(types[row[4]][2].upper(), 'DERIVEDRECORD')
                test.assertEqual(types[row[6]][2].upper(), 'OTHERVALUE')
                test.assertEqual(int(payload[0]) & 31, int(types[row[4]][6]) & 31)
            for feature in ('selected-type-alias-inputs', 'string-subscript-inputs',
                            'type-alias-declaration-sites', 'member-receiver-inputs'):
                test.assertEqual(model.capabilities[1][feature], 'available')
            # The wire ABI is selectable. Compare the new payload against the
            # independently exported T fact instead of copying native enum slots.
            _, stable = test.invoke([source], backend=backend,
                                    extra=('-semantic-type-abi', 'stable-v1'))
            rows = [line.split('\t') for line in stable.read_text().splitlines()]
            from sidecar import unescape
            payloads = [unescape(row[4]).split('\t') for row in rows
                        if row[0] == 'K' and row[3] == 'member-receiver-input']
            test.assertTrue(payloads)
            test.assertTrue(all(int(payload[0]) & 31 == 20 for payload in payloads))
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), native)

# end of declaration_observations.py
