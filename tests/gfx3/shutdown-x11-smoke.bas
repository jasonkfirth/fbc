''
'' Project: FreeBASIC graphics runtime tests
'' -----------------------------------------
''
'' File: shutdown-x11-smoke.bas
''
'' Purpose:
''
''     Verify that an X11 graphics program terminates after its main routine.
''
'' Responsibilities:
''
''     - start the threaded gfxlib2 X11 backend
''     - allow the window worker to complete several update cycles
''     - leave the screen active so fb_End() must stop and join that worker
''
'' This file intentionally does NOT contain:
''
''     - an explicit SCREEN 0 shutdown
''     - interactive input
''     - gfxlib3 coverage
''

#include once "fbgfx.bi"

if screenres( 160, 120, 32, 1, fb.GFX_RESIZABLE ) <> 0 then end 1
windowtitle "FreeBASIC X11 shutdown smoke"

for frame as integer = 1 to 8
	cls
	draw string ( 8, 8 ), "X11 shutdown smoke"
	flip
	sleep 10, 1
next

print "PASS X11 shutdown"
end 0

'' end of shutdown-x11-smoke.bas
