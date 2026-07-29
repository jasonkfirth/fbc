''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: screeninfo-desktop-win32-smoke.bas
''
'' Purpose:
''
''     Verify SCREENINFO preserves gfxlib2's desktop-query behavior before
''     a graphics mode has been opened.
''
'' Responsibilities:
''
''     - compare width, height, depth, and refresh with Win32 DEVMODE
''     - require zero framebuffer bpp and pitch without an active SCREEN
''     - require an empty driver name before backend selection
''
'' This file intentionally does NOT contain:
''
''     - a SCREENRES mode or GPU backend test
''     - window creation or display-mode changes
''     - non-Win32 desktop enumeration
''
#ifndef GFX2_REFERENCE
    #ifndef __FB_GFXLIB3__
        #define __FB_GFXLIB3__
    #endif
#endif

#include once "fbgfx.bi"
#include once "windows.bi"

dim as DEVMODEA native_mode
dim as integer desktop_width, desktop_height, desktop_depth
dim as integer bytes_per_pixel, pitch, desktop_refresh
dim as string driver

native_mode.dmSize = sizeof( native_mode )
if EnumDisplaySettingsA( 0, ENUM_CURRENT_SETTINGS, @native_mode ) = 0 then _
    end 1

screeninfo desktop_width, desktop_height, desktop_depth, bytes_per_pixel, _
    pitch, desktop_refresh, driver
if desktop_width <> native_mode.dmPelsWidth then end 2
if desktop_height <> native_mode.dmPelsHeight then end 2
if desktop_depth <> native_mode.dmBitsPerPel then end 2
if desktop_refresh <> native_mode.dmDisplayFrequency then end 2
if bytes_per_pixel <> 0 orelse pitch <> 0 then end 3
if len( driver ) <> 0 then end 4

print "GFX3_SCREENINFO_DESKTOP_WIN32_PASS " & desktop_width & "x" & _
    desktop_height & "x" & desktop_depth & "@" & desktop_refresh
end 0

'' end of screeninfo-desktop-win32-smoke.bas
