"""Project: FreeBASIC semantic sidecar tests
File: macro_expression_spans.py
Purpose: Verify source boundaries for mixed macro and ordinary expressions.
Responsibilities: Full/reduced parity, hidden provenance and unchanged emission.
This file intentionally does NOT infer expression structure from source text.
"""

from sidecar import source_range


def check_boundaries(test):
    source = test.source('#define VALUE 4\n#define EMPTY\n'
                         'EMPTY EMPTY EMPTY\n'
                         'dim n as long = 7, result_value as long\n'
                         'result_value = VALUE + n\n'
                         'result_value = n + VALUE\n'
                         'result_value = n + VALUE + n\n'
                         'result_value = (VALUE + n) + n\n'
                         'result_value = n + 4\n')
    expected = [test.span(source, text) for text in
                ('VALUE + n', 'n + VALUE', 'n + VALUE + n', '(VALUE + n) + n', 'n + 4')]
    for backend in test.backends:
        test.invoke([source], mode='off', backend=backend)
        emission = test.emission_path(source.stem, backend).read_bytes()
        for mode in ('full', 'expressions'):
            for hide_macros in (False, True):
                with test.subTest(backend=backend, mode=mode, hide_macros=hide_macros):
                    extra = ('-semantic-model-compact',) if hide_macros else ()
                    model = test.compile(source, backend=backend, mode=mode, extra=extra)
                    for index, span in enumerate(expected):
                        matches = [row for row in model.records['E'] if source_range(row, 3) == span and row[10] == 'add']
                        test.assertTrue(matches, (mode, span))
                        test.assertEqual({row[2] for row in matches}, {'1' if index == len(expected) - 1 else '0'})
                    if hide_macros:
                        test.assertFalse(model.records['MI'])
                        test.assertFalse(model.records['MD'])
                    test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), emission)

# end of macro_expression_spans.py
