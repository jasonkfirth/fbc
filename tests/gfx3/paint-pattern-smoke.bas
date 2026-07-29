''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: paint-pattern-smoke.bas
''
'' Purpose:
''
''     Verify that patterned PAINT keeps gfxlib2's absolute target-coordinate
''     tile alignment when the active VIEW begins away from the origin.
''
'' Responsibilities:
''
''     - construct a distinct 8 by 8 32-bit PAINT tile
''     - fill a bordered region through a nonzero VIEW
''     - check relative and SCREEN-coordinate reads against absolute tiling
''
'' This file intentionally does NOT contain:
''
''     - flood-fill performance measurements
''     - pattern coverage for every logical color depth
''     - visible-window presentation assumptions
''

#ifdef GFX3_TEST
	#define __FB_GFXLIB3__
#endif
#include once "fbgfx.bi"

const border_color = rgb( 255, 255, 255 )

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#elseif defined(GFX3_OPENGL_TEST)
	const backend_flags = 0
#else
	const backend_flags = fb.GFX_NULL
#endif

function tile_color( byval x as integer, byval y as integer ) as ulong
	return rgb( (x and 7) * 29, (y and 7) * 31, ((x xor y) and 7) * 33 )
end function

dim as string paint_pattern = ""
for y as integer = 0 to 7
	for x as integer = 0 to 7
		paint_pattern += mkl( tile_color( x, y ) )
	next
next

if screenres( 20, 20, 32, 1, backend_flags ) <> 0 then end 1

view ( 3, 4 )-( 12, 13 )
line ( 0, 0 )-( 9, 9 ), border_color, b
paint ( 1, 1 ), paint_pattern, border_color

'' Relative coordinates (1,1) map to physical target coordinate (4,5).
if culng( point( 1, 1 ) ) <> tile_color( 4, 5 ) then end 2
if culng( point( 2, 1 ) ) <> tile_color( 5, 5 ) then end 3
if culng( point( 1, 2 ) ) <> tile_color( 4, 6 ) then end 4

view screen
if culng( point( 4, 5 ) ) <> tile_color( 4, 5 ) then end 5
if culng( point( 3, 5 ) ) <> border_color then end 6

screen 0
end 0

'' end of paint-pattern-smoke.bas
