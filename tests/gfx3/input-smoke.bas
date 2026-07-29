''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: input-smoke.bas
''
'' Purpose:
''
''     Verify the Win32 native input adapter through unchanged FreeBASIC
''     keyboard, mouse, and SCREENEVENT APIs.
''
'' Responsibilities:
''
''     - inject deterministic native window messages into a visible mode
''     - check event queue, key queue, and MULTIKEY state transitions
''     - check GETMOUSE button, wheel, coordinate, and SETMOUSE behavior
''     - check native handle, desktop, and window-position SCREENCONTROL paths
''     - check close-event and KEY_QUIT compatibility
''
'' This file intentionally does NOT contain:
''
''     - physical keyboard or mouse interaction
''     - non-Win32 platform input checks
''     - line-input or graphical cursor tests
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"
#include once "windows.bi"

const native_title = "gfxlib3 input smoke"

extern "C"
	declare function fb_KeyHit alias "fb_KeyHit"() as long
end extern

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#else
	const backend_flags = 0
#endif

sub drain_events()
	dim as fb.EVENT event
	while screenevent( @event )
	wend
end sub

sub drain_keys()
	while fb_KeyHit() <> 0
		dim as string ignored = inkey()
	wend
end sub

function find_event( byval wanted as integer, byref event as fb.EVENT ) _
	as integer
	for attempt as integer = 1 to 32
		if screenevent( @event ) = 0 then
			''
			'' gfxlib2 and gfxlib3 publish native messages from their window
			'' threads. Give that thread a bounded opportunity to run instead of
			'' assuming SCREENEVENT itself pumps the operating-system queue.
			''
			sleep 1, 1
			continue for
		end if
		if event.type = wanted then return -1
	next
	return 0
end function

if screenres( 96, 64, 32, 1, backend_flags ) <> 0 then end 1
windowtitle native_title
screensync

dim as HWND native_window = FindWindow( 0, strptr( native_title ) )
if native_window = 0 then end 2

dim as longint control_window, control_display
screencontrol fb.GET_WINDOW_HANDLE, control_window, control_display
if control_window <> cast( longint, native_window ) then end 2
if control_display <> 0 then end 2
dim as integer desktop_width, desktop_height
screencontrol fb.GET_DESKTOP_SIZE, desktop_width, desktop_height
if desktop_width <= 0 orelse desktop_height <= 0 then end 2
dim as integer original_window_x, original_window_y
screencontrol fb.GET_WINDOW_POS, original_window_x, original_window_y
dim as integer moved_window_x = original_window_x + 1
dim as integer moved_window_y = original_window_y + 1
screencontrol fb.SET_WINDOW_POS, moved_window_x, moved_window_y
screencontrol fb.GET_WINDOW_POS, moved_window_x, moved_window_y
if moved_window_x <> original_window_x + 1 then end 2
if moved_window_y <> original_window_y + 1 then end 2
screencontrol fb.SET_WINDOW_POS, original_window_x, original_window_y

'' Force a stable focused state so the test does not depend on the desktop's
'' foreground-window policy while an automated runner launches the program.
PostMessage( native_window, WM_ACTIVATE, makelong( WA_ACTIVE, 0 ), 0 )
screensync
drain_events()

dim as fb.EVENT event
dim as LPARAM key_data = 1 or (MapVirtualKey( VK_A, 0 ) shl 16)
PostMessage( native_window, WM_KEYDOWN, VK_A, key_data )
''
'' A real keyboard press supplies both messages. PostMessage does not update
'' Win32's physical keyboard-state table, so whether TranslateMessage also
'' synthesizes WM_CHAR for this artificial keydown is not a stable test input.
'' Check the scancode event first, discard any synthesized character, then
'' inject exactly one WM_CHAR to exercise KEYHIT and INKEY deterministically.
''
if find_event( fb.EVENT_KEY_PRESS, event ) = 0 then end 3
if event.scancode <> fb.SC_A then end 4
if multikey( fb.SC_A ) = 0 then end 5
drain_keys()
PostMessage( native_window, WM_CHAR, asc( "a" ), 1 )
screensync
if fb_KeyHit() = 0 then end 6

dim as string key = inkey()
if key <> "a" then end 7

PostMessage( native_window, WM_KEYDOWN, VK_A, _
	key_data or &h40000000 )
if find_event( fb.EVENT_KEY_REPEAT, event ) = 0 then end 8
drain_keys()
PostMessage( native_window, WM_CHAR, asc( "a" ), 1 )
screensync
if inkey() <> "a" then end 9

