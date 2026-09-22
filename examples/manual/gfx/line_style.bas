'' examples/manual/gfx/line_style.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'LINE (GRAPHICS)'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgLinegraphics
'' --------

' Draws 2 lines with 2 different line styles in 2 different colors
If ScreenRes( 320, 240 ) <> 0 Then
	Print "Could not set the requested graphics mode"
	Sleep
	End 1
End If

Line (10, 100)-(309, 140),  4, B, &b1010101010101010 ' red box with dashed border

Line (20, 115)-(299, 115),  9,  , &b1111000011111111 ' blue dashed line
Line (20, 125)-(299, 125), 10,  , &b0000000011110000 ' green dashed line

Sleep
