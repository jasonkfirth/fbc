''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: mixed-rectangle-primitive-order-smoke.bas
''
'' Purpose:
''
''     Verify opaque rectangles can share one ordered Vulkan primitive packet
''     with points and styled lines.
''
'' Responsibilities:
''
''     - alternate filled boxes, outline boxes, PSET, and LINE commands
''     - verify last-writer behavior at overlapping interiors and borders
''     - verify an adjacent ellipse retains exact fallback ordering
''     - reuse the in-flight submission slots with changing final colours
''
'' This file intentionally does NOT contain:
''
''     - alpha-blended primitives
''     - a backend-specific performance threshold
''     - CPU framebuffer access
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

const as uinteger red_value = rgb( 240, 24, 24 )
const as uinteger green_value = rgb( 20, 220, 60 )
const as uinteger blue_value = rgb( 32, 80, 240 )
const as uinteger yellow_value = rgb( 250, 220, 30 )

if screenres( 96, 64, 32, 1, backend_flags ) <> 0 then end 1

screenlock
pset ( 12, 12 ), red_value
line ( 8, 12 )-( 20, 12 ), green_value
line ( 10, 10 )-( 14, 14 ), blue_value, bf
pset ( 12, 12 ), yellow_value

line ( 30, 10 )-( 40, 20 ), blue_value, bf
line ( 30, 10 )-( 40, 20 ), red_value, b
line ( 32, 15 )-( 38, 15 ), green_value

line ( -20, 30 )-( 8, 40 ), red_value, bf
line ( 0, 35 )-( 12, 35 ), blue_value
screenunlock

if point( 12, 12 ) <> yellow_value then end 2
if point( 31, 11 ) <> blue_value then end 3
if point( 30, 15 ) <> red_value then end 4
if point( 35, 15 ) <> green_value then end 5
if point( 0, 35 ) <> blue_value then end 6
if point( 8, 34 ) <> red_value then end 7
if point( 20, 34 ) <> rgb( 0, 0, 0 ) then end 8

'' The tile packet does not approximate midpoint ellipses. This sequence
'' deliberately crosses that packet boundary and must retain public FIFO order.
screenlock
line ( 72, 8 )-( 92, 28 ), red_value, bf
circle ( 82, 18 ), 7, green_value
line ( 80, 16 )-( 84, 20 ), blue_value, bf
screenunlock

if point( 82, 11 ) <> green_value then end 9
if point( 82, 18 ) <> blue_value then end 10

for pass as integer = 0 to 11
	dim as uinteger pass_color = rgb( 24 + pass * 14, _
		210 - pass * 9, 36 + pass * 11 )

	screenlock
	line ( 50, 30 )-( 70, 50 ), red_value, bf
	pset ( 60, 40 ), green_value
	line ( 52, 38 )-( 68, 42 ), blue_value, bf
	line ( 52, 40 )-( 68, 40 ), pass_color
	screenunlock
	if point( 60, 40 ) <> pass_color then end 11
next

screen 0
end 0

'' end of mixed-rectangle-primitive-order-smoke.bas
