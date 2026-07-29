''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: android-gfx3-extension-header-smoke.bas
''
'' Purpose:
''
''     Verify that Android packaging recognizes fbgfx3.bi as an opt-in even
''     when the program does not repeat the gfxlib3 source define.
''
'' Responsibilities:
''
''     - require fbgfx3.bi to select the public gfxlib3 declarations
''     - create, clear, read, and destroy an opaque GPU surface
''     - report the automatically selected Android GPU renderer
''
'' This file intentionally does NOT contain:
''
''     - a source-level __FB_GFXLIB3__ define
''     - a forced Vulkan requirement
''     - CPU-image compatibility coverage
''

#include once "fbgfx3.bi"

#ifndef __FB_GFXLIB3__
	#error "fbgfx3.bi did not select gfxlib3"
#endif

if screenres( 32, 24, 32, 1, 0 ) <> 0 then end 1

dim as any ptr surface = fb.Gfx3SurfaceCreate( 8, 8, 32, , &h00112233u )
if surface = 0 then
	screen 0
	end 2
end if
if fb.Gfx3SurfaceClear( surface, &h00445566u ) <> 0 then
	fb.Gfx3SurfaceDestroy( surface )
	screen 0
	end 3
end if
if culng( point( 3, 3, surface ) ) <> &h00445566u then
	fb.Gfx3SurfaceDestroy( surface )
	screen 0
	end 4
end if
if fb.Gfx3SurfaceDestroy( surface ) <> 0 then
	screen 0
	end 5
end if

dim as string driver
screencontrol fb.GET_DRIVER_NAME, driver
if instr( lcase( driver ), "opengl" ) = 0 andalso _
	instr( lcase( driver ), "vulkan" ) = 0 then
	screen 0
	end 6
end if

screen 0
print "GFX3_ANDROID_EXTENSION_HEADER_PASS " & driver
end 0

'' end of android-gfx3-extension-header-smoke.bas
