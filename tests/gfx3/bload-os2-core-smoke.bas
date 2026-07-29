''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: bload-os2-core-smoke.bas
''
'' Purpose:
''
''     Verify BLOAD accepts the historical OS/2 V1 BITMAPCOREHEADER format.
''
'' Responsibilities:
''
''     - write a bounded 4-bit OS/2 core-header BMP fixture
''     - verify its three-byte BGR palette is decoded correctly
''     - verify packed high-nibble indexed pixels reach a 32-bit image
''     - verify the decoded pixel uploads to an active GPU-backed page
''
'' This file intentionally does NOT contain:
''
''     - OS/2 V2 or RLE BMP coverage
''     - Windows bitmap header coverage
''     - malformed-file fuzzing
''
#ifndef __FB_GFXLIB3__
    #define __FB_GFXLIB3__
#endif
#include once "fbgfx.bi"

const filename = "gfx3-bload-os2-core-smoke.bmp"
dim as ushort word_value
dim as ulong dword_value
dim as ubyte byte_value, red_value, green_value, blue_value
dim as integer i
dim as any ptr image

open filename for binary access write as #1
word_value = &h4D42 : put #1, , word_value
dword_value = 78 : put #1, , dword_value
word_value = 0 : put #1, , word_value : put #1, , word_value
dword_value = 74 : put #1, , dword_value
dword_value = 12 : put #1, , dword_value
word_value = 1 : put #1, , word_value : put #1, , word_value
word_value = 1 : put #1, , word_value
word_value = 4 : put #1, , word_value

for i = 0 to 15
    red_value = 0 : green_value = 0 : blue_value = 0
    if i = 1 then red_value = &h80 : green_value = &h40 : blue_value = &h20
    put #1, , blue_value : put #1, , green_value : put #1, , red_value
next i

byte_value = &h10 : put #1, , byte_value
byte_value = 0 : put #1, , byte_value : put #1, , byte_value : put #1, , byte_value
close #1

#if defined( __FB_ANDROID__ ) or defined( GFX3_AUTOMATIC_TEST )
    if screenres( 16, 16, 32 ) <> 0 then end 1
#elseif defined( GFX3_OPENGL_TEST )
    if screenres( 16, 16, 32, 1, fb.GFX_OPENGL ) <> 0 then end 1
#elseif defined( GFX3_VULKAN_TEST )
    if screenres( 16, 16, 32, 1, fb.GFX_VULKAN ) <> 0 then end 1
#else
    if screenres( 16, 16, 32, 1, fb.GFX_NULL ) <> 0 then end 1
#endif
image = imagecreate( 1, 1, 0, 32 )
if image = 0 then end 2
if bload( filename, image ) <> 0 then end 3
if point( 0, 0, image ) <> rgb( &h80, &h40, &h20 ) then end 4
if bload( filename ) <> 0 then end 5
if point( 0, 0 ) <> rgb( &h80, &h40, &h20 ) then end 6

imagedestroy image
screen 0
kill filename
print "gfxlib os2 core bload PASS"
end 0

'' end of bload-os2-core-smoke.bas
