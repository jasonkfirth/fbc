'' FreeBASIC FreeType examples
'' File: bitmap.bi
'' Purpose: Blend an eight-bit glyph into a caller-owned 32-bit image.
'' Responsibilities: Signed source pitch, destination stride and rectangle clipping.
'' This file contains no font loading, window creation or buffer ownership.
#pragma once
#include once "freetype2/freetype.bi"

private function blend_bitmap(byval bitmap as FT_Bitmap ptr, byval pixels as any ptr, _
	byval image_width as integer, byval image_height as integer, byval image_pitch as integer, _
	byval x as integer, byval y as integer, byval ink_color as ulong) as integer
	if bitmap = NULL or pixels = NULL then return 0
	if image_width <= 0 or image_height <= 0 or image_width > 16384 or image_height > 16384 then return 0
	if image_pitch < image_width*4 or culngint(image_pitch)*image_height > (cast(uinteger,-1) shr 1) then return 0
	if bitmap->width = 0 or bitmap->rows = 0 then return -1
	if bitmap->buffer = NULL or bitmap->pixel_mode <> FT_PIXEL_MODE_GRAY or bitmap->num_grays <> 256 then return 0
	if bitmap->width > 8192 or bitmap->rows > 8192 then return 0
	dim source_pitch as longint = bitmap->pitch
	dim row_size as longint = abs(source_pitch)
	if row_size < bitmap->width or row_size*bitmap->rows > (cast(uinteger,-1) shr 1) then return 0
	if x <= -cint(bitmap->width) or y <= -cint(bitmap->rows) or x >= image_width or y >= image_height then return -1
	dim first_x as integer = iif(x<0, -x, 0), first_y as integer = iif(y<0, -y, 0)
	dim target_x as integer = iif(x<0, 0, x), target_y as integer = iif(y<0, 0, y)
	dim count_x as integer = bitmap->width-first_x, count_y as integer = bitmap->rows-first_y
	if count_x > image_width-target_x then count_x = image_width-target_x
	if count_y > image_height-target_y then count_y = image_height-target_y
	dim source_top as ubyte ptr = bitmap->buffer
	'' A negative-pitch bitmap stores its bottom row at the buffer base.
	'' FreeType's own bitmap conversion starts at the high-address top row.
	if source_pitch < 0 then source_top += row_size*(bitmap->rows-1)
	for row_index as integer = 0 to count_y-1
		dim source_row as ubyte ptr = source_top+(first_y+row_index)*source_pitch+first_x
		'' ULong is one 32-bit pixel on both targets; UInteger is eight bytes on Win64.
		dim target_row as ulong ptr = cast(ulong ptr, cast(ubyte ptr,pixels)+(target_y+row_index)*image_pitch)+target_x
		for column_index as integer = 0 to count_x-1
			dim coverage as ulong = source_row[column_index]
			dim background as ulong = target_row[column_index]
			'' Coverage spans 0..255. Divide by 255 so full coverage replaces the pixel.
			dim red as ulong = (((ink_color shr 16) and 255)*coverage+((background shr 16) and 255)*(255-coverage)+127)\255
			dim green as ulong = (((ink_color shr 8) and 255)*coverage+((background shr 8) and 255)*(255-coverage)+127)\255
			dim blue as ulong = ((ink_color and 255)*coverage+(background and 255)*(255-coverage)+127)\255
			target_row[column_index] = rgb(red,green,blue)
		next
	next
	return -1
end function
'' end of bitmap.bi
