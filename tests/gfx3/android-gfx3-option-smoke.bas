''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: android-gfx3-option-smoke.bas
''
'' Purpose:
''
''     Verify that fbc-android propagates -gfx3 into a real Android package
''     without requiring a source-level define.
''
'' Responsibilities:
''
''     - require the compiler-injected gfxlib3 define and Vulkan declaration
''     - open the best available Android GPU renderer automatically
''     - report the selected renderer before clean NativeActivity shutdown
''
'' This file intentionally does NOT contain:
''
''     - a source-level gfxlib3 define
''     - a Vulkan-only device requirement
''     - controller or touch interaction
''

#include once "fbgfx.bi"

#ifndef __FB_GFXLIB3__
	#error "Android -gfx3 did not define __FB_GFXLIB3__"
#endif

if fb.GFX_VULKAN <> &h200 then
	print "GFX3_ANDROID_OPTION_FAIL constant"
	end 1
end if

if screenres( 64, 48, 32, 1, 0 ) <> 0 then
	print "GFX3_ANDROID_OPTION_FAIL screenres"
	end 2
end if

dim as string driver
screencontrol fb.GET_DRIVER_NAME, driver

if instr( lcase( driver ), "opengl" ) = 0 andalso _
   instr( lcase( driver ), "vulkan" ) = 0 then
	screen 0
	print "GFX3_ANDROID_OPTION_FAIL driver=" & driver
	end 3
end if

screen 0
print "GFX3_ANDROID_OPTION_PASS " & driver
end 0

'' end of android-gfx3-option-smoke.bas
