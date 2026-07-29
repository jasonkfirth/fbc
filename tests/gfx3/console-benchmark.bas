''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: console-benchmark.bas
''
'' Purpose:
''
''     Measure the graphical-console commands independently from DRAW STRING.
''
'' Responsibilities:
''
''     - exercise WIDTH, LOCATE, PRINT, colour changes, and scroll behaviour
''     - force one ordered pixel readback after the glyph workload
''     - run unchanged source against gfxlib2 and selected gfxlib3 backends
''
'' This file intentionally does NOT contain:
''
''     - keyboard or LINE INPUT timing
''     - application string-construction timing
''     - a vendor-specific performance threshold
''
#include once "fbgfx.bi"

#ifdef GFX3_OPENGL_TEST
    const backend_flags = fb.GFX_OPENGL
#elseif defined( GFX3_VULKAN_TEST )
    const backend_flags = fb.GFX_VULKAN
#else
    const backend_flags = 0
#endif

#ifdef __FB_ANDROID__
    const print_count = 512
#else
    const print_count = 4000
#endif

dim as double started
dim as double elapsed
dim as integer ordered_pixel

if screenres( 640, 480, 32, 1, backend_flags ) <> 0 then end 1
width 80, 30
color rgb( 220, 240, 255 ), rgb( 0, 0, 40 )

started = timer
for index as integer = 0 to print_count - 1
    locate ( index mod 30 ) + 1, ( ( index * 7 ) mod 80 ) + 1
    color rgb( index and 255, ( index * 3 ) and 255, ( index * 11 ) and 255 )
    print "gfxlib3 console "; index
next
ordered_pixel = point( 0, 0 )
elapsed = timer - started

screen 0
print "console_benchmark_seconds="; elapsed
print "console_benchmark_microseconds="; clng( elapsed * 1000000.0 )
print "console_benchmark_prints="; print_count
print "console_benchmark_pixel="; ordered_pixel
end 0

'' end of console-benchmark.bas
