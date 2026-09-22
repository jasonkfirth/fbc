'' examples/manual/switches/option-gosub.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'OPTION GOSUB'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOptiongosub
'' --------

'' Compile with the "-lang fblite" compiler switch

#lang "fblite"

'' turn on gosub support
'' This page deliberately enables the legacy control-flow dialect feature.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-OPT-007
Option GoSub

GoSub there
backagain:
	Print "backagain"
	End

there:
	Print "there"
	'' The matching GOSUB above establishes the RETURN destination in this lesson.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL-CF-007
	Return
