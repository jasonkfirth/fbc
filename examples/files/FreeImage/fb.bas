'' FreeBASIC FreeImage examples
'' File: fb.bas
'' Purpose: Load common image formats into a 32-bit FB image.
'' Responsibilities: Check copy bounds and release decoder/image resources.
'' This file contains no image writer or FreeImage implementation.

#include "FreeImage.bi"
#include "crt.bi"
#include "fbgfx.bi"

function loadImageFile( filename as string ) as any ptr
	if len( filename ) = 0 then
		return NULL
	end if

	'' find out the image format
	dim as FREE_IMAGE_FORMAT form = FreeImage_GetFileType( filename, 0 )
	if form = FIF_UNKNOWN then
		form = FreeImage_GetFIFFromFilename( filename )
	end if

	'' exit if unknown
	if form = FIF_UNKNOWN then
		return NULL
	end if

	'' always load jpegs accurately
	dim as long flags = 0
	if form = FIF_JPEG then
		flags = JPEG_ACCURATE
	end if
	const MAX_IMAGE_PIXELS = 16 * 1024 * 1024
	'' Where supported, inspect dimensions without acquiring a pixel buffer.
	if FreeImage_FIFSupportsNoPixels(form) then
		dim metadata as FIBITMAP ptr = FreeImage_Load(form, filename, flags or FIF_LOAD_NOPIXELS)
		if metadata = NULL then return NULL
		dim metadata_width as ulong = FreeImage_GetWidth(metadata)
		dim metadata_height as ulong = FreeImage_GetHeight(metadata)
		FreeImage_Unload(metadata)
		if metadata_width = 0 or metadata_height = 0 or _
			culngint(metadata_width) * metadata_height > MAX_IMAGE_PIXELS then return NULL
	end if

	'' load the image into memory
	dim as FIBITMAP ptr image = FreeImage_Load( form, filename, flags )
	if image = NULL then
		'' FreeImage failed to read in the image
		return NULL
	end if

	'' flip the image so it matches fb's coordinate system
	If FreeImage_FlipVertical(image) = 0 Then
		FreeImage_Unload image
		Return NULL
	End If

	'' convert to 32 bits per pixel
	dim as FIBITMAP ptr image32 = FreeImage_ConvertTo32bits( image )
	If image32 = NULL Then
		FreeImage_Unload image
		Return NULL
	End If

	'' get the image's size
	dim as integer w = FreeImage_GetWidth( image )
	dim as integer h = FreeImage_GetHeight( image )
	'' A display example limits its output to 16 million pixels (64 MiB).
	'' Validate before arithmetic or allocating another image-sized buffer.
	If w <= 0 Or h <= 0 OrElse CULngInt(w) * h > MAX_IMAGE_PIXELS Then
		FreeImage_Unload image32
		FreeImage_Unload image
		Return NULL
	End If

	'' create an fb image of the same size
	dim as fb.Image ptr sprite = imagecreate( w, h, 0, 32 )
	If sprite = NULL Then
		FreeImage_Unload image32
		FreeImage_Unload image
		Return NULL
	End If

	dim as byte ptr target = cptr( byte ptr, sprite + 1 )
	dim as integer target_pitch = sprite->pitch

	dim as byte ptr source = FreeImage_GetBits( image32 )
	If source = NULL Then
		ImageDestroy sprite
		FreeImage_Unload image32
		FreeImage_Unload image
		Return NULL
	End If

	dim as integer source_pitch = FreeImage_GetPitch( image32 )
	If source_pitch < w * 4 Or target_pitch < w * 4 Then
		ImageDestroy sprite
		FreeImage_Unload image32
		FreeImage_Unload image
		Return NULL
	End If

	'' and copy over the pixels, row by row
	w *= 4
	for y as integer = 0 to h - 1
		memcpy( target + (y * target_pitch), _
		        source + (y * source_pitch), _
		        w )
	next

	FreeImage_Unload( image32 )
	FreeImage_Unload( image )

	return sprite
end function

#ifndef IMAGE_EXAMPLE_NO_MAIN
	if screenres(640, 480, 32) <> 0 then
		print "could not set the requested graphics mode"
		end 1
	end if

	FreeImage_Initialise( )

	dim as string filename = command(1)
	if filename = "" then filename = exepath() + "/../../fblogo.bmp"

	dim as any ptr image = loadImageFile( filename )
	if image <> NULL then
		put (0, 0), image
	else
		print "problem while loading file : " & filename
		FreeImage_DeInitialise( )
		end 1
	end if

	sleep
	if image <> NULL then
		imagedestroy( image )
	end if

	FreeImage_DeInitialise( )
#endif

'' end of fb.bas
