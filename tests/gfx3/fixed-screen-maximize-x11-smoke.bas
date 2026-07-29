''
'' Project: FreeBASIC graphics runtime tests
'' -----------------------------------------
''
'' File: fixed-screen-maximize-x11-smoke.bas
''
'' Purpose:
''
''     Verify fixed logical screen maximize through an EWMH X11 window
''     manager without changing BASIC-visible framebuffer dimensions.
''
'' Responsibilities:
''
''     - request maximize and restore through _NET_WM_STATE
''     - capture integer-scaled pixels and black native letterboxing
''     - verify SETMOUSE and GETMOUSE use logical framebuffer coordinates
''     - prove fixed maximize does not post a logical resize event
''
'' This file intentionally does NOT contain:
''
''     - GFX_RESIZABLE framebuffer migration checks
''     - bare-X-server behavior without an EWMH window manager
''     - Win32, Android, Wayland, or multi-monitor behavior
''

#ifdef GFX3_TEST
	#define __FB_GFXLIB3__
#endif
#include once "fbgfx.bi"
#include once "X11/Xlib.bi"
#include once "X11/Xutil.bi"

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#elseif defined( GFX3_OPENGL_TEST )
	const backend_flags = fb.GFX_OPENGL
#else
	const backend_flags = 0
#endif

function request_maximize( byval display as Display ptr, _
	byval root_window as Window, byval native_window as Window, _
	byval add_state as integer ) as integer
	dim as XEvent event

	event.xclient.type = ClientMessage
	event.xclient.display = display
	event.xclient.window = native_window
	event.xclient.message_type = XInternAtom( display, "_NET_WM_STATE", false )
	event.xclient.format = 32
	event.xclient.data.l( 0 ) = iif( add_state, 1, 0 )
	event.xclient.data.l( 1 ) = XInternAtom( display, _
		"_NET_WM_STATE_MAXIMIZED_VERT", false )
	event.xclient.data.l( 2 ) = XInternAtom( display, _
		"_NET_WM_STATE_MAXIMIZED_HORZ", false )
	event.xclient.data.l( 3 ) = 1
	event.xclient.data.l( 4 ) = 0
	if XSendEvent( display, root_window, false, _
	    SubstructureRedirectMask or SubstructureNotifyMask, @event ) = 0 then
		return 0
	end if
	XFlush( display )
	return -1
end function

function wait_for_client( byval display as Display ptr, _
	byval native_window as Window, byval maximized as integer, _
	byref client_width as integer, byref client_height as integer ) as integer
	dim as XWindowAttributes attributes

	for attempt as integer = 1 to 400
		screensync
		if XGetWindowAttributes( display, native_window, @attributes ) <> 0 then
			client_width = attributes.width
			client_height = attributes.height
			if maximized then
				if client_width > 160 andalso client_height > 120 then return -1
			else
				if client_width = 160 andalso client_height = 120 then return -1
			end if
		end if
		sleep 5, 1
	next
	return 0
end function

function native_color( byval display as Display ptr, _
	byval native_window as Window, byval colormap as Colormap, _
	byval x as integer, byval y as integer ) as ulong
	dim as XImage ptr image
	dim as XColor native_xcolor

	image = XGetImage( display, native_window, x, y, 1, 1, AllPlanes, ZPixmap )
	if image = 0 then return &hFFFFFFFFul
	native_xcolor.pixel = XGetPixel( image, 0, 0 )
	XDestroyImage( image )
	if XQueryColor( display, colormap, @native_xcolor ) = 0 then _
		return &hFFFFFFFFul
	return culng( native_xcolor.red shr 8 ) shl 16 or _
		culng( native_xcolor.green shr 8 ) shl 8 or _
		culng( native_xcolor.blue shr 8 )
end function

sub settle_presentation()
	for attempt as integer = 1 to 120
		screensync
		sleep 5, 1
	next
end sub

if screenres( 160, 120, 32, 1, backend_flags ) <> 0 then end 1
windowtitle "FreeBASIC fixed-screen X11 maximize smoke"
line ( 0, 0 )-( 159, 119 ), rgb( 16, 80, 192 ), bf
line ( 0, 0 )-( 3, 3 ), rgb( 255, 0, 0 ), bf
line ( 156, 116 )-( 159, 119 ), rgb( 0, 255, 0 ), bf
''
'' Keep this marker entirely in the deferred PSET batch. Its small block makes
'' X11 front-buffer capture reliable while still requiring SCREENSYNC to
'' submit the point packet before presentation.
''
for y as integer = 59 to 62
	for x as integer = 78 to 81
		pset ( x, y ), rgb( 255, 0, 255 )
	next
