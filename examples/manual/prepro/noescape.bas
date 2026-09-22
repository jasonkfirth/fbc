'' examples/manual/prepro/noescape.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'Operator $ (Non-Escaped String Literal)'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOpPpNoescape
'' --------

'' Compile with -lang fblite or qb

#lang "fblite"

Print "Default"
Print "Backslash  : \\"
Print !"Backslash !: \\"
Print $"Backslash $: \\"
Print

'' This deliberate late option begins the second half of the literal comparison.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-OPT-001
Option Escape

Print "Option Escape"
'' These three literals demonstrate the changed default while preserving the
'' ! and $ prefixes as distinct literal forms.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-OPT-005
Print "Backslash  : \\"
'' FB-LINTER: DISABLE-NEXT-LINE FBL-OPT-005
Print !"Backslash !: \\"
'' FB-LINTER: DISABLE-NEXT-LINE FBL-OPT-005
Print $"Backslash $: \\"
Print

'' OUTPUT:

'' Default
'' Backslash  : \\
'' Backslash !: \
'' Backslash $: \\

'' Option Escape
'' Backslash  : \
'' Backslash !: \
'' Backslash $: \\

'' end of noescape.bas
