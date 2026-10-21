"""Project: FreeBASIC semantic sidecar tests
File: primitive_type_inputs.py
Purpose: Qualify original primitive selections and deliberate exclusions.
Responsibilities: Real models, transport/modes, module closure and corruption.
This file intentionally does NOT find primitive uses by scanning source text.
"""
import subprocess

from sidecar import Model, SourceEdit
from parsed_expression_coverage import write_rows
from semantic_primitive_types import FEATURE, INPUT, COUNT


def check_inputs(test):
    source = test.fixture('primitive-type-inputs.bas')
    output = test.emission_path(source.stem, 'gcc')
    extra = ('-o', str(output))
    test.invoke([source], mode='off', extra=extra)
    plain = output.read_bytes()
    _, path = test.invoke([source], extra=extra)
    model = Model.read(path)
    model.validate_physical_locations()
    test.assertEqual(output.read_bytes(), plain)
    test.assertEqual(model.capabilities[1][FEATURE], 'available')
    test.assertEqual(sum(row[3].startswith(INPUT) for row in model.records['K']), 13)
    binary = test.working / 'primitive-binary.fbcsem'
    command = [str(test.compiler), '-prefix', str(test.toolchain_prefix), '-i', str(test.root / 'inc'),
               '-gen', 'gcc', '-maxerr', '20', '-r', '-semantic-model', str(binary), *extra, str(source)]
    reply = subprocess.run(command, capture_output=True, timeout=120, check=False)
    test.assertEqual(reply.returncode, 0, reply.stdout + reply.stderr)
    transported = Model.read(binary)
    transported.validate_physical_locations()
    test.assertEqual(transported.rows, model.rows)
    for mode in ('bindings', 'expressions', 'macro-compact'):
        if mode == 'macro-compact':
            _, reduced_path = test.invoke([source], extra=(*extra, '-semantic-model-compact'))
            reduced = Model.read(reduced_path)
            reduced.validate_physical_locations()
            test.assertEqual(reduced.capabilities[1][FEATURE], 'available')
            test.assertEqual(sum(row[3].startswith(INPUT) for row in reduced.records['K']), 13)
            test.assertFalse(any(row[4].startswith(INPUT) for row in reduced.records['MR']))
        else:
            reduced = test.compile(source, mode=mode, extra=extra)
            test.assertEqual(reduced.capabilities[1][FEATURE], 'unavailable')
            test.assertFalse(any(row[3].startswith((INPUT, COUNT)) for row in reduced.records['K']))
        test.assertEqual(output.read_bytes(), plain)
    first = test.source('#lang "fb"\nDim first_value As Long\n', 'primitive-first.bas')
    second = test.source('#lang "fb"\nDim second_value As Double\n', 'primitive-second.bas')
    _, path = test.invoke([first, second])
    modules = Model.read(path)
    modules.validate_physical_locations()
    test.assertEqual({row[3][len(COUNT):]: row[4] for row in modules.records['K'] if row[3].startswith(COUNT)},
                     {'1': '1', '2': '1'})


def check_unverified_inputs(test):
    source = test.working / 'primitive-unverified.bas'
    source.write_bytes(b'dim value as string = "\xe9"\nprint value\n')
    output = test.emission_path(source.stem, 'gcc')
    extra = ('-o', str(output))
    test.invoke([source], mode='off', extra=extra)
    plain = output.read_bytes()
    _, path = test.invoke([source], extra=extra)
    model = Model.read(path)
    model.validate_physical_locations()
    test.assertEqual(output.read_bytes(), plain)
    observations = [row for row in model.records['K'] if row[3].startswith(INPUT)]
    test.assertEqual(len(observations), 1)
    location = next(row for row in model.records['LOC']
                    if row[0:2] == ['LOC', 'source-context']
                    and row[3] == observations[0][3])
    test.assertEqual(location[9:12], ['-1', '-1', 'unverified'])
    binding = next(row for row in model.records['LOC']
                   if row[1] == 'binding' and row[5] == '1')
    with test.assertRaisesRegex(ValueError, 'not mapped'):
        model.plan_edits([SourceEdit('binding', int(binding[2]), 'range', b'value')])
    binary = test.working / 'primitive-unverified-binary.fbcsem'
    command = [str(test.compiler), '-prefix', str(test.toolchain_prefix),
               '-i', str(test.root / 'inc'), '-gen', 'gcc', '-maxerr', '20', '-r',
               '-semantic-model', str(binary), *extra, str(source)]
    reply = subprocess.run(command, capture_output=True, timeout=120, check=False)
    test.assertEqual(reply.returncode, 0, reply.stdout + reply.stderr)
    transported = Model.read(binary)
    transported.validate_physical_locations()
    test.assertEqual(transported.rows, model.rows)
    test.assertEqual(output.read_bytes(), plain)


