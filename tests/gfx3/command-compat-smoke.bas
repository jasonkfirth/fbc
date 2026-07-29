''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: command-compat-smoke.bas
''
'' Purpose:
''
''     Verify the command families that complete gfxlib3's initial public ABI
''     closure through observable pixels and a BMP round trip.
''
'' Responsibilities:
''
''     - verify border-based PAINT on screen and image targets
''     - verify DRAW movement and color commands on both target types
''     - verify built-in glyph decoding and rendering
''     - verify 32-bit BMP BSAVE/BLOAD pixel preservation
''
'' This file intentionally does NOT contain:
''
''     - custom font construction
''     - compressed or bitfield BMP fixtures
''     - visible-window presentation assumptions
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"

const temporary_filename = "gfx3-command-compat-smoke.bmp"

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#elseif defined(GFX3_OPENGL_TEST)
	const backend_flags = 0
#else
	const backend_flags = fb.GFX_NULL
#endif

if screenres( 48, 48, 32, 1, backend_flags ) <> 0 then end 1

line (2, 2)-(14, 14), rgb( 0, 0, 255 ), b
paint (5, 5), rgb( 255, 0, 0 ), rgb( 0, 0, 255 )
if point( 5, 5 ) <> rgb( 255, 0, 0 ) then end 2
if point( 2, 5 ) <> rgb( 0, 0, 255 ) then end 3
if point( 1, 5 ) <> rgb( 0, 0, 0 ) then end 4

draw "BM 20,4 C" & rgb( 0, 255, 0 ) & " R8 D8 L8 U8"
if point( 24, 4 ) <> rgb( 0, 255, 0 ) then
	dim as uinteger observed_draw_pixel = point( 24, 4 )
	screen 0
	print "DRAW horizontal pixel: &h" & hex( observed_draw_pixel )
	end 5
end if
if point( 28, 8 ) <> rgb( 0, 255, 0 ) then
	dim as uinteger observed_draw_pixel = point( 28, 8 )
	screen 0
	print "DRAW vertical pixel: &h" & hex( observed_draw_pixel )
	end 6
end if

draw string (2, 20), "A", rgb( 255, 255, 255 )
dim as integer screen_glyph_pixels
for y as integer = 20 to 27
	for x as integer = 2 to 9
		if point( x, y ) = rgb( 255, 255, 255 ) then
			screen_glyph_pixels += 1
		end if
	next
next
if screen_glyph_pixels = 0 then end 7

dim as any ptr source_image = imagecreate( 24, 24, rgb( 0, 0, 0 ), 32 )
dim as any ptr loaded_image = imagecreate( 24, 24, rgb( 1, 2, 3 ), 32 )
if source_image = 0 orelse loaded_image = 0 then end 8

line source_image, (1, 1)-(12, 12), rgb( 255, 255, 0 ), b
paint source_image, (3, 3), rgb( 20, 40, 60 ), rgb( 255, 255, 0 )
if point( 3, 3, source_image ) <> rgb( 20, 40, 60 ) then end 9

draw source_image, "BM 2,16 C" & rgb( 255, 0, 255 ) & " R10"
if point( 7, 16, source_image ) <> rgb( 255, 0, 255 ) then end 10

draw string source_image, (14, 2), "i", rgb( 0, 255, 255 )
dim as integer image_glyph_pixels
for y as integer = 2 to 9
	for x as integer = 14 to 21
		if point( x, y, source_image ) = rgb( 0, 255, 255 ) then
			image_glyph_pixels += 1
		end if
	next
next
if image_glyph_pixels = 0 then end 11

if bsave( temporary_filename, source_image, 0 ) <> 0 then end 12
if bload( temporary_filename, loaded_image ) <> 0 then end 13
if point( 3, 3, loaded_image ) <> rgb( 20, 40, 60 ) then end 14
if point( 7, 16, loaded_image ) <> rgb( 255, 0, 255 ) then end 15

kill temporary_filename
imagedestroy loaded_image
imagedestroy source_image
screen 0
end 0

'' end of command-compat-smoke.bas
