''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: mixed-primitive-order-smoke.bas
''
'' Purpose:
''
''     Verify BASIC draw order when opaque line packets and outline ellipses
''     share one GPU winner pass.
''
'' Responsibilities:
''
''     - alternate PSET, LINE, and CIRCLE commands on one destination
''     - exercise overlapping pixels in both order directions
''     - force a synchronized POINT read after the asynchronous draw stream
''
'' This file intentionally does NOT contain:
''
''     - alpha-blended primitives
''     - a performance threshold
''     - assumptions about how many renderer drains are needed
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"

#ifdef GFX3_OPENGL_TEST
	const backend_flags = fb.GFX_OPENGL
#elseif defined( GFX3_VULKAN_TEST )
	const backend_flags = fb.GFX_VULKAN
#else
	const backend_flags = fb.GFX_NULL
#endif

const red_value = rgb( 240, 24, 24 )
const green_value = rgb( 20, 220, 60 )
const blue_value = rgb( 32, 80, 240 )
const yellow_value = rgb( 250, 220, 30 )

if screenres( 64, 64, 32, 1, backend_flags ) <> 0 then end 1

screenlock
line (0, 32)-(63, 32), red_value
circle (32, 32), 10, green_value
line (22, 32)-(42, 32), blue_value
circle (32, 32), 5, yellow_value

line (0, 48)-(63, 48), red_value
pset (22, 48), green_value
circle (32, 48), 10, blue_value
pset (42, 48), yellow_value
screenunlock

'' The radius-ten endpoints were overwritten by the later blue line.
if point( 22, 32 ) <> blue_value then end 2
if point( 42, 32 ) <> blue_value then end 3

'' The final radius-five outline overwrites the blue line at its endpoints.
if point( 27, 32 ) <> yellow_value then end 4
if point( 37, 32 ) <> yellow_value then end 5

'' Neither outline covers its center, so the blue line remains visible there.
if point( 32, 32 ) <> blue_value then end 6

'' The circle follows the first point, while the second point follows it.
if point( 22, 48 ) <> blue_value then end 7
if point( 42, 48 ) <> yellow_value then end 8

'' Reuse every in-flight backend slot and verify that an older winner
'' generation cannot leak into a later bounded resolve.
for pass as integer = 0 to 11
	dim as ulong pass_color = rgb( 32 + pass * 12, 180 - pass * 7, _
		48 + pass * 9 )

	screenlock
	line (0, 10)-(63, 10), red_value
	circle (32, 10), 8, green_value
	line (24, 10)-(40, 10), pass_color
	screenunlock
	if point( 24, 10 ) <> pass_color then end 9
next

screen 0
end 0

'' end of mixed-primitive-order-smoke.bas
