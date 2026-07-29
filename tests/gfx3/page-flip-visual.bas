''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: page-flip-visual.bas
''
'' Purpose:
''
''     Keep a two-page GPU frame visible long enough for an external window
''     capture to verify the ordinary SCREENSET and SCREENCOPY frame path.
''
'' Responsibilities:
''
''     - draw only on the non-visible work page
''     - copy that page to the visible page through SCREENCOPY
''     - keep the resulting GPU presentation available for inspection
''
'' This file intentionally does NOT contain:
''
''     - input handling
''     - CPU screen locking
''     - interactive game logic
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"

const as string native_title = "gfxlib3 page flip visual"

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#else
	'' Exercise the same automatic renderer selection used by normal programs.
	const backend_flags = 0
#endif

if screenres( 256, 192, 32, 2, backend_flags ) <> 0 then end 1
windowtitle native_title
screenset 1, 0

for y as integer = 0 to 191
	for x as integer = 0 to 255
		pset ( x, y ), rgb( x, y, x xor y )
	next
next

line ( 16, 16 )-( 239, 175 ), rgb( 20, 220, 20 ), bf
line ( 48, 48 )-( 207, 143 ), rgb( 20, 20, 220 ), bf

'' The nested colours make a stale visible page immediately obvious.  The
'' preceding 49,152 PSET operations also cover software-rasterizer workloads.
screencopy
screensync

sleep 8000, 1
screen 0

end 0

'' end of page-flip-visual.bas
