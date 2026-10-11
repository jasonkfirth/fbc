"""Project: FreeBASIC semantic sidecar tests
File: parsed_expression_coverage.py
Purpose: Verify original operand coverage before expanded-model replay is skipped.
Responsibilities: Pre-folding inputs, statement counts and damaged receipt refusal.
This file intentionally does NOT infer completeness from macro replacement text.
"""
from collections import Counter
import subprocess

from sidecar import DETAIL_TAGS, Model, source_range

FEATURE = 'parsed-expression-input-coverage'
IIF_FEATURE = 'parsed-iif-input-counts'
PREFIXES = ('parsed-operand-count:', 'parsed-expression-inputs:', 'parsed-iif-count:')
BODY = '''#Define BOTH(a, b) ((a) And (b))
#Define PICK(a, b) IIf(a, b, b + 1)
Sub CoverageInputs(ByVal a As Long, ByVal b As Long)
    Dim r As Long
    r = BOTH(a, b) ' macro
    r = Not (a Or b) ' unary
    r = (1 And 0) Or (0 Xor 1) ' folded
    r = CInt(a) ' cast
    r = IIf(a > b, a + 1, b - 1) ' conditional
    r = IIf(a, IIf(b, 1, 2), 3) ' nested-iif
    r = PICK(a, b) ' macro-iif
    If BOTH(a, b) Then ' header
        r = a Xor b ' nested
    End If
End Sub
'''
EXPECTED = {
    'macro': Counter({('binary', 'and'): 1, ('group', 'parentheses'): 3}),
    'unary': Counter({('binary', 'or'): 1, ('unary', 'not'): 1, ('group', 'parentheses'): 1}),
    'folded': Counter({('binary', 'and'): 1, ('binary', 'xor'): 1,
                       ('binary', 'or'): 1, ('group', 'parentheses'): 2}),
    'cast': Counter({('cast', 'cast'): 1}),
    'conditional': Counter({('binary', 'greater-than'): 1, ('binary', 'add'): 1,
                            ('binary', 'subtract'): 1}),
    'nested-iif': Counter(),
    'macro-iif': Counter({('binary', 'add'): 1}),
    'header': Counter({('binary', 'and'): 1, ('group', 'parentheses'): 3}),
    'nested': Counter({('binary', 'xor'): 1}),
}


def fixture(test):
    name = 'parsed-expression-coverage.bas'
    return test.source("' Project: FreeBASIC semantic sidecar tests\n' File: " + name + '\n'
                       "' Purpose: Exercise original operand completeness.\n"
                       "' Responsibilities: Macro, folded and nested inputs.\n"
                       "' This file intentionally does NOT execute the probes.\n"
                       '#Lang "fb"\n' + BODY + "' end of " + name + '\n', name)


def check_inputs(test):
    source = fixture(test)
    labels = {line: text.rsplit("' ", 1)[1] for line, text in enumerate(source.read_text().splitlines(), 1)
              if "' " in text and not text.startswith("'")}
    for backend in test.backends:
        with test.subTest(backend=backend):
            plain_result, _ = test.invoke([source], mode='off', backend=backend)
            plain = test.emission_path(source.stem, backend).read_bytes()
            full_result, artifact = test.invoke([source], backend=backend)
            model = Model.read(artifact)
            test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), plain)
            test.assertEqual(full_result.stdout + full_result.stderr,
                             plain_result.stdout + plain_result.stderr)
            test.assertEqual(model.capabilities[1][FEATURE], 'available')
            test.assertEqual(model.capabilities[1][IIF_FEATURE], 'available')
            actual = {label: Counter() for label in EXPECTED}
            covered = set()
            for statement, row in model.statements.items():
                label = labels.get(source_range(row, 12)[1])
                if label not in EXPECTED:
                    continue
                properties = model.properties['symbol', int(row[4])]
                test.assertEqual(properties[f'parsed-expression-inputs:{statement}'], 'available', label)
                owned = [operation for identity, operation in model.expression_operands.items()
                         if model.statement_owners.get(('expression', identity)) == statement]
                test.assertEqual(int(properties[f'parsed-operand-count:{statement}']), len(owned))
                test.assertEqual(int(properties[f'parsed-iif-count:{statement}']),
                                 {'conditional': 1, 'nested-iif': 2, 'macro-iif': 1}.get(label, 0), label)
                actual[label].update((operation[2], operation[3]) for operation in owned)
                covered.add(label)
            test.assertEqual(covered, set(EXPECTED))
            test.assertEqual(actual, EXPECTED)
            for mode in ('bindings', 'expressions'):
                compact = test.compile(source, mode=mode, backend=backend)
                test.assertEqual(compact.capabilities[1][FEATURE], 'unavailable')
                test.assertEqual(compact.capabilities[1][IIF_FEATURE], 'unavailable')
                test.assertFalse(any(row[3].startswith(PREFIXES) for row in compact.records['K']))
                test.assertEqual(test.emission_path(source.stem, backend).read_bytes(), plain)


