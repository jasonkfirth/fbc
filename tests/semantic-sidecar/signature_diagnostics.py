"""Project: FreeBASIC compiler observations
File: signature_diagnostics.py
Purpose: Exercise native procedure contract rejection independently of ASTs.
Responsibilities: Accepted overloads, exact duplicates, actual mismatches and sites.
This file intentionally does NOT resolve BASIC names or compare signatures.
"""
from diagnostics import Diagnostics


# Each anchor is the compiler-selected name of the rejected declaration.
# Empty anchors mean no signature conflict, even when another error rejects it.
CASES = (
    ('single', 'fb', 'Declare Function Sample(ByVal x As Long) As Long', (), True),
    ('identical', 'fb', 'Declare Function Sample() As Long\nDeclare Function Sample() As Long', (), False),
    ('alias-type', 'fb', 'Type Number As Long\nDeclare Function Sample() As Number\nDeclare Function Sample() As Long', (), False),
    ('result', 'fb', 'Declare Function Sample() As Long\nDeclare Function Sample() As String', ('Declare Function Sample() As String',), False),
    ('arity', 'fb', 'Declare Sub Sample(ByVal x As Long)\nDeclare Sub Sample()', ('Declare Sub Sample()',), False),
    ('parameter', 'fb', 'Declare Sub Sample(ByVal x As Long)\nDeclare Sub Sample(ByVal x As String)', ('Declare Sub Sample(ByVal x As String)',), False),
    ('mode', 'fb', 'Declare Sub Sample(ByVal x As Long)\nDeclare Sub Sample(ByRef x As Long)', ('Declare Sub Sample(ByRef x As Long)',), False),
    ('rank', 'fb', 'Declare Sub Sample(x() As Long)\nDeclare Sub Sample(x(Any, Any) As Long)', ('Declare Sub Sample(x(Any, Any) As Long)',), False),
    ('default', 'fblite', 'Declare Sub Sample(ByVal x As Long = 1)\nDeclare Sub Sample(ByVal x As Long = 2)', ('Declare Sub Sample(ByVal x As Long = 2)',), False),
    ('optional', 'fblite', 'Declare Sub Sample(ByVal x As Long = 1)\nDeclare Sub Sample(ByVal x As Long)', ('Declare Sub Sample(ByVal x As Long)',), False),
    ('alias', 'fb', 'Declare Sub Sample Alias "first"()\nDeclare Sub Sample Alias "second"()', ('Declare Sub Sample Alias "second"()',), False),
    ('convention', 'fb', 'Declare Sub Sample Cdecl()\nDeclare Sub Sample Stdcall()', ('Declare Sub Sample Stdcall()',), False),
    ('byref-result', 'fb', 'Declare Function Sample() ByRef As Long\nDeclare Function Sample() As Long', ('Declare Function Sample() As Long',), False),
    ('kind', 'fb', 'Declare Sub Sample()\nDeclare Function Sample() As Long', ('Declare Function Sample() As Long',), False),
    ('namespaces', 'fb', 'Namespace First\nDeclare Sub Sample()\nEnd Namespace\nNamespace Second\nDeclare Sub Sample(ByVal x As String)\nEnd Namespace', (), True),
    ('members', 'fb', 'Type First\nvalue As Long\nDeclare Sub Sample()\nEnd Type\nType Second\nvalue As Long\nDeclare Sub Sample(ByVal x As String)\nEnd Type', (), True),
    ('overloads', 'fb', 'Declare Sub Sample Overload(ByVal x As Long)\nDeclare Sub Sample Overload(ByVal x As String)', (), True),
    ('overload-result', 'fb', 'Declare Function Sample Overload(ByVal x As Long) As Long\nDeclare Function Sample Overload(ByVal x As Long) As String', ('Declare Function Sample Overload(ByVal x As Long) As String',), False),
    ('body-result', 'fb', 'Declare Function Sample() As Long\nFunction Sample() As String\nReturn "value"\nEnd Function', ('Function Sample() As String',), False),
    ('body-mode', 'fb', 'Declare Sub Sample(ByVal x As Long)\nSub Sample(ByRef x As Long)\nEnd Sub', ('Sub Sample(ByRef x As Long)',), False),
    ('body-byref', 'fb', 'Declare Function Sample() ByRef As Long\nFunction Sample() As Long\nReturn 1\nEnd Function', ('Function Sample() As Long',), False),
    ('body-default-warning', 'fb', 'Declare Sub Sample(ByVal x As Long = 1)\nSub Sample(ByVal x As Long = 2)\nEnd Sub', (), True),
    ('legacy-repeat', 'fblite', 'Declare Sub Sample(ByVal x As Long = 1)\nDeclare Sub Sample(ByVal y As Long = 1)', (), True),
    ('continued', 'fb', 'Declare Function Sample() As Long\nDeclare Function Sample( _\nByVal x As String _\n) As String', ('Declare Function Sample( _',), False),
    ('generated-name', 'fb', '#define GeneratedName Sample\nDeclare Function Sample() As Long\nDeclare Function GeneratedName() As String', ('Declare Function GeneratedName() As String',), False),
    ('multiple-errors', 'fb', 'Declare Function Sample(ByVal x As Long) As Long\nFunction Sample(ByRef x As String) As String\nReturn "value"\nEnd Function\nDim broken As MissingType', ('Function Sample(ByRef x As String) As String',), False),
    ('remapped', 'fb', 'Declare Sub Sample()\n#line 900\nDeclare Sub Sample(ByVal x As String)', ('Declare Sub Sample(ByVal x As String)',), False),
    ('inactive', 'fb', 'Declare Sub Sample()\n#if 0\nDeclare Sub Sample(ByVal x As String)\n#endif', (), True),
    ('unrelated-error', 'fb', 'Declare Function Sample() As Long\nDim Sample As Long', (), False),
    ('invalid-header', 'fb', 'Declare Sub Sample(ByVal x As Long)\nDeclare Sub Sample(ByVal x As MissingType)', (), False),
    ('invalid-body-header', 'fb', 'Declare Sub Sample(ByVal x As Long)\nSub Sample(ByVal x As MissingType)\nEnd Sub', (), False),
    ('earlier-invalid-header', 'fb', 'Declare Function Sample() As MissingType\nDeclare Function Sample() As String', (), False),
)


