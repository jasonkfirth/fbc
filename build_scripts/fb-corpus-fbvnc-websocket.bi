/'
    Project: FreeBASIC browser corpus
    File: fb-corpus-fbvnc-websocket.bi

    Purpose:
        Implement the fb-VNC transport primitives for the browser build.

    Responsibilities:
        Preserve the viewer's nonblocking read and disconnect contracts.
        Yield to the browser during connection setup. RFB and UI code live
        in the original viewer modules.
'/

extern "C"
	declare function VncWsOpen alias "fb_vnc_ws_open"( byval address as const zstring ptr ) as long
	declare sub VncWsClose alias "fb_vnc_ws_close"( byval handle as long )
	declare function VncWsState alias "fb_vnc_ws_state"( byval handle as long ) as long
	declare function VncWsAvailable alias "fb_vnc_ws_available"( byval handle as long ) as long
	declare function VncWsRead alias "fb_vnc_ws_read"( byval handle as long, byval buffer as ubyte ptr, byval length as long ) as long
	declare function VncWsWrite alias "fb_vnc_ws_write"( byval handle as long, byval buffer as const ubyte ptr, byval length as long ) as long
end extern

' These values follow the browser WebSocket readyState contract.
const VNC_WS_CONNECTING = 0
const VNC_WS_OPEN = 1
const VNC_WS_CLOSING = 2
const VNC_WS_CONNECT_TIMEOUT_MS = 10000

function NetOpen( byref host as const string, byval port as integer, byref handle as integer, byref errorText as string ) as integer
	/' UiParseServer passes the complete WebSocket URL in host. '/
	errorText = ""
	handle = VncWsOpen( strptr( host ) )
	if( handle = 0 ) then
		errorText = "Could not open the WebSocket URL."
		return 0
	end if

	dim as double startedAt = timer
	while( VncWsState( handle ) = VNC_WS_CONNECTING )
		if( ElapsedMilliseconds( startedAt ) >= VNC_WS_CONNECT_TIMEOUT_MS ) then exit while
		sleep 10, 1
	wend
	if( VncWsState( handle ) <> VNC_WS_OPEN ) then
		NetClose( handle )
		errorText = "The WebSocket connection failed or timed out."
		return 0
	end if
	return -1
end function

sub NetClose( byref handle as integer )
	if( handle <> 0 ) then
		VncWsClose( handle )
		handle = 0
	end if
end sub

function NetHasData( byval handle as integer ) as integer
	return VncWsAvailable( handle ) > 0
end function

function NetConnectionEnded( byval handle as integer ) as integer
	if( NetHasData( handle ) ) then return 0
	dim as long connectionState = VncWsState( handle )
	return connectionState < 0 orelse connectionState >= VNC_WS_CLOSING
end function

function NetReadSomeNow( byval handle as integer, byval buffer as ubyte ptr, byval maximumBytes as integer, byref bytesRead as integer, byref errorText as string ) as integer
	bytesRead = 0
	errorText = ""
	if( NetConnectionEnded( handle ) ) then
		errorText = "The WebSocket connection closed or rejected incoming data."
		return 0
	end if
	dim as long received = VncWsRead( handle, buffer, maximumBytes )
	if( received < 0 ) then
		errorText = "The WebSocket receive buffer could not be read."
		return 0
	end if
	bytesRead = received
	return -1
end function

private function NetWriteChunk( byval handle as integer, byval buffer as const ubyte ptr, byval count as integer, byref errorText as string ) as integer
	if( VncWsWrite( handle, buffer, count ) = 0 ) then
		errorText = "The WebSocket send failed or the outgoing queue is full."
		return 0
	end if
	return -1
end function

/' end of fb-corpus-fbvnc-websocket.bi '/
