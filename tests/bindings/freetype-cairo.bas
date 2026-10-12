'' FreeBASIC native text/2D qualification
'' File: freetype-cairo.bas
'' Purpose: Verify native layouts and the original glyph blending helper.
'' Responsibilities: Clipping, padded/negative pitch, guard bytes and coverage.
'' This file contains no replacement SDK or borrowed glyph deallocation.
#lang "fb"
#define FREETYPE_EXAMPLE_NO_MAIN
#include once "../../examples/graphics/FreeType/drawstr.bas"
#include once "cairo/cairo.bi"
Print "FT"; SizeOf(FT_Long); SizeOf(FT_Pos); SizeOf(FT_Bitmap); OffsetOf(FT_Bitmap,buffer); _
	OffsetOf(FT_Bitmap,pitch); SizeOf(FT_FaceRec); OffsetOf(FT_FaceRec,glyph); _
	SizeOf(FT_GlyphSlotRec); OffsetOf(FT_GlyphSlotRec,bitmap); OffsetOf(FT_GlyphSlotRec,bitmap_left); _
	OffsetOf(FT_GlyphSlotRec,bitmap_top); OffsetOf(FT_GlyphSlotRec,advance); SizeOf(FT_SizeRec)
Print "CAIRO"; SizeOf(cairo_matrix_t); SizeOf(cairo_rectangle_int_t); SizeOf(cairo_glyph_t)
dim source_bytes(0 to 23) as ubyte = {0,64,128,192,255,0,0,0,255,192,128,64,0,0,0,0,17,33,99,177,233,0,0,0}
dim bitmap as FT_Bitmap
bitmap.width=5:bitmap.rows=3:bitmap.pitch=8:bitmap.buffer=@source_bytes(0)
bitmap.pixel_mode=FT_PIXEL_MODE_GRAY:bitmap.num_grays=256
dim destination(0 to 95) as ubyte
dim output_file as string=command(1)
dim file_number as integer=freefile
if open(output_file,for binary,access write,as #file_number)<>0 then end 1
for flow as integer=0 to 1
	bitmap.pitch=iif(flow=0,8,-8)
	for x as integer=-6 to 5
		for y as integer=-4 to 4
			for i as integer=0 to 95:destination(i)=&ha5:next
			for row_index as integer=0 to 2
				dim row_data as ulong ptr=cast(ulong ptr,@destination(16)+row_index*20)
				for column_index as integer=0 to 3:row_data[column_index]=rgb(20,40,60):next
			next
			if blend_bitmap(@bitmap,@destination(16),4,3,20,x,y,rgb(220,130,10))=0 then end 1
			for i as integer=0 to 15
				if destination(i)<>&ha5 or destination(76+i)<>&ha5 then end 1
			next
			for row_index as integer=0 to 2
				for i as integer=16 to 19
					if destination(16+row_index*20+i)<>&ha5 then end 1
				next
			next
			if put(#file_number,,destination(16),60)<>0 then end 1
		next
	next
next
if close(#file_number)<>0 then end 1
bitmap.pixel_mode=FT_PIXEL_MODE_MONO
if blend_bitmap(@bitmap,@destination(16),4,3,20,0,0,rgb(0,0,0))<>0 then end 1
Print "GLYPH_BOUNDS=PASS cases=216 guarded_bytes=44"
'' end of freetype-cairo.bas
