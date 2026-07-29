''
'' Project: FreeBASIC gfxlib3 benchmarks
'' -------------------------------------
''
'' File: mixed-sprite-benchmark.bas
''
'' Purpose:
''
''     Compare the completed throughput of the alternating-image sprite stream
''     used by games against gfxlib2 and each gfxlib3 desktop backend.
''
'' Responsibilities:
''
''     - create 32 stable, non-uniform RGB565 CPU images
''     - alternate their sources across a scattered transparent PUT workload
''     - include cache and shader warmup outside the measured interval
''     - force ordered completion and verify the final source pixel
''
'' This file intentionally does NOT contain:
''
''     - scaled, rotated, or projective sprites
''     - per-frame presentation or frame limiting
''     - a hardware-specific pass threshold
''

#include once "fbgfx.bi"

#ifndef MIXED_SPRITE_FRAME_COUNT
	const frame_count = 60
#else
	const frame_count = MIXED_SPRITE_FRAME_COUNT
#endif

const image_count = 32
const sprites_per_frame = 1024
const sprite_width = 32
const sprite_height = 32

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#elseif defined( GFX3_OPENGL_TEST )
	const backend_flags = fb.GFX_OPENGL
#else
	const backend_flags = 0
#endif

dim as any ptr sprite( 0 to image_count - 1 )
dim as double started
dim as double elapsed
dim as integer expected_pixel
dim as integer final_pixel

if screenres( 640, 480, 16, 1, backend_flags ) <> 0 then end 1

for image_index as integer = 0 to image_count - 1
	sprite( image_index ) = imagecreate( sprite_width, sprite_height, _
		rgb( 255, 0, 255 ), 16 )
	if sprite( image_index ) = 0 then end 2
	for y as integer = 0 to sprite_height - 1
		for x as integer = 0 to sprite_width - 1
			if ( ( x + y + image_index ) and 3 ) <> 0 then
				pset sprite( image_index ), ( x, y ), _
					rgb( ( 31 + image_index * 7 + x * 3 ) and 255, _
						( 73 + image_index * 11 + y * 5 ) and 255, _
						( 191 + image_index * 13 + x + y ) and 255 )
			end if
		next
	next
	pset sprite( image_index ), ( 0, 0 ), _
		rgb( 32 + image_index * 5, 210 - image_index * 3, _
			40 + image_index * 6 )
next
expected_pixel = point( 0, 0, sprite( image_count - 1 ) )

'' Warm all image residency entries and the ordered mixed-source batch path.
for sprite_index as integer = 0 to sprites_per_frame - 1
	dim as integer image_index = sprite_index mod image_count

	put ( ( sprite_index * 17 ) mod ( 640 - sprite_width ), _
		( sprite_index * 29 ) mod ( 480 - sprite_height ) ), _
		sprite( image_index ), trans
next
put ( 0, 0 ), sprite( image_count - 1 ), trans
final_pixel = point( 0, 0 )
if final_pixel <> expected_pixel then end 3

started = timer
for frame_index as integer = 1 to frame_count
	for sprite_index as integer = 0 to sprites_per_frame - 1
		dim as integer image_index = _
			( sprite_index + frame_index ) mod image_count

		put ( ( sprite_index * 17 ) mod ( 640 - sprite_width ), _
			( sprite_index * 29 ) mod ( 480 - sprite_height ) ), _
			sprite( image_index ), trans
	next
next
put ( 0, 0 ), sprite( image_count - 1 ), trans
final_pixel = point( 0, 0 )
elapsed = timer - started

for image_index as integer = 0 to image_count - 1
	imagedestroy sprite( image_index )
next
screen 0

if final_pixel <> expected_pixel then end 4
print "mixed_sprite_benchmark_seconds="; elapsed
print "mixed_sprite_benchmark_sprites="; _
	frame_count * sprites_per_frame + 1
print "mixed_sprite_benchmark_sprites_per_second="; _
	( frame_count * sprites_per_frame + 1 ) / elapsed
print "mixed_sprite_benchmark_pixel="; final_pixel

'' end of mixed-sprite-benchmark.bas