def write_rows(path, rows):
    # Recount the generic footer so deletion controls exercise the statement
    # receipt rather than being rejected solely by the file's record total.
    counts = Counter(row[0] for row in rows)
    footer = rows[-1]
    footer[2:11] = [str(counts[tag]) for tag in ('M', 'P', 'S', 'V', 'N', 'E', 'B', 'I', 'D')]
    footer[12] = str(sum(counts[tag] for tag in DETAIL_TAGS))
    path.write_text('\n'.join('\t'.join(row) for row in rows) + '\n', encoding='utf-8')


def control_rows(rows, mutation):
    changed = [row[:] for row in rows]
    available = {row[3][len(PREFIXES[1]):] for row in changed if row[0] == 'K'
                 and row[3].startswith(PREFIXES[1]) and row[4] == 'available'}
    count = next(row for row in changed if row[0] == 'K' and row[3].startswith(PREFIXES[0])
                 and row[3][len(PREFIXES[0]):] in available and int(row[4]) > 0)
    statement = count[3][len(PREFIXES[0]):]
    coverage = next(row for row in changed if row[:4] == ['K', 'symbol', count[2], PREFIXES[1] + statement])
    if mutation == 'wrong-count':
        count[4] = str(int(count[4]) + 1)
    elif mutation == 'missing-count':
        changed.remove(count)
    elif mutation == 'missing-status':
        changed.remove(coverage)
    elif mutation == 'invalid-status':
        coverage[4] = 'partial'
    elif mutation == 'duplicate-count':
        changed.insert(-1, count[:])
    elif mutation == 'duplicate-status':
        changed.insert(-1, coverage[:])
    elif mutation == 'wrong-owner':
        count[2] = next(row[1] for row in changed if row[0] == 'S' and row[1] != count[2])
    elif mutation == 'wrong-statement':
        count[3] = PREFIXES[0] + '4294967295'
    elif mutation in ('missing-capability', 'unavailable-capability'):
        capability = next(row for row in changed if row[0] == 'CAP' and row[2] == FEATURE)
        if mutation == 'missing-capability':
            changed.remove(capability)
        else:
            capability[3] = 'unavailable'
    elif mutation in ('missing-operation', 'missing-operand'):
        owners = {row[2]: row[3] for row in changed if row[:2] == ['OWN', 'expression']}
        operation = next(row for row in changed if row[0] == 'EX' and owners.get(row[1]) == statement)
        if mutation == 'missing-operation':
            changed.remove(operation)
        else:
            operation[4] = '0'
            coverage[4] = 'available'
    elif mutation == 'unavailable':
        coverage[4] = 'unavailable'
    elif mutation.startswith('iif-'):
        iif_count = next(row for row in changed if row[0] == 'K' and row[3].startswith(PREFIXES[2])
                         and int(row[4]) > 0)
        if mutation == 'iif-missing-pair':
            owners = {row[2]: row[3] for row in changed if row[:2] == ['OWN', 'expression']}
            occurrence = next(row for row in changed if row[0] == 'H' and row[5] == 'parsed-iif'
                              and owners.get(row[4]) == iif_count[3][len(PREFIXES[2]):])
            inputs = next(row for row in changed if row[:4] == ['K', 'expression', occurrence[4], 'original-iif-inputs'])
            changed.remove(occurrence)
            changed.remove(inputs)
        elif mutation == 'iif-missing-count':
            changed.remove(iif_count)
        elif mutation == 'iif-wrong-count':
            iif_count[4] = str(int(iif_count[4]) + 1)
        elif mutation == 'iif-duplicate-count':
            changed.insert(-1, iif_count[:])
        elif mutation == 'iif-wrong-owner':
            iif_count[2] = next(row[1] for row in changed if row[0] == 'S' and row[1] != iif_count[2])
        elif mutation in ('iif-missing-capability', 'iif-unavailable-capability'):
            capability = next(row for row in changed if row[0] == 'CAP' and row[2] == IIF_FEATURE)
            if mutation == 'iif-missing-capability':
                changed.remove(capability)
            else:
                capability[3] = 'unavailable'
        else:
            raise AssertionError(mutation)
    elif mutation == 'legacy-ex-only':
        changed = [row for row in changed if not (row[0] == 'K' and row[3].startswith(PREFIXES[2]))
                   and not (row[0] == 'CAP' and row[2] == IIF_FEATURE)]
    elif mutation == 'legacy':
        changed = [row for row in changed if not (row[0] == 'K' and row[3].startswith(PREFIXES))
                   and not (row[0] == 'CAP' and row[2] in (FEATURE, IIF_FEATURE))]
    else:
        raise AssertionError(mutation)
    return changed


