''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: gamepad-output-abi-smoke.bas
''
'' Purpose:
''
''     Verify that GETJOYSTICK and GETXPAD write only the native-width
''     Integer output fields declared by the FreeBASIC runtime.
''
'' Responsibilities:
''
''     - place guard values immediately after integer button outputs
''     - query missing gamepad slots through the gfxlib3 null backend
''     - verify the button and D-pad outputs preserve adjacent memory
''
'' This file intentionally does NOT contain:
''
''     - controller discovery or physical input assumptions
''     - renderer performance checks
''     - gfxlib2 compatibility checks
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"

'' fblint: disable-next-line FBL910 -- Guard fields must match the target's FreeBASIC INTEGER ABI.
type GuardedInteger
	value as integer
	guard as integer
end type

const GAMEPAD_OUTPUT_GUARD as integer = &h13579BDF

dim joystickButtons as GuardedInteger
dim xpadButtons as GuardedInteger
dim xpadDpad as GuardedInteger
dim axis1 as single
dim axis2 as single
dim axis3 as single
dim axis4 as single
dim axis5 as single
dim axis6 as single
dim axis7 as single
dim axis8 as single
dim leftX as single
dim leftY as single
dim rightX as single
dim rightY as single
dim leftTrigger as single
dim rightTrigger as single
dim status as integer

if screenres(16, 16, 32, 1, FB.GFX_NULL) <> 0 then end 1

joystickButtons.guard = GAMEPAD_OUTPUT_GUARD
status = getjoystick(0, joystickButtons.value, axis1, axis2, axis3, axis4, _
	axis5, axis6, axis7, axis8)
if joystickButtons.guard <> GAMEPAD_OUTPUT_GUARD then
	screen 0
	print "GFX3_GAMEPAD_OUTPUT_ABI_FAIL joystick-guard=" & hex(joystickButtons.guard)
	end 2
end if
if status = 0 then
	screen 0
	print "GFX3_GAMEPAD_OUTPUT_ABI_FAIL joystick-status=" & str(status)
	end 3
end if
if joystickButtons.value <> -1 or axis1 <> -1000.0 or axis8 <> -1000.0 then
	screen 0
	print "GFX3_GAMEPAD_OUTPUT_ABI_FAIL joystick-status=" & str(status) & _
		" buttons=" & str(joystickButtons.value) & _
		" first-axis=" & str(axis1) & " last-axis=" & str(axis8)
	end 4
end if

xpadButtons.guard = GAMEPAD_OUTPUT_GUARD
xpadDpad.guard = GAMEPAD_OUTPUT_GUARD
status = getxpad(0, xpadButtons.value, leftX, leftY, rightX, rightY, _
	leftTrigger, rightTrigger, xpadDpad.value)
if xpadButtons.guard <> GAMEPAD_OUTPUT_GUARD then
	screen 0
	print "GFX3_GAMEPAD_OUTPUT_ABI_FAIL xpad-button-guard=" & hex(xpadButtons.guard)
	end 5
end if
if xpadDpad.guard <> GAMEPAD_OUTPUT_GUARD then
	screen 0
	print "GFX3_GAMEPAD_OUTPUT_ABI_FAIL dpad-guard=" & hex(xpadDpad.guard)
	print "integer-size=" & sizeof(integer) & " guarded-size=" & sizeof(GuardedInteger)
	end 6
end if
if status <> 0 then
	screen 0
	print "GFX3_GAMEPAD_OUTPUT_ABI_FAIL xpad-status=" & str(status)
	end 7
end if
if xpadButtons.value <> 0 or xpadDpad.value <> 0 or _
	leftX <> 0.0 or rightTrigger <> 0.0 then
	screen 0
	print "GFX3_GAMEPAD_OUTPUT_ABI_FAIL xpad-missing-values buttons=" & _
		str(xpadButtons.value) & " dpad=" & str(xpadDpad.value) & _
		" left-x=" & str(leftX) & " right-trigger=" & str(rightTrigger)
	end 8
end if

screen 0
print "GFX3_GAMEPAD_OUTPUT_ABI_PASS"
end 0

'' end of gamepad-output-abi-smoke.bas
