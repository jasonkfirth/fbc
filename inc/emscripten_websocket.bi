''
'' Project: FreeBASIC JavaScript target
'' -----------------------------------
''
'' File: emscripten_websocket.bi
''
'' Purpose:
''
''     Provide the FreeBASIC declarations for Emscripten's browser WebSocket
''     API.
''
'' Responsibilities:
''
''     * create WebSocket connections from JavaScript-target programs
''     * register open, message, error and close callbacks
''     * send text or binary data and release socket handles
''
'' This file intentionally does NOT contain:
''
''     * a portable socket API
''     * a WebSocket protocol implementation
''     * connection retry or application message framing policy
''
'' The functions declared here are supplied by Emscripten. They are available
'' only when fbc-js links a browser or Node.js program. Browser callbacks run
'' asynchronously, so a program must remain active after creating its socket.
'' Add -Wl -lwebsocket to the fbc-js command line so Emscripten links its
'' optional WebSocket implementation.
''

#pragma once

#ifndef __FB_JS__
	#error "emscripten_websocket.bi requires the FreeBASIC JavaScript target"
#endif

#include once "crt/stddef.bi"

extern "C"

'' Values returned by Emscripten WebSocket functions.
const EMSCRIPTEN_RESULT_SUCCESS             = 0
const EMSCRIPTEN_RESULT_DEFERRED            = 1
const EMSCRIPTEN_RESULT_NOT_SUPPORTED       = -1
const EMSCRIPTEN_RESULT_FAILED_NOT_DEFERRED = -2
const EMSCRIPTEN_RESULT_INVALID_TARGET      = -3
const EMSCRIPTEN_RESULT_UNKNOWN_TARGET      = -4
const EMSCRIPTEN_RESULT_INVALID_PARAM       = -5
const EMSCRIPTEN_RESULT_FAILED              = -6
const EMSCRIPTEN_RESULT_NO_DATA             = -7

'' WebSocket.readyState values from the browser WebSocket interface.
const EMSCRIPTEN_WEBSOCKET_CONNECTING = 0
const EMSCRIPTEN_WEBSOCKET_OPEN       = 1
const EMSCRIPTEN_WEBSOCKET_CLOSING    = 2
const EMSCRIPTEN_WEBSOCKET_CLOSED     = 3

''
'' Emscripten uses an integer table handle, not a native socket descriptor.
'' A positive handle identifies a connected or connecting WebSocket. A negative
'' result from emscripten_websocket_new() is an EMSCRIPTEN_RESULT_* failure.
''
type EMSCRIPTEN_WEBSOCKET_T as long

type EmscriptenWebSocketOpenEvent
	socket as EMSCRIPTEN_WEBSOCKET_T
end type

type EmscriptenWebSocketMessageEvent
	socket   as EMSCRIPTEN_WEBSOCKET_T
	data     as ubyte ptr
	numBytes as uinteger
	isText   as ubyte
end type

type EmscriptenWebSocketErrorEvent
	socket as EMSCRIPTEN_WEBSOCKET_T
end type

'' The close reason is UTF-8 and is always NUL-terminated by Emscripten.
type EmscriptenWebSocketCloseEvent
	socket   as EMSCRIPTEN_WEBSOCKET_T
	wasClean as ubyte
	code     as ushort
	reason   as zstring * 512
end type

''
'' The URL and protocol strings are copied while emscripten_websocket_new()
'' runs. They therefore may refer to temporary FreeBASIC strings. Set
'' createOnMainThread when a socket must outlive the creating worker thread.
''
type EmscriptenWebSocketCreateAttributes
	url                 as const zstring ptr
	protocols           as const zstring ptr
	createOnMainThread  as ubyte
end type

type em_websocket_open_callback_func as function( _
	byval eventType as long, _
	byval websocketEvent as const EmscriptenWebSocketOpenEvent ptr, _
	byval userData as any ptr _
) as ubyte

type em_websocket_message_callback_func as function( _
	byval eventType as long, _
	byval websocketEvent as const EmscriptenWebSocketMessageEvent ptr, _
	byval userData as any ptr _
) as ubyte

type em_websocket_error_callback_func as function( _
	byval eventType as long, _
	byval websocketEvent as const EmscriptenWebSocketErrorEvent ptr, _
	byval userData as any ptr _
) as ubyte

type em_websocket_close_callback_func as function( _
	byval eventType as long, _
	byval websocketEvent as const EmscriptenWebSocketCloseEvent ptr, _
	byval userData as any ptr _
) as ubyte

declare function emscripten_websocket_is_supported alias "emscripten_websocket_is_supported" () as ubyte
declare function emscripten_websocket_new alias "emscripten_websocket_new" ( _
	byval createAttributes as EmscriptenWebSocketCreateAttributes ptr _
) as EMSCRIPTEN_WEBSOCKET_T