def check_rejection(test):
    _, path = test.invoke([test.fixture('primitive-type-inputs.bas')])
    model = Model.read(path)
    model.validate_physical_locations()
    base = [line.split('\t') for line in path.read_text(encoding='ascii').splitlines()]
    rewritten = test.working / 'primitive-roundtrip.fbcsem'
    write_rows(rewritten, [row[:] for row in base])
    test.assertEqual(Model.read(rewritten).rows, model.rows)
    cases = ('missing-input', 'missing-closure', 'count', 'duplicate', 'identity',
             'payload', 'ordinal', 'keyword', 'dtype', 'pointer-depth', 'owner',
             'macro-origin', 'physical-origin', 'unverified-bytes',
             'orphan-openings', 'unavailable')
    for case in cases:
        with test.subTest(case=case):
            rows = [row[:] for row in base]
            selected = next(row for row in rows if row[0] == 'K' and row[3].startswith(INPUT))
            closure = next(row for row in rows if row[0] == 'K' and row[3].startswith(COUNT))
            if case == 'missing-input':
                rows.remove(selected)
            elif case == 'missing-closure':
                rows.remove(closure)
            elif case == 'count':
                closure[4] = str(int(closure[4]) + 1)
            elif case == 'duplicate':
                rows.insert(len(rows) - 1, selected[:])
            elif case == 'identity':
                selected[3] = INPUT + '0' + selected[3][len(INPUT):]
            elif case == 'owner':
                selected[2] = str(next(identity for identity, row in model.symbols.items() if row[3] == '1'))
            elif case == 'macro-origin':
                rows.remove(next(row for row in rows if row[0] == 'MR' and row[4].startswith(INPUT)))
            elif case in ('physical-origin', 'unverified-bytes'):
                location = next(row for row in rows
                                if row[0:2] == ['LOC', 'source-context']
                                and row[3] == selected[3])
                test.assertEqual(location[11], 'mapped')
                if case == 'physical-origin':
                    test.assertFalse(any(row[0] == 'MR' and row[4] == selected[3] for row in rows))
                    rows.remove(location)
                else:
                    location[11] = 'unverified'
            elif case == 'orphan-openings':
                rows = [row for row in rows if not (row[0] == 'K' and row[3].startswith(INPUT))]
                closure[4] = '0'
            elif case == 'unavailable':
                next(row for row in rows if row[0] == 'CAP' and row[2] == FEATURE)[3] = 'unavailable'
            else:
                fields = selected[4].split('%09')
                test.assertEqual(len(fields), 5)
                if case == 'payload': fields[0] = '0' + fields[0]
                if case == 'ordinal': fields[4] = str(int(fields[4]) + 1)
                if case == 'keyword':
                    symbol = next(row[1] for row in model.records['T'] if row[3] == 'keyword' and row[2].upper() == 'DIM')
                    fields[2] = model.properties['symbol', int(symbol)]['keyword-token']
                if case == 'dtype': fields[3] = '18'  # DOUBLE cannot be a selected LONG.
                if case == 'pointer-depth': fields[3] = str(11 | (9 << 5))
                selected[4] = '%09'.join(fields)
            damaged = test.working / ('primitive-' + case + '.fbcsem')
            write_rows(damaged, rows)
            with test.assertRaises(ValueError):
                Model.read(damaged).validate_physical_locations()


# end of primitive_type_inputs.py
