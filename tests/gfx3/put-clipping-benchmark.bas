''
'' Project: FreeBASIC gfxlib3 benchmarks
'' -------------------------------------
''
'' File: put-clipping-benchmark.bas
''
'' Purpose:
''
''     Measure ordinary transparent sprites intersecting only a screen edge.
''
'' Responsibilities:
''
''     - keep every sprite partially visible so clipping cannot be elided
''     - alternate all edges and corners to avoid a one-axis specialization
''     - draw into a non-visible page so display refresh does not distort timing
''     - end with an ordered POINT that waits for completed rendering
''     - report producer and completion portions independently
''
'' This file intentionally does NOT contain:
''
''     - fully offscreen command-culling measurements
''     - scaling, rotation, page copy, or presentation timing
''     - a hardware-specific pass threshold
''

#include once "fbgfx.bi"

#ifndef PUT_CLIPPING_BENCHMARK_FRAME_COUNT
	const frame_count = 300
#else
	const frame_count = PUT_CLIPPING_BENCHMARK_FRAME_COUNT
#endif

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#elseif defined( GFX3_OPENGL_TEST )
	const backend_flags = fb.GFX_OPENGL
#else
	const backend_flags = 0
#endif

const sprites_per_frame = 1024
const sprite_size = 64
const sprite_total = frame_count * sprites_per_frame

if screenres( 640, 480, 16, 2, backend_flags ) <> 0 then end 1
screenset 1, 0

dim shared as any ptr sprite

sprite = imagecreate( sprite_size, sprite_size, rgb( 255, 0, 255 ) )
if sprite = 0 then end 2

for source_y as integer = 0 to sprite_size - 1
	for source_x as integer = 0 to sprite_size - 1
		if ( ( source_x + source_y ) and 3 ) <> 0 then
			pset sprite, ( source_x, source_y ), _
				rgb( 30 + source_x * 3, 50 + source_y * 3, _
					220 - source_x * 2 )
		end if
	next
next
pset sprite, ( 0, 0 ), rgb( 220, 80, 40 )
pset sprite, ( 63, 63 ), rgb( 60, 190, 245 )

sub draw_clipped_stream( byval count as integer, byval salt as integer )
	for sprite_index as integer = 0 to count - 1
		dim as integer moving_x = ( sprite_index * 37 + salt ) mod 577
		dim as integer moving_y = ( sprite_index * 29 + salt ) mod 417

		select case sprite_index and 7
		case 0
			put ( -63, moving_y ), sprite, trans
		case 1
			put ( 639, moving_y ), sprite, trans
		case 2
			put ( moving_x, -63 ), sprite, trans
		case 3
			put ( moving_x, 479 ), sprite, trans
		case 4
			put ( -63, -63 ), sprite, trans
		case 5
			put ( 639, -63 ), sprite, trans
		case 6
			put ( -63, 479 ), sprite, trans
		case else
			put ( 639, 479 ), sprite, trans
		end select
	next
end sub

draw_clipped_stream( 4096, 17 )
dim as ulong final_pixel = point( 0, 0 )

dim as double started = timer
for frame_index as integer = 1 to frame_count
	draw_clipped_stream( sprites_per_frame, frame_index )
next
dim as double submitted = timer - started
final_pixel = point( 0, 0 )
dim as double elapsed = timer - started

imagedestroy sprite
screen 0
print "put_clipping_benchmark_seconds="; elapsed
print "put_clipping_benchmark_submit_seconds="; submitted
print "put_clipping_benchmark_completion_seconds="; elapsed - submitted
print "put_clipping_benchmark_sprites="; sprite_total
print "put_clipping_benchmark_pixel="; final_pixel

'' end of put-clipping-benchmark.bas
