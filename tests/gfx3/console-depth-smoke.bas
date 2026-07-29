''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: console-depth-smoke.bas
''
'' Purpose:
''
''     Verify SCREEN(row,column,colourflag) retains gfxlib2's distinct indexed
''     attribute packing and RGB565 colour expansion.
''
'' Responsibilities:
''
''     - write one graphical-console cell in an 8-bit mode
''     - validate character and packed foreground/background indexes
''     - write one cell in a 16-bit mode and validate expanded RGB colours
''
'' This file intentionally does NOT contain:
''
''     - font rasterization checks
''     - page switching or scroll checks
''     - text input and cursor rendering
''

#ifndef __FB_GFXLIB3__
	#define __FB_GFXLIB3__
#endif
#include once "fbgfx.bi"

dim requested as string = lcase( command( 1 ) )
dim flags as integer

select case requested
case "", "null"
	flags = fb.GFX_NULL
case "automatic"
	flags = 0
case "opengl"
	flags = fb.GFX_OPENGL
case "vulkan"
	flags = fb.GFX_VULKAN
case else
	end 1
end select

if screenres( 320, 200, 8, 1, flags ) <> 0 then end 2
color 37, 12
locate 2, 3
print "I";
if screen( 2, 3 ) <> asc( "I" ) then end 3
if screen( 2, 3, 1 ) <> ( 37 or ( 12 shl 8 ) ) then end 4
if screen( 2, 3, 2 ) <> ( 37 or ( 12 shl 8 ) ) then end 5
screen 0

if screenres( 320, 200, 16, 1, flags ) <> 0 then end 6
color rgb( 255, 0, 0 ), rgb( 0, 0, 255 )
locate 2, 3
print "R";
if screen( 2, 3 ) <> asc( "R" ) then end 7
if screen( 2, 3, 1 ) <> rgb( 255, 0, 0 ) then end 8
if screen( 2, 3, 2 ) <> rgb( 0, 0, 255 ) then end 9
screen 0

print "GFX_CONSOLE_DEPTH_PASS"
end 0

'' end of console-depth-smoke.bas