def source_text(language, code):
    return ("' Project: FreeBASIC native signature diagnostics\n"
            "' File: generated signature case\n"
            "' Purpose: Exercise the compiler's selected declaration contracts.\n"
            "' Responsibilities: This case's named prototype and body checks.\n"
            "' This file intentionally does NOT execute the declared procedures.\n"
            f'#lang "{language}"\n{code}\n'
            "' end of generated signature case\n")


def expected_points(source, anchors):
    # Test markers locate reporting sites only. Signature classification is
    # asserted from native DI kinds, never derived from these source strings.
    points = []
    for anchor in anchors:
        matches = [(line, text) for line, text in enumerate(source.splitlines(), 1) if text == anchor]
        assert len(matches) == 1, anchor
        line, text = matches[0]
        points.append((line, text.index('Sample') if 'Sample' in text else text.index('Function')))
    return sorted(points)


def check_signatures(test):
    for backend in test.backends:
        for label, language, code, anchors, succeeded in CASES:
            with test.subTest(backend=backend, signature_case=label):
                text = source_text(language, code)
                source = test.source(text, 'signature-' + label + '.bas')
                artifact = test.working / ('signature-' + label + '-' + backend + '.fbcdia')
                result, _ = test.invoke([source], mode='off', backend=backend, success=succeeded,
                                       extra=('-semantic-diagnostics', str(artifact)))
                diagnostics = Diagnostics.read(artifact, result.returncode)
                test.assertEqual(diagnostics.signature_modules, set(range(1, len(diagnostics.modules) + 1)))
                classified = [row for row in diagnostics.records if row[5] == 'procedure-signature-mismatch']
                points = sorted({(int(row[12]), int(row[13])) for row in classified})
                test.assertEqual(points, expected_points(text, anchors), (label, result.stdout))
                test.assertTrue(all(row[3] == 'error' and row[11] == '1' for row in classified))
                test.assertTrue(all(row[6] == test.compiler_path(source) for row in classified))

# end of signature_diagnostics.py
