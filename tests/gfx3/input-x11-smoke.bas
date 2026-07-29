''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: input-x11-smoke.bas
''
'' Purpose:
''
''     Verify the X11 native input adapter through unchanged FreeBASIC
''     keyboard, mouse, focus, and SCREENEVENT APIs.
''
'' Responsibilities:
''
''     - send deterministic X11 events from a separate Display connection
''     - check key, motion, wheel, and five-button translation
''     - verify pointer confinement is released and restored across focus
''     - check native Display and Window publication and close delivery
''
'' This file intentionally does NOT contain:
''
''     - physical keyboard or mouse interaction
''     - Win32 message handling
''     - line-input or graphical cursor tests
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"
#include once "X11/Xlib.bi"
#include once "X11/keysymdef.bi"

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#else
	const backend_flags = 0
#endif

type XTEST_FAKE_BUTTON_EVENT as function cdecl( byval as Display ptr, _
	byval as ulong, byval as long, byval as culong ) as long
type XTEST_FAKE_KEY_EVENT as function cdecl( byval as Display ptr, _
	byval as ulong, byval as long, byval as culong ) as long

sub drain_events()
	dim as fb.EVENT event
	while screenevent( @event )
	wend
end sub

function find_event( byval wanted as integer, byref event as fb.EVENT ) _
	as integer
	for attempt as integer = 1 to 64
		if screenevent( @event ) = 0 then
			''
			'' Both graphics libraries publish X11 messages asynchronously.
			'' Keep the test bounded while allowing the window thread to run.
			''
			sleep 1, 1
			continue for
		end if
		if event.type = wanted then return -1
	next
	return 0
end function

sub send_focus( byval display as Display ptr, byval native_window as Window, _
	byval event_type as integer )
	if event_type = FocusIn then
		XSetInputFocus( display, native_window, RevertToParent, CurrentTime )
	else
		XSetInputFocus( display, XDefaultRootWindow( display ), _
			RevertToParent, CurrentTime )
	end if
	XSync( display, false )
	sleep 40, 1
end sub

sub send_motion( byval display as Display ptr, byval native_window as Window, _
	byval x as integer, byval y as integer )
	XWarpPointer( display, None, native_window, 0, 0, 0, 0, x, y )
	XSync( display, false )
	sleep 40, 1
end sub

sub send_button( byval display as Display ptr, _
	byval fake_button as XTEST_FAKE_BUTTON_EVENT, _
	byval native_button as integer, byval pressed as integer )
	if fake_button( display, native_button, pressed, 0 ) = 0 then
		end 92
	end if
	XSync( display, false )
	sleep 40, 1
end sub

sub send_key( byval display as Display ptr, _
	byval fake_key as XTEST_FAKE_KEY_EVENT, byval keycode as KeyCode, _
	byval pressed as integer )
	if fake_key( display, keycode, pressed, 0 ) = 0 then
		end 93
	end if
	XSync( display, false )
	sleep 40, 1
end sub

if screenres( 96, 64, 32, 1, backend_flags ) <> 0 then end 1
screensync

dim as longint native_window_value, native_display_value
screencontrol fb.GET_WINDOW_HANDLE, native_window_value, native_display_value
if native_window_value = 0 orelse native_display_value = 0 then end 2
dim as Window native_window = cast( Window, native_window_value )
dim as Display ptr test_display = XOpenDisplay( 0 )
if test_display = 0 then end 3
dim as Window root_window = XDefaultRootWindow( test_display )
dim as any ptr xtst_library = dylibload( "libXtst.so.6" )
if xtst_library = 0 then end 3
dim as XTEST_FAKE_BUTTON_EVENT fake_button = _
	dylibsymbol( xtst_library, "XTestFakeButtonEvent" )
dim as XTEST_FAKE_KEY_EVENT fake_key = _
	dylibsymbol( xtst_library, "XTestFakeKeyEvent" )
if fake_button = 0 orelse fake_key = 0 then end 3

drain_events()
send_focus( test_display, native_window, FocusOut )
screensync
drain_events()
send_focus( test_display, native_window, FocusIn )
screensync
dim as fb.EVENT event
if find_event( fb.EVENT_WINDOW_GOT_FOCUS, event ) = 0 then end 4

