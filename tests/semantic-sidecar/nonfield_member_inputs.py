"""Project: FreeBASIC semantic sidecar tests
File: nonfield_member_inputs.py
Purpose: Qualify original non-field member roles independently of the linter.
Responsibilities: Exact occurrences, receivers, transport, modes and refusals.
This file intentionally does NOT execute sample methods or install a compiler.
"""
from collections import Counter
from pathlib import Path
import re
import subprocess

from sidecar import Model, number
from member_selection_inputs import modules
from semantic_nonfield_members import validate_nonfield_members, FEATURE, BINDING, REFERENCE, CLOSURE


def validate(model):
    # Keep this explicit until the shared reader's active qualification ends.
    validate_nonfield_members(model, number, modules(model))


def expected_sites(source):
    result = []
    for line, text in enumerate(source.read_text(encoding='utf-8').splitlines(), 1):
        if "' NFM: " not in text:
            continue
        code, annotation = text.split("' NFM: ", 1)
        role, receiver, name = annotation.split(',')
        matches = list(re.finditer(r'\b' + re.escape(name) + r'\b', code, re.I))
        if len(matches) != 1:
            raise AssertionError('Ambiguous independent fixture annotation: ' + text)
        result.append((line, matches[0].start(), name.upper(), role, receiver.upper()))
    return sorted(result)


def check_inputs(test):
    for backend in test.backends:
        with test.subTest(backend=backend):
            source = test.fixture('nonfield-members.bas')
            output = test.emission_path(source.stem, backend)
            extra = ('-o', str(output))
            test.invoke([source], mode='off', backend=backend, extra=extra)
            plain = output.read_bytes()
            _, artifact = test.invoke([source], backend=backend, extra=extra)
            model = Model.read(artifact)
            validate(model)
            model.validate_physical_locations()
            test.assertEqual(model.capabilities[1][FEATURE], 'available')
            test.assertEqual(output.read_bytes(), plain)
            actual = []
            for ordinal, row in enumerate(model.records['B'], 1):
                value = model.properties['symbol', int(row[1])].get(BINDING + str(ordinal))
                if value is None or row[3] != '1':
                    continue
                role, receiver = value.split('\t')
                actual.append((int(row[5]), int(row[6]), model.symbols[int(row[1])][2].upper(), role,
                               model.symbols[int(receiver)][2].upper() if int(receiver) else '-'))
            test.assertEqual(sorted(actual), expected_sites(source))
            observed = Counter()
            for (domain, member), properties in model.properties.items():
                for key, value in properties.items():
                    if domain != 'symbol' or not key.startswith(REFERENCE):
                        continue
                    role, receiver, expansion = value.split('\t')
                    definition = model.macro_definitions[int(model.macro_invocations[int(expansion)][3])]
                    observed[definition[5].upper(), role,
                             model.symbols[int(receiver)][2].upper() if int(receiver) else '-',
                             model.symbols[member][2].upper()] += 1
            expected = Counter({
                ('MIXED_DATA', 'expression', 'MEMBEROVERLAY', 'SHARED_VALUE'): 1,
                ('MIXED_DATA', 'symbol', '-', 'SHARED_VALUE'): 1,
                ('MIXED_DATA', 'expression', 'MEMBEROVERLAY', 'CONSTANT_VALUE'): 1,
                ('MIXED_DATA', 'symbol', '-', 'CONSTANT_VALUE'): 1,
                ('MIXED_CALL', 'expression', 'MEMBEROVERLAY', 'STATIC_VALUE'): 2,
                ('MIXED_CALL', 'type', 'MEMBEROVERLAY', 'STATIC_VALUE'): 2,
                ('NESTED_CALL', 'expression', 'MEMBEROVERLAY', 'CHOOSE'): 1,
                ('NESTED_CALL', 'type', 'MEMBEROVERLAY', 'STATIC_VALUE'): 1,
            })
            test.assertEqual(observed, expected)
            binary = test.working / 'nonfield-binary.fbcsem'
            command = [str(test.compiler), '-prefix', str(test.toolchain_prefix), '-i', str(test.root / 'inc'),
                       '-gen', backend, '-maxerr', '20', '-r', '-semantic-model', str(binary),
                       '-o', str(output), str(source)]
            result = subprocess.run(command, capture_output=True, timeout=120, check=False)
            test.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            transported = Model.read(binary)
            validate(transported)
            test.assertTrue(transported.rows == model.rows, 'Binary transport changed original member roles')
            test.assertEqual(output.read_bytes(), plain)
            for mode in ('bindings', 'expressions', 'macro-compact'):
                if mode == 'macro-compact':
                    _, reduced_path = test.invoke([source], backend=backend,
                                                  extra=(*extra, '-semantic-model-compact'))
                    reduced = Model.read(reduced_path)
                else:
                    reduced = test.compile(source, backend=backend, mode=mode, extra=extra)
                validate(reduced)
                test.assertEqual(reduced.capabilities[1][FEATURE], 'unavailable')
                test.assertFalse(any(key.startswith('selected-nonfield-')
                                     for properties in reduced.properties.values() for key in properties))
                test.assertEqual(output.read_bytes(), plain)
            empty_source = test.source('#lang "fb"\nConst ordinary_value = 1\n', 'empty-nonfield.bas')
            empty = test.compile(empty_source, backend=backend)
            validate(empty)
            closures = [row for row in empty.records['K'] if row[3].startswith(CLOSURE)]
            test.assertEqual(len(closures), 1)
            test.assertEqual(closures[0][4], '0')


