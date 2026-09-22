'' examples/manual/math/fix.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'FIX'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgFix
'' --------

Print Fix(1.9)  '' will print  1
'' A negative input is necessary to demonstrate FIX's truncation toward zero.
'' FB-LINTER: DISABLE-NEXT-LINE FBL408 FBL-NUM-014
Print Fix(-1.9) '' will print -1
