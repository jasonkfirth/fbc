''
'' Project: FreeBASIC gfxlib3 benchmarks
'' -------------------------------------
''
'' File: sprite-offload-benchmark.bas
''
'' Purpose:
''
''     Measure ordinary sprite completion and the BASIC work that can proceed
''     while asynchronous rendering remains in flight.
''
'' Responsibilities:
''
''     - submit the same patterned RGB565 sprite stream in two measured phases
''     - report producer and ordered completion time for the baseline phase
''     - run deterministic application-side integer work before the second wait
''     - count useful CPU iterations completed during the rendering window
''     - validate the final rendered pixel after each phase
''
'' This file intentionally does NOT contain:
''
''     - page copy or presentation timing
''     - scaled, rotated, or projective sprites
''     - a hardware-specific pass threshold
''

#include once "fbgfx.bi"

#ifndef SPRITE_OFFLOAD_FRAME_COUNT
	const frame_count = 300
#else
	const frame_count = SPRITE_OFFLOAD_FRAME_COUNT
#endif

#ifndef SPRITE_OFFLOAD_CPU_SECONDS
	const cpu_work_target = 0.25
#else
	const cpu_work_target = SPRITE_OFFLOAD_CPU_SECONDS
#endif

#ifdef GFX3_OPENGL_TEST
	const backend_flags = fb.GFX_OPENGL
#elseif defined( GFX3_VULKAN_TEST )
	const backend_flags = fb.GFX_VULKAN
#else
	const backend_flags = 0
#endif

const sprites_per_frame = 1024
const sprite_width = 13
const sprite_height = 16
const readback_x = 0
const readback_y = 32
const expected_pixel = 3784439

dim shared as any ptr sprite_image

sub draw_sprite_stream( byval frames as integer )
	for frame_index as integer = 1 to frames
		for sprite_index as integer = 0 to sprites_per_frame - 1
			put ( ( sprite_index * 17 ) mod 627, _
				32 + ( ( sprite_index * 29 ) mod 432 ) ), _
				sprite_image, trans
		next
	next
end sub

sub run_cpu_work( byval seconds as double, byref iterations as ulongint, _
	byref checksum as ulong )
	dim as double started = timer
	dim as ulong state = &h9E3779B9u

	iterations = 0
	do
		for burst_index as integer = 1 to 4096
			state xor= state shl 13
			state xor= state shr 17
			state xor= state shl 5
		next
		iterations += 4096
	loop while timer - started < seconds
	checksum = state
end sub

if screenres( 640, 480, 16, 2, backend_flags ) <> 0 then end 1
screenset 1, 0

sprite_image = imagecreate( sprite_width, sprite_height, rgb( 255, 0, 255 ) )
if sprite_image = 0 then end 2

for sprite_y as integer = 0 to sprite_height - 1
	for sprite_x as integer = 0 to sprite_width - 1
		if ( ( sprite_x + sprite_y ) mod 4 ) <> 0 then
			pset sprite_image, ( sprite_x, sprite_y ), _
				rgb( 40 + sprite_x * 12, 55 + sprite_y * 10, _
					230 - sprite_x * 7 )
		end if
	next
next
pset sprite_image, ( 0, 0 ), rgb( 60, 190, 245 )

'' Warm source residency, packet storage, renderer state, and timer paths.
draw_sprite_stream( 4 )
dim as ulong pixel_value = culng( point( readback_x, readback_y ) )
if pixel_value <> expected_pixel then end 3

dim as double baseline_started = timer
draw_sprite_stream( frame_count )
dim as double baseline_submitted = timer - baseline_started
pixel_value = culng( point( readback_x, readback_y ) )
dim as double baseline_elapsed = timer - baseline_started
if pixel_value <> expected_pixel then end 4

dim as double overlap_started = timer
draw_sprite_stream( frame_count )
dim as double overlap_submitted = timer - overlap_started
dim as double cpu_started = timer
dim as ulongint cpu_iterations
dim as ulong cpu_checksum
run_cpu_work( cpu_work_target, cpu_iterations, cpu_checksum )
dim as double cpu_elapsed = timer - cpu_started
pixel_value = culng( point( readback_x, readback_y ) )
dim as double overlap_elapsed = timer - overlap_started
if pixel_value <> expected_pixel then end 5
if ( cpu_iterations = 0 ) or ( cpu_checksum = 0 ) then end 6

imagedestroy sprite_image
screen 0
print "sprite_offload_baseline_seconds="; baseline_elapsed
print "sprite_offload_baseline_submit_seconds="; baseline_submitted
print "sprite_offload_baseline_completion_seconds="; _
	baseline_elapsed - baseline_submitted
print "sprite_offload_overlap_seconds="; overlap_elapsed
print "sprite_offload_overlap_submit_seconds="; overlap_submitted
print "sprite_offload_cpu_seconds="; cpu_elapsed
print "sprite_offload_residual_completion_seconds="; _
	overlap_elapsed - overlap_submitted - cpu_elapsed
print "sprite_offload_cpu_iterations="; cpu_iterations
print "sprite_offload_cpu_checksum="; cpu_checksum
print "sprite_offload_pixel="; pixel_value

'' end of sprite-offload-benchmark.bas
