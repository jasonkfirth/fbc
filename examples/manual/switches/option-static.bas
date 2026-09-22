'' examples/manual/switches/option-static.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'OPTION STATIC'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOptionstatic
'' --------

'' Compile with the "-lang fblite" compiler switch

#lang "fblite"

Option Dynamic

Dim foo(0 To 100) As Integer		' declares a variable-length array

'' This deliberate mid-file change contrasts the prior dynamic declaration
'' with the fixed declaration below.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-OPT-001 FBL-OPT-002
Option Static

Dim bar(0 To 100) As Integer		' declares a fixed-length array

'' end of option-static.bas
