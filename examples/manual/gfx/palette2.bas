'' examples/manual/gfx/palette2.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'PALETTE'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgPalette
'' --------

' Getting a single color, form 2.
Dim As ULong r, g, b
'' Mode 13 is the documented compatibility mode for this PALETTE lesson.
'' FB-LINTER: DISABLE-NEXT-LINE FBL734
Screen 13
Palette Get 32, r, g, b
Print "Color 32 hues:"
Print Using "Red:### Green:### Blue:###"; r; g; b
Sleep