MUTATIONS = ('wrong-count', 'missing-count', 'missing-status', 'invalid-status',
             'duplicate-count', 'duplicate-status', 'wrong-owner', 'wrong-statement',
             'missing-capability', 'unavailable-capability', 'missing-operation', 'missing-operand',
             'iif-missing-pair', 'iif-missing-count', 'iif-wrong-count', 'iif-duplicate-count',
             'iif-wrong-owner', 'iif-missing-capability', 'iif-unavailable-capability')


def check_rejection(test):
    for backend in test.backends:
        _, artifact = test.invoke([fixture(test)], backend=backend)
        rows = [line.split('\t') for line in artifact.read_text().splitlines()]
        path = test.working / 'coverage-control.sem'
        for mutation in MUTATIONS:
            with test.subTest(backend=backend, mutation=mutation):
                write_rows(path, control_rows(rows, mutation))
                with test.assertRaises(ValueError):
                    Model.read(path)
        for mutation in ('unavailable', 'legacy', 'legacy-ex-only'):
            with test.subTest(backend=backend, mutation=mutation):
                write_rows(path, control_rows(rows, mutation))
                Model.read(path)


def main():
    import argparse
    import hashlib
    import json
    from pathlib import Path

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--model', type=Path, required=True)
    parser.add_argument('--native-reader', type=Path, required=True)
    parser.add_argument('--control-path', type=Path, required=True)
    options = parser.parse_args()
    paths = (options.model, options.native_reader)
    pins = {path: hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
    Model.read(options.model)
    rows = [line.split('\t') for line in options.model.read_text().splitlines()]
    outcomes = []
    for mutation in ('original', *MUTATIONS, 'unavailable', 'legacy', 'legacy-ex-only'):
        expected = 2 if mutation in MUTATIONS else 0
        changed = [row[:] for row in rows] if mutation == 'original' else control_rows(rows, mutation)
        write_rows(options.control_path, changed)
        if expected:
            try:
                Model.read(options.control_path)
            except ValueError:
                pass
            else:
                raise AssertionError('Independent reader accepted ' + mutation)
        else:
            Model.read(options.control_path)
        result = subprocess.run([str(options.native_reader), str(options.control_path), 'expression-coverage'],
                                capture_output=True, text=True, timeout=30, check=False)
        if result.returncode != expected:
            raise AssertionError(f'{mutation}: native exit {result.returncode}, expected {expected}: '
                                 + result.stdout + result.stderr)
        if mutation.startswith('legacy') and result.stdout.strip() != 'Complete expression input statements: 0':
            raise AssertionError('Legacy producer incorrectly certifies complete inputs: ' + result.stdout)
        if mutation == 'original' and int(result.stdout.rsplit(':', 1)[1]) <= 0:
            raise AssertionError('Genuine producer fails to certify its complete inputs')
        outcomes.append({'Mutation': mutation, 'NativeExit': result.returncode,
                         'Output': result.stdout.strip()})
    if any(hashlib.sha256(path.read_bytes()).hexdigest() != digest for path, digest in pins.items()):
        raise AssertionError('Qualification input changed')
    print(json.dumps({'Passed': True, 'Cases': len(outcomes),
                      'Pins': {str(path): digest for path, digest in pins.items()},
                      'Outcomes': outcomes}, indent=2))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

# end of parsed_expression_coverage.py
