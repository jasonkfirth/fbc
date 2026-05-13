''
'' FreeBASIC macOS smoke tests
'' ---------------------------
''
'' File: gfx-darwin-truecolor-smoke.bas
''
'' Purpose:
''
''     Exercise the Darwin gfxlib driver with a 32-bit truecolor
''     framebuffer.
''
'' Responsibilities:
''
''     - open a truecolor graphics screen through the real Darwin driver
''     - draw known RGB values
''     - exercise mouse position/cursor calls
''     - force a screen update for the smoke-test dump hook
''
'' This file intentionally does NOT contain:
''
''     - OpenGL rendering
''     - null-driver coverage
''     - platform build orchestration
''

#include once "fbgfx.bi"

const SKIP_NO_GFX = 77

if( screenres( 64, 64, 32 ) <> 0 ) then
	end SKIP_NO_GFX
end if

windowtitle "FreeBASIC Darwin gfx smoke"
setmouse 7, 7, 1, 0
line (0, 0)-(63, 63), rgb( 0, 0, 0 ), bf
line (8, 8)-(23, 23), rgb( 255, 0, 0 ), bf
line (24, 8)-(39, 23), rgb( 0, 255, 0 ), bf
line (40, 8)-(55, 23), rgb( 0, 0, 255 ), bf
screensync
sleep 80, 1

end 0

'' end of gfx-darwin-truecolor-smoke.bas
