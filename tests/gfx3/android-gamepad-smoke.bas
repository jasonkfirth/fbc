''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: android-gamepad-smoke.bas
''
'' Purpose:
''
''     Verify the Android NativeActivity gamepad bridge links into gfxlib3
''     and preserves the public missing-controller contract.
''
'' Responsibilities:
''
''     - open an automatically selected Android GPU mode
''     - query GETJOYSTICK and GETXPAD slot zero
''     - validate either historical missing values or bounded live values
''     - report a machine-readable result through standard output
''
'' This file intentionally does NOT contain:
''
''     - ADB controller-event injection
''     - physical controller pairing or disconnect coverage
''     - vibration, battery, or controller-name support
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"

const XPAD_STATUS_MISSING = 0
const XPAD_STATUS_CONNECTED = 1

dim buttons as integer
dim dpad as integer
dim a1 as single
dim a2 as single
dim a3 as single
dim a4 as single
dim a5 as single
dim a6 as single
dim a7 as single
dim a8 as single
dim lx as single
dim ly as single
dim rx as single
dim ry as single
dim lt as single
dim rt as single
dim joystick_status as integer
dim xpad_status as integer

if screenres( 320, 240, 32, 1, 0 ) <> 0 then
	print "GFX3_ANDROID_GAMEPAD_FAIL screenres"
	end 1
end if

joystick_status = getjoystick( 0, buttons, a1, a2, a3, a4, a5, a6, a7, a8 )
if joystick_status = 0 then
	if a1 < -1.0 orelse a1 > 1.0 orelse _
	   a2 < -1.0 orelse a2 > 1.0 orelse _
	   a3 < -1.0 orelse a3 > 1.0 orelse _
	   a4 < -1.0 orelse a4 > 1.0 orelse _
	   a5 < -1.0 orelse a5 > 1.0 orelse _
	   a6 < -1.0 orelse a6 > 1.0 orelse _
	   a7 < -1.0 orelse a7 > 1.0 orelse _
	   a8 < -1.0 orelse a8 > 1.0 then
		print "GFX3_ANDROID_GAMEPAD_FAIL joystick-range"
		end 2
	end if
else
	if buttons <> -1 orelse a1 <> -1000.0 orelse a2 <> -1000.0 orelse _
	   a3 <> -1000.0 orelse a4 <> -1000.0 orelse a5 <> -1000.0 orelse _
	   a6 <> -1000.0 orelse a7 <> -1000.0 orelse a8 <> -1000.0 then
		print "GFX3_ANDROID_GAMEPAD_FAIL joystick-missing"
		end 3
	end if
end if

xpad_status = getxpad( 0, buttons, lx, ly, rx, ry, lt, rt, dpad )
if xpad_status = XPAD_STATUS_CONNECTED then
	if lx < -1.0 orelse lx > 1.0 orelse ly < -1.0 orelse ly > 1.0 orelse _
	   rx < -1.0 orelse rx > 1.0 orelse ry < -1.0 orelse ry > 1.0 orelse _
	   lt < 0.0 orelse lt > 1.0 orelse rt < 0.0 orelse rt > 1.0 then
		print "GFX3_ANDROID_GAMEPAD_FAIL xpad-range"
		end 4
	end if
else
	if xpad_status <> XPAD_STATUS_MISSING orelse buttons <> 0 orelse _
	   lx <> 0.0 orelse ly <> 0.0 orelse rx <> 0.0 orelse ry <> 0.0 orelse _
	   lt <> 0.0 orelse rt <> 0.0 orelse dpad <> 0 then
		print "GFX3_ANDROID_GAMEPAD_FAIL xpad-missing"
		end 5
	end if
end if

screen 0
print "GFX3_ANDROID_GAMEPAD_PASS joystick=" & joystick_status & _
	" xpad=" & xpad_status
end 0

'' end of android-gamepad-smoke.bas
