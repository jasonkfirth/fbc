''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: custom-font-depth-smoke.bas
''
'' Purpose:
''
''     Verify the custom DRAW STRING bitmap-font protocol at the practical
''     indexed, RGB565, and 32-bit screen depths.
''
'' Responsibilities:
''
''     - construct a native-depth custom A/B font image
''     - verify TRANS masking and PSET copying
''     - preserve unsupported-character spacing at every depth
''
'' This file intentionally does NOT contain:
''
''     - GPU-only surface targets
''     - custom PUT blender callbacks
''     - alternate font header versions
''

#ifdef GFX3_TEST
	#define __FB_GFXLIB3__
#endif
#include once "fbgfx.bi"

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#elseif defined(GFX3_OPENGL_TEST)
	const backend_flags = 0
#else
	const backend_flags = fb.GFX_NULL
#endif

function glyph_a_color( byval depth as integer ) as ulong
	select case depth
	case 8
		return 37
	case else
		return &h00CC2211u
	end select
end function

function glyph_b_color( byval depth as integer ) as ulong
	select case depth
	case 8
		return 92
	case else
		return &h0022AA44u
	end select
end function

function transparent_color( byval depth as integer ) as ulong
	select case depth
	case 8
		return 0
	case else
		return &h00FF00FFu
	end select
end function

function background_color( byval depth as integer ) as ulong
	select case depth
	case 8
		return 11
	case else
		return &h00112233u
	end select
end function

function pack_rgb565( byval rgb_value as ulong ) as ulong
	return ((rgb_value shr 3) and &h001F) or _
		((rgb_value shr 5) and &h07E0) or ((rgb_value shr 8) and &hF800)
end function

function expand_rgb565( byval packed_value as ulong ) as ulong
	return ((packed_value and &h001F) shl 3) or _
		((packed_value shr 2) and &h0007) or _
		((packed_value and &h07E0) shl 5) or _
		((packed_value shr 1) and &h0300) or _
		((packed_value and &hF800) shl 8) or _
		((packed_value shl 3) and &h070000)
end function

function expected_screen_color( byval depth as integer, _
	byval source_color as ulong ) as ulong

	select case depth
	case 8
		return source_color and &hFF
	case 16
		'' POINT expands gfxlib2's native RGB565 screen storage to RGB888.
		return expand_rgb565( pack_rgb565( source_color ) )
	case else
		return source_color
	end select
end function

function check_depth( byval depth as integer, byval failure_base as integer ) _
	as integer

	dim as any ptr font_image
	dim as ubyte ptr font_pixels
	dim as ulong background
	dim as ulong glyph_a
	dim as ulong glyph_b

	if screenres( 32, 16, depth, 1, backend_flags ) <> 0 then
		return failure_base
	end if
	font_image = imagecreate( 6, 5, transparent_color( depth ), depth )
	if font_image = 0 then
		screen 0
		return failure_base + 1
	end if
	imageinfo font_image, , , , , font_pixels
	if font_pixels = 0 then
		imagedestroy font_image
		screen 0
		return failure_base + 2
	end if

	'' Header bytes: version 0, first A, last B, then both glyph widths.
	font_pixels[0] = 0
	font_pixels[1] = asc( "A" )
	font_pixels[2] = asc( "B" )
	font_pixels[3] = 3
	font_pixels[4] = 3
	pset font_image, ( 1, 1 ), glyph_a_color( depth )
	pset font_image, ( 0, 2 ), glyph_a_color( depth )
	pset font_image, ( 2, 2 ), glyph_a_color( depth )
	pset font_image, ( 0, 3 ), glyph_a_color( depth )
	pset font_image, ( 1, 3 ), glyph_a_color( depth )
	pset font_image, ( 2, 3 ), glyph_a_color( depth )
	pset font_image, ( 0, 4 ), glyph_a_color( depth )
	pset font_image, ( 2, 4 ), glyph_a_color( depth )
	for y as integer = 1 to 4
		for x as integer = 3 to 5
			pset font_image, ( x, y ), glyph_b_color( depth )
		next
	next

	background = culng( background_color( depth ) )
	line ( 0, 0 )-( 31, 15 ), background, bf
	background = culng( point( 0, 0 ) )
	glyph_a = expected_screen_color( depth, glyph_a_color( depth ) )
	glyph_b = expected_screen_color( depth, glyph_b_color( depth ) )

	draw string ( 2, 2 ), "ABX", , font_image, trans
	if culng( point( 3, 2 ) ) <> glyph_a then
		imagedestroy font_image
		screen 0
		return failure_base + 3
	end if
	if culng( point( 2, 2 ) ) <> background then
		imagedestroy font_image
		screen 0
		return failure_base + 4
	end if
	if culng( point( 5, 4 ) ) <> glyph_b then
		imagedestroy font_image
		screen 0
		return failure_base + 5
	end if
	if culng( point( 8, 2 ) ) <> background then
		imagedestroy font_image
		screen 0
		return failure_base + 6
	end if

	draw string ( 2, 8 ), "AX", , font_image, pset
	if culng( point( 2, 8 ) ) <> _
		expected_screen_color( depth, transparent_color( depth ) ) then
		imagedestroy font_image
		screen 0
		return failure_base + 7
	end if
	if culng( point( 5, 8 ) ) <> background then
		imagedestroy font_image
		screen 0
		return failure_base + 8
	end if

	imagedestroy font_image
	screen 0
	return 0
end function

dim as integer result
result = check_depth( 8, 10 )
if result <> 0 then end result
result = check_depth( 16, 20 )
if result <> 0 then end result
result = check_depth( 32, 30 )
if result <> 0 then end result

end 0

'' end of custom-font-depth-smoke.bas
