''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: driver-selection-precedence-smoke.bas
''
'' Purpose:
''
''     Prove the public gfxlib2 driver-selection precedence contract while
''     gfxlib3 is selected: SCREENCONTROL SET_DRIVER_NAME overrides FBGFX,
''     and clearing that override returns control to FBGFX.
''
'' Responsibilities:
''
''     - set a case-insensitive explicit Null driver override
''     - prove that it wins over an incompatible FBGFX request
''     - clear the override and prove FBGFX selects Null again
''
'' This file intentionally does NOT contain:
''
''     - a hardware-specific Vulkan or OpenGL requirement
''     - an assertion about an unavailable GPU backend's error message
''     - a performance measurement
''

#ifndef GFX2_REFERENCE
    #define __FB_GFXLIB3__
#endif
#include once "fbgfx.bi"

function selected_null_driver() as integer
    dim driver_name as string

    screencontrol fb.GET_DRIVER_NAME, driver_name
    function = iif( lcase( driver_name ) = "null", true, false )
end function

setenviron "FBGFX=Vulkan"
screencontrol fb.SET_DRIVER_NAME, "nUlL"

if screenres( 16, 16, 32, 1, 0 ) <> 0 then
    print "GFX_DRIVER_PRECEDENCE_FAIL explicit screenres"
    end 1
end if

if selected_null_driver() = false then
    print "GFX_DRIVER_PRECEDENCE_FAIL explicit name"
    screen 0
    end 2
end if

screen 0

screencontrol fb.SET_DRIVER_NAME, ""
setenviron "FBGFX=NuLl"

if screenres( 16, 16, 32, 1, 0 ) <> 0 then
    print "GFX_DRIVER_PRECEDENCE_FAIL environment screenres"
    end 3
end if

if selected_null_driver() = false then
    print "GFX_DRIVER_PRECEDENCE_FAIL environment name"
    screen 0
    end 4
end if

screen 0
setenviron "FBGFX="
print "GFX_DRIVER_PRECEDENCE_PASS"
end 0

'' end of driver-selection-precedence-smoke.bas
