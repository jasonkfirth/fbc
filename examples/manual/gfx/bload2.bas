'' examples/manual/gfx/bload2.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'BLOAD'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgBload
'' --------

'Load a 48x48 bitmap into an image
If ScreenRes(320, 200, 32) <> 0 Then
	Print "Could not set the requested graphics mode"
	Sleep
	End 1
End If

Dim myImage As Any Ptr = ImageCreate( 48, 48 )

If myImage = 0 Then
	Print "Could not create the image buffer"
	Sleep
	End 1
End If

If BLoad("picture.bmp", myImage) <> 0 Then
	ImageDestroy myImage
	Print "Could not load picture.bmp"
	Sleep
	End 1
End If

Put (10, 10), myImage
ImageDestroy( myImage )
Sleep
