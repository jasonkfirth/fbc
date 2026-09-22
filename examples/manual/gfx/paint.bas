'' examples/manual/gfx/paint.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'PAINT'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgPaint
'' --------

' draws a white circle painted blue inside
'' Mode 13 is the documented compatibility mode for this PAINT lesson.
'' FB-LINTER: DISABLE-NEXT-LINE FBL734
Screen 13
Circle (160, 100), 30, 15
Paint (160, 100), 1, 15
Sleep
