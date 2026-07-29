''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: screenlist-x11-smoke.bas
''
'' Purpose:
''
''     Verify that an X11/RandR desktop contributes its current native mode
''     to the public SCREENLIST iterator.
''
'' Responsibilities:
''
''     - obtain the active X11 desktop dimensions through SCREENCONTROL
''     - require the corresponding 32-bit mode exactly once in SCREENLIST
''     - use the public iterator rather than a private XRandR declaration
''
'' This file intentionally does NOT contain:
''
''     - RandR mode changes or fullscreen policy tests
''     - assumptions about how many modes a desktop exposes
''     - Win32, Android, or Wayland behavior
''
#include once "fbgfx3.bi"

dim as integer desktop_width, desktop_height
dim as integer expected_mode, entry, matches

if screenres( 48, 32, 32, 1, fb.GFX_OPENGL ) <> 0 then end 1
screencontrol fb.GET_DESKTOP_SIZE, desktop_width, desktop_height
screen 0

if desktop_width <= 0 orelse desktop_width > &h7FFF then end 2
if desktop_height <= 0 orelse desktop_height > &hFFFF then end 2
expected_mode = (desktop_width shl 16) or desktop_height

entry = screenlist( 32 )
while entry <> 0
    if entry = expected_mode then matches += 1
    entry = screenlist()
wend
if matches <> 1 then end 3

print "GFX3_SCREENLIST_X11_PASS " & desktop_width & "x" & desktop_height
end 0

'' end of screenlist-x11-smoke.bas
