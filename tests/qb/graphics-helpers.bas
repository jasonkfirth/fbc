'' FreeBASIC QB tests: graphics-helpers.bas
'' Check graphics helper declarations, temporary strings and 32-bit outputs.
'' Uses the Null driver; no native fonts or Visual Basic parser are involved.
' TEST_MODE : COMPILE_AND_RUN_OK
' TEST_LANG : qb

#include once "fbgfx.bi"

dim pixel_width as long, pixel_height as long
if __screenres( 32, 32, 8, 1, GFX_NULL ) <> 0 then end 1
if DrawStringSize( "A" + chr$(0), pixel_width, pixel_height ) <> 0 then end 2
if pixel_width <> 16 or pixel_height < 1 then end 3
if PaintPattern( 0, 4, 5, chr$(255), 2, 0, 1 ) <> 0 then end 4
if point( 31, 31 ) <> 2 then end 5
screen 0
print "QB graphics helpers: passed"

'' end of graphics-helpers.bas
