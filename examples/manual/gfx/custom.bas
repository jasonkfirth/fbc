'' examples/manual/gfx/custom.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'CUSTOM'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgCustomgfx
'' --------

Const DEFAULT_THRESHOLD As Single = 0.5
Const LOWER_THRESHOLD As Single = 0.2
Const UPPER_THRESHOLD As Single = 0.8
Const IMAGE_HALF_SIZE As Integer = 16
Const LOWER_CENTER_X As Integer = 80
Const CENTER_X As Integer = 160
Const UPPER_CENTER_X As Integer = 240
Const CENTER_Y As Integer = 100

Function dither ( ByVal source_pixel As ULong, ByVal destination_pixel As ULong, ByVal parameter As Any Ptr ) As ULong

	''either returns the source pixel or the destination pixel, depending on the outcome of rnd

	Dim threshold As Single = DEFAULT_THRESHOLD
	If parameter <> 0 Then threshold = *CPtr(Single Ptr, parameter)

	If Rnd() < threshold Then
		Return source_pixel
	Else
		Return destination_pixel
	End If

End Function


Dim img As Any Ptr, threshold As Single

'' set up a screen
Randomize
If ScreenRes(320, 200, 16, 2) <> 0 Then
	Print "Could not set the graphics mode"
Else
	ScreenSet 0, 1

  '' create an image
	img = ImageCreate(32, 32)
	If img = 0 Then
		Print "Could not create the image"
	Else
		Line img, ( 0,  0)-(15,  15), RGB(255,   0,   0), bf
		Line img, (16,  0)-(31,  15), RGB(  0,   0, 255), bf
		Line img, ( 0, 16)-(15,  31), RGB(  0, 255,   0), bf
		Line img, (16, 16)-(31,  31), RGB(255,   0, 255), bf

		'' dither the image with varying thresholds
		Do Until Len(Inkey)

			Cls

			threshold = LOWER_THRESHOLD
			Put (LOWER_CENTER_X - IMAGE_HALF_SIZE, CENTER_Y - IMAGE_HALF_SIZE), img, Custom, @dither, @threshold

			'' default threshold = DEFAULT_THRESHOLD
			Put (CENTER_X - IMAGE_HALF_SIZE, CENTER_Y - IMAGE_HALF_SIZE), img, Custom, @dither

			threshold = UPPER_THRESHOLD
			Put (UPPER_CENTER_X - IMAGE_HALF_SIZE, CENTER_Y - IMAGE_HALF_SIZE), img, Custom, @dither, @threshold

			ScreenCopy
			'' The dither preview polls keyboard input and yields between frames.
			'' FB-LINTER: DISABLE-NEXT-LINE FBL602
			Sleep 25

		Loop

		'' free the image memory
		ImageDestroy img
	End If
End If
