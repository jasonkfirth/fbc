''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: small-filled-rectangle-batch-smoke.bas
''
'' Purpose:
''
''     Verify that dense small opaque LINE ... BF calls preserve their pixels
''     while using gfxlib3's ordered primitive batch.
''
'' Responsibilities:
''
''     - draw a software-rasterizer-sized grid of two-by-two boxes
''     - copy the completed non-visible page to the display page
''     - check representative interior and neighbouring pixels
''
'' This file intentionally does NOT contain:
''
''     - alpha blended rectangles
''     - SCREENLOCK access
''     - timing or throughput assertions tied to one GPU
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"

const as uinteger even_colour = rgb( 40, 170, 230 )
const as uinteger odd_colour = rgb( 230, 110, 40 )

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#elseif defined( GFX3_OPENGL_TEST )
	const backend_flags = fb.GFX_OPENGL
#else
	const backend_flags = 0
#endif

if screenres( 320, 240, 32, 2, backend_flags ) <> 0 then end 1
screenset 1, 0

for y as integer = 0 to 119
	for x as integer = 0 to 159
		if ( ( x xor y ) and 1 ) = 0 then
			line ( x shl 1, y shl 1 )-( ( x shl 1 ) + 1, ( y shl 1 ) + 1 ), even_colour, bf
		else
			line ( x shl 1, y shl 1 )-( ( x shl 1 ) + 1, ( y shl 1 ) + 1 ), odd_colour, bf
		end if
	next
next

screencopy 1, 0
screensync

if point( 0, 0 ) <> even_colour then end 2
if point( 1, 1 ) <> even_colour then end 3
if point( 2, 0 ) <> odd_colour then end 4
if point( 319, 239 ) <> even_colour then end 5

screen 0
end 0

'' end of small-filled-rectangle-batch-smoke.bas
