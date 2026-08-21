''
'' FreeBASIC AROS graphics smoke test
'' -----------------------------------
''
'' File: gfx-smoke.bas
''
'' Purpose:
''
''     Isolate native AROS gfxlib2 initialization, presentation, and teardown.
''
'' Responsibilities:
''
''     - create a true-colour Intuition window
''     - present a distinctive test pattern
''     - terminate without waiting for input
''
'' This file intentionally does NOT contain:
''
''     - sfxlib initialization
''     - emulator orchestration
''     - interactive acceptance criteria
''

#include once "fbgfx.bi"

screenres 320, 240, 32
if screenptr = 0 then
	end 1
end if

line (0, 0)-(319, 239), rgb(20, 40, 160), bf
line (16, 16)-(303, 223), rgb(250, 190, 20), b
draw string (56, 108), "AROS GFXLIB2 PASS", rgb(255, 255, 255)
sleep 1500, 1
screen 0

'' end of gfx-smoke.bas
