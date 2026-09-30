''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: screenptr-depth-smoke.bas
''
'' Purpose:
''
''     Verify raw SCREENPTR uploads for indexed, RGB565, and 32-bit pages.
''
'' Responsibilities:
''
''     - write one pixel through SCREENPTR at each supported storage width
''     - require SCREENUNLOCK to publish the written pixel to POINT
''     - verify palette-index and true-color interpretation
''
'' This file intentionally does NOT contain:
''
''     - GPU-private handles or texture-format assumptions
''     - page-flipping or multi-page synchronization checks
''
''
#include once "fbgfx.bi"

#ifdef GFX3_OPENGL_TEST
	const backend_flags = 0
#elseif defined( GFX3_VULKAN_TEST )
	const backend_flags = fb.GFX_VULKAN
#else
	const backend_flags = fb.GFX_NULL
#endif

const screen_width = 32
const screen_height = 16
const pixel_x = 5
const pixel_y = 3

dim as integer depths(0 to 2) = { 8, 16, 32 }
dim as any ptr pixels
dim as integer depth
dim as integer reported_width
dim as integer reported_height
dim as integer reported_depth
dim as integer bytes_per_pixel
dim as integer pitch
dim as integer pixel_offset
dim as integer expected_pixel
dim as integer actual_pixel

for depth_index as integer = 0 to ubound( depths )
	depth = depths( depth_index )
	if screenres( screen_width, screen_height, depth, 1, backend_flags ) <> 0 then
		screen 0
		print "GFX3_SCREENPTR_DEPTH_FAIL screenres "; depth
		end 1
	end if

	screeninfo reported_width, reported_height, reported_depth, _
		bytes_per_pixel, pitch
	if reported_width <> screen_width or reported_height <> screen_height or _
		reported_depth <> depth or bytes_per_pixel <> depth \ 8 or _
		pitch < screen_width * bytes_per_pixel or pitch <= 0 or _
		pitch > &h7FFFFFFF \ screen_height or pixel_x < 0 or _
		pixel_x >= screen_width or pixel_y < 0 or pixel_y >= screen_height then
		screen 0
		print "GFX3_SCREENPTR_DEPTH_FAIL screeninfo "; depth
		end 2
	end if

	if depth = 8 then
		palette 5, 255, 0, 0
		expected_pixel = 5
	elseif depth = 16 then
		expected_pixel = rgb( 255, 0, 0 ) and &h00FFFFFF
	else
		expected_pixel = rgb( 17, 34, 51 )
	end if

	screenlock
	pixels = screenptr
	if pixels <> 0 then
		pixel_offset = pixel_y * pitch + pixel_x * bytes_per_pixel
		select case depth
		case 8
			'' FBL-LINTER: DISABLE-NEXT-LINE FBL-PTR-019 REASON: SCREENINFO validated the page dimensions, pitch, and pixel offset.
			*cast( ubyte ptr, pixels + pixel_offset ) = 5
		case 16
			'' FBL-LINTER: DISABLE-NEXT-LINE FBL-PTR-019 REASON: SCREENINFO validated the page dimensions, pitch, and pixel offset.
			*cast( ushort ptr, pixels + pixel_offset ) = &hF800
		case 32
			'' FBL-LINTER: DISABLE-NEXT-LINE FBL-PTR-019 REASON: SCREENINFO validated the page dimensions, pitch, and pixel offset.
			*cast( uinteger ptr, pixels + pixel_offset ) = rgb( 17, 34, 51 )
		end select
	end if
	screenunlock
	if pixels = 0 then
		screen 0
		print "GFX3_SCREENPTR_DEPTH_FAIL screenptr "; depth
		end 3
	end if
	screensync

	actual_pixel = point( pixel_x, pixel_y )
	if actual_pixel <> expected_pixel then
		screen 0
		print "GFX3_SCREENPTR_DEPTH_FAIL point depth="; depth; _
			" expected="; expected_pixel; " actual="; actual_pixel
		end 4
	end if
	screen 0
next

print "GFX3_SCREENPTR_DEPTH_PASS"
end 0

'' end of screenptr-depth-smoke.bas
