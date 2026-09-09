'' FreeBASIC graphics tests: drawstring-size.bas
'' Check bitmap-font measurement against rendering and malformed font rows.
'' Uses caller-owned images and the Null driver; no host fonts or GUI layout.
' TEST_MODE : COMPILE_AND_RUN_OK

#include once "fbgfx.bi"

dim shared as integer checks

private sub require( byval condition as boolean, byref message as const string )
	checks += 1
	if condition then exit sub
	screen 0
	print "FAIL: " & message
	end 1
end sub

dim as long pixel_width = 99, pixel_height = 99
require( fb.DrawStringSize( "A", pixel_width, pixel_height ) <> 0, "display required for built-in font" )
require( pixel_width = 0 and pixel_height = 0, "zero dimensions on failure" )
require( screenres( 80, 60, 32, 1, fb.GFX_NULL ) = 0, "screen mode" )
require( fb.DrawStringSize( "A" & chr( 0, 13, 10 ), pixel_width, pixel_height ) = 0, "measure byte text" )
require( pixel_width = 32 and pixel_height > 0, "all bytes advance" )
dim as long builtin_height = pixel_height
require( fb.DrawStringSize( "", pixel_width, pixel_height ) = 0, "empty text" )
require( pixel_width = 0 and pixel_height = builtin_height, "empty text retains font height" )
require( fb.DrawStringSize( "X", pixel_width, pixel_width ) <> 0, "aliased outputs rejected" )

'' The screen is 32-bit and this target/font pair is 8-bit. A glyph width
'' check must use the font's visible width, not the screen's pixel depth.
dim as any ptr font_image = imagecreate( 5, 6, 0, 8 )
dim as any ptr target = imagecreate( 32, 12, 0, 8 )
require( font_image <> 0 and target <> 0, "image allocation" )
dim as ubyte ptr pixels
imageinfo font_image, , , , , pixels
pixels[0] = 0 : pixels[1] = asc( "A" ) : pixels[2] = asc( "B" )
pixels[3] = 3 : pixels[4] = 2
line font_image, ( 0, 1 )-( 2, 5 ), 7, bf
line font_image, ( 3, 1 )-( 4, 5 ), 9, bf
require( fb.DrawStringSize( "ABX", pixel_width, pixel_height, font_image ) = 0, "custom size" )
require( pixel_width = 10 and pixel_height = 5, "custom advances" )
draw string target, ( 0, -2 ), "XA", , font_image, pset
require( err = 0, "clipped custom draw" )
for y as integer = 0 to 11
	for x as integer = 0 to 31
		dim as integer expected = 0
		if y < 3 and x >= 5 and x < 8 then expected = 7
		require( point( x, y, target ) = expected, "clipping preserves unsupported advance" )
	next
next

'' Clipping both sides must preserve image row padding, not just visible pixels.
dim as any ptr narrow = imagecreate( 1, 2, 0, 8 )
require( narrow <> 0, "narrow image" )
dim as ubyte ptr narrow_pixels
dim as long narrow_pitch
imageinfo narrow, , , , narrow_pitch, narrow_pixels
for byte_index as integer = 0 to narrow_pitch * 2 - 1
	narrow_pixels[byte_index] = 99
next
draw string narrow, ( -1, 0 ), "A", , font_image, pset
require( err = 0, "both-side clipping" )
for byte_index as integer = 0 to narrow_pitch * 2 - 1
	dim as integer expected = 99
	if byte_index mod narrow_pitch = 0 then expected = 7
	require( narrow_pixels[byte_index] = expected, "row padding preserved" )
next
imagedestroy narrow

'' Measurement must not move the pen, change the viewport or use WINDOW units.
view ( 4, 4 )-( 70, 50 )
window screen ( 0, 0 )-( 10, 10 )
pset ( 2, 3 ), 0
for repetition as integer = 1 to 2000
	require( fb.DrawStringSize( "A" & chr( 0 ) & "B", pixel_width, pixel_height, font_image ) = 0, "temporary caption" )
	require( pixel_width = 10 and pixel_height = 5, "NUL advance" )
next
require( point( 2 ) = 2 and point( 3 ) = 3, "measurement preserves pen" )
window
view

pixels[1] = 0 : pixels[2] = 255
require( fb.DrawStringSize( "A", pixel_width, pixel_height, font_image ) <> 0, "width table exceeds row" )
require( pixel_width = 0 and pixel_height = 0, "bad font zero outputs" )
draw string target, ( 0, 0 ), "A", , font_image, pset
require( err <> 0, "drawing rejects same width table" )
pixels[1] = asc( "B" ) : pixels[2] = asc( "A" )
pixels[3] = 4 : pixels[4] = 4
require( fb.DrawStringSize( "A", pixel_width, pixel_height, font_image ) <> 0, "glyphs cannot occupy row padding" )
pixels[3] = 3 : pixels[4] = 0
require( fb.DrawStringSize( "ABA", pixel_width, pixel_height, font_image ) = 0, "reversed byte range and zero width" )
require( pixel_width = 6 and pixel_height = 5, "zero-width glyph advance" )
screen 0
require( fb.DrawStringSize( "A", pixel_width, pixel_height, font_image ) = 0, "custom measurement without display" )
imagedestroy target
imagedestroy font_image
print "drawstring-size: "; checks; " checks passed"

'' end of drawstring-size.bas
