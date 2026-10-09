"""Project: FreeBASIC compiler observations
File: variable_collisions.py
Purpose: Exercise compiler-selected variable redefinition identities.
Responsibilities: Scopes, suffixes, raw spellings and physical reporting sites.
This file intentionally does NOT resolve names or emulate duplicate checks.
"""
from diagnostics import Diagnostics

# Explicit expected points are zero-based name columns within the code block.
CASES = (
    ('single', 'fb', 'Dim Sample As Long', (), True),
    ('collision', 'fb', 'Dim Sample As Long\nDim sample As Long', ((2, 4),), False),
    ('identical', 'fb', 'Dim Sample As Long\nDim Sample As Long', (), False),
    ('different-type', 'fb', 'Dim Sample As Long\nDim sample As String', ((2, 4),), False),
    ('leading-type', 'fb', 'Dim As Long Sample\nDim As Long sample', ((2, 12),), False),
    ('second-name', 'fb', 'Dim first As Long, Sample As Long\nDim other As Long, sample As Long', ((2, 19),), False),
    ('same-statement', 'fb', 'Dim Sample As Long, sample As Long', ((1, 20),), False),
    ('continued', 'fb', 'Dim Sample As Long\nDim _\nsample _\nAs Long', ((3, 0),), False),
    ('array', 'fb', 'Dim Sample(1 To 3) As Long\nDim sample(1 To 3) As Long', ((2, 4),), False),
    ('shared', 'fb', 'Dim Shared Sample As Long\nDim Shared sample As Long', ((2, 11),), False),
    ('static', 'fb', 'Sub Worker()\nStatic Sample As Long\nStatic sample As Long\nEnd Sub', ((3, 7),), False),
    ('auto', 'fb', 'Var Sample = 1\nVar sample = 2', ((2, 4),), False),
    ('parameter', 'fb', 'Sub Worker(ByVal Sample As Long)\nDim sample As Long\nEnd Sub', ((2, 4),), False),
    ('prototype-parameter', 'fb', 'Declare Sub Worker(ByVal Original As Long)\nSub Worker(ByVal Sample As Long)\nDim sample As Long\nEnd Sub', ((3, 4),), False),
    ('parameter-identical', 'fb', 'Sub Worker(ByVal Sample As Long)\nDim Sample As Long\nEnd Sub', (), False),
    ('separate-procedures', 'fb', 'Sub First()\nDim Sample As Long\nEnd Sub\nSub Second()\nDim sample As Long\nEnd Sub', (), True),
    ('nested-scope', 'fb', 'Dim Sample As Long\nScope\nDim sample As Long\nEnd Scope', (), True),
    ('sibling-scopes', 'fb', 'Scope\nDim Sample As Long\nEnd Scope\nScope\nDim sample As Long\nEnd Scope', (), True),
    ('namespaces', 'fb', 'Namespace First\nDim Sample As Long\nEnd Namespace\nNamespace Second\nDim sample As Long\nEnd Namespace', (), True),
    ('extern-repeat', 'fb', 'Extern Sample As Long\nExtern sample As Long', (), True),
    ('extern-definition', 'fb', 'Extern Sample As Long\nDim Shared sample As Long', (), True),
    ('redim', 'fb', 'Dim Sample() As Long\nRedim sample(1 To 3) As Long', (), True),
    ('constant-conflict', 'fb', 'Const Sample = 1\nDim sample As Long', (), False),
    ('procedure-conflict', 'fb', 'Declare Sub Sample()\nDim sample As Long', (), False),
    ('legacy-suffixes', 'fblite', 'Dim Sample%\nDim sample&', (), True),
    ('legacy-same-suffix', 'fblite', 'Dim Sample%\nDim sample%', ((2, 4),), False),
    ('legacy-unsuffixed', 'fblite', 'Dim Sample As Long\nDim sample As String', ((2, 4),), False),
    ('qb-suffixes', 'qb', 'Dim Sample%\nDim sample&', (), True),
    ('qb-same-suffix', 'qb', 'Dim Sample%\nDim sample%', ((2, 4),), False),
    ('inactive', 'fb', 'Dim Sample As Long\n#if 0\nDim sample As Long\n#endif', (), True),
    ('generated-name', 'fb', '#define GeneratedName sample\nDim Sample As Long\nDim GeneratedName As Long', ((3, 0),), False),
    ('generated-declaration', 'fb', '#define GeneratedDecl Dim sample As Long\nDim Sample As Long\nGeneratedDecl', (), False),
    ('remapped', 'fb', 'Dim Sample As Long\n#line 900\nDim sample As Long', ((3, 4),), False),
)


def source_text(language, code):
    return ("' Project: FreeBASIC native variable collision diagnostics\n"
            "' File: generated variable collision case\n"
            "' Purpose: Exercise the compiler's selected variable bindings.\n"
            "' Responsibilities: This case's scope and spelling checks.\n"
            "' This file intentionally does NOT execute rejected declarations.\n"
            f'#lang "{language}"\n{code}\n'
            "' end of generated variable collision case\n")


def expected_points(points):
    return sorted((line + 6, column) for line, column in points)


def check_collisions(test):
    for backend in test.backends:
        for label, language, code, points, succeeded in CASES:
            with test.subTest(backend=backend, variable_collision_case=label):
                source = test.source(source_text(language, code), 'variable-' + label + '.bas')
                artifact = test.working / ('variable-' + label + '-' + backend + '.fbcdia')
                result, _ = test.invoke([source], mode='off', backend=backend, success=succeeded,
                                       extra=('-semantic-diagnostics', str(artifact)))
                observed = Diagnostics.read(artifact, result.returncode)
                test.assertEqual(observed.variable_modules, set(range(1, len(observed.modules) + 1)))
                classified = [row for row in observed.records if row[5] == 'variable-case-collision']
                physical = [(int(row[12]), int(row[13])) for row in classified if row[11] == '1']
                test.assertEqual(sorted(physical), expected_points(points), (label, result.stdout))
                test.assertTrue(all(row[3] == 'error' for row in classified))
                if label == 'generated-declaration':
                    test.assertEqual(len(classified), 1)
                    test.assertEqual(classified[0][11:14], ['0', '0', '0'])

# end of variable_collisions.py
