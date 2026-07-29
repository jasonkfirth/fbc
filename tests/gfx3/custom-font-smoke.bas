''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: custom-font-smoke.bas
''
'' Purpose:
''
''     Compare the documented custom DRAW STRING font-image protocol against
''     gfxlib2 and exercise it on each gfxlib3 target class.
''
'' Responsibilities:
''
''     - build a two-glyph, 32-bit custom font in the public image format
''     - verify transparent and PSET custom-font PUT behavior
''     - verify unsupported-character spacing
''     - verify screen, CPU image, and GPU-resident gfxlib3 targets
''
'' This file intentionally does NOT contain:
''
''     - a scalable-font API
''     - a custom blender callback
''     - a visible-window presentation assumption
''
#ifdef GFX2_REFERENCE
    #include once "fbgfx.bi"
#else
    #ifndef __FB_GFXLIB3__
        #define __FB_GFXLIB3__
    #endif
    #include once "fbgfx3.bi"
#endif

#ifdef GFX2_REFERENCE
    const backend_flags = fb.GFX_NULL
#elseif defined(GFX3_NULL_TEST)
    const backend_flags = fb.GFX_NULL
#elseif defined(GFX3_VULKAN_TEST)
    const backend_flags = fb.GFX_VULKAN
#elseif defined(GFX3_OPENGL_TEST)
    const backend_flags = fb.GFX_OPENGL
#else
    '' Automatic selection is expected to choose GLES on Android and the best
    '' available desktop accelerator elsewhere.
    const backend_flags = 0
#endif

'' 32-bit gfxlib2 ordinary PSET stores RGB pixels with an unset alpha byte.
'' Keep the custom font in that native representation rather than relying on
'' alpha-primitive mode, which is a separate compatibility contract.
const as ulong background_color = &h00112233u
const as ulong glyph_a_color = &h00CC2211u
const as ulong glyph_b_color = &h0022AA44u
const as ulong transparent_color = &h00FF00FFu

function image_hash( byval image as any ptr, byval image_width as integer, _
    byval height as integer ) as ulong
    dim as ulong value = &h811C9DC5u

    for y as integer = 0 to height - 1
        for x as integer = 0 to image_width - 1
            value xor= culng( point( x, y, image ) )
            value *= &h01000193u
        next
    next

    return value
end function

if screenres( 32, 16, 32, 1, backend_flags ) <> 0 then end 1

dim as any ptr font_image = imagecreate( 6, 5, transparent_color, 32 )
dim as any ptr image_target = imagecreate( 32, 16, background_color, 32 )
dim as ubyte ptr font_pixels

if font_image = 0 orelse image_target = 0 then end 2
imageinfo font_image, , , , , font_pixels
if font_pixels = 0 then end 3

'' Header bytes: version 0, first character A, last character B, and two
'' three-pixel glyph widths. The glyph pixels start at y = 1.
font_pixels[0] = 0
font_pixels[1] = asc( "A" )
font_pixels[2] = asc( "B" )
font_pixels[3] = 3
font_pixels[4] = 3

'' gfxlib2's 32-bit TRANS method uses magenta rather than zero as its mask.
'' A has one masked interior pixel. B occupies its complete three-wide
'' rectangle, making TRANS and PSET differences unambiguous.
pset font_image, ( 1, 1 ), glyph_a_color
pset font_image, ( 0, 2 ), glyph_a_color
pset font_image, ( 2, 2 ), glyph_a_color
pset font_image, ( 0, 3 ), glyph_a_color
pset font_image, ( 1, 3 ), glyph_a_color
pset font_image, ( 2, 3 ), glyph_a_color
pset font_image, ( 0, 4 ), glyph_a_color
pset font_image, ( 2, 4 ), glyph_a_color
for y as integer = 1 to 4
    for x as integer = 3 to 5
        pset font_image, ( x, y ), glyph_b_color
    next
next

if culng( point( 0, 1, font_image ) ) <> transparent_color then
    dim as ulong font_mask_pixel = culng( point( 0, 1, font_image ) )
    screen 0
    print "custom font mask expected " & hex( transparent_color, 8 ) & _
        ", got " & hex( font_mask_pixel, 8 )
    end 15
end if

line ( 0, 0 )-( 31, 15 ), background_color, bf
if culng( point( 0, 0 ) ) <> background_color then
    dim as ulong initial_background = culng( point( 0, 0 ) )
    screen 0
    print "custom font initial background expected " & hex( background_color, 8 ) & _
        ", got " & hex( initial_background, 8 )
    end 14
end if
draw string ( 2, 2 ), "ABX", , font_image, trans
if culng( point( 3, 2 ) ) <> glyph_a_color then
    dim as ulong actual_color = culng( point( 3, 2 ) )
    screen 0
    print "custom font A expected " & hex( glyph_a_color, 8 ) & _
        ", got " & hex( actual_color, 8 )
    end 4
end if
if culng( point( 2, 2 ) ) <> background_color then
    dim as ulong actual_background = culng( point( 2, 2 ) )
    screen 0
    print "custom font transparent expected " & hex( background_color, 8 ) & _
        ", got " & hex( actual_background, 8 )
    end 5
end if
if culng( point( 5, 4 ) ) <> glyph_b_color then end 6
'' X is unsupported and therefore advances by the four-pixel font height.
if culng( point( 8, 2 ) ) <> background_color then
    dim as ulong unsupported_pixel = culng( point( 8, 2 ) )
    screen 0
    print "custom font unsupported character expected " & _
        hex( background_color, 8 ) & ", got " & hex( unsupported_pixel, 8 )
    end 7
end if
draw string ( 2, 8 ), "AX", , font_image, pset
if culng( point( 5, 8 ) ) <> background_color then end 8

draw string image_target, ( 2, 2 ), "A", , font_image, pset
if culng( point( 2, 2, image_target ) ) <> transparent_color then end 9
if culng( point( 3, 2, image_target ) ) <> glyph_a_color then end 10
'' gfxlib2 skips unsupported glyphs for PSET too. Their advance must not turn
'' into a copied magenta TRANS key in the destination image.
draw string image_target, ( 2, 8 ), "AX", , font_image, pset
if culng( point( 5, 8, image_target ) ) <> background_color then end 11

dim as ulong screen_hash = image_hash( 0, 32, 16 )
dim as ulong image_hash_value = image_hash( image_target, 32, 16 )

#ifndef GFX2_REFERENCE
    dim as any ptr gpu_target = fb.Gfx3SurfaceCreate( 32, 16, 32, , _
        background_color )
    if gpu_target = 0 then end 12
    draw string gpu_target, ( 2, 2 ), "A", , font_image, pset
    if culng( point( 2, 2, gpu_target ) ) <> transparent_color then end 13
    if culng( point( 3, 2, gpu_target ) ) <> glyph_a_color then end 14
    draw string gpu_target, ( 2, 8 ), "AX", , font_image, pset
    if culng( point( 5, 8, gpu_target ) ) <> background_color then end 15
    if fb.Gfx3SurfaceDestroy( gpu_target ) <> 0 then end 16
#endif

imagedestroy image_target
imagedestroy font_image
screen 0
print "gfxlib custom font hashes " & hex( screen_hash, 8 ) & " " & _
    hex( image_hash_value, 8 )
end 0

'' end of custom-font-smoke.bas
