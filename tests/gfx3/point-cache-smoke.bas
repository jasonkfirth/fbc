''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: point-cache-smoke.bas
''
'' Purpose:
''
''     Verify the compatibility POINT cache never hides a screen mutation.
''
'' Responsibilities:
''
''     - seed one cached POINT result
''     - invalidate it through PSET, PUT, CLS, and SCREENLOCK writes
''     - retain a read after an unrelated LINE operation
''
'' This file intentionally does NOT contain:
''
''     - timing thresholds
''     - multi-page presentation coverage
''     - GPU-only surface readback tests
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"

#ifdef GFX3_OPENGL_TEST
	const backend_flags = fb.GFX_OPENGL
#elseif defined( GFX3_VULKAN_TEST )
	const backend_flags = fb.GFX_VULKAN
#else
	const backend_flags = 0
#endif

dim as any ptr source_image

if screenres( 32, 32, 32, 1, backend_flags ) <> 0 then end 1

'' First POINT seeds the page-local cache with the initial black pixel.
if point( 2, 2 ) <> rgb( 0, 0, 0 ) then end 2

pset ( 2, 2 ), rgb( 10, 20, 30 )
if point( 2, 2 ) <> rgb( 10, 20, 30 ) then end 3

'' A non-overlapping operation must not change the cached coordinate.
line ( 20, 20 )-( 24, 20 ), rgb( 40, 50, 60 )
if point( 2, 2 ) <> rgb( 10, 20, 30 ) then end 4

source_image = imagecreate( 2, 2, rgb( 70, 80, 90 ), 32 )
if source_image = 0 then end 5
put ( 2, 2 ), source_image, pset
if point( 2, 2 ) <> rgb( 70, 80, 90 ) then end 6

'' SCREENLOCK exposes writable memory, which invalidates every prior GPU cache.
screenlock
pset ( 2, 2 ), rgb( 100, 110, 120 )
screenunlock
if point( 2, 2 ) <> rgb( 100, 110, 120 ) then end 7

cls rgb( 130, 140, 150 )
'' CLS accepts the active text/background policy on some legacy modes. The
'' important cache invariant is that it cannot return the pre-CLS pixel.
if point( 2, 2 ) = rgb( 100, 110, 120 ) then end 8

imagedestroy source_image
screen 0
end 0

'' end of point-cache-smoke.bas
