''
'' Project: FreeBASIC JavaScript target
'' -----------------------------------
''
'' File: emscripten-websocket.bas
''
'' Purpose:
''
''     Demonstrate a small browser WebSocket client using the optional
''     Emscripten binding.
''
'' Responsibilities:
''
''     * connect to a WebSocket echo service
''     * send one UTF-8 message after the connection opens
''     * report the asynchronous result and release the socket handle
''
'' This file intentionally does NOT contain:
''
''     * a production reconnect policy
''     * application protocol framing
''     * a WebSocket server implementation
''
'' Build this example with:
''
''     fbc-js -Wl -lwebsocket -x emscripten-websocket.html emscripten-websocket.bas
''
'' Start emscripten-websocket-host.js before running this client. Replace the
'' local URL below when connecting to another endpoint.
''

#include once "emscripten_websocket.bi"

dim shared as EMSCRIPTEN_WEBSOCKET_T socketHandle
dim shared as string messageText

function OnOpen( _
	byval eventType as long, _
	byval websocketEvent as const EmscriptenWebSocketOpenEvent ptr, _
	byval userData as any ptr _
) as ubyte
	print "WebSocket connected"

	if emscripten_websocket_send_utf8_text( websocketEvent->socket, strptr( messageText ) ) <> EMSCRIPTEN_RESULT_SUCCESS then
		print "Could not send WebSocket message"
	end if

	return 1
end function

function OnMessage( _
	byval eventType as long, _
	byval websocketEvent as const EmscriptenWebSocketMessageEvent ptr, _
	byval userData as any ptr _
) as ubyte
	if websocketEvent->isText = 0 then
		print "Received binary WebSocket data (" & websocketEvent->numBytes & " bytes)"
	else
		print "Received: " & *cptr( zstring ptr, websocketEvent->data )
	end if

	if emscripten_websocket_close( websocketEvent->socket, 1000, strptr( "done" ) ) <> EMSCRIPTEN_RESULT_SUCCESS then
		print "Could not close WebSocket"
	end if

	return 1
end function

function OnError( _
	byval eventType as long, _
	byval websocketEvent as const EmscriptenWebSocketErrorEvent ptr, _
	byval userData as any ptr _
) as ubyte
	print "WebSocket error"
	return 1
end function

function OnClose( _
	byval eventType as long, _
	byval websocketEvent as const EmscriptenWebSocketCloseEvent ptr, _
	byval userData as any ptr _
) as ubyte
	print "WebSocket closed: " & websocketEvent->code
	emscripten_websocket_delete( websocketEvent->socket )
	socketHandle = 0
	return 1
end function

dim as EmscriptenWebSocketCreateAttributes attributes
dim as string websocketUrl

messageText = "Hello from FreeBASIC"
websocketUrl = "ws://127.0.0.1:18087"

if emscripten_websocket_is_supported() = 0 then
	print "WebSockets are not supported by this JavaScript host"
	system 1
end if

attributes.url = strptr( websocketUrl )
attributes.protocols = 0
attributes.createOnMainThread = 0

print "Connecting to " & websocketUrl

socketHandle = emscripten_websocket_new( @attributes )
if socketHandle <= 0 then
	print "Could not create WebSocket: " & socketHandle
	system 1
end if

if emscripten_websocket_set_onopen_callback( socketHandle, 0, @OnOpen ) <> EMSCRIPTEN_RESULT_SUCCESS _
or emscripten_websocket_set_onmessage_callback( socketHandle, 0, @OnMessage ) <> EMSCRIPTEN_RESULT_SUCCESS _
or emscripten_websocket_set_onerror_callback( socketHandle, 0, @OnError ) <> EMSCRIPTEN_RESULT_SUCCESS _
or emscripten_websocket_set_onclose_callback( socketHandle, 0, @OnClose ) <> EMSCRIPTEN_RESULT_SUCCESS then
	print "Could not install WebSocket callbacks"
	emscripten_websocket_delete( socketHandle )
	system 1
end if

'' The callbacks run from the browser event loop. Sleep yields with Asyncify.
do while socketHandle <> 0
	sleep 50
loop

'' end of emscripten-websocket.bas
