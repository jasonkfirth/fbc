''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: paint-pattern-depth-smoke.bas
''
'' Purpose:
''
''     Verify that patterned PAINT keeps gfxlib2's target-origin tile rule
''     for indexed, RGB565, and 32-bit logical screen depths.
''
'' Responsibilities:
''
''     - encode an 8 by 8 tile in each depth's native pattern layout
''     - fill through a nonzero VIEW at 8, 16, and 32 bits
''     - check relative and SCREEN-coordinate reads after each fill
''
'' This file intentionally does NOT contain:
''
''     - palette animation or indexed presentation checks
''     - pattern performance measurements
''     - visible-window presentation assumptions
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

function tile_rgb( byval x as integer, byval y as integer ) as ulong
	return rgb( (x and 7) * 29, (y and 7) * 31, ((x xor y) and 7) * 33 )
end function

function tile_index( byval x as integer, byval y as integer ) as ulong
	return culng( ((x and 7) * 19 + ((y and 7) * 7) + 3) and &hFF )
end function

function tile_rgb565( byval x as integer, byval y as integer ) as ulong
	dim as ulong rgb_value = tile_rgb( x, y )
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

function expected_point( byval depth as integer, byval x as integer, _
	byval y as integer ) as ulong

	select case depth
	case 8
		return tile_index( x, y )
	case 16
		return expand_rgb565( tile_rgb565( x, y ) )
	case else
		return tile_rgb( x, y )
	end select
end function

function pattern_pixel( byval depth as integer, byval x as integer, _
	byval y as integer ) as string

	select case depth
	case 8
		return chr( tile_index( x, y ) )
	case 16
		return mkshort( tile_rgb565( x, y ) )
	case else
		return mkl( tile_rgb( x, y ) )
	end select
end function

function check_depth( byval depth as integer, byval failure_base as integer ) _
	as integer

	dim as string pattern = ""
	dim as ulong border_color = rgb( 255, 255, 255 )

	for y as integer = 0 to 7
		for x as integer = 0 to 7
			pattern += pattern_pixel( depth, x, y )
		next
	next
	if screenres( 20, 20, depth, 1, backend_flags ) <> 0 then
		return failure_base
	end if
	view ( 3, 4 )-( 12, 13 )
	line ( 0, 0 )-( 9, 9 ), border_color, b
	paint ( 1, 1 ), pattern, border_color
	if culng( point( 1, 1 ) ) <> expected_point( depth, 4, 5 ) then
		screen 0
		return failure_base + 1
	end if
	if culng( point( 2, 1 ) ) <> expected_point( depth, 5, 5 ) then
		screen 0
		return failure_base + 2
	end if
	view screen
	if culng( point( 4, 5 ) ) <> expected_point( depth, 4, 5 ) then
		screen 0
		return failure_base + 3
	end if
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

'' end of paint-pattern-depth-smoke.bas