declare function emscripten_websocket_get_ready_state alias "emscripten_websocket_get_ready_state" ( _
	byval socket as EMSCRIPTEN_WEBSOCKET_T, _
	byval readyState as ushort ptr _
) as long
declare function emscripten_websocket_get_buffered_amount alias "emscripten_websocket_get_buffered_amount" ( _
	byval socket as EMSCRIPTEN_WEBSOCKET_T, _
	byval bufferedAmount as size_t ptr _
) as long
declare function emscripten_websocket_get_url alias "emscripten_websocket_get_url" ( _
	byval socket as EMSCRIPTEN_WEBSOCKET_T, _
	byval url as zstring ptr, _
	byval urlLength as long _
) as long
declare function emscripten_websocket_get_url_length alias "emscripten_websocket_get_url_length" ( _
	byval socket as EMSCRIPTEN_WEBSOCKET_T, _
	byval urlLength as long ptr _
) as long
declare function emscripten_websocket_get_extensions alias "emscripten_websocket_get_extensions" ( _
	byval socket as EMSCRIPTEN_WEBSOCKET_T, _
	byval extensions as zstring ptr, _
	byval extensionsLength as long _
) as long
declare function emscripten_websocket_get_extensions_length alias "emscripten_websocket_get_extensions_length" ( _
	byval socket as EMSCRIPTEN_WEBSOCKET_T, _
	byval extensionsLength as long ptr _
) as long
declare function emscripten_websocket_get_protocol alias "emscripten_websocket_get_protocol" ( _
	byval socket as EMSCRIPTEN_WEBSOCKET_T, _
	byval protocol as zstring ptr, _
	byval protocolLength as long _
) as long
declare function emscripten_websocket_get_protocol_length alias "emscripten_websocket_get_protocol_length" ( _
	byval socket as EMSCRIPTEN_WEBSOCKET_T, _
	byval protocolLength as long ptr _
) as long

declare function emscripten_websocket_send_utf8_text alias "emscripten_websocket_send_utf8_text" ( _
	byval socket as EMSCRIPTEN_WEBSOCKET_T, _
	byval textData as const zstring ptr _
) as long
declare function emscripten_websocket_send_binary alias "emscripten_websocket_send_binary" ( _
	byval socket as EMSCRIPTEN_WEBSOCKET_T, _
	byval binaryData as any ptr, _
	byval dataLength as uinteger _
) as long
declare function emscripten_websocket_close alias "emscripten_websocket_close" ( _
	byval socket as EMSCRIPTEN_WEBSOCKET_T, _
	byval code as ushort, _
	byval reason as const zstring ptr _
) as long
declare function emscripten_websocket_delete alias "emscripten_websocket_delete" ( _
	byval socket as EMSCRIPTEN_WEBSOCKET_T _
) as long
declare sub emscripten_websocket_deinitialize alias "emscripten_websocket_deinitialize" ()

''
'' Emscripten's short callback forms are C macros. The generated C program
'' cannot use those macros directly, so these declarations target the exported
'' on-thread functions and select the thread that installed the callback.
''
declare function emscripten_websocket_set_onopen_callback_on_thread alias "emscripten_websocket_set_onopen_callback_on_thread" ( _
	byval socket as EMSCRIPTEN_WEBSOCKET_T, _
	byval userData as any ptr, _
	byval callback as em_websocket_open_callback_func, _
	byval targetThread as size_t _
) as long
declare function emscripten_websocket_set_onmessage_callback_on_thread alias "emscripten_websocket_set_onmessage_callback_on_thread" ( _
	byval socket as EMSCRIPTEN_WEBSOCKET_T, _
	byval userData as any ptr, _
	byval callback as em_websocket_message_callback_func, _
	byval targetThread as size_t _
) as long
declare function emscripten_websocket_set_onerror_callback_on_thread alias "emscripten_websocket_set_onerror_callback_on_thread" ( _
	byval socket as EMSCRIPTEN_WEBSOCKET_T, _
	byval userData as any ptr, _
	byval callback as em_websocket_error_callback_func, _
	byval targetThread as size_t _
) as long
declare function emscripten_websocket_set_onclose_callback_on_thread alias "emscripten_websocket_set_onclose_callback_on_thread" ( _
	byval socket as EMSCRIPTEN_WEBSOCKET_T, _
	byval userData as any ptr, _
	byval callback as em_websocket_close_callback_func, _
	byval targetThread as size_t _
) as long

'' Emscripten defines this callback context as pthread_t 0x2.
const EM_CALLBACK_THREAD_CONTEXT_CALLING_THREAD = 2

#macro emscripten_websocket_set_onopen_callback(socket, userData, callback)
	emscripten_websocket_set_onopen_callback_on_thread( socket, userData, callback, EM_CALLBACK_THREAD_CONTEXT_CALLING_THREAD )
#endmacro

#macro emscripten_websocket_set_onmessage_callback(socket, userData, callback)
	emscripten_websocket_set_onmessage_callback_on_thread( socket, userData, callback, EM_CALLBACK_THREAD_CONTEXT_CALLING_THREAD )
#endmacro

#macro emscripten_websocket_set_onerror_callback(socket, userData, callback)
	emscripten_websocket_set_onerror_callback_on_thread( socket, userData, callback, EM_CALLBACK_THREAD_CONTEXT_CALLING_THREAD )
#endmacro

#macro emscripten_websocket_set_onclose_callback(socket, userData, callback)
	emscripten_websocket_set_onclose_callback_on_thread( socket, userData, callback, EM_CALLBACK_THREAD_CONTEXT_CALLING_THREAD )
#endmacro

end extern

'' end of emscripten_websocket.bi
