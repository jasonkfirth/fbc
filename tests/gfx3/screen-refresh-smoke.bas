''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: screen-refresh-smoke.bas
''
'' Purpose:
''
''     Verify gfxlib2-compatible refresh reporting for windowed graphics modes.
''
'' Responsibilities:
''
''     - open a graphics mode with a refresh request
''     - compare SCREENINFO and GET_SCREEN_REFRESH against the driver value
''     - close the mode through the normal public path
''
'' This file intentionally does NOT contain:
''
''     - a desktop display-mode change
''     - timing or hardware-vblank measurement
''     - timing or hardware-vblank measurement
''

#ifndef GFX2_REFERENCE
    #ifndef __FB_GFXLIB3__
        #define __FB_GFXLIB3__
    #endif
#endif

#include once "fbgfx.bi"

const requested_refresh = 73
const expected_refresh = 0

#ifdef GFX_OPENGL_TEST
    const backend_flags = fb.GFX_OPENGL
#else
    const backend_flags = fb.GFX_NULL
#endif

if screenres( 32, 24, 32, 1, backend_flags, requested_refresh ) <> 0 then
    end 1
end if

dim as integer width_value, height_value, depth_value
dim as integer bytes_per_pixel, pitch, refresh_value
dim as integer control_refresh
dim as string driver

screeninfo width_value, height_value, depth_value, bytes_per_pixel, pitch, _
    refresh_value, driver
if width_value <> 32 orelse height_value <> 24 orelse depth_value <> 32 then
    end 2
end if
'' gfxlib2's windowed and null drivers do not report a requested refresh. ''
if refresh_value <> expected_refresh then end 3

screencontrol fb.GET_SCREEN_REFRESH, control_refresh
if control_refresh <> expected_refresh then end 4

screen 0
print "GFX_SCREEN_REFRESH_PASS " & requested_refresh & "->" & _
    expected_refresh
end 0

'' end of screen-refresh-smoke.bas
