''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: android-touch-smoke.bas
''
'' Purpose:
''
''     Prove that an Android NativeActivity motion event reaches the public
''     GETTOUCH API as a native contact, not merely as desktop mouse fallback.
''
'' Responsibilities:
''
''     - open an automatically selected Android GPU mode
''     - wait a bounded interval for one physical contact
''     - validate its coordinates, stable non-negative id, and hit testing
''
'' This file intentionally does NOT contain:
''
''     - synthetic Android input injection
''     - a device-specific screen-size expectation
''     - multi-contact gesture generation
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"

if screenres( 320, 240, 32, 1, 0 ) <> 0 then
	print "GFX3_ANDROID_TOUCH_FAIL screenres"
	end 1
end if

dim contact_count as integer
dim touch_x as integer
dim touch_y as integer
dim touch_id as integer
dim found as integer

for poll_index as integer = 0 to 1499
	contact_count = gettouchcount()
	if contact_count > 0 then
		if gettouch( 0, touch_x, touch_y, touch_id ) <> 0 then
			print "GFX3_ANDROID_TOUCH_FAIL gettouch"
			screen 0
			end 2
		end if
		if ( touch_x < 0 ) or ( touch_x >= 320 ) or _
		   ( touch_y < 0 ) or ( touch_y >= 240 ) or ( touch_id < 0 ) then
			print "GFX3_ANDROID_TOUCH_FAIL values"
			screen 0
			end 3
		end if
		if gettouchhit( touch_x, touch_y, touch_x, touch_y ) = 0 then
			print "GFX3_ANDROID_TOUCH_FAIL hit"
			screen 0
			end 4
		end if
		found = -1
		exit for
	end if
	sleep 10, 1
next

screen 0
if found = 0 then
	print "GFX3_ANDROID_TOUCH_FAIL timeout"
	end 5
end if

print "GFX3_ANDROID_TOUCH_PASS"
end 0

'' end of android-touch-smoke.bas
