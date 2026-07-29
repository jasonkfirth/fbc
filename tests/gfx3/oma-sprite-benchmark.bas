''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: oma-sprite-benchmark.bas
''
'' Purpose:
''
''     Measure the repeated transparent sprite PUT workload used by OMA games.
''
'' Responsibilities:
''
''     - draw a stable, non-uniform 16-bit CPU image 1,024 times per frame
''     - copy the completed work page to the visible page
''     - force one ordered read from a pixel changed by the sprite stream
''     - separate producer time from the final renderer drain where possible
''     - permit isolated copy/readback measurements through build-time flags
''
'' This file intentionally does NOT contain:
''
''     - game-specific behavior
''     - device-vendor assumptions
''     - a pass or fail performance threshold
''
'' Build-time measurement controls:
''
''     OMA_BENCHMARK_SKIP_COPY
''         Omit SCREENCOPY to isolate transparent PUT and ordered POINT cost.
''
''     OMA_BENCHMARK_FINAL_READBACK
''         Retain the normal drawing and page-copy sequence, but move the
''         completion readback to the end of the complete workload. The read
''         coordinate is covered by a sprite, so POINT cannot use an unchanged
''         cache entry. This is throughput, not per-frame latency.
''
''     OMA_BENCHMARK_FRAME_COUNT
''         Override the normal 30-frame OMA sample with a longer run when a
''         statistically steadier throughput measurement is required.
''
#include once "fbgfx.bi"

#ifndef OMA_BENCHMARK_FRAME_COUNT
const frame_count = 30
#else
const frame_count = OMA_BENCHMARK_FRAME_COUNT
#endif
const sprites_per_frame = 1024
const warm_sprite_count = 4096
const sprite_width = 13
const sprite_height = 16
const readback_x = 0
const readback_y = 32

#ifdef GFX3_OPENGL_TEST
    const backend_flags = fb.GFX_OPENGL
#elseif defined( GFX3_VULKAN_TEST )
    const backend_flags = fb.GFX_VULKAN
#else
    const backend_flags = 0
#endif

dim as any ptr sprite_image
dim as double started
dim as double elapsed
dim as double submitted_elapsed
dim as double completion_elapsed
dim as integer frame_index
dim as integer sprite_index
dim as integer pixel_value

if screenres( 640, 480, 16, 2, backend_flags ) <> 0 then end 1
screenset 1, 0

sprite_image = imagecreate( sprite_width, sprite_height, rgb( 255, 0, 255 ) )
if sprite_image = 0 then end 2

'' A real sprite exercises texture sampling and transparent-key rejection. A
'' uniform IMAGECREATE would be recognized as a filled rectangle by gfxlib3 and
'' would not measure the ordinary sprite shader path requested by this fixture.
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

'' Warm the image cache, renderer thread, and presentation path.
for sprite_index = 0 to warm_sprite_count - 1
    put ( ( sprite_index * 17 ) mod 627, 32 + ( ( sprite_index * 29 ) mod 432 ) ), sprite_image, trans
next
#ifndef OMA_BENCHMARK_SKIP_COPY
screencopy 1, 0
#endif
pixel_value = point( readback_x, readback_y )

started = timer
for frame_index = 1 to frame_count
    for sprite_index = 0 to sprites_per_frame - 1
        put ( ( sprite_index * 17 ) mod 627, 32 + ( ( sprite_index * 29 ) mod 432 ) ), sprite_image, trans
    next
    #ifndef OMA_BENCHMARK_SKIP_COPY
    screencopy 1, 0
    #endif
    #ifndef OMA_BENCHMARK_FINAL_READBACK
    pixel_value = point( readback_x, readback_y )
    #endif
next
submitted_elapsed = timer - started
#ifdef OMA_BENCHMARK_FINAL_READBACK
pixel_value = point( readback_x, readback_y )
#endif
elapsed = timer - started
completion_elapsed = elapsed - submitted_elapsed

imagedestroy sprite_image
screen 0
print "oma_sprite_benchmark_seconds="; elapsed
print "oma_sprite_benchmark_submit_seconds="; submitted_elapsed
print "oma_sprite_benchmark_completion_seconds="; completion_elapsed
print "oma_sprite_benchmark_frames="; frame_count
print "oma_sprite_benchmark_pixel="; pixel_value

'' end of oma-sprite-benchmark.bas
