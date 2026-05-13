''
'' FreeBASIC macOS smoke tests
'' ---------------------------
''
'' File: gfx-darwin-screen-modes-smoke.bas
''
'' Purpose:
''
''     Exercise legacy SCREEN modes through the native Darwin gfxlib
''     driver.
''
'' Responsibilities:
''
''     - call the SCREEN statement for a requested legacy mode
''     - draw known color boxes in supported graphics modes
''     - confirm unsupported legacy modes leave graphics mode closed
''     - force a screen update for the Darwin framebuffer dump hook
''
'' This file intentionally does NOT contain:
''
''     - generic gfxlib unit assertions
''     - screenres() coverage
''     - platform build orchestration
''

#include once "fbgfx.bi"

const MODE_OK = 0
const MODE_UNEXPECTED_SUCCESS = 1
const MODE_UNEXPECTED_FAILURE = 2

dim shared requested_mode as integer

requested_mode = val( environ( "FBGFX_SMOKE_MODE" ) )

if( requested_mode < 0 or requested_mode > 13 ) then
	end MODE_UNEXPECTED_FAILURE
end if

if( requested_mode = 0 ) then
	screen 0
	end MODE_OK
end if

if( requested_mode >= 3 and requested_mode <= 6 ) then
	screen requested_mode
	if( screenptr() <> 0 ) then
		screen 0
		end MODE_UNEXPECTED_SUCCESS
	end if

	screen 0
	end MODE_OK
end if

on error goto supported_mode_failed

screen requested_mode
if( screenptr() = 0 ) then
	end MODE_UNEXPECTED_FAILURE
end if

select case requested_mode
case 2, 10, 11
	palette 1, 255, 255, 255
	line (0, 0)-(95, 63), 0, bf
	line (8, 8)-(23, 23), 1, bf
case else
	palette 1, 255, 0, 0
	palette 2, 0, 255, 0
	palette 3, 0, 0, 255
	line (0, 0)-(95, 63), 0, bf
	line (8, 8)-(23, 23), 1, bf
	line (32, 8)-(47, 23), 2, bf
	line (56, 8)-(71, 23), 3, bf
end select

screensync
sleep 80, 1
screen 0

end MODE_OK

supported_mode_failed:
	end MODE_UNEXPECTED_FAILURE

'' end of gfx-darwin-screen-modes-smoke.bas
