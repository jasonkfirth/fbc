''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: fullscreen-x11-smoke.bas
''
'' Purpose:
''
''     Verify that the X11 presentation adapter applies the public fullscreen
''     and no-frame flags without requesting a persistent XRandR mode change.
''
'' Responsibilities:
''
''     - require fullscreen client bounds to match the current X11 root
''     - require GFX_NO_FRAME to preserve the requested client dimensions
''     - exercise the public screenres flag path for a GPU backend
''
'' This file intentionally does NOT contain:
''
''     - XRandR display-mode changes
''     - decoration policy assertions owned by a particular window manager
''     - Win32, Android, Wayland, or multi-monitor behavior
''
#include once "fbgfx3.bi"
#include once "X11/Xlib.bi"

#ifdef GFX3_FULLSCREEN_VULKAN
	const renderer_flags = fb.GFX_VULKAN
	const renderer_name = "Vulkan"
#else
	const renderer_flags = fb.GFX_OPENGL
	const renderer_name = "OpenGL"
#endif

function client_size_matches( byval display as Display ptr, _
	byval window_handle as Window, byval expected_width as integer, _
	byval expected_height as integer ) as integer
	dim as XWindowAttributes attributes

	if XGetWindowAttributes( display, window_handle, @attributes ) = 0 then
		return 0
	end if
	return attributes.width = expected_width andalso _
		attributes.height = expected_height
end function

function wait_for_client_size( byval display as Display ptr, _
	byval window_handle as Window, byval expected_width as integer, _
	byval expected_height as integer ) as integer
	for attempt as integer = 1 to 50
		screensync
		if client_size_matches( display, window_handle, expected_width, _
			expected_height ) then return -1
		sleep 20, 1
	next
	return 0
end function

if screenres( 96, 64, 32, 1, renderer_flags or fb.GFX_FULLSCREEN ) <> 0 then end 1
screensync

dim as longint native_window_value, native_display_value
screencontrol fb.GET_WINDOW_HANDLE, native_window_value, native_display_value
if native_window_value = 0 orelse native_display_value = 0 then end 2
dim as Window window_handle = cast( Window, native_window_value )
dim as Display ptr display = XOpenDisplay( 0 )
if display = 0 then end 3
dim as integer screen_number = XDefaultScreen( display )
if wait_for_client_size( display, window_handle, XDisplayWidth( display, _
	screen_number ), XDisplayHeight( display, screen_number ) ) = 0 then end 4
screen 0

if screenres( 96, 64, 32, 1, renderer_flags or fb.GFX_NO_FRAME ) <> 0 then end 5
screensync
screencontrol fb.GET_WINDOW_HANDLE, native_window_value, native_display_value
if native_window_value = 0 orelse native_display_value = 0 then end 6
window_handle = cast( Window, native_window_value )
if wait_for_client_size( display, window_handle, 96, 64 ) = 0 then end 7
XCloseDisplay( display )
screen 0

print "GFX3_FULLSCREEN_X11_PASS " & renderer_name
end 0

'' end of fullscreen-x11-smoke.bas
