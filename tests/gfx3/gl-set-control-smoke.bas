''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: gl-set-control-smoke.bas
''
'' Purpose:
''
''     Verify the legacy SCREENCONTROL SET_GL_* compatibility state that
''     programs use to configure or inspect an OpenGL graphics mode.
''
'' Responsibilities:
''
''     - round-trip configurable color, depth, accumulation, and sample values
''     - preserve the established initial GET_GL_2D_MODE and GET_GL_SCALE state
''     - prove setter/query behavior remains available before and during a mode
''
'' This file intentionally does NOT contain:
''
''     - direct OpenGL calls or a writable OpenGL context
''     - assertions about a driver accepting a requested pixel format
''     - presentation timing or shader behavior
''

#ifndef GFX2_REFERENCE
    #ifndef __FB_GFXLIB3__
        #define __FB_GFXLIB3__
    #endif
#endif

#include once "fbgfx.bi"

sub require_control( byval control as long, byval expected as long, _
    byval failure_code as long )

    dim as long actual_value

    screencontrol control, actual_value
    if actual_value <> expected then
        print "GFX_GL_SET_CONTROL_FAIL " & control & " " & actual_value & _
            " expected " & expected
        end failure_code
    end if
end sub

screencontrol fb.SET_GL_COLOR_BITS, 27
screencontrol fb.SET_GL_COLOR_RED_BITS, 7
screencontrol fb.SET_GL_COLOR_GREEN_BITS, 8
screencontrol fb.SET_GL_COLOR_BLUE_BITS, 9
screencontrol fb.SET_GL_COLOR_ALPHA_BITS, 3
screencontrol fb.SET_GL_DEPTH_BITS, 19
screencontrol fb.SET_GL_STENCIL_BITS, 5
screencontrol fb.SET_GL_ACCUM_BITS, 24
screencontrol fb.SET_GL_ACCUM_RED_BITS, 6
screencontrol fb.SET_GL_ACCUM_GREEN_BITS, 7
screencontrol fb.SET_GL_ACCUM_BLUE_BITS, 8
screencontrol fb.SET_GL_ACCUM_ALPHA_BITS, 3
screencontrol fb.SET_GL_NUM_SAMPLES, 4

require_control fb.GET_GL_COLOR_BITS, 27, 1
require_control fb.GET_GL_COLOR_RED_BITS, 7, 2
require_control fb.GET_GL_COLOR_GREEN_BITS, 8, 3
require_control fb.GET_GL_COLOR_BLUE_BITS, 9, 4
require_control fb.GET_GL_COLOR_ALPHA_BITS, 3, 5
require_control fb.GET_GL_DEPTH_BITS, 19, 6
require_control fb.GET_GL_STENCIL_BITS, 5, 7
require_control fb.GET_GL_ACCUM_BITS, 24, 8
require_control fb.GET_GL_ACCUM_RED_BITS, 6, 9
require_control fb.GET_GL_ACCUM_GREEN_BITS, 7, 10
require_control fb.GET_GL_ACCUM_BLUE_BITS, 8, 11
require_control fb.GET_GL_ACCUM_ALPHA_BITS, 3, 12
require_control fb.GET_GL_NUM_SAMPLES, 4, 13

'' gfxlib2 leaves the active 2D mode disabled until an OpenGL mode exists. ''
screencontrol fb.SET_GL_2D_MODE, 9
screencontrol fb.SET_GL_SCALE, 3
require_control fb.GET_GL_2D_MODE, 0, 14
require_control fb.GET_GL_SCALE, 1, 15

if screenres( 32, 24, 32, 1, fb.GFX_NULL ) <> 0 then end 16

'' gfxlib2 applies ordinary SET_GL_* state immediately even during a mode. ''
screencontrol fb.SET_GL_COLOR_BITS, 29
screencontrol fb.SET_GL_NUM_SAMPLES, 6
require_control fb.GET_GL_COLOR_BITS, 29, 17
require_control fb.GET_GL_NUM_SAMPLES, 6, 18
require_control fb.GET_GL_2D_MODE, 0, 19
require_control fb.GET_GL_SCALE, 1, 20

screen 0
print "GFX_GL_SET_CONTROL_PASS"
end 0

'' end of gl-set-control-smoke.bas