next
settle_presentation

dim as longint native_window_value, native_display_value
screencontrol fb.GET_WINDOW_HANDLE, native_window_value, native_display_value
if native_window_value = 0 orelse native_display_value = 0 then end 2
dim as Window native_window = cast( Window, native_window_value )
dim as Display ptr display = XOpenDisplay( 0 )
if display = 0 then end 3
dim as Window root_window = XDefaultRootWindow( display )

dim event as fb.EVENT
while screenevent( @event )
wend
if request_maximize( display, root_window, native_window, -1 ) = 0 then end 4

dim as integer client_width, client_height
if wait_for_client( display, native_window, -1, client_width, _
	client_height ) = 0 then end 5

dim as integer logical_width, logical_height, depth, bytes_per_pixel
dim as integer pitch, refresh_rate
dim as string driver
screeninfo logical_width, logical_height, depth, bytes_per_pixel, pitch, _
	refresh_rate, driver
if logical_width <> 160 orelse logical_height <> 120 then end 6
if pitch <> 640 then end 7

dim as integer scale_x = client_width \ logical_width
dim as integer scale_y = client_height \ logical_height
dim as integer scale = iif( scale_x < scale_y, scale_x, scale_y )
if scale < 2 then end 8
dim as integer scaled_width = logical_width * scale
dim as integer scaled_height = logical_height * scale
dim as integer offset_x = ( client_width - scaled_width ) \ 2
dim as integer offset_y = ( client_height - scaled_height ) \ 2
if offset_x = 0 andalso offset_y = 0 then end 9

dim as XWindowAttributes attributes
if XGetWindowAttributes( display, native_window, @attributes ) = 0 then end 10
if offset_x > 0 then
	if native_color( display, native_window, attributes.colormap, _
	    offset_x \ 2, client_height \ 2 ) <> 0 then end 11
else
	if native_color( display, native_window, attributes.colormap, _
	    client_width \ 2, offset_y \ 2 ) <> 0 then end 12
end if
if native_color( display, native_window, attributes.colormap, _
    offset_x + scale + scale \ 2, offset_y + scale + scale \ 2 ) <> _
    &hFF0000ul then end 13
if native_color( display, native_window, attributes.colormap, _
    offset_x + scaled_width - scale - 1 - scale \ 2, _
    offset_y + scaled_height - scale - 1 - scale \ 2 ) <> _
    &h00FF00ul then end 14
if native_color( display, native_window, attributes.colormap, _
    client_width \ 2, client_height \ 2 ) <> &hFF00FFul then end 15

if setmouse( 37, 41, 1, 0 ) <> 0 then end 16
settle_presentation
dim as Window root_return, child_return
dim as long root_x, root_y, window_x, window_y
dim as ulong pointer_mask
if XQueryPointer( display, native_window, @root_return, @child_return, _
    @root_x, @root_y, @window_x, @window_y, @pointer_mask ) = 0 then end 17
if window_x <> offset_x + 37 * scale + scale \ 2 then end 18
if window_y <> offset_y + 41 * scale + scale \ 2 then end 19

XWarpPointer( display, None, native_window, 0, 0, 0, 0, window_x, window_y )
XSync( display, false )
dim as integer mouse_x, mouse_y, mouse_wheel, mouse_buttons
dim as integer mouse_result = -1
for attempt as integer = 1 to 400
	screensync
	sleep 5, 1
	mouse_result = getmouse( mouse_x, mouse_y, mouse_wheel, mouse_buttons )
	if mouse_result = 0 andalso mouse_x = 37 andalso mouse_y = 41 then exit for
next
if mouse_result <> 0 then end 20
if mouse_x <> 37 orelse mouse_y <> 41 then end 21

while screenevent( @event )
	if event.type = fb.EVENT_WINDOW_RESIZE then end 22
wend

if request_maximize( display, root_window, native_window, 0 ) = 0 then end 23
if wait_for_client( display, native_window, 0, client_width, _
	client_height ) = 0 then end 24
screeninfo logical_width, logical_height, depth, bytes_per_pixel, pitch, _
	refresh_rate, driver
if logical_width <> 160 orelse logical_height <> 120 orelse pitch <> 640 then _
	end 25

XCloseDisplay( display )
print "PASS "; driver; " fixed X11 scale "; scale
screen 0
end 0

'' end of fixed-screen-maximize-x11-smoke.bas
