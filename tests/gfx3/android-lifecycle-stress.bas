''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: android-lifecycle-stress.bas
''
'' Purpose:
''
''     Exercise a long bounded sequence of Android gfxlib3 mode creation and
''     teardown operations before the NativeActivity process exits.
''
'' Responsibilities:
''
''     - replace GLES-backed modes with varying dimensions and page counts
''     - verify an exact PSET/POINT value after every mode creation
''     - alternate replacement and explicit double SCREEN 0 teardown
''     - provide a deterministic completion marker for repeated ADB launches
''
'' This file intentionally does NOT contain:
''
''     - input or keyboard-overlay testing
''     - Vulkan-hardware assertions
''     - unbounded soak testing
''
#define __FB_GFXLIB3__
#include once "fbgfx.bi"

const iteration_count = 256
dim as string driver_name

for iteration as integer = 0 to iteration_count - 1
    dim mode_width as integer = 32 + ( iteration mod 4 ) * 8
    dim mode_height as integer = 24 + ( iteration mod 3 ) * 8
    dim pages as integer = 1 + ( iteration mod 3 )
    dim x as integer = ( iteration * 7 ) mod mode_width
    dim y as integer = ( iteration * 11 ) mod mode_height
    dim expected_color as ulong = rgba( iteration and &hff, _
        ( iteration * 3 ) and &hff, ( iteration * 5 ) and &hff, _
        128 + ( iteration and &h7f ) )

    if screenres( mode_width, mode_height, 32, pages ) <> 0 then end 10
    screencontrol fb.GET_DRIVER_NAME, driver_name
    if instr( lcase( driver_name ), "opengl" ) = 0 then end 11

    pset ( x, y ), expected_color
    if cuint( point( x, y ) ) <> expected_color then end 12

    if ( iteration and 1 ) <> 0 then
        screen 0
        screen 0
    end if
next

screen 0
print "GFX3_ANDROID_LIFECYCLE_STRESS_PASS " & driver_name & _
    " (" & iteration_count & " opens)"
end 0

'' end of android-lifecycle-stress.bas
