''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: custom-font-custom-blender-depth-smoke.bas
''
'' Purpose:
''
''     Verify that custom DRAW STRING fonts preserve the PUT CUSTOM callback
''     contract at indexed, RGB565, and 32-bit target depths.
''
'' Responsibilities:
''
''     - construct a native-depth single-glyph bitmap font
''     - require CUSTOM to invoke the callback for every glyph rectangle pixel
''     - preserve the historical no-write gap for an unsupported character
''     - cover screen, CPU-image, and gfxlib3 GPU-surface destinations
''
'' This file intentionally does NOT contain:
''
''     - custom PUT arithmetic already covered by image-smoke.bas
''     - TRANS or PSET custom-font coverage
''     - variable-width multi-glyph parsing
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
#elseif defined(GFX3_VULKAN_TEST)
	const backend_flags = fb.GFX_VULKAN
#elseif defined(GFX3_OPENGL_TEST)
	const backend_flags = 0
#else
	const backend_flags = fb.GFX_NULL
#endif

dim shared as integer callback_count

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
	byval native_color as ulong ) as ulong

	select case depth
	case 8
		return native_color and &hFF
	case 16
		return expand_rgb565( native_color and &hFFFF )
	case else
		return native_color
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

function callback_color( byval depth as integer ) as ulong
	select case depth
	case 8
		return &h5Au
	case 16
		return &h1234u
	case else
		return &h00123456u
	end select
end function

function custom_font_blender _
	( _
		byval source_pixel as ulong, _
		byval destination_pixel as ulong, _
		byval parameter as any ptr _
	) as ulong

	dim as integer ptr depth = cptr( integer ptr, parameter )

	'' Count calls so a renderer cannot replace CUSTOM with a transparent copy.
	callback_count += 1
	return callback_color( *depth )
end function

function check_target( byval target as any ptr, byval depth as integer, _
	byval font_image as any ptr, byval callback_depth as integer, _
	byval failure_base as integer ) as integer

	dim as ulong expected_background
	dim as ulong expected_callback = expected_screen_color( depth, _
		callback_color( depth ) )

	if depth = 16 then
		'' A BASIC blender returns a public RGB color. gfxlib2 converts that
		'' result to the target's RGB565 storage before POINT expands it again.
		expected_callback = expected_screen_color( depth, _
			pack_rgb565( callback_color( depth ) ) )
	end if
	'' Capture the public target value before CUSTOM runs. GPU-surface creation
	'' has its own clear-color API, while this check is solely about preserving
	'' the unsupported glyph's no-write gap.
	expected_background = culng( point( 0, 0, target ) )
	callback_count = 0
	draw string target, ( 2, 2 ), "AXA", , font_image, custom, _
		@custom_font_blender, @callback_depth
	if callback_count <> 24 then return failure_base + 1
	'' A is three columns wide and four pixels high. CUSTOM writes every pixel,
	'' including the font's magenta/zero TRANS key, through the callback.
	if culng( point( 2, 2, target ) ) <> expected_callback then _
		return failure_base + 2
	if culng( point( 4, 5, target ) ) <> expected_callback then _
		return failure_base + 3
	'' X is absent from the font and advances by the four-pixel font height.
	if culng( point( 6, 3, target ) ) <> expected_background then _
		return failure_base + 4
	if culng( point( 9, 2, target ) ) <> expected_callback then _
		return failure_base + 5
	return 0
end function

function check_depth( byval depth as integer, byval failure_base as integer ) _
	as integer

	dim as any ptr font_image
	dim as any ptr image_target
	dim as ubyte ptr font_pixels
	dim as integer result
	dim as integer callback_depth = depth

	if screenres( 16, 8, depth, 1, backend_flags ) <> 0 then
		return failure_base
	end if
	font_image = imagecreate( 3, 5, 0, depth )
	image_target = imagecreate( 16, 8, background_color( depth ), depth )
	if font_image = 0 orelse image_target = 0 then
		if image_target <> 0 then imagedestroy image_target
		if font_image <> 0 then imagedestroy font_image
		screen 0
		return failure_base + 1
	end if
	imageinfo font_image, , , , , font_pixels
	if font_pixels = 0 then
		imagedestroy image_target
		imagedestroy font_image
		screen 0
		return failure_base + 2
	end if

	'' Version 0, only A, three-pixel glyph width. Font pixels are deliberately
	'' varied because CUSTOM must call the callback, not copy them directly.
	font_pixels[0] = 0
	font_pixels[1] = asc( "A" )
	font_pixels[2] = asc( "A" )
	font_pixels[3] = 3
	for y as integer = 1 to 4
		for x as integer = 0 to 2
			pset font_image, ( x, y ), rgb( x * 61, y * 41, 33 )
		next
	next

	line ( 0, 0 )-( 15, 7 ), background_color( depth ), bf
	result = check_target( 0, depth, font_image, callback_depth, _
		failure_base + 10 )
	if result <> 0 then
		imagedestroy image_target
		imagedestroy font_image
		screen 0
		return result
	end if
	result = check_target( image_target, depth, font_image, callback_depth, _
		failure_base + 20 )
	if result <> 0 then
		imagedestroy image_target
		imagedestroy font_image
		screen 0
		return result
	end if

#ifndef GFX2_REFERENCE
	dim as any ptr gpu_target = fb.Gfx3SurfaceCreate( 16, 8, depth, , _
		background_color( depth ) )
	if gpu_target = 0 then
		imagedestroy image_target
		imagedestroy font_image
		screen 0
		return failure_base + 30
	end if
	result = check_target( gpu_target, depth, font_image, callback_depth, _
		failure_base + 31 )
	if fb.Gfx3SurfaceDestroy( gpu_target ) <> 0 andalso result = 0 then
		result = failure_base + 37
	end if
	if result <> 0 then
		imagedestroy image_target
		imagedestroy font_image
		screen 0
		return result
	end if
#endif

	imagedestroy image_target
	imagedestroy font_image
	screen 0
	return 0
end function

dim as integer result
result = check_depth( 8, 10 )
if result <> 0 then end result
result = check_depth( 16, 60 )
if result <> 0 then end result
result = check_depth( 32, 110 )
if result <> 0 then end result

end 0

'' end of custom-font-custom-blender-depth-smoke.bas
