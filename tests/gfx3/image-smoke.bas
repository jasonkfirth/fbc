''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: image-smoke.bas
''
'' Purpose:
''
''     Exercise the public CPU FB.IMAGE and GPU transfer compatibility slice.
''
'' Responsibilities:
''
''     - verify image allocation, information, and image-target primitives
''     - verify GET downloads and built-in PUT uploads
''     - verify partial cache validation after direct image-memory writes
''     - verify that a custom BASIC blender crosses the CPU barrier correctly
''
'' This file intentionally does NOT contain:
''
''     - file codecs, PAINT, DRAW, or text rendering
''     - visible window or input checks
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#elseif defined(GFX3_OPENGL_TEST)
	const backend_flags = 0
#else
	const backend_flags = fb.GFX_NULL
#endif

function custom_blender _
	( _
		byval source_pixel as ulong, _
		byval destination_pixel as ulong, _
		byval parameter as any ptr _
	) as ulong

	function = source_pixel xor destination_pixel
end function

if screenres( 32, 32, 32, 2, backend_flags ) <> 0 then end 1

dim as any ptr source_image = imagecreate( 8, 8, rgba( 1, 2, 3, 4 ), 32 )
dim as any ptr captured_image = imagecreate( 8, 8, 0, 32 )
dim as any ptr solid_image = imagecreate( 2, 2, rgba( 12, 34, 56, 255 ), 32 )
dim as any ptr generation_image = imagecreate( 2, 2, rgba( 3, 4, 5, 255 ), 32 )
if source_image = 0 orelse captured_image = 0 orelse solid_image = 0 _
	orelse generation_image = 0 then end 2

dim as long image_width, image_height, image_bpp, image_pitch, image_size
dim as any ptr image_pixels
if imageinfo( source_image, image_width, image_height, image_bpp, _
	image_pitch, image_pixels, image_size ) <> 0 then end 3
if image_width <> 8 orelse image_height <> 8 orelse image_bpp <> 4 then end 4
if image_pitch < 32 orelse image_pixels = 0 orelse image_size <= image_pitch then end 5

pset source_image, (1, 1), rgba( 10, 20, 30, 40 )
if point( 1, 1, source_image ) <> rgba( 10, 20, 30, 40 ) then end 6

line source_image, (2, 2)-(5, 5), rgba( 50, 60, 70, 80 ), bf
if point( 3, 3, source_image ) <> rgba( 50, 60, 70, 80 ) then end 7

circle source_image, (4, 4), 2, rgba( 90, 100, 110, 120 )
if point( 6, 4, source_image ) <> rgba( 90, 100, 110, 120 ) then end 8

line (0, 0)-(31, 31), 0, bf
put (0, 0), source_image, pset
dim as ulong transferred_color = point( 4, 4 )
if transferred_color <> rgba( 50, 60, 70, 80 ) then
	print "PUT PSET: expected &h"; hex( rgba( 50, 60, 70, 80 ) ); _
		", got &h"; hex( transferred_color )
	end 9
end if

'' IMAGEINFO exposes writable pixels, so a later direct edit must refresh the
'' cached GPU source before the next compatible PUT.
dim as ulong ptr writable_source = cast( ulong ptr, image_pixels )
writable_source[0] = rgba( 77, 88, 99, 255 )
put (8, 0), source_image, pset
if point( 8, 0 ) <> rgba( 77, 88, 99, 255 ) then end 14

''
'' A partial PUT validates only the source rectangle it can observe. The first
'' draw does not include the directly changed pixel; the second one does and
'' must refresh the cached GPU image before sampling that region.
writable_source[7] = rgba( 190, 80, 45, 255 )
put (0, 8), source_image, (0, 0)-(3, 3), pset
put (4, 8), source_image, (4, 0)-(7, 3), pset
if point( 7, 8 ) <> rgba( 190, 80, 45, 255 ) then end 16

get (0, 0)-(7, 7), captured_image
if point( 4, 4, captured_image ) <> rgba( 50, 60, 70, 80 ) then end 10

line (16, 0)-(23, 7), rgba( 1, 1, 1, 1 ), bf
put (16, 0), source_image, custom, @custom_blender, 0
if point( 20, 4 ) <> (rgba( 50, 60, 70, 80 ) xor rgba( 1, 1, 1, 1 )) then end 11

put (0, 12), source_image, preset
put (8, 12), source_image, and
put (16, 12), source_image, or
put (24, 12), source_image, xor
put (0, 20), source_image, trans
put (8, 20), source_image, alpha
put (16, 20), source_image, add
put (24, 20), source_image, alpha, 128

''
'' A whole-image solid PUT takes the desktop GPU rectangle specialization.
'' The following AND uses the same cached source and proves that its backing
'' texture was still initialized for a later destination-reading operation.
put (24, 24), solid_image, pset
if point( 24, 24 ) <> rgba( 12, 34, 56, 255 ) then end 12
line (26, 24)-(27, 25), rgba( 240, 15, 170, 255 ), bf
put (26, 24), solid_image, and
if point( 26, 24 ) <> rgba( 0, 2, 40, 255 ) then end 13

'' A normal image-target drawing command advances gfxlib3's owned-image
'' generation.  The second PUT must upload this mutation without needing the
'' expensive defensive pixel snapshot used after IMAGEINFO.
put (0, 28), generation_image, pset
pset generation_image, (0, 0), rgba( 111, 122, 133, 255 )
put (4, 28), generation_image, pset
if point( 4, 28 ) <> rgba( 111, 122, 133, 255 ) then end 15

imagedestroy captured_image
imagedestroy source_image
imagedestroy solid_image
imagedestroy generation_image
screen 0
end 0

'' end of image-smoke.bas
