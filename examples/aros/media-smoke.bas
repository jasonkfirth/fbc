''
'' FreeBASIC AROS media smoke test
'' --------------------------------
''
'' File: media-smoke.bas
''
'' Purpose:
''
''     Exercise native AROS graphics presentation, input polling, and sound.
''
'' Responsibilities:
''
''     - create a windowed gfxlib2 screen
''     - draw changing true-colour content
''     - request audible sfxlib output
''     - terminate automatically for unattended emulator tests
''
'' This file intentionally does NOT contain:
''
''     - acceptance-test log parsing
''     - emulator orchestration
''     - platform implementation details
''

#include once "fbgfx.bi"

screenres 320, 240, 32
if screenptr = 0 then
	print "AROS_MEDIA_SMOKE: graphics initialization failed"
	end 1
end if

screenlock
for y as integer = 0 to 239
	for x as integer = 0 to 319
		pset (x, y), rgb(x and 255, y and 255, (x xor y) and 255)
	next
next
screenunlock

sound 880, 6
sleep 500
print "AROS_MEDIA_SMOKE: PASS"

'' end of media-smoke.bas