PostMessage( native_window, WM_KEYUP, VK_A, key_data or &hC0000000 )
if find_event( fb.EVENT_KEY_RELEASE, event ) = 0 then end 10
if multikey( fb.SC_A ) <> 0 then end 11
PostMessage( native_window, WM_CHAR, asc( "b" ), 1 )
if getkey() <> asc( "b" ) then end 12

'' Graphics SLEEP must notice a queued key without consuming it.
PostMessage( native_window, WM_CHAR, asc( "s" ), 1 )
dim as double sleep_start = timer
sleep 1000
if timer - sleep_start > 0.75 then end 12
if inkey() <> "s" then end 12
sleep 0

SetForegroundWindow( native_window )
dim as POINT native_mouse = ( 10, 11 )
ClientToScreen( native_window, @native_mouse )
SetCursorPos( native_mouse.x, native_mouse.y )
sleep 20, 1
PostMessage( native_window, WM_ACTIVATE, makelong( WA_ACTIVE, 0 ), 0 )
if setmouse( 10, 11, 1, 0 ) <> 0 then end 13
screensync
drain_events()
PostMessage( native_window, WM_MOUSEMOVE, 0, makelong( 10, 11 ) )
PostMessage( native_window, WM_MOUSEMOVE, 0, makelong( 15, 14 ) )
if find_event( fb.EVENT_MOUSE_MOVE, event ) = 0 then end 14
if event.x <> 15 orelse event.y <> 14 then end 15
if event.dx <> 5 orelse event.dy <> 3 then end 16

dim as integer mouse_x, mouse_y, mouse_z, mouse_buttons, mouse_clip
if getmouse( mouse_x, mouse_y, mouse_z, mouse_buttons, mouse_clip ) <> 0 _
	then end 17
if mouse_x < 0 orelse mouse_x >= 96 then end 18
if mouse_y < 0 orelse mouse_y >= 64 then end 18

PostMessage( native_window, WM_LBUTTONDOWN, MK_LBUTTON, _
	makelong( 15, 14 ) )
if find_event( fb.EVENT_MOUSE_BUTTON_PRESS, event ) = 0 then end 19
if event.button <> fb.BUTTON_LEFT then end 20
if getmouse( mouse_x, mouse_y, mouse_z, mouse_buttons, mouse_clip ) <> 0 _
	then end 21
if (mouse_buttons and fb.BUTTON_LEFT) = 0 then end 22
if gettouchcount() <> 1 then end 22
dim as integer touch_x, touch_y, touch_id
if gettouch( 0, touch_x, touch_y, touch_id ) <> 0 then end 22
if touch_x <> mouse_x orelse touch_y <> mouse_y orelse touch_id <> 0 _
	then end 22
if gettouchhit( mouse_x - 1, mouse_y - 1, mouse_x + 1, mouse_y + 1 ) = 0 _
	then end 22
if gettouchhit( mouse_x, mouse_y, 1 ) = 0 then end 22

PostMessage( native_window, WM_LBUTTONUP, 0, makelong( 15, 14 ) )
if find_event( fb.EVENT_MOUSE_BUTTON_RELEASE, event ) = 0 then end 23
if gettouchcount() <> 0 then end 23
PostMessage( native_window, WM_MOUSEWHEEL, _
	makelong( 0, WHEEL_DELTA ), 0 )
if find_event( fb.EVENT_MOUSE_WHEEL, event ) = 0 then end 24
if event.z <> 1 then end 25
PostMessage( native_window, &h020E, _
	makelong( 0, WHEEL_DELTA ), 0 )
if find_event( fb.EVENT_MOUSE_HWHEEL, event ) = 0 then end 26
if event.w <> 1 then end 27

if setmouse( 20, 21, 1, 0 ) <> 0 then end 28
if getmouse( mouse_x, mouse_y, mouse_z, mouse_buttons, mouse_clip ) <> 0 _
	then end 29
if mouse_x <> 20 orelse mouse_y <> 21 then end 30
if mouse_z <> 1 orelse mouse_clip <> 0 then end 31

drain_events()
PostMessage( native_window, WM_ACTIVATE, WA_INACTIVE, 0 )
if find_event( fb.EVENT_WINDOW_LOST_FOCUS, event ) = 0 then end 32
PostMessage( native_window, WM_ACTIVATE, makelong( WA_ACTIVE, 0 ), 0 )
if find_event( fb.EVENT_WINDOW_GOT_FOCUS, event ) = 0 then end 33

drain_events()
PostMessage( native_window, WM_CLOSE, 0, 0 )
sleep 80, 1
if IsWindowVisible( native_window ) <> 0 then end 34
if screenevent() = 0 then end 34
if find_event( fb.EVENT_WINDOW_CLOSE, event ) = 0 then end 35
if inkey() <> chr( &hFF, asc( "k" ) ) then end 36

screen 0
end 0

'' end of input-smoke.bas
