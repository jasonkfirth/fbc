''
'' Project: FreeBASIC gfxlib3 benchmarks
'' -------------------------------------
''
'' File: palette-family-benchmark.bas
''
'' Purpose:
''
''     Measure every public PALETTE command family independently.
''
'' Responsibilities:
''
''     - time single-entry PALETTE writes and their ordered GPU publication
''     - time single-entry PALETTE GET queries from compatibility state
''     - time complete PALETTE USING updates and PALETTE GET USING reads
''     - preserve one source file for gfxlib2, OpenGL, Vulkan, and GLES
''
'' This file intentionally does NOT contain:
''
''     - true-colour drawing throughput
''     - private renderer palette entry points
''     - a hardware-specific pass threshold
''

#if defined( __FB_ANDROID__ ) and not defined( GFX2_REFERENCE )
    #ifndef __FB_GFXLIB3__
        #define __FB_GFXLIB3__
    #endif
#endif

#include once "fbgfx.bi"

#ifdef GFX3_OPENGL_TEST
    const backend_flags = fb.GFX_OPENGL
#elseif defined( GFX3_VULKAN_TEST )
    const backend_flags = fb.GFX_VULKAN
#else
    const backend_flags = 0
#endif

#ifdef __FB_ANDROID__
    const entry_iterations = 256
    const table_iterations = 32
#else
    const entry_iterations = 2048
    const table_iterations = 256
#endif

dim as long palette_data( 0 to 255 )
dim as long red_value
dim as long green_value
dim as long blue_value
dim as ulongint checksum
dim as double started
dim as double set_seconds
dim as double get_seconds
dim as double using_seconds
dim as double get_using_seconds
dim as integer final_pixel

for index as integer = 0 to 255
    palette_data( index ) = ( index and 63 ) or _
        ( ( ( index * 3 ) and 63 ) shl 8 ) or _
        ( ( ( index * 7 ) and 63 ) shl 16 )
next

if screenres( 320, 240, 8, 1, backend_flags ) <> 0 then end 1

started = timer
for index as integer = 0 to entry_iterations - 1
    palette index and 255, index and 255, ( index * 3 ) and 255, _
        ( index * 7 ) and 255
next
screensync
set_seconds = timer - started

started = timer
for index as integer = 0 to entry_iterations - 1
    palette get index and 255, red_value, green_value, blue_value
    checksum += culng( red_value + green_value + blue_value )
next
get_seconds = timer - started

started = timer
for iteration as integer = 0 to table_iterations - 1
    palette_data( iteration and 255 ) xor= &h00010101
    palette using palette_data( 0 )
next
screensync
using_seconds = timer - started

started = timer
for iteration as integer = 0 to table_iterations - 1
    palette get using palette_data( 0 )
    checksum += culng( palette_data( iteration and 255 ) )
next
get_using_seconds = timer - started

pset ( 0, 0 ), 7
final_pixel = point( 0, 0 )
screen 0

print "palette_family_set_seconds="; set_seconds
print "palette_family_get_seconds="; get_seconds
print "palette_family_using_seconds="; using_seconds
print "palette_family_get_using_seconds="; get_using_seconds
print "palette_family_checksum="; checksum
print "palette_family_pixel="; final_pixel
end 0

'' end of palette-family-benchmark.bas
