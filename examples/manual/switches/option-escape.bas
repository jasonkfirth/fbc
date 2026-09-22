'' examples/manual/switches/option-escape.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'OPTION ESCAPE'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgOptionescape
'' --------

'' Compile with the "-lang fblite" compiler switch

#lang "fblite"

Option Escape

'' These backslashes are the values demonstrated by OPTION ESCAPE.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-OPT-005
Print "Warning \a\t The path is:\r\n c:\\Freebasic\\Examples"
'' FB-LINTER: DISABLE-NEXT-LINE FBL-OPT-005
Print $"This string doesn't have expanded escape sequences: \r\n\t"

#include "crt.bi"

Dim As Long a = 2
Dim As Long b = 3
'' FB-LINTER: DISABLE-NEXT-LINE FBL-OPT-005
printf("%d * %d = %d\r\n", a, b, CLng(a * b))

'' end of option-escape.bas
