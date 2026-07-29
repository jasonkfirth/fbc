''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: trans-batch-depth-smoke.bas
''
'' Purpose:
''
''     Verify packed transparent sprite rendering at every native screen depth.
''
'' Responsibilities:
''
''     - exercise indexed, RGB565, and 32-bit transparent keys
''     - force several adjacent PUT TRANS operations into one packed command
''     - verify opaque writes and transparent destination preservation
''
'' This file intentionally does NOT contain:
''
''     - scaling or rotation
''     - source-to-destination depth conversion
''     - custom PUT callbacks
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#elseif defined( GFX3_OPENGL_TEST )
	const backend_flags = fb.GFX_OPENGL
#else
	const backend_flags = 0
#endif

function pack_rgb565( byval source_value as ulong ) as ulong
	return ( ( source_value and &hF8 ) shl 8 ) or _
		( ( source_value and &hFC00 ) shr 5 ) or _
		( ( source_value and &hF80000 ) shr 19 )
end function

function expand_rgb565( byval source_value as ulong ) as ulong
	dim as ulong red_value = ( source_value shr 11 ) and 31
	dim as ulong green_value = ( source_value shr 5 ) and 63
	dim as ulong blue_value = source_value and 31

	red_value = ( red_value shl 3 ) or ( red_value shr 2 )
	green_value = ( green_value shl 2 ) or ( green_value shr 4 )
	blue_value = ( blue_value shl 3 ) or ( blue_value shr 2 )
	return red_value or ( green_value shl 8 ) or ( blue_value shl 16 )
end function

function expected_color( byval depth as integer, byval source_value as ulong ) as ulong
	select case depth
	case 8
		return source_value and &hFF
	case 16
		return expand_rgb565( pack_rgb565( source_value ) )
	case else
		return source_value
	end select
end function

function check_depth( byval depth as integer, byval failure_base as integer ) _
	as integer

	dim as ulong transparent_color
	dim as ulong source_color
	dim as ulong background_color
	dim as any ptr source_image

	select case depth
	case 8
		transparent_color = 0
		source_color = 3
		background_color = 5
	case else
		transparent_color = rgb( 255, 0, 255 )
		source_color = rgb( 40, 170, 230 )
		background_color = rgb( 15, 80, 120 )
	end select
	if screenres( 16, 12, depth, 1, backend_flags ) <> 0 then
		return failure_base
	end if
	source_image = imagecreate( 2, 2, transparent_color, depth )
	if source_image = 0 then
		screen 0
		return failure_base + 1
	end if
	pset source_image, ( 0, 0 ), source_color
	line ( 0, 0 )-( 15, 11 ), background_color, bf

	'' Four adjacent calls ensure the renderer uses its packed BLITS route.
	put ( 0, 0 ), source_image, trans
	put ( 4, 0 ), source_image, trans
	put ( 0, 4 ), source_image, trans
	put ( 4, 4 ), source_image, trans
	if culng( point( 4, 4 ) ) <> expected_color( depth, source_color ) then
		imagedestroy source_image
		screen 0
		return failure_base + 2
	end if
	if culng( point( 5, 5 ) ) <> expected_color( depth, background_color ) then
		imagedestroy source_image
		screen 0
		return failure_base + 3
	end if

	imagedestroy source_image
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

'' end of trans-batch-depth-smoke.bas
