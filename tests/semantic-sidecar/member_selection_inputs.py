"""Project: FreeBASIC semantic sidecar tests
File: member_selection_inputs.py
Purpose: Preserve selected member roles through lowering and macro expansion.
Responsibilities: Exact receivers, query distinctions, transport and refusals.
This file intentionally does NOT execute union reads or implement a lint rule.
"""
import re
import subprocess

from sidecar import Model, number
from semantic_member_selections import validate_member_selections, FEATURE, BINDING, REFERENCE, CLOSURE

BODY = '''Type PlainValue
    number As Integer
End Type
Union OverlayValue
    number As Integer
    nested_value As PlainValue
End Union
Type AnonymousValue
    Union
        first_value As Integer
        second_value As Double
    End Union
End Type
Dim overlay As OverlayValue, ordinary As PlainValue, anonymous As AnonymousValue
overlay.number = 1 ' SELECT: expression,OverlayValue,number
overlay.nested_value.number = 2 ' SELECT: expression,OverlayValue,nested_value|expression,PlainValue,number
Dim ptr_value As OverlayValue Ptr = @overlay
ptr_value->number = 3 ' SELECT: expression,OverlayValue,number
With overlay
    .number = 4 ' SELECT: expression,OverlayValue,number
End With
anonymous.first_value = 5 ' SELECT: expression,AnonymousValue,first_value
Dim query_expression As Integer = SizeOf(overlay.number) ' SELECT: expression,OverlayValue,number
Dim query_type As Integer = SizeOf(OverlayValue.number) ' SELECT: type,OverlayValue,number
Dim query_nested As Integer = SizeOf(OverlayValue.nested_value.number) ' SELECT: type,OverlayValue,nested_value|type,PlainValue,number
Dim query_len As Integer = Len(OverlayValue.number) ' SELECT: type,OverlayValue,number
Dim query_typeof As TypeOf(overlay.number) ' SELECT: expression,OverlayValue,number
Dim query_type_member As TypeOf(OverlayValue.number) ' SELECT: type,OverlayValue,number
Dim query_nested_typeof As Integer = SizeOf(TypeOf(overlay.number)) ' SELECT: expression,OverlayValue,number
#Define MIXED_MEMBER(v,t) (SizeOf(t.number)+(v).number)
Dim mixed_result As Integer = MIXED_MEMBER(overlay,OverlayValue)
#Define READ_NESTED(v) ((v).nested_value.number)
Dim nested_result As Integer = READ_NESTED(overlay)
#Define WRAPPED_MEMBER(v,t) MIXED_MEMBER(v,t)
Dim wrapped_result As Integer = WRAPPED_MEMBER(overlay,OverlayValue)
#Define QUERY_MEMBER(t) SizeOf(t.nested_value.number)
Dim type_result As Integer = QUERY_MEMBER(OverlayValue)
Dim offset_result As Integer = OffsetOf(OverlayValue,number)
#Undef OffsetOf
#Define OffsetOf(t,f) overlay.f
Dim source_offset As Integer = OffsetOf(OverlayValue,number)
'''


def modules(model):
    module, binding = 0, 0
    result = {}
    for row in model.rows:
        if row[0] == 'M':
            module += 1
        elif row[0] == 'S':
            result['symbol', int(row[1])] = module
        elif row[0] == 'B':
            binding += 1
            result['binding', binding] = module
    return result


def validate(model):
    # Kept explicit as well as registered in the reader so a staged contract
    # can be checked without changing another worker's reader input snapshot.
    validate_member_selections(model, number, modules(model))


def fixture(test):
    from assignment_inputs import source_text
    return test.source(source_text(BODY, 'member-selections.bas'), 'member-selections.bas')


def expected_physical(text):
    expected = []
    for line, content in enumerate(text.splitlines(), 1):
        if "' SELECT: " not in content:
            continue
        code, annotations = content.split("' SELECT: ", 1)
        for annotation in annotations.split('|'):
            role, receiver, field = annotation.strip().split(',')
            matches = list(re.finditer(r'\b' + re.escape(field) + r'\b', code, re.I))
            if len(matches) != 1:
                raise AssertionError('Fixture needs one exact member token: ' + content)
            expected.append((line, matches[0].start(), field.upper(), role, receiver.upper()))
    return sorted(expected)


