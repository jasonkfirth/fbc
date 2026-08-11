/'
    FreeBASIC gfxlib2 regression test
    ---------------------------------

    File: circle-pen-position.bas

    Purpose:

        Verify QuickBASIC-compatible pen positioning after CIRCLE draws an arc.

    Responsibilities:

        - exercise negative start and end angles that draw radial lines
        - verify that CIRCLE leaves the graphics pen at the circle center
        - use the null driver so the test does not require a display

    This file intentionally does NOT contain:

        - pixel-level arc rasterization checks
        - interactive graphics behavior
'/

' TEST_MODE : COMPILE_AND_RUN_OK

#include once "fbgfx.bi"

dim as integer pen_x, pen_y

if( screenres( 64, 64, 32, 1, fb.GFX_NULL ) <> 0 ) then
	end 1
end if

circle ( 30, 24 ), 10, rgb( 255, 255, 255 ), -5.0, -2.0
screencontrol fb.GET_PEN_POS, pen_x, pen_y

if( (pen_x <> 30) orelse (pen_y <> 24) ) then
	end 2
end if

/' end of circle-pen-position.bas '/
