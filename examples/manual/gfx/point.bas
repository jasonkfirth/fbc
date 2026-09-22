'' examples/manual/gfx/point.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'POINT'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgPoint
'' --------

' Set an appropriate screen mode - 320 x 240 x 8bpp indexed color
If ScreenRes( 320, 240, 8 ) <> 0 Then
	Print "Could not set the requested graphics mode"
	Sleep
	End 1
End If

' Draw a line using color 12 (light red)
Line (20, 20)-(100, 100), 12

' Print the color of a point on the line
Print Point(20, 20)

' Sleep before the program closes
Sleep
