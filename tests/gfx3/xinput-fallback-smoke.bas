''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: xinput-fallback-smoke.bas
''
'' Purpose:
''
''     Exercise the Win32 XInput polling path when no controller is present.
''
'' Responsibilities:
''
''     - open a real automatic GPU mode and allow its render thread to poll
''     - check gfxlib2-compatible missing-device values after that poll
''     - close the window without leaving an input worker behind
''
'' This file intentionally does NOT contain:
''
''     - physical controller button or axis coverage
''     - XInput DLL injection or mock controller drivers
''     - Android input tests
''

#include once "fbgfx3.bi"

dim as integer buttons
dim as single axis1, axis2, axis3, axis4, axis5, axis6, axis7, axis8
dim as single left_x, left_y, right_x, right_y, left_trigger, right_trigger
dim as integer dpad

if screenres( 48, 32, 32 ) <> 0 then end 1
screensync
sleep 20, 1

if getjoystick( 0, buttons, axis1, axis2, axis3, axis4, axis5, axis6, _
	axis7, axis8 ) = 0 then end 2
if buttons <> -1 then end 3
if axis1 <> -1000.0 orelse axis8 <> -1000.0 then end 4

if getxpad( 0, buttons, left_x, left_y, right_x, right_y, left_trigger, _
	right_trigger, dpad ) <> fb.XPAD_STATUS_MISSING then end 5
if buttons <> 0 orelse left_x <> 0.0 orelse left_y <> 0.0 then end 6
if right_x <> 0.0 orelse right_y <> 0.0 then end 6
if left_trigger <> 0.0 orelse right_trigger <> 0.0 orelse dpad <> 0 then _
	end 6

screen 0
print "GFX3_XINPUT_FALLBACK_PASS"

'' end of xinput-fallback-smoke.bas
