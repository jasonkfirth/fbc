''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: screenlist-win32-smoke.bas
''
'' Purpose:
''
''     Verify Win32 SCREENLIST contains a mode reported by the active desktop.
''
'' Responsibilities:
''
''     - query the current Win32 DEVMODE without changing it
''     - start the public SCREENLIST iterator at that mode's depth
''     - require the matching packed width/height entry exactly once
''
'' This file intentionally does NOT contain:
''
''     - fullscreen or display-mode changes
''     - standard-table fallback expectations
''     - non-Win32 display enumeration
''

#include once "fbgfx3.bi"
#include once "windows.bi"

dim as DEVMODEA native_mode
dim as integer requested_depth, expected, entry, matches

native_mode.dmSize = sizeof( native_mode )
if EnumDisplaySettingsA( 0, ENUM_CURRENT_SETTINGS, @native_mode ) = 0 then _
    end 1
if native_mode.dmPelsWidth > &h7FFF orelse native_mode.dmPelsHeight > &hFFFF _
    then end 2
requested_depth = native_mode.dmBitsPerPel
if requested_depth = 15 then requested_depth = 16
if requested_depth = 24 then requested_depth = 32
if requested_depth <> 16 andalso requested_depth <> 32 then end 3
expected = (native_mode.dmPelsWidth shl 16) or native_mode.dmPelsHeight

entry = screenlist( requested_depth )
while entry <> 0
    if entry = expected then matches += 1
    entry = screenlist()
wend
if matches <> 1 then end 4

print "GFX3_SCREENLIST_WIN32_PASS " & native_mode.dmPelsWidth & "x" & _
    native_mode.dmPelsHeight & "x" & native_mode.dmBitsPerPel
end 0

'' end of screenlist-win32-smoke.bas
