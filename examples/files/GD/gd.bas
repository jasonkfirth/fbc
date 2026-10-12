'' FreeBASIC GD examples
'' File: gd.bas
'' Purpose: Draw an image and save it as GIF, PNG and JPEG.
'' Responsibilities: Release native buffers and report encoding/write failures.
'' This file contains no replacement encoder or user-input event loop.

#include once "gd.bi"

enum
	FORMAT_GIF
	FORMAT_PNG
	FORMAT_JPEG
end enum

function saveImageAs _
	( _
		byval img as gdImagePtr, _
		byref filename as string, _
		byval imageformat as integer _
	) as integer

	if img = NULL then return 0
	dim encoded as any ptr
	dim encoded_size as long
	'' The Ptr APIs return GD-owned storage and a C int byte count. This avoids
	'' passing an application's FILE record into a DLL built with another CRT.
	select case( imageformat )
	case FORMAT_GIF
		encoded = gdImageGifPtr(img, @encoded_size)
	case FORMAT_PNG
		encoded = gdImagePngPtr(img, @encoded_size)
	case FORMAT_JPEG
		encoded = gdImageJpegPtr(img, @encoded_size, 50)
	end select
	if encoded = NULL then return 0
	if encoded_size <= 0 then
		gdFree(encoded)
		return 0
	end if
	dim file_handle as FILE ptr = fopen(filename, "wb")
	if file_handle = NULL then
		gdFree(encoded)
		return 0
	end if
	dim written as uinteger = fwrite(encoded, 1, encoded_size, file_handle)
	dim close_status as long = fclose(file_handle)
	gdFree(encoded)
	return written = encoded_size and close_status = 0
end function

const IMG_WIDTH = 320
const IMG_HEIGHT = 200

#ifndef IMAGE_EXAMPLE_NO_MAIN
'' create image
dim as gdImagePtr img = gdImageCreateTrueColor( IMG_WIDTH, IMG_HEIGHT )
if img = NULL then end 1

'' draw on it
print "creating example image..."
randomize( timer() )
for i as integer = 1 to 1000
	dim as integer x1 = rnd * (IMG_WIDTH-1)
	dim as integer y1 = rnd * (IMG_HEIGHT-1)
	dim as integer x2 = rnd * (IMG_WIDTH-1)
	dim as integer y2 = rnd * (IMG_HEIGHT-1)
	dim as uinteger col = rgba( rnd * 255, rnd * 255, rnd * 255, 0 )
	gdImageLine( img, x1, y1, x2, y2, col )
next

'' save to files
print "saving it as test.gif, test.png and test.jpg in " & curdir( )
dim status as integer
if saveImageAs(img, "test.gif", FORMAT_GIF) = 0 then status = 1
if saveImageAs(img, "test.png", FORMAT_PNG) = 0 then status = 1
if saveImageAs(img, "test.jpg", FORMAT_JPEG) = 0 then status = 1
gdImageDestroy(img)
if status <> 0 then print "could not save all three images"
end status
#endif

'' end of gd.bas
