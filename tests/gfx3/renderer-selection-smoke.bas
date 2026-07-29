''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: renderer-selection-smoke.bas
''
'' Purpose:
''
''     Exercise automatic gfxlib3 GPU renderer selection through the public
''     FreeBASIC graphics API.
''
'' Responsibilities:
''
''     - open an automatic mode or an explicitly forced GPU mode
''     - verify the selected renderer is a supported GPU backend
''     - prove exact primitive readback after automatic selection
''
'' This file intentionally does NOT contain:
''
''     - a null-backend fallback
''     - renderer-specific drawing behavior beyond selection assertions
''     - performance measurements
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"

#ifdef __FB_ANDROID__
	'' NativeActivity supplies a launcher argument that is not a renderer
	'' request. Android always exercises automatic backend selection here.
	dim requested as string = ""
#else
	dim requested as string = lcase( command( 1 ) )
#endif
dim flags as integer

select case requested
case ""
	flags = 0
case "opengl"
	flags = fb.GFX_OPENGL
case "vulkan"
	flags = fb.GFX_VULKAN
case else
	end 1
end select

if screenres( 64, 48, 32, 2, flags ) <> 0 then end 2

dim driver as string
screencontrol fb.GET_DRIVER_NAME, driver

dim normalized_driver as string = lcase( driver )
if instr( normalized_driver, "vulkan" ) = 0 andalso _
   instr( normalized_driver, "opengl" ) = 0 then
	end 3
end if
if requested = "opengl" andalso instr( normalized_driver, "opengl" ) = 0 then end 4
if requested = "vulkan" andalso instr( normalized_driver, "vulkan" ) = 0 then end 5

pset ( 17, 23 ), rgba( 18, 52, 86, 120 )
if cuint( point( 17, 23 ) ) <> rgba( 18, 52, 86, 120 ) then end 6

screen 0
print "gfxlib3 automatic renderer: " & driver
end 0

'' end of renderer-selection-smoke.bas
