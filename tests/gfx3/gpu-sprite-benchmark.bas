''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: gpu-sprite-benchmark.bas
''
'' Purpose:
''
''     Measure ordinary unscaled transparent sprites after their pixels have
''     been uploaded once and retained in graphics memory.
''
'' Responsibilities:
''
''     - submit 30,720 standard PUT TRANS calls with a GPU-surface source
''     - measure the extension's direct GPU-surface-to-surface blit path
''     - force completion by reading a pixel covered by every sprite stream
''     - use the same patterned RGB565 sprite as oma-sprite-benchmark.bas
''
'' This file intentionally does NOT contain:
''
''     - scaled, rotated, or projective drawing
''     - repeated CPU uploads inside either timed region
''     - a gfxlib2 result, because gfxlib2 has no GPU-surface object
''

#define __FB_GFXLIB3__
#include once "fbgfx3.bi"

const frame_count = 30
const sprites_per_frame = 1024
const sprite_width = 13
const sprite_height = 16
const sprite_pitch = sprite_width * sizeof( ushort )
const readback_x = 0
const readback_y = 32

#ifdef GFX3_OPENGL_TEST
	const backend_flags = fb.GFX_OPENGL
#elseif defined( GFX3_VULKAN_TEST )
	const backend_flags = fb.GFX_VULKAN
#else
	const backend_flags = 0
#endif

function native_rgb565( byval red_value as ulong, _
	byval green_value as ulong, byval blue_value as ulong ) as ushort
	return cushort( ( ( red_value and &hF8u ) shl 8 ) or _
		( ( green_value and &hFCu ) shl 3 ) or ( blue_value shr 3 ) )
end function

dim as ushort sprite_pixels( 0 to sprite_width * sprite_height - 1 )
dim as any ptr source_surface
dim as any ptr destination_surface
dim as double started
dim as double public_put_seconds
dim as double direct_blit_seconds
dim as integer pixel_value

if screenres( 640, 480, 16, 1, backend_flags ) <> 0 then end 1

for sprite_y as integer = 0 to sprite_height - 1
	for sprite_x as integer = 0 to sprite_width - 1
		dim as integer pixel_index = sprite_y * sprite_width + sprite_x

		if ( ( sprite_x + sprite_y ) mod 4 ) = 0 then
			sprite_pixels( pixel_index ) = &hF81Fu
		else
			sprite_pixels( pixel_index ) = native_rgb565( _
				40 + sprite_x * 12, 55 + sprite_y * 10, _
				230 - sprite_x * 7 )
		end if
	next
next
sprite_pixels( 0 ) = native_rgb565( 60, 190, 245 )

source_surface = fb.Gfx3SurfaceCreate( sprite_width, sprite_height, 16, _
	fb.GFX3_SURFACE_SAMPLED or fb.GFX3_SURFACE_TRANSFER_DESTINATION )
destination_surface = fb.Gfx3SurfaceCreate( 640, 480, 16, _
	fb.GFX3_SURFACE_ALL )
if source_surface = 0 orelse destination_surface = 0 then end 2
if fb.Gfx3SurfaceUpload( source_surface, 0, 0, sprite_width, _
	sprite_height, sprite_pitch, @sprite_pixels( 0 ) ) <> 0 then end 3

'' Warm resource lookup, batching, shaders, and the final ordered readback.
for sprite_index as integer = 0 to sprites_per_frame - 1
	put ( ( sprite_index * 17 ) mod 627, _
		32 + ( ( sprite_index * 29 ) mod 432 ) ), source_surface, trans
next
pixel_value = point( readback_x, readback_y )

started = timer
for frame_index as integer = 1 to frame_count
	for sprite_index as integer = 0 to sprites_per_frame - 1
		put ( ( sprite_index * 17 ) mod 627, _
			32 + ( ( sprite_index * 29 ) mod 432 ) ), source_surface, trans
	next
next
pixel_value = point( readback_x, readback_y )
public_put_seconds = timer - started

if fb.Gfx3SurfaceClear( destination_surface, 0 ) <> 0 then end 4
for sprite_index as integer = 0 to sprites_per_frame - 1
	if fb.Gfx3SurfaceBlit( destination_surface, source_surface, 0, 0, _
		sprite_width, sprite_height, ( sprite_index * 17 ) mod 627, _
		32 + ( ( sprite_index * 29 ) mod 432 ), fb.GFX3_PUT_TRANS ) <> 0 _
		then end 5
next
pixel_value = point( readback_x, readback_y, destination_surface )

started = timer
for frame_index as integer = 1 to frame_count
	for sprite_index as integer = 0 to sprites_per_frame - 1
		if fb.Gfx3SurfaceBlit( destination_surface, source_surface, 0, 0, _
			sprite_width, sprite_height, ( sprite_index * 17 ) mod 627, _
			32 + ( ( sprite_index * 29 ) mod 432 ), _
			fb.GFX3_PUT_TRANS ) <> 0 then end 6
	next
next
pixel_value = point( readback_x, readback_y, destination_surface )
direct_blit_seconds = timer - started

if fb.Gfx3SurfaceDestroy( destination_surface ) <> 0 then end 7
if fb.Gfx3SurfaceDestroy( source_surface ) <> 0 then end 8
screen 0

print "gpu_sprite_public_put_seconds="; public_put_seconds
print "gpu_sprite_direct_blit_seconds="; direct_blit_seconds
print "gpu_sprite_benchmark_sprites="; frame_count * sprites_per_frame
print "gpu_sprite_benchmark_pixel="; pixel_value

'' end of gpu-sprite-benchmark.bas
