''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: fullscreen-win32-smoke.bas
''
'' Purpose:
''
''     Verify the Win32 presentation adapter applies the public fullscreen
''     and no-frame flags without changing the user's desktop display mode.
''
'' Responsibilities:
''
''     - require a fullscreen window to occupy its selected monitor
''     - require GFX_NO_FRAME to create a popup at the requested client size
''     - exercise the public screenres flag path for a GPU backend
''
'' This file intentionally does NOT contain:
''
''     - persistent ChangeDisplaySettings display-mode changes
''     - input, resize, or multi-monitor policy tests
''     - non-Win32 platform behavior
''
#include once "fbgfx3.bi"
#include once "windows.bi"

#ifdef GFX3_FULLSCREEN_VULKAN
    const renderer_flags = fb.GFX_VULKAN
    const renderer_name = "Vulkan"
#else
    const renderer_flags = fb.GFX_OPENGL
    const renderer_name = "OpenGL"
#endif

const title = "gfxlib3 fullscreen smoke"
dim as HWND window_handle
dim as MONITORINFO monitor_info
dim as RECT window_rect
dim as RECT client_rect

if screenres( 96, 64, 32, 1, renderer_flags or fb.GFX_FULLSCREEN ) <> 0 then end 1
windowtitle title
screensync
window_handle = FindWindow( 0, strptr( title ) )
if window_handle = 0 then end 2
if (GetWindowLongPtr( window_handle, GWL_STYLE ) and WS_POPUP) = 0 then end 3
monitor_info.cbSize = sizeof( monitor_info )
if GetMonitorInfo( MonitorFromWindow( window_handle, MONITOR_DEFAULTTOPRIMARY ), _
    @monitor_info ) = 0 then end 4
if GetWindowRect( window_handle, @window_rect ) = 0 then end 5
if window_rect.left <> monitor_info.rcMonitor.left then end 6
if window_rect.top <> monitor_info.rcMonitor.top then end 6
if window_rect.right <> monitor_info.rcMonitor.right then end 6
if window_rect.bottom <> monitor_info.rcMonitor.bottom then end 6
screen 0

if screenres( 96, 64, 32, 1, renderer_flags or fb.GFX_NO_FRAME ) <> 0 then end 7
windowtitle title
screensync
window_handle = FindWindow( 0, strptr( title ) )
if window_handle = 0 then end 8
if (GetWindowLongPtr( window_handle, GWL_STYLE ) and WS_POPUP) = 0 then end 9
if GetClientRect( window_handle, @client_rect ) = 0 then end 10
if client_rect.right - client_rect.left <> 96 then end 11
if client_rect.bottom - client_rect.top <> 64 then end 11
screen 0

print "GFX3_FULLSCREEN_WIN32_PASS " & renderer_name
end 0

'' end of fullscreen-win32-smoke.bas
