"""Project: FreeBASIC semantic sidecar tests
File: branch_observations.py
Purpose: Preserve exact cleanup flags and grouped built-in power observations.
Responsibilities: Independent source expectations and unchanged native emission.
This file intentionally does NOT execute external-call or destructor subjects.
"""
# Storage payloads have sixteen tab-separated fields. Field 0 is the native
# AST class (9 is CALL), field 4 the selected operation, and field 15 coverage.
# The written probes contain fifteen cleanup controls and ten power observations.


def check_observations(test):
    for backend in test.backends:
        for filename in ('variable_cleanup_probe.bas', 'grouped_power_probe.bas'):
            with test.subTest(backend=backend, source=filename):
                original = test.root / 'tests/semantic-sidecar' / filename
                source = test.source(original.read_text(), filename)
                test.invoke([source], mode='off', backend=backend)
                native = test.emission_path(source.stem, backend).read_bytes()
                # Model's primitive vocabulary follows the compiler's native
                # enum. Select that ABI rather than relying on driver defaults.
                model = test.compile(source, backend=backend,
                                     extra=('-semantic-type-abi', 'native'))
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), native)
                if filename == 'variable_cleanup_probe.bas':
                    expected = {}
                    for line in source.read_text().splitlines():
                        if "' CLEANUP: " in line:
                            for item in line.split("' CLEANUP: ", 1)[1].split():
                                name, value = item.split('=')
                                expected[name.upper()] = value
                    test.assertEqual(len(expected), 15)
                    test.assertEqual(model.capabilities[1]['variable-cleanup-inputs'], 'available')
                    symbols = {row[1]: row for row in model.records['T']}
                    observed = {}
                    for row in model.records['K']:
                        if row[3] != 'automatic-cleanup':
                            continue
                        symbol = symbols[row[2]]
                        test.assertEqual(symbol[3], 'variable')
                        test.assertIn(row[4], ('0', '1'))
                        if symbol[18] == 'source' and symbol[2].upper() in expected:
                            observed.setdefault(symbol[2].upper(), set()).add(row[4])
                    test.assertEqual(observed, {name: {value} for name, value in expected.items()})
                else:
                    powers, families = [], set()
                    for row in model.records['K']:
                        if not any(row[3].startswith(family + '-tree:')
                                   for family in ('comparison', 'logical', 'assignment')):
                            continue
                        fields = row[4].split('\t')
                        if fields[0] != '9':
                            continue
                        if fields[4] == 'power':
                            test.assertEqual(fields[15], 'complete')
                            powers.append(row)
                            families.add(row[3].split('-', 1)[0])
                        else:
                            test.assertEqual(fields[15], 'opaque')
                    test.assertEqual(len(powers), 10)
                    test.assertEqual(families, {'comparison', 'logical', 'assignment'})

# end of branch_observations.py
