''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: console-smoke.bas
''
'' Purpose:
''
''     Verify that ordinary FreeBASIC console statements are redirected into
''     page-specific gfxlib3 GPU console state.
''
'' Responsibilities:
''
''     - check COLOR, CLS, WIDTH, LOCATE, POS, and CSRLIN hooks
''     - verify PRINT glyph/background pixels and SCREEN character/color reads
''     - verify logical pages retain independent character cells
''     - exercise bottom-row scrolling
''
'' This file intentionally does NOT contain:
''
''     - keyboard input, line input, or cursor visualization
''     - alternate 8 by 14 or 8 by 16 fonts
''     - native window event checks
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

if screenres( 64, 64, 32, 2, backend_flags ) <> 0 then end 1
if loword( width() ) <> 8 orelse hiword( width() ) <> 8 then end 2

color rgb( 255, 0, 0 ), rgb( 0, 0, 255 )
cls
locate 2, 3
print "AB";
if screen( 2, 3 ) <> asc( "A" ) then end 3
if screen( 2, 4 ) <> asc( "B" ) then end 4
if screen( 2, 3, 1 ) <> rgb( 255, 0, 0 ) then end 5
if screen( 2, 3, 2 ) <> rgb( 0, 0, 255 ) then end 6
if pos( 0 ) <> 5 orelse csrlin <> 2 then end 7

dim as integer foreground_pixels, background_pixels
for y as integer = 8 to 15
	for x as integer = 16 to 23
		select case point( x, y )
		case rgb( 255, 0, 0 )
			foreground_pixels += 1
		case rgb( 0, 0, 255 )
			background_pixels += 1
		end select
	next
next
if foreground_pixels = 0 orelse background_pixels = 0 then end 8

screenset 1, 1
color rgb( 0, 255, 0 ), rgb( 0, 0, 0 )
locate 1, 1
print "Z";
if screen( 1, 1 ) <> asc( "Z" ) then end 9
screenset 0, 0
if screen( 2, 3 ) <> asc( "A" ) then end 10

locate 8, 1
print "X"
print "Y";
if screen( 8, 1 ) <> asc( "Y" ) then end 11

cls
if screen( 1, 1 ) <> 32 then end 12
if point( 0, 0 ) <> rgb( 0, 0, 0 ) then end 13

screen 0
end 0

'' end of console-smoke.bas
