''
'' FreeBASIC AROS graphics smoke test
'' ----------------------------------
''
'' File: gfx-colour-smoke.bas
''
'' Purpose:
''
''     Make primary colour ordering visible on the native AROS gfxlib2 path.
''
'' Responsibilities:
''
''     - create a 32-bit gfxlib2 display
''     - show unblended red, green, and blue reference regions
''     - keep the result visible long enough for an emulator capture
''     - release the display before terminating
''
'' This file intentionally does NOT contain:
''
''     - palette-mode coverage
''     - sound initialization
''     - user input or gameplay logic
''

#include once "fbgfx.bi"

const TEST_WIDTH = 360
const TEST_HEIGHT = 240
const HOLD_SECONDS = 12
dim marker_file as integer
dim framebuffer as uinteger ptr
dim scanline_offset as integer
dim primary_colour as uinteger
dim elapsed_seconds as double
dim current_time as double
dim start_time as double
dim x as integer
dim y as integer

screenres TEST_WIDTH, TEST_HEIGHT, 32
if screenptr = 0 then
	end 1
end if

''
'' Primary colour layout
''
''     Sample points near the bottom of each region avoid the labels.  The
''     intended RGB values are therefore unambiguous in captured output.
''
''     The AROS backend presents when the graphics lock is released.  Direct
''     framebuffer writes test the same 0xFFRRGGBB RGB() layout the backend
''     converts, without relying on a separate drawing-primitive path.
''
screenlock
framebuffer = screenptr
if framebuffer = 0 then
    screenunlock
    end 1
end if

for y = 0 to TEST_HEIGHT - 1
    scanline_offset = y * TEST_WIDTH
    for x = 0 to TEST_WIDTH - 1
        if x < 120 then
            primary_colour = rgb(255, 0, 0)
        elseif x < 240 then
            primary_colour = rgb(0, 255, 0)
        else
            primary_colour = rgb(0, 0, 255)
        end if
        framebuffer[scanline_offset + x] = primary_colour
    next x
next y

draw string (34, 18), "RED", rgb(255, 255, 255)
draw string (145, 18), "GREEN", rgb(255, 255, 255)
draw string (275, 18), "BLUE", rgb(255, 255, 255)
screenunlock
sleep 0

''
'' The host test runner waits for this file before taking its screenshot.  It
'' is written only after all three rectangles have reached the framebuffer.
''
marker_file = freefile
open "GfxSmoke:gfx-colour-smoke.drawn" for output as #marker_file
print #marker_file, "AROS_GFX_COLOUR_SMOKE: DRAWN"
close #marker_file

''
'' AROS can deliver a window activation event immediately after SCREENRES.
'' Use SLEEP 0 only to keep gfxlib2's presentation and event processing alive;
'' the TIMER loop, rather than an interruptible sleep, owns the hold period.
''
start_time = timer
do
    sleep 0
    current_time = timer
    elapsed_seconds = current_time - start_time
    if elapsed_seconds < 0 then
        elapsed_seconds += 86400
    end if
loop until elapsed_seconds >= HOLD_SECONDS
screen 0

'' end of gfx-colour-smoke.bas
