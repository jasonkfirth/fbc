''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: bload-rle-smoke.bas
''
'' Purpose:
''
''     Verify bounded Windows RLE8 and RLE4 BMP streams are decoded through
''     the public BLOAD API before their palette indexes reach the GPU path.
''
'' Responsibilities:
''
''     - exercise encoded, absolute, delta, end-of-line, and end-of-bitmap RLE8
''     - exercise packed absolute RLE4 indexes
''     - verify bottom-up BMP row ordering and exact palette output
''     - load the same decoded pixels through a GPU-backed screen page
''
'' This file intentionally does NOT contain:
''
''     - malformed-stream fuzzing
''     - OS/2 RLE variants
''     - arbitrary external BMP corpus coverage
''
#ifndef __FB_GFXLIB3__
    #define __FB_GFXLIB3__
#endif
#include once "fbgfx.bi"

const rle8_filename = "gfx3-bload-rle8-smoke.bmp"
const rle4_filename = "gfx3-bload-rle4-smoke.bmp"
const invalid_filename = "gfx3-bload-rle-invalid-smoke.bmp"
dim as ushort word_value
dim as ulong dword_value
dim as ubyte byte_value, red_value, green_value, blue_value
dim as integer i
dim as any ptr image

'' 3 by 2 RLE8: bottom red/delta/red sequence, then a top absolute sequence.
open rle8_filename for binary access write as #1
word_value = &h4D42 : put #1, , word_value
dword_value = 86 : put #1, , dword_value
word_value = 0 : put #1, , word_value : put #1, , word_value
dword_value = 66 : put #1, , dword_value
dword_value = 40 : put #1, , dword_value
dword_value = 3 : put #1, , dword_value
dword_value = 2 : put #1, , dword_value
word_value = 1 : put #1, , word_value
word_value = 8 : put #1, , word_value
dword_value = 1 : put #1, , dword_value
dword_value = 20 : put #1, , dword_value
dword_value = 0 : put #1, , dword_value : put #1, , dword_value
dword_value = 3 : put #1, , dword_value : put #1, , dword_value
for i = 0 to 2
    red_value = 0 : green_value = 0 : blue_value = 0
    if i = 1 then red_value = &hFF
    if i = 2 then green_value = &hFF
    put #1, , blue_value : put #1, , green_value : put #1, , red_value
    byte_value = 0 : put #1, , byte_value
next i
byte_value = 1 : put #1, , byte_value : byte_value = 1 : put #1, , byte_value
byte_value = 0 : put #1, , byte_value : byte_value = 2 : put #1, , byte_value
byte_value = 1 : put #1, , byte_value : byte_value = 0 : put #1, , byte_value
byte_value = 1 : put #1, , byte_value : byte_value = 1 : put #1, , byte_value
byte_value = 0 : put #1, , byte_value : put #1, , byte_value
byte_value = 0 : put #1, , byte_value : byte_value = 3 : put #1, , byte_value
byte_value = 2 : put #1, , byte_value : byte_value = 1 : put #1, , byte_value
byte_value = 2 : put #1, , byte_value : byte_value = 0 : put #1, , byte_value
put #1, , byte_value : put #1, , byte_value
byte_value = 0 : put #1, , byte_value : byte_value = 1 : put #1, , byte_value
close #1

'' A two-pixel image may not accept a three-pixel encoded RLE8 run.
open invalid_filename for binary access write as #1
word_value = &h4D42 : put #1, , word_value
dword_value = 70 : put #1, , dword_value
word_value = 0 : put #1, , word_value : put #1, , word_value
dword_value = 62 : put #1, , dword_value
dword_value = 40 : put #1, , dword_value
dword_value = 2 : put #1, , dword_value : dword_value = 1 : put #1, , dword_value
word_value = 1 : put #1, , word_value
word_value = 8 : put #1, , word_value
dword_value = 1 : put #1, , dword_value
dword_value = 4 : put #1, , dword_value
dword_value = 0 : put #1, , dword_value : put #1, , dword_value
dword_value = 2 : put #1, , dword_value : put #1, , dword_value
byte_value = 0 : put #1, , byte_value : put #1, , byte_value
put #1, , byte_value : put #1, , byte_value
byte_value = 0 : put #1, , byte_value : put #1, , byte_value
byte_value = &hFF : put #1, , byte_value
byte_value = 0 : put #1, , byte_value
byte_value = 3 : put #1, , byte_value : byte_value = 1 : put #1, , byte_value
byte_value = 0 : put #1, , byte_value : put #1, , byte_value
close #1