send_motion( test_display, native_window, 10, 11 )
screensync
drain_events()
send_motion( test_display, native_window, 14, 15 )
screensync
if find_event( fb.EVENT_MOUSE_MOVE, event ) = 0 then end 5
if event.x <> 14 orelse event.y <> 15 then end 6

send_button( test_display, fake_button, 8, true )
screensync
if find_event( fb.EVENT_MOUSE_BUTTON_PRESS, event ) = 0 then end 7
if event.button <> fb.BUTTON_X1 then end 8
send_button( test_display, fake_button, 8, false )
screensync
if find_event( fb.EVENT_MOUSE_BUTTON_RELEASE, event ) = 0 then end 9
if event.button <> fb.BUTTON_X1 then end 10

drain_events()
send_button( test_display, fake_button, 10, true )
screensync
while screenevent( @event )
	if event.type = fb.EVENT_MOUSE_BUTTON_PRESS then end 11
wend
send_button( test_display, fake_button, 10, false )
screensync
drain_events()

send_button( test_display, fake_button, Button4, true )
screensync
if find_event( fb.EVENT_MOUSE_WHEEL, event ) = 0 then end 12
if event.z <> 1 then end 13
send_button( test_display, fake_button, Button4, false )
send_button( test_display, fake_button, 7, true )
screensync
if find_event( fb.EVENT_MOUSE_HWHEEL, event ) = 0 then end 14
if event.w <> 1 then end 15
send_button( test_display, fake_button, 7, false )
screensync
drain_events()

dim as KeyCode a_keycode = XKeysymToKeycode( test_display, XK_a_ )
if a_keycode = 0 then end 16
send_key( test_display, fake_key, a_keycode, true )
screensync
if find_event( fb.EVENT_KEY_PRESS, event ) = 0 then end 17
if event.scancode <> fb.SC_A then end 18
if multikey( fb.SC_A ) = 0 then end 19
if inkey() <> "a" then end 20
send_key( test_display, fake_key, a_keycode, false )
screensync
if find_event( fb.EVENT_KEY_RELEASE, event ) = 0 then end 21
if multikey( fb.SC_A ) <> 0 then end 22

if setmouse( 20, 21, 1, 1 ) <> 0 then end 23
screensync
dim as integer grab_result = XGrabPointer( test_display, root_window, false, _
	PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None, CurrentTime )
if grab_result <> AlreadyGrabbed then end 24

send_focus( test_display, native_window, FocusOut )
screensync
if find_event( fb.EVENT_WINDOW_LOST_FOCUS, event ) = 0 then end 25
grab_result = XGrabPointer( test_display, root_window, false, _
	PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None, CurrentTime )
if grab_result <> GrabSuccess then end 26
XUngrabPointer( test_display, CurrentTime )
XSync( test_display, false )

send_focus( test_display, native_window, FocusIn )
screensync
if find_event( fb.EVENT_WINDOW_GOT_FOCUS, event ) = 0 then end 27
grab_result = XGrabPointer( test_display, root_window, false, _
	PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None, CurrentTime )
if grab_result <> AlreadyGrabbed then end 28
if setmouse( 20, 21, 1, 0 ) <> 0 then end 29
screensync
grab_result = XGrabPointer( test_display, root_window, false, _
	PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None, CurrentTime )
if grab_result <> GrabSuccess then end 30
XUngrabPointer( test_display, CurrentTime )
XSync( test_display, false )

dim as XEvent close_event
close_event.xclient.type = ClientMessage
close_event.xclient.display = test_display
close_event.xclient.window = native_window
close_event.xclient.format = 32
close_event.xclient.data.l( 0 ) = XInternAtom( test_display, _
	"WM_DELETE_WINDOW", false )
if XSendEvent( test_display, native_window, false, NoEventMask, _
	@close_event ) = 0 then end 31
XSync( test_display, false )
sleep 40, 1
screensync
if find_event( fb.EVENT_WINDOW_CLOSE, event ) = 0 then end 32
if inkey() <> chr( &hFF, asc( "k" ) ) then end 33

XCloseDisplay( test_display )
dylibfree( xtst_library )
screen 0
end 0

'' end of input-x11-smoke.bas
