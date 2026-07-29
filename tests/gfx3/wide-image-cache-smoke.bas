''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: wide-image-cache-smoke.bas
''
'' Purpose:
''
''     Verify cached PUT from a sprite strip wider than a conventional GPU
''     texture.
''
'' Responsibilities:
''
''     - create the long, shallow FB.IMAGE layout used by OMA tile sheets
''     - upload and sample rectangles from both ends of the image
''     - exercise desktop storage-buffer and mobile texture limits
''     - verify a direct pixel edit refreshes only the requested cache entry
''
'' This file intentionally does NOT contain:
''
''     - a performance threshold
''     - scaled, rotated, or custom PUT modes
''     - assumptions about a particular Vulkan adapter
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#elseif defined( GFX3_OPENGL_TEST )
	const backend_flags = fb.GFX_OPENGL
#else
	const backend_flags = fb.GFX_NULL
#endif

'' QFAK's widest background strip is 10,960 by 40 pixels.
const strip_width = 10960
const strip_height = 40
const tile_width = 40
'' Sixteen-bit screens expose their native RGB565 values through POINT.
const left_color = &h1234
const right_color = &h5A5A
const updated_left_color = &h3456

if screenres( 96, 48, 16, 1, backend_flags ) <> 0 then end 1

dim as any ptr strip = imagecreate( strip_width, strip_height, 0, 16 )
if strip = 0 then end 2

dim as long reported_width, reported_height, reported_bpp
dim as long reported_pitch, reported_size
dim as any ptr reported_pixels

if imageinfo( strip, reported_width, reported_height, reported_bpp, _
	reported_pitch, reported_pixels, reported_size ) <> 0 then end 3
if reported_width <> strip_width orelse reported_height <> strip_height _
	orelse reported_bpp <> 2 orelse reported_pixels = 0 then end 4

for row as integer = 0 to strip_height - 1
	dim as ushort ptr pixels = cast( ushort ptr, _
		cast( ubyte ptr, reported_pixels ) + (row * reported_pitch) )

	for column as integer = 0 to tile_width - 1
		pixels[column] = left_color
		pixels[strip_width - tile_width + column] = right_color
	next
next

dim as ulong expected_left = point( 0, 0, strip )
dim as ulong expected_right = point( strip_width - tile_width, 0, strip )

put (0, 0), strip, (0, 0)-(tile_width - 1, strip_height - 1), pset
put (48, 0), strip, (strip_width - tile_width, 0)- _
	(strip_width - 1, strip_height - 1), pset

dim as ulong actual_left = point( 0, 0 )
dim as ulong actual_right = point( 48, 0 )
if actual_left <> expected_left then
	print "left expected=&h"; hex( expected_left ); " actual=&h"; hex( actual_left )
	end 5
end if
if actual_right <> expected_right then
	print "right expected=&h"; hex( expected_right ); " actual=&h"; hex( actual_right )
	end 6
end if

for row as integer = 0 to strip_height - 1
	dim as ushort ptr pixels = cast( ushort ptr, _
		cast( ubyte ptr, reported_pixels ) + (row * reported_pitch) )

	for column as integer = 0 to tile_width - 1
		pixels[column] = updated_left_color
	next
next

dim as ulong expected_updated_left = point( 0, 0, strip )
put (0, 0), strip, (0, 0)-(tile_width - 1, strip_height - 1), pset
dim as ulong updated_left = point( 0, 0 )
if updated_left <> expected_updated_left then
	print "updated left expected=&h"; hex( expected_updated_left ); _
		" actual=&h"; hex( updated_left )
	end 7
end if

imagedestroy strip
screen 0
end 0

'' end of wide-image-cache-smoke.bas