def check_inputs(test):
    for backend in test.backends:
        with test.subTest(backend=backend):
            source = fixture(test)
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
                values = model.properties['symbol', int(row[1])]
                selected = values.get(BINDING + str(ordinal))
                if row[2] != 'reference' or row[3] != '1' or selected is None:
                    continue
                role, receiver_text = selected.split('\t')
                receiver = int(receiver_text)
                actual.append((int(row[5]), int(row[6]), model.symbols[int(row[1])][2],
                               role, model.symbols[receiver][2]))
            test.assertEqual(sorted(actual), expected_physical(source.read_text(encoding='utf-8')))
            # The exact same selected field appears as a type member and an
            # object member in a single expansion. Invocation-level labels
            # cannot substitute for these two independent occurrence roles.
            observed = {}
            for (domain, field), values in model.properties.items():
                if domain != 'symbol':
                    continue
                for key, value in values.items():
                    if key.startswith(REFERENCE):
                        role, receiver, expansion = value.split('\t')
                        invocation = model.macro_invocations[int(expansion)]
                        definition = model.macro_definitions[int(invocation[3])]
                        observed.setdefault(definition[5], []).append((role, model.symbols[int(receiver)][2], model.symbols[field][2]))
            test.assertEqual(sorted(observed['MIXED_MEMBER']), sorted([('expression', 'OVERLAYVALUE', 'NUMBER'),
                             ('type', 'OVERLAYVALUE', 'NUMBER')] * 2))
            test.assertEqual(sorted(observed['READ_NESTED']), [('expression', 'OVERLAYVALUE', 'NESTED_VALUE'),
                             ('expression', 'PLAINVALUE', 'NUMBER')])
            test.assertEqual(sorted(observed['QUERY_MEMBER']), [('type', 'OVERLAYVALUE', 'NESTED_VALUE'),
                             ('type', 'PLAINVALUE', 'NUMBER')])
            test.assertEqual(len(observed['OFFSETOF']), 2)
            test.assertTrue(all(role == 'expression' for role, _, _ in observed['OFFSETOF']))
            selected_definitions = [row for row in model.records['MD'] if row[5] == 'OFFSETOF']
            test.assertEqual({model.types[int(row[2])][18] for row in selected_definitions}, {'compiler', 'source'})
            binary = test.working / ('members-' + backend + '.fbcsem')
            reply = subprocess.run([str(test.compiler), '-prefix', str(test.toolchain_prefix),
                                    '-i', str(test.root / 'inc'), '-gen', backend, '-r', '-maxerr', '20',
                                    '-semantic-model', str(binary), *extra, str(source)],
                                   cwd=test.working, capture_output=True, text=True, timeout=120)
            test.assertEqual(reply.returncode, 0, reply.stdout + reply.stderr)
            test.assertTrue(Model.read(binary).rows == model.rows, 'Binary transport changed selected member facts')
            validate(Model.read(binary))
            test.assertEqual(output.read_bytes(), plain)
            for mode in ('bindings', 'expressions'):
                compact = test.compile(source, mode=mode, backend=backend, extra=extra)
                validate(compact)
                test.assertEqual(compact.capabilities[1][FEATURE], 'unavailable')
                test.assertFalse(any(key.startswith('selected-member-') for values in compact.properties.values() for key in values))
                test.assertEqual(output.read_bytes(), plain)
            _, compact_macro = test.invoke([source], backend=backend, extra=(*extra, '-semantic-model-compact'))
            compact_model = Model.read(compact_macro)
            test.assertEqual(compact_model.capabilities[1][FEATURE], 'unavailable')
            test.assertFalse(any(key.startswith('selected-member-') for values in compact_model.properties.values() for key in values))
            test.assertEqual(output.read_bytes(), plain)


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
    else: raise ValueError('Unknown selected member mutation')
    return changed


def check_rejection(test):
    from parsed_expression_coverage import write_rows
    for backend in test.backends:
        source = fixture(test)
        _, artifact = test.invoke([source], backend=backend)
        validate(Model.read(artifact))
        rows = [line.split('\t') for line in artifact.read_text(encoding='ascii').splitlines()]
        damaged = test.working / 'damaged-member.tsv'
        for mutation in MUTATIONS:
            with test.subTest(backend=backend, mutation=mutation):
                write_rows(damaged, control_rows(rows, mutation))
                with test.assertRaises(ValueError):
                    validate(Model.read(damaged))

# end of member_selection_inputs.py
