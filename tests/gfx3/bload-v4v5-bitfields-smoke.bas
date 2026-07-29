''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: bload-v4v5-bitfields-smoke.bas
''
'' Purpose:
''
''     Verify the Windows V3, V4, and V5 BMP header forms accepted by
''     gfxlib2 load through gfxlib3's checked bitfield path.
''
'' Responsibilities:
''
''     - write bounded one-pixel V3, V4, and V5 bitfield BMP fixtures
''     - put masks in the header extension at their documented offsets
''     - confirm RGB and alpha expansion reaches a CPU FB.IMAGE exactly
''
'' This file intentionally does NOT contain:
''
''     - OS/2 V2 or non-BMP image codecs
''     - malformed-header fuzzing
''     - graphics-card presentation checks
''

#ifndef GFX2_REFERENCE
    #ifndef __FB_GFXLIB3__
        #define __FB_GFXLIB3__
    #endif
#endif
#include once "fbgfx.bi"

sub write_bitfields_bmp( byref filename as string, byval header_size as ulong )
    dim as ushort word_value
    dim as ulong dword_value
    dim as integer extension_dwords, i

    open filename for binary access write as #1
    word_value = &h4D42 : put #1, , word_value
    dword_value = 14 + header_size + 4 : put #1, , dword_value
    word_value = 0 : put #1, , word_value : put #1, , word_value
    dword_value = 14 + header_size : put #1, , dword_value

    dword_value = header_size : put #1, , dword_value
    dword_value = 1 : put #1, , dword_value : put #1, , dword_value
    word_value = 1 : put #1, , word_value
    word_value = 32 : put #1, , word_value
    dword_value = 3 : put #1, , dword_value
    dword_value = 4 : put #1, , dword_value
    dword_value = 0
    put #1, , dword_value : put #1, , dword_value
    put #1, , dword_value : put #1, , dword_value

    dword_value = &h00FF0000 : put #1, , dword_value
    dword_value = &h0000FF00 : put #1, , dword_value
    dword_value = &h000000FF : put #1, , dword_value
    dword_value = &hFF000000 : put #1, , dword_value

    extension_dwords = (header_size - 56) \ 4
    dword_value = 0
    for i = 1 to extension_dwords
        put #1, , dword_value
    next i

    dword_value = &h80402010 : put #1, , dword_value
    close #1
end sub

sub check_bload( byref filename as string, byval image as any ptr )
    if bload( filename, image ) <> 0 then end 3
    if point( 0, 0, image ) <> rgba( 64, 32, 16, 128 ) then end 4
end sub

const v3_filename = "gfx3-bload-v3-bitfields-smoke.bmp"
const v4_filename = "gfx3-bload-v4-bitfields-smoke.bmp"
const v5_filename = "gfx3-bload-v5-bitfields-smoke.bmp"
dim as any ptr image

write_bitfields_bmp v3_filename, 56
write_bitfields_bmp v4_filename, 108
write_bitfields_bmp v5_filename, 124

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
check_bload v3_filename, image
check_bload v4_filename, image
check_bload v5_filename, image

imagedestroy image
screen 0
kill v3_filename
kill v4_filename
kill v5_filename
print "gfxlib v3/v4/v5 bitfields bload PASS"
end 0

'' end of bload-v4v5-bitfields-smoke.bas
