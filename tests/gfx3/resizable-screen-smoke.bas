''
'' Project: FreeBASIC graphics runtime tests
'' -----------------------------------------
''
'' File: resizable-screen-smoke.bas
''
'' Purpose:
''
''     Verify the opt-in resizable SCREEN contract shared by gfxlib2 and
''     gfxlib3 on Win32.
''
'' Responsibilities:
''
''     - resize and maximize a framed graphics window through Win32
''     - verify completed EVENT_WINDOW_RESIZE dimensions
''     - verify SCREENINFO and SCREENCONTROL track the logical framebuffer
''     - verify every screen page preserves its overlapping top-left image
''     - verify newly exposed pixels are initialized to black
''
'' This file intentionally does NOT contain:
''
''     - interactive resize instructions
''     - backend-specific framebuffer access
''     - non-Win32 window-system calls
''

#ifdef GFX3_TEST
	#define __FB_GFXLIB3__
#endif
#include once "fbgfx.bi"
#include once "windows.bi"

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_RESIZABLE or fb.GFX_VULKAN
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
windowtitle "FreeBASIC resizable SCREEN smoke"
screensync

dim as HWND native_window = FindWindow( 0, _
	strptr( "FreeBASIC resizable SCREEN smoke" ) )
if native_window = 0 then end 2

screenset 0, 0
pset ( 10, 10 ), rgb( 255, 0, 0 )
screenset 1, 0
pset ( 11, 11 ), rgb( 0, 255, 0 )
screenset 0, 0
screensync

if SetWindowPos( native_window, 0, 0, 0, 500, 360, _
	SWP_NOMOVE or SWP_NOZORDER ) = 0 then end 3

dim as fb.EVENT event
if wait_for_resize( event ) = 0 then end 4
if event.width <= 160 orelse event.height <= 120 then end 5

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
if ( point( logical_width - 1, logical_height - 1 ) and &hFFFFFF ) <> 0 then end 11
screenset 1, 0
if ( point( 11, 11 ) and &hFFFFFF ) <> &h00FF00 then end 12
if ( point( logical_width - 1, logical_height - 1 ) and &hFFFFFF ) <> 0 then end 13

while screenevent( @event )
wend
screenlock
dim as any ptr locked_pointer = screenptr
if locked_pointer = 0 then end 14
if PostMessage( native_window, WM_SIZE, SIZE_RESTORED, _
	makelong( logical_width + 32, logical_height + 24 ) ) = 0 then end 15
sleep 100, 1
if screenptr <> locked_pointer then end 16
screenunlock
if wait_for_resize( event ) = 0 then end 17
screeninfo logical_width, logical_height, depth, bytes_per_pixel, pitch, _
	refresh_rate, driver
if logical_width <> event.width orelse logical_height <> event.height then end 18
screenset 0, 0
if ( point( 10, 10 ) and &hFFFFFF ) <> &hFF0000 then end 19
screenset 1, 0
if ( point( 11, 11 ) and &hFFFFFF ) <> &h00FF00 then end 20

while screenevent( @event )
wend
if SetWindowPos( native_window, 0, 0, 0, 300, 240, _
	SWP_NOMOVE or SWP_NOZORDER ) = 0 then end 21
if wait_for_resize( event ) = 0 then end 22
if event.width >= logical_width orelse event.height >= logical_height then end 23
screeninfo logical_width, logical_height, depth, bytes_per_pixel, pitch, _
	refresh_rate, driver
if logical_width <> event.width orelse logical_height <> event.height then end 24
screenset 0, 0
if ( point( 10, 10 ) and &hFFFFFF ) <> &hFF0000 then end 25
screenset 1, 0
if ( point( 11, 11 ) and &hFFFFFF ) <> &h00FF00 then end 26

while screenevent( @event )
wend
dim as BOOL previous_visibility = ShowWindow( native_window, SW_MAXIMIZE )
if wait_for_resize( event ) = 0 then end 27
screencontrol fb.GET_SCREEN_SIZE, control_width, control_height
if control_width <> event.width orelse control_height <> event.height then end 28
if control_width < logical_width orelse control_height < logical_height then end 29

print "PASS "; driver; " "; control_width; "x"; control_height
screen 0
if screenres( 160, 120, 32, 1, _
	fb.GFX_RESIZABLE or fb.GFX_FULLSCREEN ) = 0 then end 30
end 0

'' end of resizable-screen-smoke.bas
