'' examples/manual/gfx/flip.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'FLIP'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgFlip
'' --------

' Set up a 320x240 32-bit screen with two video pages.
If ScreenRes( 320, 240, 32, 2 ) <> 0 Then
	Print "Could not set the requested graphics mode"
	Sleep
	End 1
End If
ScreenSet 1, 0                'Sets the working page to 1 and the displayed page to 0


For n As Integer = 50 To 270

	Cls
	Circle (n, 50), 50 , RGB(255, 255, 0) 'Draws a circle with a 50 pixel radius in yellow on page 1
	Flip 1, 0    'Copies our circle from page 1 to page 0

	Sleep 25
Next

Print "Now wasn't that neat!"
Print "Push any key."
Flip 1, 0    'Copies our text from page 1 to page 0
Sleep
