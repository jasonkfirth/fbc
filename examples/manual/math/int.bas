'' examples/manual/math/int.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'INT'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgInt
'' --------

Print Int(1.9)  '' will print  1
'' A negative input is necessary to demonstrate INT's rounding toward minus infinity.
'' FB-LINTER: DISABLE-NEXT-LINE FBL408 FBL-NUM-014
Print Int(-1.9) '' will print -2
