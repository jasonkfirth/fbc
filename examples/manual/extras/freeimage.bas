'' examples/manual/extras/freeimage.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'FreeImage'
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ExtLibfreeimage
'' --------
''
'' Purpose: Load common image formats into an owned 32-bit FB image.
'' Responsibilities: Check copy bounds and release decoder/image resources.
'' This file contains no image writer or FreeImage implementation.

'' Code example for loading all common image types using FreeImage.
'' The example loads an image passed as a command line argument.

'' The function FI_Load returns a null pointer (0) if there was an error during
'' loading.  Otherwise it returns a 32-bit PUT compatible buffer.

#include "FreeImage.bi"
#include "crt.bi"
#include "fbgfx.bi"

Function FI_Load(filename As String) As Any Ptr
	If Len(filename) = 0 Then
		Return NULL
	End If

	'' Find out the image format
	Dim As FREE_IMAGE_FORMAT form = FreeImage_GetFileType(StrPtr(filename), 0)
	If form = FIF_UNKNOWN Then
		form = FreeImage_GetFIFFromFilename(StrPtr(filename))
	End If

	'' Exit if unknown
	If form = FIF_UNKNOWN Then
		Return NULL
	End If

	'' Always load jpegs accurately
	Dim As Long flags = 0
	If form = FIF_JPEG Then
		flags = JPEG_ACCURATE
	End If
	Const MAX_IMAGE_PIXELS = 16 * 1024 * 1024
	'' Inspect supported formats without acquiring their pixel buffer.
	If FreeImage_FIFSupportsNoPixels(form) Then
		Dim metadata As FIBITMAP Ptr = FreeImage_Load(form, filename, flags Or FIF_LOAD_NOPIXELS)
		If metadata = NULL Then Return NULL
		Dim metadata_width As ULong = FreeImage_GetWidth(metadata)
		Dim metadata_height As ULong = FreeImage_GetHeight(metadata)
		FreeImage_Unload(metadata)
		If metadata_width = 0 Or metadata_height = 0 Or _
			CULngInt(metadata_width) * metadata_height > MAX_IMAGE_PIXELS Then Return NULL
	End If

	'' Load the image into memory
	Dim As FIBITMAP Ptr image = FreeImage_Load(form, StrPtr(filename), flags)
	If image = NULL Then
		'' FreeImage failed to read in the image
		Return NULL
	End If

	'' Flip the image so it matches FB's coordinate system
	If FreeImage_FlipVertical(image) = 0 Then
		FreeImage_Unload image
		Return NULL
	End If

	'' Convert to 32 bits per pixel
	Dim As FIBITMAP Ptr image32 = FreeImage_ConvertTo32Bits(image)
	If image32 = NULL Then
		FreeImage_Unload image
		Return NULL
	End If

	'' Get the image's size
	Dim As UInteger w = FreeImage_GetWidth(image)
	Dim As UInteger h = FreeImage_GetHeight(image)
	'' Limit display output to 16 million pixels (64 MiB at 32 bpp).
	If w = 0 Or h = 0 OrElse CULngInt(w) * h > MAX_IMAGE_PIXELS Then
		FreeImage_Unload image32
		FreeImage_Unload image
		Return NULL
	End If

	'' Create an FB image of the same size
	Dim As fb.Image Ptr sprite = ImageCreate(w, h, 0, 32)
	If sprite = NULL Then
		FreeImage_Unload image32
		FreeImage_Unload image
		Return NULL
	End If

	Dim As Byte Ptr target = CPtr(Byte Ptr, sprite + 1)
	Dim As Integer target_pitch = sprite->pitch

	Dim As Byte Ptr source = FreeImage_GetBits(image32)
	If source = NULL Then
		ImageDestroy sprite
		FreeImage_Unload image32
		FreeImage_Unload image
		Return NULL
	End If

	Dim As Integer source_pitch = FreeImage_GetPitch(image32)
	If source_pitch < CULngInt(w) * 4 Or target_pitch < CULngInt(w) * 4 Then
		ImageDestroy sprite
		FreeImage_Unload image32
		FreeImage_Unload image
		Return NULL
	End If

	'' And copy over the pixels, row by row
	For y As Integer = 0 To (h - 1)
		memcpy(target + (y * target_pitch), _
			   source + (y * source_pitch), _
			   w * 4)
	Next

	FreeImage_Unload(image32)
	FreeImage_Unload(image)

	Return sprite
End Function

#ifndef IMAGE_EXAMPLE_NO_MAIN
If ScreenRes(640, 480, 32) <> 0 Then
	Print "Could not set the requested graphics mode"
	End 1
End If

Dim As String filename = Command(1)
FreeImage_Initialise()

Dim As Any Ptr image = FI_Load(filename)
If image <> 0 Then
	Put (0, 0), image
	ImageDestroy(image)
Else
	Print "Problem while loading file : " & filename
	FreeImage_DeInitialise()
	End 1
End If

Sleep
FreeImage_DeInitialise()
#endif

'' end of freeimage.bas