MUTATIONS = ('missing-binding', 'missing-receiver', 'bad-role', 'bad-receiver',
             'missing-reference', 'bad-reference-role', 'bad-reference-receiver',
             'bad-reference-macro', 'bad-reference-id', 'missing-origin',
             'missing-count', 'wrong-count', 'unavailable', 'duplicate-binding')


def control_rows(rows, mutation):
    changed = [row[:] for row in rows]
    binding = next(row for row in changed if row[0] == 'K' and row[3].startswith(BINDING))
    reference = next(row for row in changed if row[0] == 'K' and row[3].startswith(REFERENCE))
    closure = next(row for row in changed if row[0] == 'K' and row[3].startswith(CLOSURE))
    if mutation == 'missing-binding': changed.remove(binding)
    elif mutation == 'duplicate-binding': changed.insert(changed.index(binding) + 1, binding[:])
    elif mutation == 'missing-receiver': binding[4] = binding[4].split('%09')[0]
    elif mutation in ('bad-role', 'bad-receiver'):
        fields = binding[4].split('%09')
        fields[0 if mutation == 'bad-role' else 1] = 'read' if mutation == 'bad-role' else '0'
        binding[4] = '%09'.join(fields)
    elif mutation == 'missing-reference': changed.remove(reference)
    elif mutation == 'bad-reference-id': reference[3] = REFERENCE + '4294967295'
    elif mutation.startswith('bad-reference-'):
        fields = reference[4].split('%09')
        fields[{'bad-reference-role': 0, 'bad-reference-receiver': 1, 'bad-reference-macro': 2}[mutation]] = 'read' if mutation.endswith('role') else '0'
        reference[4] = '%09'.join(fields)
    elif mutation == 'missing-origin':
        origin = next(row for row in changed if row[0] == 'MR' and row[1] == 'symbol' and row[2] == reference[2]
                      and row[4] == 'reference-' + reference[3][len(REFERENCE):])
        changed.remove(origin)
    elif mutation == 'missing-count': changed.remove(closure)
    elif mutation == 'wrong-count': closure[4] = str(int(closure[4]) + 1)
    elif mutation == 'unavailable': next(row for row in changed if row[0] == 'CAP' and row[2] == FEATURE)[3] = 'unavailable'
    else: raise ValueError('Unknown nonfield member mutation')
    return changed


def check_rejection(test):
    from parsed_expression_coverage import write_rows
    for backend in test.backends:
        source = test.fixture('nonfield-members.bas')
        _, artifact = test.invoke([source], backend=backend)
        validate(Model.read(artifact))
        rows = [line.split('\t') for line in artifact.read_text(encoding='ascii').splitlines()]
        damaged = test.working / 'damaged-nonfield.tsv'
        for mutation in MUTATIONS:
            with test.subTest(backend=backend, mutation=mutation):
                write_rows(damaged, control_rows(rows, mutation))
                with test.assertRaises(ValueError):
                    validate(Model.read(damaged))


