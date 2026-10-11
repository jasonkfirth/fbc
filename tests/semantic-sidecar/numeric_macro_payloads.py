"""Project: FreeBASIC semantic sidecar tests
File: numeric_macro_payloads.py
Purpose: Verify original numeric payload transport through actual macro buffers.
Responsibilities: Exact spelling, typed execution parity and damaged provenance.
This file intentionally does NOT reconstruct payloads from replacement text.
"""
from collections import Counter
import codecs

from sidecar import Model
from parsed_expression_coverage import write_rows

FEATURE = 'macro-numeric-payloads'
PROPERTY = 'original-float-provenance'
FRACTION = '.' + '0' * 70 + '100000001e71'
WHOLE = '1.' + '0' * 74
EXPECTED = {
    'direct': [('2.0', 'source')],
    'definition': [(FRACTION, 'transported')],
    'whole': [(WHOLE, 'transported')],
    'alias': [(FRACTION, 'transported')],
    'argument': [(FRACTION, 'transported')],
    'nested': [(FRACTION, 'transported')],
    'trimmed': [('5.0', 'transported')],
    'body': [(FRACTION, 'transported')],
    'pair': [('6.0', 'transported'), ('7.0', 'transported')],
    'repeated': [('8.0', 'transported'), ('8.0', 'transported')],
    'paste': [('1.0', 'generated')],
    'exponent': [('1D+', 'transported')],
    'leading-dot': [('.0000001', 'transported')],
    'dot-argument': [('.0000002', 'transported')],
    'dot-nested': [('.0000001', 'transported')],
    'before-undef': [('9.0', 'transported')],
    'after-undef': [('10.0', 'transported')],
}


def check_payloads(test):
    source = test.fixture('numeric-macro-payloads.bas')
    labels = {line: text.split("' payload: ")[1]
              for line, text in enumerate(source.read_text().splitlines(), 1)
              if "' payload: " in text}
    for backend in test.backends:
        with test.subTest(backend=backend):
            plain_result, _ = test.invoke([source], mode='off', backend=backend)
            plain = test.emission_path(source.stem, backend).read_bytes()
            full_result, path = test.invoke([source], backend=backend)
            model = Model.read(path)
            model.validate_physical_locations()
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), plain)
            test.assertEqual(full_result.stdout + full_result.stderr,
                             plain_result.stdout + plain_result.stderr)
            test.assertEqual(model.capabilities[1][FEATURE], 'available')
            actual = {label: [] for label in EXPECTED}
            for row in model.records['E']:
                properties = model.properties['expression', int(row[1])]
                if PROPERTY not in properties:
                    continue
                label = labels.get(int(row[4]))
                if label in actual:
                    actual[label].append((properties['original-float-text'],
                                          properties[PROPERTY].split('\t')[0]))
            test.assertEqual({key: Counter(value) for key, value in actual.items()},
                             {key: Counter(value) for key, value in EXPECTED.items()})
            for mode in ('bindings', 'expressions'):
                compact = test.compile(source, mode=mode, backend=backend)
                test.assertEqual(compact.capabilities[1][FEATURE], 'unavailable')
                test.assertFalse(any(PROPERTY in values for values in compact.properties.values()))
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), plain)

    # Both preprocessor scanners use replacement units. UTF source byte widths
    # must not change the payload or its macro offset and reporting ownership.
    for encoding, bom in (('utf-8', b''), ('utf-8', codecs.BOM_UTF8),
                          ('utf-16-le', codecs.BOM_UTF16_LE), ('utf-16-be', codecs.BOM_UTF16_BE),
                          ('utf-32-le', codecs.BOM_UTF32_LE), ('utf-32-be', codecs.BOM_UTF32_BE)):
        with test.subTest(encoding=encoding, bom=bool(bom)):
            copied = test.working / 'numeric-encoded.bas'
            copied.write_bytes(bom + source.read_text().encode(encoding))
            model = test.compile(copied, backend=test.backends[0])
            model.validate_physical_locations()
            observed = Counter((values['original-float-text'], values[PROPERTY].split('\t')[0])
                               for values in model.properties.values() if PROPERTY in values)
            test.assertEqual(observed, Counter(item for values in EXPECTED.values() for item in values))


