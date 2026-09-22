'' examples/manual/switches/option-dynamic.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'OPTION DYNAMIC'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOptiondynamic
'' --------

'' Compile with "-lang fblite" compiler switch

#lang "fblite"

'' The declaration order intentionally shows that OPTION DYNAMIC affects only
'' declarations that follow it.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-OPT-003
Dim foo(99) As Integer      ' declares a fixed-length array

'' FB-LINTER: DISABLE-NEXT-LINE FBL-OPT-001
Option Dynamic

Dim bar(0 To 99) As Integer  ' declares a variable-length array
ReDim bar(0 To 199)          ' resize the array
For index As Integer = LBound(bar) To UBound(bar)
	bar(index) = index
Next index

'' end of option-dynamic.bas
