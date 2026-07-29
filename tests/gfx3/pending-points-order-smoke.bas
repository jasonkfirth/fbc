''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: pending-points-order-smoke.bas
''
'' Purpose:
''
''     Verify that the opaque PSET staging path keeps the last write to a
''     repeated coordinate without changing the ordering seen by later calls.
''
'' Responsibilities:
''
''     - repeat one opaque PSET coordinate in one pending point stream
''     - force an ordering boundary with LINE and POINT
''     - verify unrelated pending coordinates remain intact
''
'' This file intentionally does NOT contain:
''
''     - alpha primitive coverage
''     - performance timing
''     - CPU SCREENLOCK behaviour
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

if screenres( 32, 32, 32, 1, backend_flags ) <> 0 then end 1

pset ( 5, 6 ), rgb( 255, 0, 0 )
pset ( 7, 6 ), rgb( 0, 255, 0 )
pset ( 5, 6 ), rgb( 0, 0, 255 )

'' LINE must observe the preceding batch before the final PSET starts a new one.
line ( 0, 0 )-( 0, 0 ), rgb( 255, 255, 255 )
pset ( 5, 6 ), rgb( 255, 255, 0 )

if point( 5, 6 ) <> rgb( 255, 255, 0 ) then end 2
if point( 7, 6 ) <> rgb( 0, 255, 0 ) then end 3
if point( 0, 0 ) <> rgb( 255, 255, 255 ) then end 4

screen 0
end 0

'' end of pending-points-order-smoke.bas
