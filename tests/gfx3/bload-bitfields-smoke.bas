''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: bload-bitfields-smoke.bas
''
'' Purpose:
''
''     Verify minimal 16-bit RGB565 and 32-bit RGBA BI_BITFIELDS BMP files
''     are accepted through the public BLOAD API.
''
'' Responsibilities:
''
''     - write a bounded one-pixel Windows BMP fixture
''     - load it into a 32-bit FreeBASIC image
''     - prove RGB565 masks expand to an exact green pixel
''     - prove full-width RGBA masks preserve every source component
''
'' This file intentionally does NOT contain:
''
''     - OS/2, RLE, or indexed BMP coverage
''     - image file fuzzing
''
#ifndef GFX2_REFERENCE
    #ifndef __FB_GFXLIB3__
        #define __FB_GFXLIB3__
    #endif
#endif
#include once "fbgfx.bi"

const filename = "gfx3-bload-bitfields-smoke.bmp"
const alpha_filename = "gfx3-bload-bitfields-alpha-smoke.bmp"
dim as ushort word_value
dim as ulong dword_value
dim as any ptr image

open filename for binary access write as #1
word_value = &h4D42 : put #1, , word_value
dword_value = 70 : put #1, , dword_value
word_value = 0 : put #1, , word_value : put #1, , word_value
dword_value = 66 : put #1, , dword_value
dword_value = 40 : put #1, , dword_value
dword_value = 1 : put #1, , dword_value : put #1, , dword_value
word_value = 1 : put #1, , word_value
word_value = 16 : put #1, , word_value
dword_value = 3 : put #1, , dword_value
dword_value = 4 : put #1, , dword_value
dword_value = 0 : put #1, , dword_value : put #1, , dword_value
put #1, , dword_value : put #1, , dword_value
dword_value = &hF800 : put #1, , dword_value
dword_value = &h07E0 : put #1, , dword_value
dword_value = &h001F : put #1, , dword_value
word_value = &h07E0 : put #1, , word_value
word_value = 0 : put #1, , word_value
close #1

''
'' A 32-bit source with all four masks exercises the full-width normalization
'' path.  In particular, an all-ones alpha field would overflow a 32-bit
'' multiply before it reached the 8-bit API colour range.
''
open alpha_filename for binary access write as #1
word_value = &h4D42 : put #1, , word_value
dword_value = 74 : put #1, , dword_value
word_value = 0 : put #1, , word_value : put #1, , word_value
dword_value = 70 : put #1, , dword_value
dword_value = 40 : put #1, , dword_value
dword_value = 1 : put #1, , dword_value : put #1, , dword_value
word_value = 1 : put #1, , word_value
word_value = 32 : put #1, , word_value
dword_value = 3 : put #1, , dword_value
dword_value = 4 : put #1, , dword_value
dword_value = 0 : put #1, , dword_value : put #1, , dword_value
put #1, , dword_value : put #1, , dword_value
dword_value = &h00FF0000 : put #1, , dword_value
dword_value = &h0000FF00 : put #1, , dword_value
dword_value = &h000000FF : put #1, , dword_value
dword_value = &hFF000000 : put #1, , dword_value
dword_value = &h80402010 : put #1, , dword_value
close #1

#ifdef GFX2_REFERENCE
    setenviron "FBGFX=null"
    if screenres( 16, 16, 32 ) <> 0 then end 1
#elseif defined( __FB_ANDROID__ ) or defined( GFX3_AUTOMATIC_TEST )
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
if point( 0, 0, image ) <> rgb( 0, 255, 0 ) then end 4
'' Populate gfxlib3's CPU-image cache before replacing the image through
'' BLOAD. The second PUT below must not reuse this green GPU source.
put ( 0, 0 ), image, pset
if bload( alpha_filename, image ) <> 0 then end 5
if point( 0, 0, image ) <> rgba( 64, 32, 16, 128 ) then end 6
put ( 2, 0 ), image, pset
if point( 2, 0 ) <> rgba( 64, 32, 16, 128 ) then end 7

imagedestroy image
screen 0
kill filename
kill alpha_filename
print "gfxlib bitfields bload PASS"
end 0

'' end of bload-bitfields-smoke.bas
