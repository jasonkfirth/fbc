''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: alpha-primitives-smoke.bas
''
'' Purpose:
''
''     Verify gfxlib2-compatible alpha primitive drawing through the public
''     FreeBASIC API and the gfxlib3 GPU-surface extension.
''
'' Responsibilities:
''
''     - verify SCREENCONTROL and SCREEN flags enable alpha primitives
''     - compare screen, CPU-image, and GPU-surface pixels with gfxlib2 math
''     - exercise PSET, LINE, BF, solid PAINT, and CLS alpha drawing
''     - prove disabling the option restores solid primitive writes
''
'' This file intentionally does NOT contain:
''
''     - PUT ALPHA tests, which use a different gfxlib2 blend rule
''     - patterned PAINT tests, because gfxlib2 copies pattern pixels directly
''     - performance measurements
''

#ifndef GFX2_REFERENCE
	#define __FB_GFXLIB3__
	#include once "fbgfx3.bi"
#else
	#include once "fbgfx.bi"
#endif

function alpha_primitive_pixel( byval source as ulong, _
	byval destination as ulong ) as ulong
	dim as ulong source_alpha = source shr 24
	dim as ulong source_red_blue = source and &h00FF00FFu
	dim as ulong source_green = source and &h0000FF00u
	dim as ulong destination_red_blue = destination and &h00FF00FFu
	dim as ulong destination_green = destination and &h0000FF00u

	'' gfxlib2 deliberately divides by 256 through the right shift. The source
	'' alpha byte is retained in the result instead of being composited.
	source_red_blue = ((source_red_blue - destination_red_blue) * _
		source_alpha) shr 8
	source_green = ((source_green - destination_green) * source_alpha) shr 8

	return ((destination_red_blue + source_red_blue) and &h00FF00FFu) or _
		((destination_green + source_green) and &h0000FF00u) or _
		(source and &hFF000000u)
end function

dim requested as string = lcase( command( 1 ) )
dim backend_flags as integer

select case requested
case "", "automatic"
	backend_flags = 0
case "null"
	backend_flags = fb.GFX_NULL
case "opengl"
	backend_flags = fb.GFX_OPENGL
#ifndef GFX2_REFERENCE
case "vulkan"
	backend_flags = fb.GFX_VULKAN
#endif
case else
	end 1
end select

const as ulong destination_color = &hFF204060u
const as ulong source_color = &h8030C080u
dim as ulong expected_color = alpha_primitive_pixel( source_color, _
	destination_color )

if screenres( 64, 64, 32, 2, backend_flags ) <> 0 then end 2

dim driver as string
screencontrol fb.GET_DRIVER_NAME, driver

dim alpha_enabled as integer = true
screencontrol fb.GET_ALPHA_PRIMITIVES, alpha_enabled
if alpha_enabled <> false then end 3

'' A disabled alpha-primitive option makes even a translucent source a solid
'' primitive color, including its original alpha byte.
pset ( 1, 1 ), source_color
if culng( point( 1, 1 ) ) <> source_color then end 4

screen 0

'' GFX_NULL is historically -1, so it cannot be combined with another SCREEN
'' flag. The null-backend run enables the option through SCREENCONTROL after
'' opening; GPU renderer runs verify the mode-time flag directly.
dim alpha_mode_flags as integer = backend_flags
if backend_flags <> fb.GFX_NULL then
	alpha_mode_flags or= fb.GFX_ALPHA_PRIMITIVES
end if
if screenres( 64, 64, 32, 2, alpha_mode_flags ) <> 0 then end 5

alpha_enabled = false
screencontrol fb.GET_ALPHA_PRIMITIVES, alpha_enabled
if backend_flags = fb.GFX_NULL then
	if alpha_enabled <> false then end 6
	alpha_enabled = true
	screencontrol fb.SET_ALPHA_PRIMITIVES, alpha_enabled
else
	if alpha_enabled = false then end 7
end if

pset ( 2, 2 ), destination_color
pset ( 2, 2 ), source_color
dim as ulong actual_color = culng( point( 2, 2 ) )
if actual_color <> expected_color then
	screen 0
	print "screen PSET expected " & hex( expected_color, 8 ) & _
		", got " & hex( actual_color, 8 )
	end 8
