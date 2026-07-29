''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: gl-control-smoke.bas
''
'' Purpose:
''
''     Verify that SCREENCONTROL exposes the immutable capabilities of the
''     render-thread-owned OpenGL context without sharing that context.
''
'' Responsibilities:
''
''     - force the desktop OpenGL backend
''     - read all public colour, depth, stencil, accumulation, and sample data
''     - verify colour totals and legacy OpenGL-2D defaults
''     - confirm the extension result is a safe BASIC string
''
'' This file intentionally does NOT contain:
''
''     - raw SCREENGLPROC calls from the BASIC thread
''     - direct OpenGL state changes or drawing
''     - driver-specific extension-name assertions
''

#ifndef __FB_GFXLIB3__
    #define __FB_GFXLIB3__
#endif
#include once "fbgfx.bi"

dim as integer color_bits
dim as integer red_bits
dim as integer green_bits
dim as integer blue_bits
dim as integer alpha_bits
dim as integer depth_bits
dim as integer stencil_bits
dim as integer accum_bits
dim as integer accum_red_bits
dim as integer accum_green_bits
dim as integer accum_blue_bits
dim as integer accum_alpha_bits
dim as integer samples
dim as integer mode_2d
dim as integer scale
dim as string extensions

if screenres( 96, 80, 32, 1, fb.GFX_OPENGL ) <> 0 then
	print "GFX3_GL_CONTROL_FAIL screenres"
	end 1
end if

screencontrol fb.GET_GL_COLOR_BITS, color_bits
screencontrol fb.GET_GL_COLOR_RED_BITS, red_bits
screencontrol fb.GET_GL_COLOR_GREEN_BITS, green_bits
screencontrol fb.GET_GL_COLOR_BLUE_BITS, blue_bits
screencontrol fb.GET_GL_COLOR_ALPHA_BITS, alpha_bits
screencontrol fb.GET_GL_DEPTH_BITS, depth_bits
screencontrol fb.GET_GL_STENCIL_BITS, stencil_bits
screencontrol fb.GET_GL_ACCUM_BITS, accum_bits
screencontrol fb.GET_GL_ACCUM_RED_BITS, accum_red_bits
screencontrol fb.GET_GL_ACCUM_GREEN_BITS, accum_green_bits
screencontrol fb.GET_GL_ACCUM_BLUE_BITS, accum_blue_bits
screencontrol fb.GET_GL_ACCUM_ALPHA_BITS, accum_alpha_bits
screencontrol fb.GET_GL_NUM_SAMPLES, samples
screencontrol fb.GET_GL_2D_MODE, mode_2d
screencontrol fb.GET_GL_SCALE, scale
screencontrol fb.GET_GL_EXTENSIONS, extensions

screen 0

if red_bits <= 0 orelse green_bits <= 0 orelse blue_bits <= 0 then
	print "GFX3_GL_CONTROL_FAIL color-components " & red_bits & "," & _
		green_bits & "," & blue_bits
	end 2
end if
if color_bits <> red_bits + green_bits + blue_bits + alpha_bits then
	print "GFX3_GL_CONTROL_FAIL color-total"
	end 3
end if
if depth_bits < 0 orelse stencil_bits < 0 orelse accum_bits < 0 orelse _
	   accum_red_bits < 0 orelse accum_green_bits < 0 orelse _
	   accum_blue_bits < 0 orelse accum_alpha_bits < 0 orelse samples < 0 then
	print "GFX3_GL_CONTROL_FAIL negative-capability"
	end 4
end if
if accum_bits <> accum_red_bits + accum_green_bits + accum_blue_bits + _
	accum_alpha_bits then
	print "GFX3_GL_CONTROL_FAIL accum-total"
	end 5
end if
if mode_2d <> 0 orelse scale <> 1 then
	print "GFX3_GL_CONTROL_FAIL legacy-default"
	end 6
end if

print "GFX3_GL_CONTROL_PASS color=" & color_bits & _
	" depth=" & depth_bits & " extension-bytes=" & len( extensions )
end 0

'' end of gl-control-smoke.bas