'' 3 by 1 RLE4 absolute indices 1, 2, 3 packed as high/low nibbles.
open rle4_filename for binary access write as #1
word_value = &h4D42 : put #1, , word_value
dword_value = 76 : put #1, , dword_value
word_value = 0 : put #1, , word_value : put #1, , word_value
dword_value = 70 : put #1, , dword_value
dword_value = 40 : put #1, , dword_value
dword_value = 3 : put #1, , dword_value : dword_value = 1 : put #1, , dword_value
word_value = 1 : put #1, , word_value
word_value = 4 : put #1, , word_value
dword_value = 2 : put #1, , dword_value
dword_value = 6 : put #1, , dword_value
dword_value = 0 : put #1, , dword_value : put #1, , dword_value
dword_value = 4 : put #1, , dword_value : put #1, , dword_value
for i = 0 to 3
    red_value = 0 : green_value = 0 : blue_value = 0
    if i = 1 then red_value = &hFF
    if i = 2 then green_value = &hFF
    if i = 3 then blue_value = &hFF
    put #1, , blue_value : put #1, , green_value : put #1, , red_value
    byte_value = 0 : put #1, , byte_value
next i
byte_value = 0 : put #1, , byte_value : byte_value = 3 : put #1, , byte_value
byte_value = &h12 : put #1, , byte_value : byte_value = &h30 : put #1, , byte_value
byte_value = 0 : put #1, , byte_value : byte_value = 1 : put #1, , byte_value
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
image = imagecreate( 3, 2, 0, 32 )
if image = 0 then end 2
if bload( rle8_filename, image ) <> 0 then end 3
if point( 0, 0, image ) <> rgb( 0, 255, 0 ) then end 4
if point( 1, 0, image ) <> rgb( 255, 0, 0 ) then end 5
if point( 2, 0, image ) <> rgb( 0, 255, 0 ) then end 6
if point( 0, 1, image ) <> rgb( 255, 0, 0 ) then end 7
if point( 1, 1, image ) <> rgb( 0, 0, 0 ) then end 8
if point( 2, 1, image ) <> rgb( 255, 0, 0 ) then end 9
if bload( rle4_filename, image ) <> 0 then end 10
if point( 0, 0, image ) <> rgb( 255, 0, 0 ) then end 11
if point( 1, 0, image ) <> rgb( 0, 255, 0 ) then end 12
if point( 2, 0, image ) <> rgb( 0, 0, 255 ) then end 13
if bload( invalid_filename, image ) = 0 then end 14
if point( 0, 0, image ) <> rgb( 255, 0, 0 ) then end 15
if point( 1, 0, image ) <> rgb( 0, 255, 0 ) then end 16
if point( 2, 0, image ) <> rgb( 0, 0, 255 ) then end 17

''
'' The image target validates the checked decoder. The screen target proves
'' the palette-expanded rows are also uploaded through the active renderer,
'' rather than only being correct in CPU image memory.
''
if bload( rle8_filename ) <> 0 then end 18
if point( 0, 0 ) <> rgb( 0, 255, 0 ) then end 19
if point( 1, 0 ) <> rgb( 255, 0, 0 ) then end 20
if point( 2, 0 ) <> rgb( 0, 255, 0 ) then end 21
if point( 0, 1 ) <> rgb( 255, 0, 0 ) then end 22
if point( 1, 1 ) <> rgb( 0, 0, 0 ) then end 23
if point( 2, 1 ) <> rgb( 255, 0, 0 ) then end 24
if bload( rle4_filename ) <> 0 then end 25
if point( 0, 0 ) <> rgb( 255, 0, 0 ) then end 26
if point( 1, 0 ) <> rgb( 0, 255, 0 ) then end 27
if point( 2, 0 ) <> rgb( 0, 0, 255 ) then end 28
if bload( invalid_filename ) = 0 then end 29
if point( 0, 0 ) <> rgb( 255, 0, 0 ) then end 30
if point( 1, 0 ) <> rgb( 0, 255, 0 ) then end 31
if point( 2, 0 ) <> rgb( 0, 0, 255 ) then end 32

imagedestroy image
screen 0
kill rle8_filename
kill rle4_filename
kill invalid_filename
print "gfxlib rle bload PASS"
end 0

'' end of bload-rle-smoke.bas
