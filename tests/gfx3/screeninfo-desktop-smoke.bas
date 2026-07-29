''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: screeninfo-desktop-smoke.bas
''
'' Purpose:
''
''     Verify that SCREENINFO has useful desktop information before a graphics
''     mode creates a renderer or framebuffer.
''
'' Responsibilities:
''
''     - require positive native width, height, and depth
''     - require zero framebuffer bpp and pitch before SCREENRES
''     - require an empty driver string before backend selection
''
'' This file intentionally does NOT contain:
''
''     - platform-specific desktop dimension expectations
''     - graphics mode creation or GPU drawing
''     - refresh-rate policy requirements
''
#ifndef __FB_GFXLIB3__
    #define __FB_GFXLIB3__
#endif

#include once "fbgfx.bi"

dim as integer desktop_width, desktop_height, desktop_depth
dim as integer bytes_per_pixel, pitch, desktop_refresh
dim as string driver

screeninfo desktop_width, desktop_height, desktop_depth, bytes_per_pixel, _
    pitch, desktop_refresh, driver
if desktop_width <= 0 orelse desktop_height <= 0 orelse _
    desktop_depth <= 0 then end 1
if bytes_per_pixel <> 0 orelse pitch <> 0 then end 2
if len( driver ) <> 0 then end 3

print "GFX3_SCREENINFO_DESKTOP_PASS " & desktop_width & "x" & _
    desktop_height & "x" & desktop_depth
end 0

'' end of screeninfo-desktop-smoke.bas
