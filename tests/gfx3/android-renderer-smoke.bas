''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: android-renderer-smoke.bas
''
'' Purpose:
''
''     Prove automatic gfxlib3 renderer fallback and GPU drawing on a real
''     Android NativeActivity device.
''
'' Responsibilities:
''
''     - request an ordinary mode without forcing a renderer
''     - report the backend selected after Vulkan probing
''     - verify GPU primitive writes through POINT readback
''     - leave a recognizable frame visible for device screenshot checks
''
'' This file intentionally does NOT contain:
''
''     - device-specific renderer forcing
''     - touch, keyboard, audio, or lifecycle stress testing
''     - performance benchmarking
''

#ifndef __FB_GFXLIB3__
    #define __FB_GFXLIB3__
#endif
#include once "fbgfx.bi"

if screenres( 320, 240, 32, 2, 0 ) <> 0 then
	print "GFX3_ANDROID_FAIL screenres"
	end 1
end if

dim driver as string
screencontrol fb.GET_DRIVER_NAME, driver
print "GFX3_ANDROID_DRIVER=" & driver
if instr( lcase( driver ), "opengl es" ) = 0 then
	print "GFX3_ANDROID_FAIL unexpected-driver"
	end 2
end if

line ( 0, 0 )-( 319, 239 ), rgb( 8, 18, 34 ), bf
line ( 16, 16 )-( 303, 223 ), rgb( 30, 70, 130 ), bf
line ( 28, 28 )-( 291, 211 ), rgb( 90, 180, 255 ), b
circle ( 160, 120 ), 68, rgb( 255, 190, 40 ), , , 0.72, f
line ( 72, 118 )-( 248, 122 ), rgb( 20, 40, 80 ), bf
line ( 158, 52 )-( 162, 188 ), rgb( 20, 40, 80 ), bf
pset ( 7, 9 ), rgba( 18, 52, 86, 120 )

if cuint( point( 7, 9 ) ) <> rgba( 18, 52, 86, 120 ) then
	print "GFX3_ANDROID_FAIL point"
	end 3
end if
if cuint( point( 20, 20 ) ) <> rgb( 30, 70, 130 ) then
	print "GFX3_ANDROID_FAIL rectangle"
	end 4
end if

screenset 1, 1
line ( 0, 0 )-( 319, 239 ), rgb( 14, 30, 54 ), bf
line ( 0, 0 )-( 319, 239 ), rgb( 45, 95, 165 ), bf
line ( 24, 24 )-( 295, 215 ), rgb( 20, 45, 82 ), bf
line ( 24, 24 )-( 295, 215 ), rgb( 125, 215, 255 ), b
screensync

print "GFX3 ANDROID PASS: " & driver
sleep 10000
screen 0
end 0

'' end of android-renderer-smoke.bas