def check_keyword_calls(test):
    from assignment_inputs import source_text
    body = '''Type LifetimeValue
    value As Long
    Declare Constructor()
    Declare Constructor(ByVal item As Long)
    Declare Destructor()
End Type
Constructor LifetimeValue()
End Constructor
Constructor LifetimeValue(ByVal item As Long)
End Constructor
Destructor LifetimeValue()
End Destructor
Sub KeywordSelections()
    Dim instance_value As LifetimeValue
    instance_value.Constructor(CLng(1)) ' KEYWORD: constructor
    instance_value.Destructor() ' KEYWORD: destructor
End Sub
'''
    for backend in test.backends:
        source = test.source(source_text(body, 'keyword-members.bas'), 'keyword-members.bas')
        model = test.compile(source, backend=backend)
        validate(model)
        for line, text in enumerate(source.read_text(encoding='utf-8').splitlines(), 1):
            if "' KEYWORD: " not in text:
                continue
            kind = text.split("' KEYWORD: ", 1)[1]
            uses = [(ordinal, row) for ordinal, row in enumerate(model.records['B'], 1)
                    if row[2] == 'reference' and int(row[5]) == line and model.symbols[int(row[1])][3] == '3']
            test.assertEqual(len(uses), 1, 'An overload head must not duplicate its selected keyword member')
            ordinal, row = uses[0]
            target = int(row[1])
            selected = next(row for row in reversed(model.records['F']) if int(row[1]) == target)
            test.assertEqual(selected[2], kind)
            test.assertEqual(int(selected[4]), 1 if kind == 'constructor' else 0)
            role, receiver = model.properties['symbol', target][BINDING + str(ordinal)].split('\t')
            test.assertEqual((role, model.symbols[int(receiver)][2]), ('expression', 'LIFETIMEVALUE'))


def check_enum_owners(test):
    from assignment_inputs import source_text
    body = '''Union EnumOverlay
    value As Long
    Enum Kind
        code_value = 4
    End Enum
End Union
Sub EnumSelections()
    Dim object_value As EnumOverlay
    Dim result_value As Long = object_value.Kind.code_value
    result_value = EnumOverlay.Kind.code_value
End Sub
'''
    for backend in test.backends:
        source = test.source(source_text(body, 'enum-members.bas'), 'enum-members.bas')
        model = test.compile(source, backend=backend)
        validate(model)
        observed = Counter()
        for (domain, member), properties in model.properties.items():
            for key, value in properties.items():
                if domain == 'symbol' and key.startswith(BINDING):
                    role, receiver = value.split('\t')
                    observed[model.symbols[member][2], role,
                             model.symbols[int(receiver)][2] if int(receiver) else '-'] += 1
        test.assertEqual(observed, Counter({('KIND', 'expression', 'ENUMOVERLAY'): 1,
                                           ('CODE_VALUE', 'expression', 'ENUMOVERLAY'): 1,
                                           ('CODE_VALUE', 'symbol', '-'): 1}))


if __name__ == '__main__':
    import argparse
    import hashlib
    import json
    import sys
    import unittest
    from test_sidecar import SidecarTests

    parser = argparse.ArgumentParser()
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    if args.receipt.exists():
        raise ValueError('Choose a fresh receipt')

    class NonfieldTests(SidecarTests):
        test_nonfield_inputs = check_inputs
        test_nonfield_rejection = check_rejection
        test_nonfield_keyword_calls = check_keyword_calls
        test_nonfield_enum_owners = check_enum_owners

    NonfieldTests.root = Path(__file__).resolve().parents[2]
    NonfieldTests.compiler = args.compiler.resolve()
    NonfieldTests.toolchain_prefix = Path('C:/FreeBASIC')
    NonfieldTests.backends = ['gcc', 'clang', 'llvm']
    inputs = [Path(__file__), Path(__file__).with_name('nonfield-members.bas'),
              Path(__file__).with_name('semantic_nonfield_members.py'), args.compiler]
    reader = Path(__file__).resolve().parent
    inputs += [Path(module.__file__).resolve() for module in tuple(sys.modules.values())
               if getattr(module, '__file__', None) and Path(module.__file__).resolve().parent == reader]
    inputs += [reader / name for name in ('assignment_inputs.py', 'parsed_expression_coverage.py')]
    pins = {str(path.resolve()): hashlib.sha256(path.read_bytes()).hexdigest().upper() for path in inputs}
    suite = unittest.TestSuite(NonfieldTests(name) for name in
                               ('test_nonfield_inputs', 'test_nonfield_rejection',
                                'test_nonfield_keyword_calls', 'test_nonfield_enum_owners'))
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    drift = [path for path, value in pins.items()
             if hashlib.sha256(Path(path).read_bytes()).hexdigest().upper() != value]
    args.receipt.write_text(json.dumps({'Passed': result.wasSuccessful() and not drift,
                                       'TestsRun': result.testsRun, 'Backends': NonfieldTests.backends,
                                       'InvalidControls': len(MUTATIONS) * len(NonfieldTests.backends),
                                       'Pins': pins, 'InputDrift': drift, 'SubjectExecuted': False,
                                       'CompilerPromoted': False, 'InventoryClosures': 0}, indent=2) + '\n', encoding='utf-8')
    raise SystemExit(0 if result.wasSuccessful() and not drift else 1)

# end of nonfield_member_inputs.py
