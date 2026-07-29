''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: android-keyboard-overlay-smoke.bas
''
'' Purpose:
''
''     Provide a live-device acceptance point for gfxlib3's GPU-presented
''     Android keyboard control and its IME input bridge.
''
'' Responsibilities:
''
''     - open an automatically selected Android GPU mode
''     - leave an unambiguous frame behind the native-pixel KB control
''     - reject any mouse press leaked from a tap on that control
''     - accept a lowercase A committed through the software keyboard
''
'' This file intentionally does NOT contain:
''
''     - synthetic Android touch or IME injection
''     - a device-specific screen-size assumption
''     - assertions about the system keyboard's visual theme
''
#include once "fbgfx3.bi"

if screenres( 320, 240, 32, 1, 0 ) <> 0 then
	print "GFX3_ANDROID_KEYBOARD_FAIL screenres"
	end 1
end if

line ( 0, 0 )-( 319, 239 ), rgb( 12, 22, 40 ), bf
line ( 20, 20 )-( 299, 219 ), rgb( 56, 116, 196 ), bf
line ( 28, 28 )-( 291, 211 ), rgb( 240, 196, 64 ), b
draw string ( 82, 106 ), "Tap KB, then type A", rgb( 255, 255, 255 )
screensync
print "GFX3_ANDROID_KEYBOARD_READY"

dim as fb.EVENT event
dim as string pressed

for poll_index as integer = 1 to 12000
	while screenevent( @event )
		if event.type = fb.EVENT_MOUSE_BUTTON_PRESS then
			print "GFX3_ANDROID_KEYBOARD_FAIL leaked-mouse"
			screen 0
			end 2
		end if
	wend
	pressed = inkey
	if pressed = "a" then exit for
	sleep 10, 1
next

screen 0
if pressed <> "a" then
	print "GFX3_ANDROID_KEYBOARD_FAIL timeout"
	end 3
end if
print "GFX3_ANDROID_KEYBOARD_PASS"
end 0

'' end of android-keyboard-overlay-smoke.bas
