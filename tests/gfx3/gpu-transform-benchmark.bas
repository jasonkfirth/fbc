''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: gpu-transform-benchmark.bas
''
'' Purpose:
''
''     Measure completed GPU-resident scaling, rotation, and Mode 7 work.
''
'' Responsibilities:
''
''     - keep source and destination pixels in graphics memory
''     - submit enough transformed pixels to amortize timer resolution
''     - include an ordered readback in each completed-work measurement
''     - print stable machine-readable timing records
''
'' This file intentionally does NOT contain:
''
''     - a gfxlib2 comparison with a different transform algorithm
''     - vendor-specific pass or fail thresholds
''     - timing of bitmap decoding or initial upload
''

#include once "fbgfx3.bi"

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#elseif defined( GFX3_OPENGL_TEST )
	const backend_flags = fb.GFX_OPENGL
#else
	const backend_flags = fb.GFX_NULL
#endif

const source_width = 128
const source_height = 128
const target_width = 640
const target_height = 480
#ifdef __FB_ANDROID__
	'' Older mobile drivers have a much shorter watchdog interval. The pixel
	'' work is identical; only the sample count is reduced to keep the device
	'' responsive while still amortizing its timer resolution.
	const scale_iterations = 150
	const rotate_iterations = 75
	const mode7_iterations = 20
#else
	const scale_iterations = 1500
	const rotate_iterations = 750
	const mode7_iterations = 200
#endif

dim source_pixels( 0 to source_width * source_height - 1 ) as ulong
dim source as any ptr
dim target as any ptr
dim driver_name as string
dim final_pixel as ulong
dim started as double
dim scale_seconds as double
dim rotate_seconds as double
dim mode7_seconds as double
dim index as long
dim x as long
dim y as long

for y = 0 to source_height - 1
	for x = 0 to source_width - 1
		source_pixels( y * source_width + x ) = _
			rgba( ( x * 2 ) and 255, ( y * 2 ) and 255, _
			( x xor y ) and 255, 255 )
	next
next

if screenres( target_width, target_height, 32, 1, backend_flags ) <> 0 then _
	end 1
screencontrol fb.GET_DRIVER_NAME, driver_name
source = fb.Gfx3SurfaceCreate( source_width, source_height, 32, _
	fb.GFX3_SURFACE_ASSET )
target = fb.Gfx3SurfaceCreate( target_width, target_height, 32 )
if source = 0 orelse target = 0 then end 2
if fb.Gfx3SurfaceUpload( source, 0, 0, source_width, source_height, _
	source_width * 4, @source_pixels( 0 ) ) <> 0 then end 3

started = timer
for index = 0 to scale_iterations - 1
	x = ( index * 37 ) mod ( target_width - 192 )
	y = ( index * 23 ) mod ( target_height - 160 )
	if fb.Gfx3SurfaceBlitScaled( target, source, 0, 0, _
		source_width, source_height, x, y, 192, 160 ) <> 0 then end 4
next
if fb.Gfx3SurfaceDownload( target, 320, 240, 1, 1, 4, _
	@final_pixel ) <> 0 then end 5
scale_seconds = timer - started

started = timer
for index = 0 to rotate_iterations - 1
	x = 96 + ( ( index * 53 ) mod ( target_width - 192 ) )
	y = 96 + ( ( index * 29 ) mod ( target_height - 192 ) )
	if fb.Gfx3SurfaceBlitRotated( target, source, 0, 0, _
		source_width, source_height, x, y, index * 3.7 ) <> 0 then end 6
next
if fb.Gfx3SurfaceDownload( target, 320, 240, 1, 1, 4, _
	@final_pixel ) <> 0 then end 7
rotate_seconds = timer - started

started = timer
for index = 0 to mode7_iterations - 1
	if fb.Gfx3SurfaceMode7( target, source, 0, 0, _
		source_width, source_height, 0, 0, target_width, target_height, _
		64.0 + index * 0.125, 64.0, 48.0, index * 0.7, _
		180.0, 220.0 ) <> 0 then end 8
next
if fb.Gfx3SurfaceDownload( target, 320, 300, 1, 1, 4, _
	@final_pixel ) <> 0 then end 9
mode7_seconds = timer - started

if fb.Gfx3SurfaceDestroy( target ) <> 0 then end 10
if fb.Gfx3SurfaceDestroy( source ) <> 0 then end 11
screen 0
print "GFX3_TRANSFORM_BACKEND="; driver_name
print "GFX3_TRANSFORM_SCALE_SECONDS="; scale_seconds
print "GFX3_TRANSFORM_ROTATE_SECONDS="; rotate_seconds
print "GFX3_TRANSFORM_MODE7_SECONDS="; mode7_seconds
print "GFX3_TRANSFORM_FINAL_PIXEL="; final_pixel
end 0

'' end of gpu-transform-benchmark.bas
