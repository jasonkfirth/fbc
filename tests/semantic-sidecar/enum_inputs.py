"""Project: FreeBASIC semantic sidecar tests
File: enum_inputs.py
Purpose: Exercise parser-owned enum groups and original initializer inputs.
Responsibilities: Membership, aliases, macros, compact modes and rejection.
This file intentionally does NOT execute enum programs or parse declarations.
"""
from sidecar import Model

FEATURE = 'enum-declaration-inputs'
BODY = '''Const SeedValue As Long = 4
Private Enum MixedState Explicit
    First = SeedValue - 7
    Second
    Third = 8
    Fourth
    Fifth = Cast(Long, 1.5)
End Enum
Const AliasValue As MixedState = MixedState.First
Type StateAlias As MixedState
Const TypedAlias As StateAlias = Cast(StateAlias, 99)
Enum AllImplicit Explicit
    ImplicitFirst
    ImplicitSecond
End Enum
Enum FirstExplicit Explicit
    Initial = 3
    Following
    LastState
End Enum
Public Enum AllExplicit Explicit
    First = 3
    Second = 9
End Enum
Extern "C"
    Enum ExternalState
        ExternalFirst = 3
        ExternalNext
        ExternalLast = 9
    End Enum
    Const ExternalAlias As ExternalState = ExternalFirst
End Extern
Enum
    AnonymousFirst = 6
    AnonymousNext
End Enum
Type NestedOwner
    Enum InnerState Explicit
        First = -8
        Second
    End Enum
    Value As InnerState
End Type
Enum CommaState Explicit
    First = 3, Second, Third = 9, Fourth
End Enum
#Macro DECLARE_STATES()
Private Enum MacroState Explicit
    First = SeedValue + 1
    Second
End Enum
#EndMacro
DECLARE_STATES()
#Include "enum-inputs.bi"
#If 0
Enum InactiveState
    InactiveFirst = 0
End Enum
#EndIf
'''
INCLUDED = '''Private Enum IncludedState Explicit
    First = 2 * 3
    Second
End Enum
Const IncludedAlias As IncludedState = IncludedState.First
'''
EXPECTED = {'MixedState': [1, 0, 1, 0, 1], 'AllImplicit': [0, 0],
            'FirstExplicit': [1, 0, 0], 'AllExplicit': [1, 1],
            'ExternalState': [1, 0, 1], 'InnerState': [1, 0],
            'CommaState': [1, 0, 1, 0], 'MacroState': [1, 0],
            'IncludedState': [1, 0]}


def source_text(body, filename='enum-inputs.bas', dialect='fb'):
    return ("' Project: FreeBASIC semantic sidecar tests\n' File: " + filename + "\n"
            "' Purpose: Exercise parser-owned enum declarations.\n"
            "' Responsibilities: Explicit and implicit original member inputs.\n"
            "' This file intentionally does NOT execute its declarations.\n"
            '#Lang "' + dialect + '"\n' + body + "' end of " + filename + '\n')


def fixture(test):
    test.source(source_text(INCLUDED, 'enum-inputs.bi'), 'enum-inputs.bi')
    return test.source(source_text(BODY))


def check_inputs(test):
    for backend in test.backends:
        with test.subTest(backend=backend):
            source = fixture(test)
            emitted = test.emission_path('enum-inputs', backend)
            extra = ('-o', str(emitted))
            test.invoke([source], mode='off', backend=backend, extra=extra)
            plain = emitted.read_bytes()
            model = test.compile(source, backend=backend, extra=extra)
            test.assertEqual(emitted.read_bytes(), plain)
            test.assertEqual(model.capabilities[1][FEATURE], 'available')
            groups, members = {}, {}
            for (domain, identity), properties in model.properties.items():
                if 'enum-declaration-input' in properties:
                    groups[identity] = properties['enum-declaration-input'].split('\t')
                if 'enum-element-input' in properties:
                    fields = properties['enum-element-input'].split('\t')
                    members.setdefault(int(fields[0]), []).append((int(fields[1]), int(fields[3]), identity))
            test.assertEqual(len(groups), 10)
            test.assertEqual(sum(len(group) for group in members.values()), 27)
            for spelling, explicit in EXPECTED.items():
                owner = test.one(model, spelling)
                ordered = sorted(members[owner])
                test.assertEqual([entry[1] for entry in ordered], explicit)
                test.assertEqual(int(groups[owner][1]), len(explicit))
            for spelling in ('AliasValue', 'TypedAlias', 'ExternalAlias', 'IncludedAlias'):
                identity = test.one(model, spelling)
                test.assertNotIn('enum-element-input', model.properties['symbol', identity])
            external = test.one(model, 'ExternalState')
            external_member = test.one(model, 'ExternalFirst')
            test.assertNotEqual(model.types[external_member][8], str(external))
            anonymous_member = test.one(model, 'AnonymousFirst')
            anonymous = int(model.properties['symbol', anonymous_member]['enum-element-input'].split('\t')[0])
            test.assertTrue(int(model.symbols[anonymous][7]) & 0x01000000)
            test.assertEqual([item[1] for item in sorted(members[anonymous])], [1, 0])
            test.assertFalse(model.named('InactiveState'))
            for mode in ('bindings', 'expressions'):
                compact = test.compile(source, mode=mode, backend=backend, extra=extra)
                test.assertEqual(emitted.read_bytes(), plain)
                test.assertEqual(compact.capabilities[1][FEATURE], 'unavailable')
                test.assertFalse(any(key in properties for properties in compact.properties.values()
                                     for key in ('enum-declaration-input', 'enum-element-input')))


