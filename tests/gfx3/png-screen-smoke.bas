''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: png-screen-smoke.bas
''
'' Purpose:
''
''     Verify PNG BSAVE and BLOAD through a GPU-backed screen page.
''
'' Responsibilities:
''
''     - download a 32-bit screen page into an RGBA PNG
''     - upload that PNG and preserve exact colour and alpha values
''     - preserve indexed pixels and palette values through a screen round trip
''     - exercise case-insensitive PNG filename selection
''
'' This file intentionally does NOT contain:
''
''     - CPU FB.IMAGE PNG coverage
''     - malformed PNG fixtures
''     - renderer-specific visual checks
''

#include once "fbgfx3.bi"

const rgba_filename = "gfx3-png-screen-rgba.PnG"
const indexed_filename = "gfx3-png-screen-indexed.png"

if screenres( 16, 12, 32, 1, fb.GFX_NULL ) <> 0 then end 1
pset ( 3, 4 ), rgba( 17, 83, 201, 119 )
if bsave( rgba_filename, 0 ) <> 0 then end 2
cls
if bload( rgba_filename, 0 ) <> 0 then end 3
if point( 3, 4 ) <> rgba( 17, 83, 201, 119 ) then end 4
screen 0

if screenres( 16, 12, 8, 1, fb.GFX_NULL ) <> 0 then end 5
palette 37, 40, 80, 120
pset ( 5, 6 ), 37
if bsave( indexed_filename, 0 ) <> 0 then end 6
palette 37, 0, 0, 0
cls
if bload( indexed_filename, 0 ) <> 0 then end 7
if point( 5, 6 ) <> 37 then end 8

dim as integer red, green, blue
palette get 37, red, green, blue
if ( red <> 40 ) or ( green <> 80 ) or ( blue <> 120 ) then end 9
screen 0

kill rgba_filename
kill indexed_filename
print "GFX3_PNG_SCREEN_PASS"
end 0

'' end of png-screen-smoke.bas
