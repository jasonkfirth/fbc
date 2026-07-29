''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: android-input-smoke.bas
''
'' Purpose:
''
''     Verify Android key events cross the NativeActivity and gfxlib3 input
''     boundaries into ordinary FreeBASIC INKEY behavior.
''
'' Responsibilities:
''
''     - open an automatically selected Android GPU mode
''     - wait a bounded interval for a lowercase A key
''     - report a machine-readable result through standard output
''
'' This file intentionally does NOT contain:
''
''     - ADB event injection
''     - software-keyboard display policy
''     - graphical keyboard controls
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"

if screenres( 320, 240, 32, 1, 0 ) <> 0 then
	print "GFX3_ANDROID_INPUT_FAIL screenres"
	end 1
end if

dim pressed as string

for poll_index as integer = 0 to 799
	pressed = inkey
	if pressed = "a" then exit for
	sleep 10, 1
next

screen 0
if pressed <> "a" then
	print "GFX3_ANDROID_INPUT_FAIL timeout"
	end 2
end if

print "GFX3_ANDROID_INPUT_PASS a"
end 0

'' end of android-input-smoke.bas
