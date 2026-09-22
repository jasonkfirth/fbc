'' examples/manual/gfx/put-xor.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'XOR'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgXorGfx
'' --------

#include Once "fbgfx.bi"

''open a graphics window
If ScreenRes(320, 200, 16) <> 0 Then
	Print "Could not set the requested graphics mode"
	Sleep
	End 1
End If

''create a sprite containing a circle
Const As Integer r = 32
Dim c As fb.Image Ptr = ImageCreate(r * 2 + 1, r * 2 + 1, 0)

If c = 0 Then
	Print "Could not create the sprite"
	Sleep
	End 1
End If

Circle c, (r, r), r, RGBA(255, 255, 255, 0), , , 1, f

''put the three sprites, overlapping each other in the middle
Put (146 - r, 108 - r), c, Xor
Put (174 - r, 108 - r), c, Xor
Put (160 - r,  84 - r), c, Xor

''free the memory used by the sprite
ImageDestroy c

''pause the program before closing
Sleep
