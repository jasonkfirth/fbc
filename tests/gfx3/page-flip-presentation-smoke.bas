''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: page-flip-presentation-smoke.bas
''
'' Purpose:
''
''     Verify that SCREENSET and SCREENCOPY preserve the selected logical GPU
''     page and its graphical-console cell state.
''
'' Responsibilities:
''
''     - check pixels after SCREENSET page changes
''     - check pixels after SCREENCOPY to the visible page
''     - preserve partial VIEW SCREEN copy semantics through the general path
''     - prove the accompanying graphical-console cell page is copied
''
'' This file intentionally does NOT contain:
''
''     - native compositor capture, which remains in page-flip-visual.bas
''     - a frame-rate or swap-interval benchmark
''     - CPU framebuffer access
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#else
	const backend_flags = 0
#endif

if screenres( 96, 64, 32, 2, backend_flags ) <> 0 then end 1

screenset 0, 0
line ( 0, 0 ) - ( 95, 63 ), rgb( 0, 0, 255 ), bf
if point( 48, 32 ) <> rgb( 0, 0, 255 ) then end 2

screenset 1, 1
line ( 0, 0 ) - ( 95, 63 ), rgb( 255, 0, 0 ), bf
if point( 48, 32 ) <> rgb( 255, 0, 0 ) then end 3

screenset 0, 0
if point( 48, 32 ) <> rgb( 0, 0, 255 ) then end 4

screenset 1, 0
screencopy 1, 0
if point( 48, 32 ) <> rgb( 255, 0, 0 ) then end 5

locate 1, 1
print "A";
screencopy 1, 0
screenset 0, 0
if screen( 1, 1 ) <> asc( "A" ) then end 6

'' A partial page copy must not be promoted to the full-surface transfer path. ''
screenset 0, 0
line ( 0, 0 ) - ( 95, 63 ), rgb( 0, 0, 255 ), bf
screenset 1, 0
line ( 0, 0 ) - ( 95, 63 ), rgb( 255, 0, 0 ), bf
view screen ( 10, 10 ) - ( 20, 20 )
screencopy 1, 0
view screen
screenset 0, 0
if point( 15, 15 ) <> rgb( 255, 0, 0 ) then end 7
if point( 5, 5 ) <> rgb( 0, 0, 255 ) then end 8

screen 0
end 0

'' end of page-flip-presentation-smoke.bas
