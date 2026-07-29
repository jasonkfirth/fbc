''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: mode-lifecycle-smoke.bas
''
'' Purpose:
''
''     Exercise repeated public graphics-mode replacement and teardown.
''
'' Responsibilities:
''
''     - repeatedly replace active modes with different sizes and page counts
''     - prove invalid mode requests leave the current valid mode usable
''     - verify primitive submission and exact readback after every open
''     - verify SCREEN 0 is safe after both active and already-closed modes
''
'' This file intentionally does NOT contain:
''
''     - backend-internal allocation fault injection
''     - unbounded longevity testing
''     - interactive window or input assertions
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"

dim requested as string = lcase( command( 1 ) )
dim iteration_text as string = command( 2 )
dim flags as integer
dim iterations as integer = 32

select case requested
case "", "automatic"
	flags = 0
case "null"
	flags = fb.GFX_NULL
case "opengl"
	flags = fb.GFX_OPENGL
case "vulkan"
	flags = fb.GFX_VULKAN
case else
	end 1
end select

if len( iteration_text ) > 0 then iterations = valint( iteration_text )
if iterations < 1 orelse iterations > 10000 then end 2

dim last_driver as string

for iteration as integer = 0 to iterations - 1
	dim mode_width as integer = 32 + ( iteration mod 4 ) * 8
	dim mode_height as integer = 24 + ( iteration mod 3 ) * 8
	dim pages as integer = 1 + ( iteration mod 3 )
	dim x as integer = ( iteration * 7 ) mod mode_width
	dim y as integer = ( iteration * 11 ) mod mode_height
	dim test_color as ulong = rgba( iteration and &hff, _
		( iteration * 3 ) and &hff, ( iteration * 5 ) and &hff, _
		128 + ( iteration and &h7f ) )

	if screenres( mode_width, mode_height, 32, pages, flags ) <> 0 then end 10
	screencontrol fb.GET_DRIVER_NAME, last_driver

	pset ( x, y ), test_color
	if cuint( point( x, y ) ) <> test_color then end 11

	''
	'' Parameter validation happens before gfxlib3 replaces the active mode.
	'' A rejected request must therefore leave that mode and its GPU resources
	'' intact rather than partially tearing them down.
	''
	if ( iteration mod 4 ) = 0 then
		if screenres( 0, mode_height, 32, pages, flags ) = 0 then end 12
		pset ( x, y ), test_color xor &h00010101u
		if cuint( point( x, y ) ) <> ( test_color xor &h00010101u ) then end 13
	end if

	'' Alternate explicit closes with replacement by the next SCREENRES call.
	if ( iteration and 1 ) <> 0 then
		screen 0
		screen 0
	end if
next

screen 0
print "gfxlib3 mode lifecycle: " & last_driver & " (" & iterations & " opens)"
end 0

'' end of mode-lifecycle-smoke.bas
