''
'' Project: FreeBASIC graphics runtime tests
'' -----------------------------------------
''
'' File: fixed-screen-maximize-smoke.bas
''
'' Purpose:
''
''     Verify fixed logical screens maximize through integer presentation
''     scaling without changing BASIC-visible framebuffer dimensions.
''
'' Responsibilities:
''
''     - verify the normal framed window exposes a maximize button
''     - maximize and restore a fixed 160 by 120 logical screen
''     - sample whole-pixel GPU scaling and black native letterboxing
''     - verify SETMOUSE and GETMOUSE use logical framebuffer coordinates
''     - prove fixed-screen maximize does not post a logical resize event
''
'' This file intentionally does NOT contain:
''
''     - GFX_RESIZABLE framebuffer migration checks
''     - renderer-specific pixel readback
''     - interactive test instructions
''

#ifdef GFX3_TEST
	#define __FB_GFXLIB3__
#endif
#include once "fbgfx.bi"
#include once "windows.bi"
#include once "crt/stdio.bi"

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#else
	const backend_flags = 0
#endif

function native_color( byval device_context as HDC, byval x as integer, _
	byval y as integer ) as ulong
	dim as COLORREF value = GetPixel( device_context, x, y )

	if value = CLR_INVALID then return &hFFFFFFFFul
	return culng( GetRValue( value ) ) shl 16 or _
		culng( GetGValue( value ) ) shl 8 or culng( GetBValue( value ) )
end function

function native_color_matches( byval window_context as HDC, _
	byval desktop_context as HDC, byref client_origin as POINT, _
	byval x as integer, byval y as integer, byval expected as ulong, _
	byref window_color as ulong, byref desktop_color as ulong ) as integer
	window_color = native_color( window_context, x, y )
	desktop_color = native_color( desktop_context, client_origin.x + x, _
		client_origin.y + y )
	return iif( window_color = expected orelse desktop_color = expected, _
		-1, 0 )
end function

function wait_native_color_matches( byval window_context as HDC, _
	byval desktop_context as HDC, byref client_origin as POINT, _
	byval x as integer, byval y as integer, byval expected as ulong, _
	byref window_color as ulong, byref desktop_color as ulong ) as integer
	for attempt as integer = 1 to 200
		if native_color_matches( window_context, desktop_context, _
		    client_origin, x, y, expected, window_color, desktop_color ) then
			return -1
		end if
		screensync
		sleep 5, 1
	next
	return 0
end function

sub settle_presentation()
	for attempt as integer = 1 to 120
		screensync
		sleep 5, 1
	next
end sub

if screenres( 160, 120, 32, 1, backend_flags ) <> 0 then end 1
windowtitle "FreeBASIC fixed-screen maximize smoke"

line ( 0, 0 )-( 159, 119 ), rgb( 16, 80, 192 ), bf
line ( 0, 0 )-( 3, 3 ), rgb( 255, 0, 0 ), bf
line ( 156, 116 )-( 159, 119 ), rgb( 0, 255, 0 ), bf
''
'' Keep this marker entirely in the deferred PSET batch. Its small block makes
'' native compositor capture reliable while still requiring SCREENSYNC to
'' submit the point packet before presentation.
''
for y as integer = 59 to 62
	for x as integer = 78 to 81
		pset ( x, y ), rgb( 255, 0, 255 )
	next
next
settle_presentation

dim as HWND native_window = FindWindow( 0, _
	strptr( "FreeBASIC fixed-screen maximize smoke" ) )
if native_window = 0 then end 2

dim as LONG_PTR style = GetWindowLongPtr( native_window, GWL_STYLE )
if ( style and WS_MAXIMIZEBOX ) = 0 then end 3
if ( style and WS_THICKFRAME ) <> 0 then end 4

dim event as fb.EVENT
while screenevent( @event )
wend

ShowWindow( native_window, SW_MAXIMIZE )
SetForegroundWindow( native_window )
BringWindowToTop( native_window )
settle_presentation

dim client as RECT
if GetClientRect( native_window, @client ) = 0 then end 5
dim as integer client_width = client.right - client.left
dim as integer client_height = client.bottom - client.top
if client_width <= 160 orelse client_height <= 120 then end 6

dim as integer logical_width, logical_height, depth, bytes_per_pixel
dim as integer pitch, refresh_rate
dim as string driver
screeninfo logical_width, logical_height, depth, bytes_per_pixel, pitch, _
	refresh_rate, driver
if logical_width <> 160 orelse logical_height <> 120 then end 7
if pitch <> 640 then end 8

dim as integer scale_x = client_width \ logical_width
dim as integer scale_y = client_height \ logical_height
dim as integer scale = iif( scale_x < scale_y, scale_x, scale_y )
if scale < 2 then end 9
dim as integer scaled_width = logical_width * scale
dim as integer scaled_height = logical_height * scale
dim as integer offset_x = ( client_width - scaled_width ) \ 2
dim as integer offset_y = ( client_height - scaled_height ) \ 2
if offset_x = 0 andalso offset_y = 0 then end 10
fprintf( stderr, !"layout %dx%d -> %dx%d at %d,%d scale %d\n", _
	clng( logical_width ), clng( logical_height ), clng( scaled_width ), _
	clng( scaled_height ), clng( offset_x ), clng( offset_y ), clng( scale ) )

