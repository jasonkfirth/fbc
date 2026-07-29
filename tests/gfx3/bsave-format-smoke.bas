''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: bsave-format-smoke.bas
''
'' Purpose:
''
''     Exercise the public GPU-screen BSAVE BMP paths used by gfxlib2: indexed
''     palette output, RGB565 expansion, and the 24-bit override for a 32-bit
''     source.
''
'' Responsibilities:
''
''     - save each source depth from its GPU-backed screen page
''     - load the resulting BMP into a 32-bit GPU page
''     - verify the observable BLOAD colour after the documented conversion
''
'' This file intentionally does NOT contain:
''
''     - raw BSAVE block coverage
''     - arbitrary external BMP fixtures
''     - image-pointer BSAVE coverage
''

#ifndef __FB_GFXLIB3__
	#define __FB_GFXLIB3__
#endif
#include once "fbgfx.bi"

const indexed_filename = "gfx3-bsave-indexed.bmp"
const rgb565_filename = "gfx3-bsave-rgb565.bmp"
const rgb24_filename = "gfx3-bsave-rgb24.bmp"

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

if screenres( 16, 16, 8, 1, flags ) <> 0 then end 2
palette 37, 32, 16, 8
pset ( 3, 2 ), 37
if bsave( indexed_filename, 0 ) <> 0 then end 3
screen 0

if screenres( 16, 16, 32, 1, flags ) <> 0 then end 4
if bload( indexed_filename, 0 ) <> 0 then end 5
if point( 3, 2 ) <> rgb( 32, 16, 8 ) then end 6
screen 0

if screenres( 16, 16, 16, 1, flags ) <> 0 then end 7
pset ( 3, 2 ), rgb( 12, 34, 56 )
if bsave( rgb565_filename, 0 ) <> 0 then end 8
screen 0

if screenres( 16, 16, 32, 1, flags ) <> 0 then end 9
if bload( rgb565_filename, 0 ) <> 0 then end 10
if point( 3, 2 ) <> rgb( 8, 32, 57 ) then end 11
screen 0

if screenres( 16, 16, 32, 1, flags ) <> 0 then end 12
pset ( 3, 2 ), rgba( 10, 20, 30, 40 )
if bsave( rgb24_filename, 0, 0, 0, 24 ) <> 0 then end 13
cls
if bload( rgb24_filename, 0 ) <> 0 then end 14
if point( 3, 2 ) <> rgb( 10, 20, 30 ) then end 15
screen 0

kill indexed_filename
kill rgb565_filename
kill rgb24_filename
print "GFX_BSAVE_FORMAT_PASS"
end 0

'' end of bsave-format-smoke.bas