def check_legacy(test):
    body = 'Enum LegacyState\nFirst = 3\nSecond\nThird = 9\nEnd Enum\n'
    for backend in test.backends:
        for dialect in ('deprecated', 'fblite'):
            with test.subTest(backend=backend, dialect=dialect):
                source = test.source(source_text(body, dialect=dialect))
                model = test.compile(source, backend=backend)
                owner = test.one(model, 'LegacyState')
                test.assertEqual(model.properties['symbol', owner]['enum-declaration-input'].split('\t')[1], '3')
                members = [properties['enum-element-input'].split('\t')
                           for properties in model.properties.values() if 'enum-element-input' in properties]
                test.assertEqual([fields[3] for fields in sorted(members, key=lambda item: int(item[1]))], ['1', '0', '1'])


def malformed_rows(rows, mutation):
    """Damage relationships without changing record counts or basic shapes."""
    changed = [row[:] for row in rows]
    header = next(row for row in changed if row[0] == 'K' and row[3] == 'enum-declaration-input')
    member = next(row for row in changed if row[0] == 'K' and row[3] == 'enum-element-input' and row[4].split('%09')[3] == '1')
    implicit = next(row for row in changed if row[0] == 'K' and row[3] == 'enum-element-input' and row[4].split('%09')[3] == '0')
    if mutation == 'missing-header':
        header[3] = 'fixture-unknown-header'
    elif mutation == 'missing-member':
        member[3] = 'fixture-unknown-member'
    elif mutation == 'unavailable':
        next(row for row in changed if row[0] == 'CAP' and row[2] == FEATURE)[3] = 'unavailable'
    elif mutation == 'wrong-domain':
        member[1], member[2] = 'expression', member[4].split('%09')[4]
    elif mutation == 'wrong-class':
        member[2] = header[2]
    else:
        selected = header if mutation in ('wrong-header', 'wrong-count', 'oversized-count') else implicit if mutation == 'false-implicit' else member
        fields = selected[4].split('%09')
        if mutation == 'wrong-header': fields[0] = member[4].split('%09')[2]
        elif mutation == 'wrong-count': fields[1] = str(int(fields[1]) + 1)
        elif mutation == 'oversized-count': fields[1] = '999999999'
        elif mutation == 'wrong-owner': fields[0] = member[2]
        elif mutation == 'repeated-ordinal': fields[1] = implicit[4].split('%09')[1]
        elif mutation == 'wrong-statement': fields[2] = header[4].split('%09')[0]
        elif mutation == 'missing-initializer': fields[4] = '0'
        elif mutation == 'false-implicit': fields[4] = member[4].split('%09')[4]
        elif mutation == 'wrong-initializer': fields[4] = '999999'
        elif mutation == 'foreign-initializer':
            fields[4] = next(row[1] for row in changed if row[0] == 'E' and row[8] == '16' and row[1] != fields[4]
                             and next(own[3] for own in changed if own[0] == 'OWN' and own[1:3] == ['expression', row[1]]) != fields[2])
        else: raise AssertionError(mutation)
        selected[4] = '%09'.join(fields)
    return changed


MUTATIONS = ('missing-header', 'missing-member', 'unavailable', 'wrong-domain', 'wrong-class',
             'wrong-header', 'wrong-count', 'oversized-count', 'wrong-owner', 'repeated-ordinal',
             'wrong-statement', 'missing-initializer', 'false-implicit', 'wrong-initializer', 'foreign-initializer')


def check_rejection(test):
    for backend in test.backends:
        source = fixture(test)
        _, artifact = test.invoke([source], backend=backend)
        rows = [line.split('\t') for line in artifact.read_text().splitlines()]
        for mutation in MUTATIONS:
            with test.subTest(backend=backend, mutation=mutation):
                changed = malformed_rows(rows, mutation)
                malformed = test.working / 'enum-malformed.sem'
                malformed.write_text('\n'.join('\t'.join(row) for row in changed) + '\n')
                with test.assertRaises(ValueError):
                    Model.read(malformed)
        second = test.source(source_text('Enum ForeignState\nForeignFirst = 3\nForeignNext\nEnd Enum\n'), 'foreign.bas')
        _, artifact = test.invoke([source, second], backend=backend)
        rows = [line.split('\t') for line in artifact.read_text().splitlines()]
        foreign = next(row[4].split('%09')[4] for row in rows if row[0] == 'K' and row[3] == 'enum-element-input'
                       and next(symbol[2] for symbol in rows if symbol[0] == 'S' and symbol[1] == row[2]).upper() == 'FOREIGNFIRST')
        member = next(row for row in rows if row[0] == 'K' and row[3] == 'enum-element-input')
        fields = member[4].split('%09')
        fields[4] = foreign
        member[4] = '%09'.join(fields)
        malformed = test.working / 'enum-foreign.sem'
        malformed.write_text('\n'.join('\t'.join(row) for row in rows) + '\n')
        with test.assertRaises(ValueError):
            Model.read(malformed)

# end of enum_inputs.py