dim client_origin as POINT
if ClientToScreen( native_window, @client_origin ) = 0 then end 11
dim as HDC window_context = GetDC( native_window )
dim as HDC desktop_context = GetDC( 0 )
dim as ulong observed, observed_desktop
if window_context = 0 orelse desktop_context = 0 then end 12
if offset_x > 0 then
	if wait_native_color_matches( window_context, desktop_context, _
	    client_origin, offset_x \ 2, client_height \ 2, &h000000ul, observed, _
	    observed_desktop ) = 0 then
		fprintf( stderr, _
			!"letterbox got window %08X, desktop %08X, expected 00000000\n", _
			observed, observed_desktop )
		ReleaseDC( native_window, window_context )
		ReleaseDC( 0, desktop_context )
		end 13
	end if
else
	if wait_native_color_matches( window_context, desktop_context, _
	    client_origin, client_width \ 2, offset_y \ 2, &h000000ul, observed, _
	    observed_desktop ) = 0 then
		fprintf( stderr, _
			!"letterbox got window %08X, desktop %08X, expected 00000000\n", _
			observed, observed_desktop )
		ReleaseDC( native_window, window_context )
		ReleaseDC( 0, desktop_context )
		end 14
	end if
end if
if wait_native_color_matches( window_context, desktop_context, _
    client_origin, offset_x + scale + scale \ 2, _
    offset_y + scale + scale \ 2, _
    &hFF0000ul, observed, observed_desktop ) = 0 then
	fprintf( stderr, _
		!"top-left got window %08X, desktop %08X, expected 00FF0000\n", _
		observed, observed_desktop )
	ReleaseDC( native_window, window_context )
	ReleaseDC( 0, desktop_context )
	end 15
end if
if wait_native_color_matches( window_context, desktop_context, _
    client_origin, offset_x + scaled_width - scale - 1 - scale \ 2, _
    offset_y + scaled_height - scale - 1 - scale \ 2, &h00FF00ul, _
    observed, observed_desktop ) = 0 then
	fprintf( stderr, _
		!"bottom-right got window %08X, desktop %08X, expected 0000FF00\n", _
		observed, observed_desktop )
	ReleaseDC( native_window, window_context )
	ReleaseDC( 0, desktop_context )
	end 16
end if
if wait_native_color_matches( window_context, desktop_context, _
    client_origin, client_width \ 2, client_height \ 2, &hFF00FFul, observed, _
    observed_desktop ) = 0 then
	fprintf( stderr, _
		!"centre got window %08X, desktop %08X, expected 00FF00FF\n", _
		observed, observed_desktop )
	ReleaseDC( native_window, window_context )
	ReleaseDC( 0, desktop_context )
	end 17
end if
ReleaseDC( native_window, window_context )
ReleaseDC( 0, desktop_context )

dim native_mouse as POINT
dim as integer native_mouse_matched = 0
for attempt as integer = 1 to 400
	if setmouse( 37, 41, 1, 0 ) <> 0 then end 18
	screensync
	sleep 5, 1
	if GetCursorPos( @native_mouse ) = 0 then end 19
	if ScreenToClient( native_window, @native_mouse ) = 0 then end 20
	if native_mouse.x = offset_x + 37 * scale + scale \ 2 andalso _
	    native_mouse.y = offset_y + 41 * scale + scale \ 2 then
		native_mouse_matched = -1
		exit for
	end if
next
if native_mouse_matched = 0 then
	if native_mouse.x <> offset_x + 37 * scale + scale \ 2 then end 21
	end 22
end if
if PostMessage( native_window, WM_MOUSEMOVE, 0, _
    makelong( native_mouse.x, native_mouse.y ) ) = 0 then end 23
dim as integer mouse_x, mouse_y, mouse_wheel, mouse_buttons
dim as integer mouse_result = -1
dim as integer logical_mouse_matched = 0
for attempt as integer = 1 to 400
	screensync
	sleep 5, 1
	mouse_result = getmouse( mouse_x, mouse_y, mouse_wheel, mouse_buttons )
	if mouse_result = 0 andalso mouse_x = 37 andalso mouse_y = 41 then
		logical_mouse_matched = -1
		exit for
	end if
next
if mouse_result <> 0 then end 24
if logical_mouse_matched = 0 then
	fprintf( stderr, !"logical mouse got %d,%d, expected 37,41\n", _
		clng( mouse_x ), clng( mouse_y ) )
	end 25
end if

while screenevent( @event )
	if event.type = fb.EVENT_WINDOW_RESIZE then end 26
wend

ShowWindow( native_window, SW_RESTORE )
settle_presentation
if GetClientRect( native_window, @client ) = 0 then end 27
if ( client.right - client.left ) <> 160 then end 28
if ( client.bottom - client.top ) <> 120 then end 29
screeninfo logical_width, logical_height, depth, bytes_per_pixel, pitch, _
	refresh_rate, driver
if logical_width <> 160 orelse logical_height <> 120 orelse pitch <> 640 then _
	end 30

print "PASS "; driver; " fixed 160x120 -> "; client_width; "x"; client_height; _
	" scale "; scale
screen 0
end 0

'' end of fixed-screen-maximize-smoke.bas
