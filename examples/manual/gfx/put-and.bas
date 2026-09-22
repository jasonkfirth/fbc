'' examples/manual/gfx/put-and.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'AND'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgAndGfx
'' --------

''open a graphics window
If ScreenRes(320, 200, 16) <> 0 Then
	Print "Could not set the requested graphics mode"
	Sleep
	End 1
End If

Line (0, 0)-(319, 199), RGB(255, 255, 255), bf

''create 3 sprites containing cyan, magenta and yellow circles
Const As Integer r = 32
Dim As Any Ptr cc, cm, cy
cc = ImageCreate(r * 2 + 1, r * 2 + 1, RGBA(255, 255, 255, 255))

If cc = 0 Then
	Print "Could not create the cyan sprite"
	Sleep
	End 1
End If

cm = ImageCreate(r * 2 + 1, r * 2 + 1, RGBA(255, 255, 255, 255))

If cm = 0 Then
	ImageDestroy cc
	Print "Could not create the magenta sprite"
	Sleep
	End 1
End If

cy = ImageCreate(r * 2 + 1, r * 2 + 1, RGBA(255, 255, 255, 255))

If cy = 0 Then
	ImageDestroy cc
	ImageDestroy cm
	Print "Could not create the yellow sprite"
	Sleep
	End 1
End If

Circle cc, (r, r), r, RGB(0, 255, 255), , , 1, f
Circle cm, (r, r), r, RGB(255, 0, 255), , , 1, f
Circle cy, (r, r), r, RGB(255, 255, 0), , , 1, f

''put the three sprites, overlapping each other in the middle
Put (146 - r, 108 - r), cc, And
Put (174 - r, 108 - r), cm, And
Put (160 - r,  84 - r), cy, And

''free the memory used by the sprites
ImageDestroy cc
ImageDestroy cm
ImageDestroy cy

''pause the program before closing
Sleep