def check_rejection(test):
    _, path = test.invoke([test.fixture('numeric-macro-payloads.bas')], backend=test.backends[0])
    original = [line.split('\t') for line in path.read_text().splitlines()]
    for mutation in ('missing', 'foreign-domain', 'unknown-kind', 'source-for-macro',
                     'zero-expansion', 'wrong-offset', 'wrong-expansion', 'conflict',
                     'missing-capability', 'unavailable-capability'):
        with test.subTest(mutation=mutation):
            rows = [row[:] for row in original]
            receipt = next(row for row in rows if row[:2] == ['K', 'expression']
                           and row[3] == PROPERTY and row[4].startswith('transported'))
            kind, expansion, offset = receipt[4].split('%09')
            if mutation == 'missing':
                rows.remove(receipt)
            elif mutation == 'foreign-domain':
                receipt[1] = 'symbol'
            elif mutation == 'unknown-kind':
                receipt[4] = 'inferred%09' + expansion + '%09' + offset
            elif mutation == 'source-for-macro':
                receipt[4] = 'source%090%090'
            elif mutation == 'zero-expansion':
                receipt[4] = kind + '%090%09' + offset
            elif mutation == 'wrong-offset':
                receipt[4] = kind + '%09' + expansion + '%09' + str(int(offset) + 1)
            elif mutation == 'wrong-expansion':
                receipt[4] = kind + '%0999999999%09' + offset
            elif mutation == 'conflict':
                other = receipt[:]
                other[4] = 'source%090%090'
                rows.insert(-1, other)
            else:
                capability = next(row for row in rows if row[:3] == ['CAP', '1', FEATURE])
                if mutation == 'missing-capability':
                    rows.remove(capability)
                else:
                    capability[3] = 'unavailable'
            changed = test.working / ('numeric-' + mutation + '.fbcsem')
            write_rows(changed, rows)
            with test.assertRaises(ValueError):
                Model.read(changed)


def check_boundaries(test):
    # Written payloads can exceed the bounded observation while their canonical
    # replacement fits. Incompleteness must survive all subsequent re-lexing.
    name = 'numeric-overflow.bas'
    body = ("' Project: FreeBASIC semantic sidecar tests\n' File: " + name + '\n'
            "' Purpose: Preserve honest numeric observation limits.\n"
            "' Responsibilities: Long source inputs and replacement clipping.\n"
            "' This file intentionally does NOT require an unbounded token buffer.\n"
            '#Lang "fb"\n#define TOO_LONG 1.' + '0' * 1100 + '1\n'
            '#define DOT_LONG .' + '0' * 1100 + '1\n'
            '#define PASS(value) value\nPrint Int(TOO_LONG)\nPrint Int(PASS(TOO_LONG))\n'
            'Print Int(DOT_LONG)\nPrint Int(PASS(DOT_LONG))\n'
            "' end of " + name + '\n')
    source = test.source(body, name)
    plain_result, _ = test.invoke([source], mode='off', backend=test.backends[0])
    plain = test.emission_path(source.stem, test.backends[0]).read_bytes()
    result, path = test.invoke([source], backend=test.backends[0])
    model = Model.read(path)
    test.assertEqual(result.stdout + result.stderr, plain_result.stdout + plain_result.stderr)
    test.assertEqual(test.emission_path(source.stem, test.backends[0]).read_bytes(), plain)
    payloads = [values for values in model.properties.values() if PROPERTY in values]
    test.assertEqual(len(payloads), 4)
    for values in payloads:
        test.assertEqual(values['original-float-text-complete'], '0')
        test.assertEqual(len(values['original-float-text']), 1024)
        test.assertEqual(values[PROPERTY].split('\t')[0], 'transported')
    test.assertEqual(Counter(values['original-float-text'] for values in payloads),
                     Counter({'1.' + '0' * 1022: 2, '.' + '0' * 1023: 2}))

    # Grow the payload, link and invocation arrays, then reset them between
    # native modules. Reused argument-list storage must not retain old handles.
    sources = []
    for ordinal, value in enumerate(('2.0', '4.0'), 1):
        name = f'numeric-growth-{ordinal}.bas'
        sources.append(test.source("' Project: FreeBASIC semantic sidecar tests\n' File: " + name + '\n'
                                   "' Purpose: Exercise numeric observation growth and reset.\n"
                                   "' Responsibilities: Reused macro arguments in separate modules.\n"
                                   "' This file intentionally does NOT share macro definitions.\n"
                                   '#Lang "fb"\n#define PASS(value) value\n'
                                   + ('Print Int(PASS(' + value + '))\n') * 200
                                   + "' end of " + name + '\n', name))
    _, path = test.invoke(sources, backend=test.backends[0])
    model = Model.read(path)
    model.validate_physical_locations()
    test.assertEqual(len(model.records['M']), 2)
    test.assertTrue(all(features[FEATURE] == 'available' for features in model.capabilities.values()))
    payloads = [values for values in model.properties.values() if PROPERTY in values]
    test.assertEqual(Counter((values['original-float-text'], values[PROPERTY].split('\t')[0])
                             for values in payloads), Counter({('2.0', 'transported'): 200,
                                                               ('4.0', 'transported'): 200}))

# end of numeric_macro_payloads.py
