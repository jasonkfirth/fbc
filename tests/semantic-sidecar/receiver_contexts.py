"""Project: FreeBASIC semantic sidecar tests
File: receiver_contexts.py
Purpose: Preserve native field receivers outside language statement ownership.
Responsibilities: Preprocessor queries, ordinary uses and corrupted context refusal.
This file intentionally does NOT classify type queries as runtime reads.
"""

from driver_inputs import text_rows
from sidecar import Model


def check_contexts(test):
    source = test.source('type RecordValue\nfield_value as long\nend type\n'
                         'dim item as RecordValue\n'
                         '#assert typeof(item.field_value) = typeof(long)\n'
                         '#if sizeof(item.field_value) = sizeof(long)\n'
                         'print item.field_value\n#endif\n')
    for backend in test.backends:
        with test.subTest(backend=backend):
            test.invoke([source], mode='off', backend=backend)
            baseline = test.emission_path(source.stem, backend).read_bytes()
            model = test.compile(source, backend=backend)
            properties = [(identity, facts) for (domain, identity), facts in model.properties.items()
                          if domain == 'expression' and 'member-receiver-input' in facts]
            test.assertTrue(any(facts['member-receiver-context'] == 'preprocessor' for _, facts in properties))
            test.assertTrue(any(facts['member-receiver-context'] == 'statement' for _, facts in properties))
            for identity, facts in properties:
                if facts['member-receiver-context'] == 'preprocessor':
                    test.assertEqual(facts['preprocessor-expression'], '1')
                    test.assertEqual(facts['member-receiver-input'].split('\t')[3], '0')
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)
            for mode in ('bindings', 'expressions'):
                reduced = test.compile(source, mode=mode, backend=backend)
                test.assertEqual(reduced.capabilities[1]['member-receiver-contexts'], 'unavailable')
                test.assertFalse(any('member-receiver-input' in facts for facts in reduced.properties.values()))
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)
            for mutation in ('marker', 'context', 'statement', 'field'):
                with test.subTest(mutation=mutation), test.assertRaises(ValueError):
                    rows = [row.copy() for row in model.rows]
                    identity, _ = next((identity, facts) for identity, facts in properties
                                       if facts['member-receiver-context'] == 'preprocessor')
                    key = ['K', 'expression', str(identity)]
                    if mutation == 'marker':
                        rows = [row for row in rows if row[:4] != key + ['preprocessor-expression']]
                    elif mutation == 'context':
                        next(row for row in rows if row[:4] == key + ['member-receiver-context'])[4] = 'statement'
                    else:
                        row = next(row for row in rows if row[:4] == key + ['member-receiver-input'])
                        fields = row[4].split('\t')
                        fields[3 if mutation == 'statement' else 2] = '999999'
                        row[4] = '\t'.join(fields)
                    Model(text_rows(rows))

# end of receiver_contexts.py
