''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: bload-indexed-smoke.bas
''
'' Purpose:
''
''     Verify BLOAD handles the packed 1-bit and 4-bit indexed Windows BMP
''     formats that precede the existing 8-bit indexed path.
''
'' Responsibilities:
''
''     - write bounded 1-bit and 4-bit Windows information-header fixtures
''     - verify palette entry counts and four-byte BGRX interpretation
''     - verify high-bit and high-nibble source index extraction
''
'' This file intentionally does NOT contain:
''
''     - compressed RLE BMP coverage
''     - OS/2 palette coverage
''     - malformed-file fuzzing
''
#ifndef __FB_GFXLIB3__
    #define __FB_GFXLIB3__
#endif
#include once "fbgfx.bi"

const mono_filename = "gfx3-bload-indexed-mono.bmp"
const nibble_filename = "gfx3-bload-indexed-nibble.bmp"
dim as ushort word_value
dim as ulong dword_value
dim as ubyte byte_value, red_value, green_value, blue_value
dim as integer i
dim as any ptr image

'' 2 by 1, 1-bit Windows BMP: the first high-bit pixel is palette entry 1.
open mono_filename for binary access write as #1
word_value = &h4D42 : put #1, , word_value
dword_value = 66 : put #1, , dword_value
word_value = 0 : put #1, , word_value : put #1, , word_value
dword_value = 62 : put #1, , dword_value
dword_value = 40 : put #1, , dword_value
dword_value = 2 : put #1, , dword_value
dword_value = 1 : put #1, , dword_value
word_value = 1 : put #1, , word_value
word_value = 1 : put #1, , word_value
dword_value = 0 : put #1, , dword_value
dword_value = 4 : put #1, , dword_value
dword_value = 0 : put #1, , dword_value : put #1, , dword_value
dword_value = 2 : put #1, , dword_value : put #1, , dword_value
byte_value = 0 : put #1, , byte_value : put #1, , byte_value
put #1, , byte_value : put #1, , byte_value
byte_value = 0 : put #1, , byte_value : put #1, , byte_value
byte_value = &hFF : put #1, , byte_value
byte_value = 0 : put #1, , byte_value
byte_value = &h80 : put #1, , byte_value
byte_value = 0 : put #1, , byte_value : put #1, , byte_value : put #1, , byte_value
close #1

'' 1 by 1, 4-bit Windows BMP: the first high nibble selects palette entry 1.
open nibble_filename for binary access write as #1
word_value = &h4D42 : put #1, , word_value
dword_value = 122 : put #1, , dword_value
word_value = 0 : put #1, , word_value : put #1, , word_value
dword_value = 118 : put #1, , dword_value
dword_value = 40 : put #1, , dword_value
dword_value = 1 : put #1, , dword_value : put #1, , dword_value
word_value = 1 : put #1, , word_value
word_value = 4 : put #1, , word_value
dword_value = 0 : put #1, , dword_value
dword_value = 4 : put #1, , dword_value
dword_value = 0 : put #1, , dword_value : put #1, , dword_value
dword_value = 16 : put #1, , dword_value : put #1, , dword_value
for i = 0 to 15
    red_value = 0 : green_value = 0 : blue_value = 0
    if i = 1 then red_value = &h80 : green_value = &h40 : blue_value = &h20
    put #1, , blue_value : put #1, , green_value : put #1, , red_value
    byte_value = 0 : put #1, , byte_value
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
image = imagecreate( 2, 1, 0, 32 )
if image = 0 then end 2
if bload( mono_filename, image ) <> 0 then end 3
if point( 0, 0, image ) <> rgb( 255, 0, 0 ) then end 4
if point( 1, 0, image ) <> rgb( 0, 0, 0 ) then end 5
if bload( nibble_filename, image ) <> 0 then end 6
if point( 0, 0, image ) <> rgb( &h80, &h40, &h20 ) then end 7

imagedestroy image
screen 0
kill mono_filename
kill nibble_filename
print "gfxlib indexed bload PASS"
end 0

'' end of bload-indexed-smoke.bas
