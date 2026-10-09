"""Project: FreeBASIC semantic sidecar tests
File: namespace_inputs.py
Purpose: Qualify parser-selected namespace openings and unchanged code emission.
Responsibilities: Native macro counterexamples, compact exports and damaged facts.
This file intentionally does NOT implement header naming or lint policy.
"""
from sidecar import Model, unescape
from semantic_namespace_inputs import FEATURE, PREFIX

CASES = {
    'anonymous': ('Namespace\nEnd Namespace\n', [1]),
    'named': ('Namespace NamedScope\nEnd Namespace\n', [0]),
    'qualified': ('Namespace OuterScope.InnerScope\nEnd Namespace\n', [0, 0]),
    'reopened': ('Namespace NamedScope\nEnd Namespace\nNamespace NamedScope\nEnd Namespace\n', [0, 0]),
    'name-macro': ('#define NSNAME NamedScope\nNamespace NSNAME\nEnd Namespace\n', [0]),
    'empty-name-macro': ('#define NSNAME\nNamespace NSNAME\nEnd Namespace\n', [1]),
    'named-whole-macro': ('#macro OpenScope?()\nNamespace NamedScope\n#endmacro\nOpenScope()\nEnd Namespace\n', [0]),
    'anonymous-whole-macro': ('#macro OpenScope?()\nNamespace\n#endmacro\nOpenScope()\nEnd Namespace\n', [1]),
    'nested-name-macro': ('#define NamePiece NamedScope\n#macro OpenScope?()\nNamespace NamePiece\n#endmacro\nOpenScope()\nEnd Namespace\n', [0]),
    'nested-anonymous': ('Namespace OuterScope\nNamespace\nEnd Namespace\nEnd Namespace\n', [0, 1]),
    'named-alias': ('Namespace NamedScope Alias "OtherAlias"\nEnd Namespace\n', [0]),
    'colon-scopes': ('Namespace : End Namespace : Namespace NamedScope : End Namespace\n', [1, 0]),
    'continued-named': ('Namespace _\nNamedScope\nEnd Namespace\n', [0]),
    'continued-anonymous': ("Namespace _\n' no written name\nEnd Namespace\n", [1]),
    'discarded-name': ('#define IgnoreName(x)\nNamespace IgnoreName(NotANamespace)\nEnd Namespace\n', [1]),
    'inactive': ('#if 0\nNamespace\nEnd Namespace\n#endif\nNamespace NamedScope\nEnd Namespace\n', [0]),
    'logical-line': ('#line 800 "logical.bas"\n#macro OpenScope?()\nNamespace\n#endmacro\nOpenScope()\nEnd Namespace\n', [1]),
}

MUTATIONS = ('missing', 'duplicate', 'wrong-block', 'wrong-statement', 'wrong-owner',
             'nonnamespace', 'bad-flag', 'noncanonical-key', 'noncanonical-value',
             'extra-value', 'missing-capability', 'unavailable-capability')


def serialized(rows):
    counts = {tag: sum(row[0] == tag for row in rows[1:-1])
              for tag in ('M', 'P', 'S', 'V', 'N', 'E', 'B', 'I', 'D')}
    for position, tag in enumerate(counts, 2):
        rows[-1][position] = str(counts[tag])
    rows[-1][12] = str(len(rows) - 2 - sum(counts.values()))
    return '\n'.join('\t'.join(row) for row in rows) + '\n'


def damaged(rows, mutation):
    rows = [row.copy() for row in rows]
    fact = next(row for row in rows if row[0] == 'K' and row[3].startswith(PREFIX))
    fields = unescape(fact[4]).split('\t')
    if mutation == 'missing': rows.remove(fact)
    elif mutation == 'duplicate': rows.insert(-1, fact.copy())
    elif mutation == 'wrong-block': fact[3] = PREFIX + '99999999'
    elif mutation == 'wrong-statement': fields[0] = '99999999'
    elif mutation == 'wrong-owner': fact[2] = fields[1]
    elif mutation == 'nonnamespace': fields[1] = next(row[1] for row in rows if row[0] == 'S' and row[3] != '8')
    elif mutation == 'bad-flag': fields[2] = '2'
    elif mutation == 'noncanonical-key': fact[3] = PREFIX + '0' + fact[3][len(PREFIX):]
    elif mutation == 'noncanonical-value': fields[1] = '0' + fields[1]
    elif mutation == 'extra-value': fields.append('0')
    elif mutation in ('missing-capability', 'unavailable-capability'):
        capability = next(row for row in rows if row[:3] == ['CAP', '1', FEATURE])
        if mutation == 'missing-capability': rows.remove(capability)
        else: capability[3] = 'unavailable'
    else: raise AssertionError(mutation)
    fact[4] = '%09'.join(fields)
    return serialized(rows)


def check_namespace_inputs(test):
    for backend in test.backends:
        for name, (text, expected) in CASES.items():
            with test.subTest(backend=backend, case=name):
                source = test.source('#lang "fb"\n' + text, name + '.bas')
                emitted = test.emission_path('namespace-output', backend)
                extra = ('-o', str(emitted))
                baseline_result, _ = test.invoke([source], mode='off', backend=backend, extra=extra)
                baseline = emitted.read_bytes()
                result, artifact = test.invoke([source], backend=backend, extra=extra)
                model = Model.read(artifact)
                test.assertEqual(emitted.read_bytes(), baseline)
                test.assertEqual((result.stdout, result.stderr), (baseline_result.stdout, baseline_result.stderr))
                facts = [row for row in model.records['K'] if row[3].startswith(PREFIX)]
                test.assertEqual([int(row[4].split('\t')[2]) for row in facts], expected)
                test.assertEqual(model.capabilities[1][FEATURE], 'available')
                for mode in ('bindings', 'expressions'):
                    compact = test.compile(source, mode=mode, backend=backend, extra=extra)
                    test.assertEqual(emitted.read_bytes(), baseline)
                    test.assertEqual(compact.capabilities[1][FEATURE], 'unavailable')
                    test.assertFalse(any(row[3].startswith(PREFIX) for row in compact.records['K']))
                if name == 'anonymous':
                    rows = [line.split('\t') for line in artifact.read_text().splitlines()]
                    for mutation in MUTATIONS:
                        with test.subTest(mutation=mutation), test.assertRaises(ValueError):
                            Model(damaged(rows, mutation))
                    old_rows = [row.copy() for row in rows if not
                                (row[0] == 'CAP' and row[2] == FEATURE or
                                 row[0] == 'K' and row[3].startswith(PREFIX))]
                    Model(serialized(old_rows))

# end of namespace_inputs.py
