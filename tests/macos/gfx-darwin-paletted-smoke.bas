''
'' FreeBASIC macOS smoke tests
'' ---------------------------
''
'' File: gfx-darwin-paletted-smoke.bas
''
'' Purpose:
''
''     Exercise the Darwin gfxlib driver with an 8-bit paletted
''     framebuffer.
''
'' Responsibilities:
''
''     - open a paletted graphics screen through the real Darwin driver
''     - change a palette entry
''     - draw pixels that require palette-to-truecolor conversion
''     - force a screen update for the smoke-test dump hook
''
'' This file intentionally does NOT contain:
''
''     - generic gfxlib unit assertions
''     - null-driver coverage
''     - platform build orchestration
''

#include once "fbgfx.bi"

const SKIP_NO_GFX = 77

if( screenres( 64, 64, 8 ) <> 0 ) then
	end SKIP_NO_GFX
end if

palette 1, 255, 0, 0
palette 2, 0, 255, 0
palette 3, 0, 0, 255
line (0, 0)-(63, 63), 0, bf
line (8, 8)-(23, 23), 1, bf
line (24, 8)-(39, 23), 2, bf
line (40, 8)-(55, 23), 3, bf
screensync
sleep 80, 1

end 0

'' end of gfx-darwin-paletted-smoke.bas
