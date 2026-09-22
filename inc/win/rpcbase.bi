'' FreeBASIC binding for the common RPC declarations
''
'' File: rpcbase.bi
''
'' Purpose:
''   Provide the small set of RPC handle and status types shared by the
''   generated RPC header bindings.
''
'' Responsibilities:
''   * Define the opaque RPC handles used in function signatures.
''   * Define the UUID alias used by the RPC declarations.
''
'' This file intentionally does not contain RPC procedures, protocol
'' constants, or generated interface descriptions.

#pragma once

#include once "windows.bi"
#include once "guiddef.bi"

extern "Windows"

#ifndef __FB_RPC_BASE_TYPES_DEFINED
#define __FB_RPC_BASE_TYPES_DEFINED
type I_RPC_HANDLE as any ptr
type RPC_STATUS as long
type RPC_CSTR as ubyte ptr
type RPC_WSTR as ushort ptr
type RPC_BINDING_HANDLE as I_RPC_HANDLE
type RPC_IF_HANDLE as any ptr
type RPC_NS_HANDLE as any ptr
type _RPC_HTTP_REDIRECTOR_STAGE as long
enum
	RPCHTTP_RS_REDIRECT = 1
	RPCHTTP_RS_ACCESS_1
	RPCHTTP_RS_SESSION
	RPCHTTP_RS_ACCESS_2
	RPCHTTP_RS_INTERFACE
end enum
type RPC_HTTP_REDIRECTOR_STAGE as _RPC_HTTP_REDIRECTOR_STAGE
#ifndef UUID_DEFINED
#define UUID_DEFINED
type UUID as GUID
#endif
#endif

end extern

'' end of rpcbase.bi
