''
'' Project: FreeBASIC graphics runtime tests
'' -----------------------------------------
''
'' File: resizable-screen-x11-smoke.bas
''
'' Purpose:
''
''     Verify the opt-in resizable SCREEN contract shared by gfxlib2 and
''     gfxlib3 on X11.
''
'' Responsibilities:
''
''     - request deterministic client resizes through a second X11 connection
''     - verify completed EVENT_WINDOW_RESIZE dimensions
''     - verify logical size, pitch, page preservation, and black expansion
''     - verify SCREENLOCK defers replacement of direct framebuffer storage
''
'' This file intentionally does NOT contain:
''
''     - interactive resize instructions
''     - window-manager decoration assertions
''     - Win32, Android, or Wayland behavior
''

#ifdef GFX3_TEST
	#define __FB_GFXLIB3__
#endif
#include once "fbgfx.bi"
#include once "X11/Xlib.bi"

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_RESIZABLE or fb.GFX_VULKAN
#elseif defined( GFX3_OPENGL_TEST )
	const backend_flags = fb.GFX_RESIZABLE or fb.GFX_OPENGL
#else
	const backend_flags = fb.GFX_RESIZABLE
#endif

function wait_for_resize( byref event as fb.EVENT ) as integer
	for attempt as integer = 1 to 400
		while screenevent( @event )
			if event.type = fb.EVENT_WINDOW_RESIZE then return -1
		wend
		sleep 5, 1
	next
	return 0
end function

if screenres( 160, 120, 32, 2, backend_flags ) <> 0 then end 1
windowtitle "FreeBASIC resizable X11 smoke"
screensync

dim as longint native_window_value, native_display_value
screencontrol fb.GET_WINDOW_HANDLE, native_window_value, native_display_value
if native_window_value = 0 orelse native_display_value = 0 then end 2
dim as Window native_window = cast( Window, native_window_value )
dim as Display ptr test_display = XOpenDisplay( 0 )
if test_display = 0 then end 3

screenset 0, 0
pset ( 10, 10 ), rgb( 255, 0, 0 )
screenset 1, 0
pset ( 11, 11 ), rgb( 0, 255, 0 )
screenset 0, 0
screensync

XResizeWindow( test_display, native_window, 320, 240 )
XSync( test_display, false )

dim as fb.EVENT event
if wait_for_resize( event ) = 0 then end 4
if event.width <> 320 orelse event.height <> 240 then end 5

dim as integer logical_width, logical_height, depth, bytes_per_pixel
dim as integer pitch, refresh_rate
dim as string driver
screeninfo logical_width, logical_height, depth, bytes_per_pixel, pitch, _
	refresh_rate, driver
if logical_width <> event.width orelse logical_height <> event.height then end 6
if depth <> 32 orelse bytes_per_pixel <> 4 then end 7
if pitch <> logical_width * bytes_per_pixel then end 8

dim as integer control_width, control_height
screencontrol fb.GET_SCREEN_SIZE, control_width, control_height
if control_width <> logical_width orelse control_height <> logical_height then end 9

screenset 0, 0
if ( point( 10, 10 ) and &hFFFFFF ) <> &hFF0000 then end 10
if ( point( 319, 239 ) and &hFFFFFF ) <> 0 then end 11
screenset 1, 0
if ( point( 11, 11 ) and &hFFFFFF ) <> &h00FF00 then end 12
if ( point( 319, 239 ) and &hFFFFFF ) <> 0 then end 13

while screenevent( @event )
wend
screenlock
dim as any ptr locked_pointer = screenptr
if locked_pointer = 0 then end 14
XResizeWindow( test_display, native_window, 352, 264 )
XSync( test_display, false )
sleep 100, 1
if screenptr <> locked_pointer then end 15
screenunlock
if wait_for_resize( event ) = 0 then end 16
if event.width <> 352 orelse event.height <> 264 then end 17

screencontrol fb.GET_SCREEN_SIZE, control_width, control_height
if control_width <> 352 orelse control_height <> 264 then end 18
screenset 0, 0
if ( point( 10, 10 ) and &hFFFFFF ) <> &hFF0000 then end 19
screenset 1, 0
if ( point( 11, 11 ) and &hFFFFFF ) <> &h00FF00 then end 20

while screenevent( @event )
wend
XResizeWindow( test_display, native_window, 200, 150 )
XSync( test_display, false )
if wait_for_resize( event ) = 0 then end 21
if event.width <> 200 orelse event.height <> 150 then end 22
screencontrol fb.GET_SCREEN_SIZE, control_width, control_height
if control_width <> 200 orelse control_height <> 150 then end 23
screenset 0, 0
if ( point( 10, 10 ) and &hFFFFFF ) <> &hFF0000 then end 24
screenset 1, 0
if ( point( 11, 11 ) and &hFFFFFF ) <> &h00FF00 then end 25

XCloseDisplay( test_display )
print "PASS "; driver; " "; control_width; "x"; control_height
screen 0
if screenres( 160, 120, 32, 1, _
	fb.GFX_RESIZABLE or fb.GFX_FULLSCREEN ) = 0 then end 26
end 0

'' end of resizable-screen-x11-smoke.bas
