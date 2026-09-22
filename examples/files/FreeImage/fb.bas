'' Example for loading all common image types using FreeImage into an FB image,
'' for use with the FB graphics functions.

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
	dim as uinteger flags = 0
	if form = FIF_JPEG then
		flags = JPEG_ACCURATE
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
	If w <= 0 Or h <= 0 Then
		FreeImage_Unload image32
		FreeImage_Unload image
		Return NULL
	End If

	'' create an fb image of the same size
	dim as fb.Image ptr sprite = imagecreate( w, h )
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

	if screenres(640, 480, 32) <> 0 then
		print "could not set the requested graphics mode"
		sleep
		end 1
	end if

	FreeImage_Initialise( )

	dim as string filename = exepath() + "/../../fblogo.bmp"

	dim as any ptr image = loadImageFile( filename )
	if image <> NULL then
		put (0, 0), image
	else
		print "problem while loading file : " & filename
	end if

	sleep
	if image <> NULL then
		imagedestroy( image )
	end if

	FreeImage_DeInitialise( )
