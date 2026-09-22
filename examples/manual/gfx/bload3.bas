'' examples/manual/gfx/bload3.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'BLOAD'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgBload
'' --------

If ScreenRes(640, 480, 8) <> 0 Then '' 8-bit palette graphics mode
	Print "Could not set the requested graphics mode"
	Sleep
	End 1
End If

Dim pal(0 To 256-1) As Integer '' 32-bit integer array with room for 256 colors

'' load bitmap to screen, put palette into pal() array
If BLoad("picture.bmp", , @pal(0)) <> 0 Then
	Print "Could not load picture.bmp"
	Sleep
	End 1
End If

WindowTitle "Old palette"
Sleep

'' set new palette from pal() array
Palette Using pal(0)

WindowTitle "New palette"
Sleep