end if

line ( 4, 4 )-( 8, 4 ), destination_color
line ( 4, 4 )-( 8, 4 ), source_color
if culng( point( 6, 4 ) ) <> expected_color then end 9

line ( 10, 6 )-( 15, 11 ), destination_color, bf
line ( 10, 6 )-( 15, 11 ), source_color, bf
if culng( point( 12, 8 ) ) <> expected_color then end 10

line ( 18, 6 )-( 25, 13 ), &hFFFFFFFFu, b
line ( 19, 7 )-( 24, 12 ), destination_color, bf
paint ( 21, 9 ), source_color, &hFFFFFFFFu
dim as integer paint_error = err
actual_color = culng( point( 21, 9 ) )
if actual_color <> expected_color then
	screen 0
	print "PAINT expected " & hex( expected_color, 8 ) & _
		", got " & hex( actual_color, 8 ) & ", error " & paint_error
	end 11
end if

dim cpu_image as any ptr = imagecreate( 8, 8, destination_color, 32 )
if cpu_image = 0 then end 12
pset cpu_image, ( 3, 3 ), source_color
if culng( point( 3, 3, cpu_image ) ) <> expected_color then end 13
line cpu_image, ( 1, 5 )-( 6, 5 ), source_color
if culng( point( 4, 5, cpu_image ) ) <> expected_color then end 14
imagedestroy cpu_image

#ifndef GFX2_REFERENCE
	#ifndef GFX3_SKIP_GPU
	dim gpu_surface as any ptr = fb.Gfx3SurfaceCreate( 64, 64, 32, , _
		destination_color )
	if gpu_surface = 0 then end 15
	pset gpu_surface, ( 3, 3 ), source_color
	if culng( point( 3, 3, gpu_surface ) ) <> expected_color then end 16
	const as ulong gpu_destination_color = &hFF102030u
	line gpu_surface, ( 5, 5 )-( 10, 10 ), gpu_destination_color, bf
	actual_color = culng( point( 7, 7, gpu_surface ) )
	if actual_color <> gpu_destination_color then
		fb.Gfx3SurfaceDestroy( gpu_surface )
		screen 0
		print "GPU opaque BF expected " & hex( gpu_destination_color, 8 ) & _
			", got " & hex( actual_color, 8 )
		end 17
	end if
	line gpu_surface, ( 5, 5 )-( 10, 10 ), source_color, bf
	actual_color = culng( point( 7, 7, gpu_surface ) )
	if actual_color <> alpha_primitive_pixel( source_color, _
		gpu_destination_color ) then
		fb.Gfx3SurfaceDestroy( gpu_surface )
		screen 0
		print "GPU BF expected " & hex( alpha_primitive_pixel( source_color, _
			gpu_destination_color ), 8 ) & _
			", got " & hex( actual_color, 8 )
		end 18
	end if
	if fb.Gfx3SurfaceDestroy( gpu_surface ) <> 0 then end 19
	#endif
#endif

'' CLS uses the mode background color as a primitive color in gfxlib2. It
'' therefore blends over the current page when alpha primitives are enabled.
line ( 0, 0 )-( 63, 63 ), destination_color, bf
color , source_color
cls
if culng( point( 32, 32 ) ) <> expected_color then end 19

alpha_enabled = false
screencontrol fb.SET_ALPHA_PRIMITIVES, alpha_enabled
pset ( 30, 30 ), destination_color
pset ( 30, 30 ), source_color
actual_color = culng( point( 30, 30 ) )
if actual_color <> source_color then
	screen 0
	print "disabled alpha PSET expected " & hex( source_color, 8 ) & _
		", got " & hex( actual_color, 8 )
	end 20
end if

'' Re-enabling is meaningful after a mode was initialized with alpha support.
alpha_enabled = true
screencontrol fb.SET_ALPHA_PRIMITIVES, alpha_enabled
pset ( 4, 4 ), destination_color
pset ( 4, 4 ), source_color
if culng( point( 4, 4 ) ) <> expected_color then end 21

screen 0
print "gfxlib alpha primitives: " & driver
end 0

'' end of alpha-primitives-smoke.bas
