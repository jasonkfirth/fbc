''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: alpha-points-extension-smoke.bas
''
'' Purpose:
''
''     Verify the GPU point-array extension used for generated alpha masks.
''
'' Responsibilities:
''
''     - blend public Gfx3Point records on the current screen page
''     - preserve duplicate-coordinate alpha order inside one point packet
''     - apply the active WINDOW mapping to screen point arrays
''     - preserve blending order across adjacent public point packets
''     - route the same record layout to an opaque GPU surface
''     - compare the result with gfxlib2 alpha-primitive arithmetic
''
'' This file intentionally does NOT contain:
''
''     - a transfer or throughput benchmark
''     - a gfxlib2 build, because this is an opt-in gfxlib3 extension
''

#include once "fbgfx3.bi"

function alpha_primitive_pixel( byval source as ulong, _
	byval destination as ulong ) as ulong
	dim as ulong alpha_value = source shr 24
	dim as ulong source_red_blue = source and &h00FF00FFu
	dim as ulong source_green = source and &h0000FF00u
	dim as ulong destination_red_blue = destination and &h00FF00FFu
	dim as ulong destination_green = destination and &h0000FF00u

	source_red_blue = ((source_red_blue - destination_red_blue) * _
		alpha_value) shr 8
	source_green = ((source_green - destination_green) * alpha_value) shr 8
	return ((destination_red_blue + source_red_blue) and &h00FF00FFu) or _
		((destination_green + source_green) and &h0000FF00u) or _
		(source and &hFF000000u)
end function

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#elseif defined( GFX3_OPENGL_TEST )
	const backend_flags = fb.GFX_OPENGL
#else
	const backend_flags = fb.GFX_NULL
#endif

const destination_color = &hFF204060u
const source_rgb = &h0030C080u
const source_alpha = 128u
const source_color = source_rgb or (source_alpha shl 24)
dim as ulong expected_color = alpha_primitive_pixel( source_color, _
	destination_color )

if screenres( 32, 32, 32, 1, backend_flags ) <> 0 then end 1
line (0, 0)-(31, 31), destination_color, bf

dim as fb.Gfx3Point points(0 to 1)
points(0).x = 4
points(0).y = 5
points(0).color = source_rgb
points(0).alpha = source_alpha
points(1).x = 7
points(1).y = 9
points(1).color = &h00D04020u
points(1).alpha = 255

if fb.Gfx3DrawPoints( 0, @points(0), 2 ) <> 0 then end 2
if culng( point( 4, 5 ) ) <> expected_color then end 3
if culng( point( 7, 9 ) ) <> &hFFD04020u then end 4

'' Duplicate alpha points are assigned to successive GPU layers. Their exact
'' gfxlib2 transfer order must survive the parallel point path.
const duplicate_rgb = &h00E04018u
const duplicate_alpha = 96u
const duplicate_color = duplicate_rgb or (duplicate_alpha shl 24)
dim as ulong duplicate_expected = alpha_primitive_pixel( source_color, _
	destination_color )
duplicate_expected = alpha_primitive_pixel( duplicate_color, _
	duplicate_expected )
line (10, 10)-(10, 10), destination_color
points(0).x = 10
points(0).y = 10
points(0).color = source_rgb
points(0).alpha = source_alpha
points(1).x = 10
points(1).y = 10
points(1).color = duplicate_rgb
points(1).alpha = duplicate_alpha
if fb.Gfx3DrawPoints( 0, @points(0), 2 ) <> 0 then end 12
if culng( point( 10, 10 ) ) <> duplicate_expected then end 13

'' The screen form follows PSET's logical coordinate mapping. At this scale,
'' logical x values 10 and 11 address the same physical pixel and therefore
'' exercise both mapping and ordered layers.
window screen (0, 0)-(63, 63)
line (10, 14)-(10, 14), destination_color
points(0).x = 10
points(0).y = 14
points(1).x = 11
points(1).y = 14
if fb.Gfx3DrawPoints( 0, @points(0), 2 ) <> 0 then end 14
if culng( point( 10, 14 ) ) <> duplicate_expected then
	print "WINDOW duplicate expected " & hex( duplicate_expected, 8 ) & _
		", got " & hex( culng( point( 10, 14 ) ), 8 )
	end 15
end if
window

'' Adjacent alpha packets may share one backend submission, but the second
'' packet must see and blend against the first packet's result.
const second_source_rgb = duplicate_rgb
const second_source_alpha = duplicate_alpha
const second_source_color = second_source_rgb or (second_source_alpha shl 24)
dim as ulong ordered_color = alpha_primitive_pixel( source_color, _
	destination_color )
ordered_color = alpha_primitive_pixel( second_source_color, ordered_color )
line (12, 12)-(12, 12), destination_color
points(0).x = 12
points(0).y = 12
points(0).color = source_rgb
points(0).alpha = source_alpha
if fb.Gfx3DrawPoints( 0, @points(0), 1 ) <> 0 then end 5
points(0).color = second_source_rgb
points(0).alpha = second_source_alpha
if fb.Gfx3DrawPoints( 0, @points(0), 1 ) <> 0 then end 6
if culng( point( 12, 12 ) ) <> ordered_color then end 7

dim as any ptr surface = fb.Gfx3SurfaceCreate( 16, 16, 32, , _
	destination_color )
if surface = 0 then end 8
points(0).x = 2
points(0).y = 3
points(0).color = source_rgb
points(0).alpha = source_alpha
if fb.Gfx3DrawPoints( surface, @points(0), 1 ) <> 0 then end 9
if culng( point( 2, 3, surface ) ) <> expected_color then end 10
if fb.Gfx3SurfaceDestroy( surface ) <> 0 then end 11

screen 0
end 0

'' end of alpha-points-extension-smoke.bas
