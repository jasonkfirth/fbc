''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: mode-open-benchmark.bas
''
'' Purpose:
''
''     Measure the public SCREENRES path independently from drawing work.
''
'' Responsibilities:
''
''     - request the selected gfxlib3 renderer when compiled for it
''     - include window, context, and initial presentation preparation
''     - report a machine-readable elapsed time and success pixel
''
'' This file intentionally does NOT contain:
''
''     - a drawing throughput workload
''     - a vendor-specific performance threshold
''     - repeated mode changes, which belong to lifecycle stress testing
''
#include once "fbgfx.bi"

#ifdef GFX3_OPENGL_TEST
    const backend_flags = fb.GFX_OPENGL
#elseif defined( GFX3_VULKAN_TEST )
    const backend_flags = fb.GFX_VULKAN
#else
    const backend_flags = 0
#endif

dim as double started
dim as double elapsed
dim as integer pixel_value

started = timer
if screenres( 640, 480, 32, 2, backend_flags ) <> 0 then end 1
elapsed = timer - started

pset ( 0, 0 ), rgb( 12, 34, 56 )
pixel_value = point( 0, 0 )
screen 0

print "mode_open_benchmark_seconds="; elapsed
print "mode_open_benchmark_pixel="; pixel_value
end 0

'' end of mode-open-benchmark.bas
