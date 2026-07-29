''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: android-named-renderer-fallback-smoke.bas
''
'' Purpose:
''
''     Prove that a named Vulkan preference on a non-Vulkan Android device
''     falls through to the ordinary OpenGL ES renderer.
''
'' Responsibilities:
''
''     - request Vulkan only through the gfxlib2-compatible FBGFX name
''     - open an ordinary gfxlib3 mode without a force-only flag
''     - require the physical GLES fallback and exact GPU pixel readback
''
'' This file intentionally does NOT contain:
''
''     - Vulkan force-request behavior
''     - emulator-only renderer checks
''     - presentation timing measurements
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"

screencontrol fb.SET_DRIVER_NAME, ""
setenviron "FBGFX=Vulkan"

if screenres( 64, 48, 32, 1, 0 ) <> 0 then end 1

dim driver_name as string
screencontrol fb.GET_DRIVER_NAME, driver_name
if instr( lcase( driver_name ), "opengl es" ) = 0 then end 2

pset ( 17, 23 ), rgba( 18, 52, 86, 120 )
if cuint( point( 17, 23 ) ) <> rgba( 18, 52, 86, 120 ) then end 3

screen 0
setenviron "FBGFX="
print "GFX3_ANDROID_NAMED_RENDERER_FALLBACK_PASS " & driver_name
end 0

'' end of android-named-renderer-fallback-smoke.bas
