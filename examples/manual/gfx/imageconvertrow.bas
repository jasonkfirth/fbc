'' examples/manual/gfx/imageconvertrow.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'IMAGECONVERTROW'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=KeyPgImageConvertRow
'' --------

#include "fbgfx.bi"
#if __FB_LANG__ = "fb"
Using FB
#endif

Const As Long w = 64, h = 64
Dim As IMAGE Ptr img8 = 0, img32 = 0
Dim As Integer x, y

Sub releaseImages(ByRef image8 As IMAGE Ptr, ByRef image32 As IMAGE Ptr)

	If image8 <> 0 Then
		ImageDestroy image8
		image8 = 0
	End If

	If image32 <> 0 Then
		ImageDestroy image32
		image32 = 0
	End If

End Sub


'' create a 32-bit image, size w*h:
If ScreenRes(1, 1, 32, , GFX_NULL) <> 0 Then
	Print "Could not set the requested graphics mode"
	Sleep
	End 1
End If

img32 = ImageCreate(w, h)

If img32 = 0 Then
	Print "ImageCreate failed on img32!"
	Sleep
	End 1
End If


'' create an 8-bit image, size w*h:
If ScreenRes(1, 1, 8, , GFX_NULL) <> 0 Then
	releaseImages img8, img32
	Print "Could not set the requested graphics mode"
	Sleep
	End 1
End If

img8 = ImageCreate(w, h)

If img8 = 0 Then
	releaseImages img8, img32
	Print "ImageCreate failed on img8!"
	Sleep
	End 1
End If


'' fill 8-bit image with a pattern
For y = 0 To h - 1
	For x = 0 To w - 1
		PSet img8, (x, y), 56 + (x + y) Mod 24
	Next x
Next y


'' open a graphics window in 8-bit mode, and PUT the image into it:
If ScreenRes(320, 200, 8) <> 0 Then
	releaseImages img8, img32
	Print "Could not set the requested graphics mode"
	Sleep
	End 1
End If

WindowTitle "8-bit color mode"
Put (10, 10), img8

Sleep


'' copy the image data into a 32-bit image
Dim As Byte Ptr p8, p32
Dim As Long pitch8, pitch32

#ifndef ImageInfo '' older versions of FB don't have the ImageInfo feature
#define GETPITCH(img_) IIf(img_->Type=PUT_HEADER_NEW,img_->pitch,img_->old.width*img_->old.bpp)
#define GETP(img_) CPtr(Byte Ptr,img_)+IIf(img_->Type=PUT_HEADER_NEW,SizeOf(PUT_HEADER),SizeOf(_OLD_HEADER))
pitch8 = GETPITCH(img8): p8 = GETP(img8)
pitch32 = GETPITCH(img32): p32 = GETP(img32)
#else
ImageInfo( img8,  , , , pitch8,  p8  )
ImageInfo( img32, , , , pitch32, p32 )
#endif

For y = 0 To h - 1
	ImageConvertRow(@p8 [ y * pitch8 ],  8, _
					@p32[ y * pitch32], 32, _
					w)
Next y


'' open a graphics window in 32-bit mode and PUT the image into it:
If ScreenRes(320, 200, 32) <> 0 Then
	releaseImages img8, img32
	Print "Could not set the requested graphics mode"
	Sleep
	End 1
End If

WindowTitle "32-bit color mode"
Put (10, 10), img32

Sleep


'' free the images from memory:
releaseImages img8, img32
