"""Project: FreeBASIC semantic sidecar tests
File: parser_observations.py
Purpose: Verify a common native parser phase across full and reduced models.
Responsibilities: Exact sequences, value digests, omissions and damaged closure.
This file intentionally does NOT reconstruct source events from overlapping ranges.
"""

import hashlib

from driver_inputs import text_rows
from semantic_parser_observations import FEATURE, PREFIX, COUNT
from sidecar import Model


def check_sequences(test):
    source = test.source('#define VALUE 4\n'
                         'enum Flags\nAlpha = 1\nBeta = 2\nend enum\n'
                         'dim n as long = 7\ndim r as long\n'
                         'r = VALUE + n\nr = Alpha or Beta\nr = abs(n) + 1\n'
                         'print r, !"A\\0B"\n')
    expected_digest = hashlib.sha256(b'signed\t7').hexdigest()
    for backend in test.backends:
        test.invoke([source], backend=backend, mode='off')
        baseline = test.emission_path(source.stem, backend).read_bytes()
        snapshots = []
        for mode in ('full', 'expressions'):
            with test.subTest(backend=backend, mode=mode):
                model = test.compile(source, backend=backend, mode=mode)
                test.assertEqual(model.capabilities[1][FEATURE], 'available')
                test.assertTrue(model.parser_observations[1])
                test.assertTrue(any(event[10:] == ('sha256', expected_digest)
                                    for event in model.parser_observations[1]))
                test.assertTrue(all(event[10] in ('', 'sha256') for event in model.parser_observations[1]))
                snapshots.append(model.parser_observations)
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)
                if mode == 'expressions':
                    test.assertFalse(model.records['C'])
                    test.assertFalse(model.records['S'])
                    test.assertFalse(model.records['N'])
        test.assertEqual(snapshots[0], snapshots[1])
        reduced = test.compile(source, backend=backend, mode='bindings')
        test.assertEqual(reduced.capabilities[1][FEATURE], 'unavailable')
        test.assertFalse(any(row[3] == COUNT or row[3].startswith(PREFIX) for row in reduced.records['K']))


def check_rejection(test):
    model = test.compile(test.source('dim n as long = 7\nprint n + 2\n'))
    Model(text_rows(model.rows))
    observation = next(row for row in model.records['K'] if row[3].startswith(PREFIX))
    for mutation in ('missing', 'duplicate', 'ordinal', 'count', 'value-digest', 'unavailable', 'result-type', 'missing-type'):
        with test.subTest(mutation=mutation), test.assertRaises(ValueError):
            rows = [row.copy() for row in model.rows]
            row = next(item for item in rows if item[:4] == observation[:4])
            if mutation == 'missing':
                rows.remove(row)
            elif mutation == 'duplicate':
                rows.insert(-1, row.copy())
            elif mutation == 'ordinal':
                row[3] = PREFIX + '999999'
            elif mutation == 'count':
                next(item for item in rows if item[0] == 'K' and item[3] == COUNT)[4] = '0'
            elif mutation == 'unavailable':
                next(item for item in rows if item[:3] == ['CAP', '1', FEATURE])[3] = 'unavailable'
            else:
                fields = row[4].split('\t')
                if mutation == 'value-digest':
                    fields[10:] = ['sha256', '0' * 64]
                elif mutation == 'missing-type':
                    fields[8] = ''
                else:
                    fields[7] = '999999'
                row[4] = '\t'.join(fields)
            Model(text_rows(rows))
    older = [row.copy() for row in model.rows if not
             (row[0] == 'K' and (row[3] == COUNT or row[3].startswith(PREFIX))
              or row[:3] == ['CAP', '1', FEATURE])]
    Model(text_rows(older))

def check_alias_cast_phase(test):
    source = test.source('type TypedCallback as sub(byval as long)\n'
                         'dim callbacks(0 to 1) as sub()\n'
                         'cast(TypedCallback, callbacks(0))(7)\n'
                         'cast(TypedCallback, callbacks(1))(8)\n')
    for backend in test.backends:
        with test.subTest(backend=backend):
            test.invoke([source], backend=backend, mode='off')
            baseline = test.emission_path(source.stem, backend).read_bytes()
            full = test.compile(source, backend=backend)
            reduced = test.compile(source, backend=backend, mode='expressions')
            test.assertEqual(full.parser_observations, reduced.parser_observations)
            test.assertTrue(full.relations('type-alias-input'))
            test.assertFalse(reduced.relations('type-alias-input'))
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), baseline)

# end of parser_observations.py
