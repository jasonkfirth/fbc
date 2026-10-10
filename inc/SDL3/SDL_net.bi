'' Project: FreeBASIC SDL3 bindings
'' File: SDL_net.bi
'' Purpose: Declare the SDL3_net-3.2.0 C interface for FreeBASIC.
'' Responsibilities: Preserve public types, constants, and calling conventions.
'' This file intentionally does NOT contain: the upstream library implementation.
''
'' Translated from the upstream headers; this is an altered source version.
'' Regenerate with build_scripts/generate-sdl3-bindings.py.
''
'' Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>
''
'' This software is provided 'as-is', without any express or implied
'' warranty.  In no event will the authors be held liable for any damages
'' arising from the use of this software.
''
'' Permission is granted to anyone to use this software for any purpose,
'' including commercial applications, and to alter it and redistribute it
'' freely, subject to the following restrictions:
''
'' 1. The origin of this software must not be misrepresented; you must not
''    claim that you wrote the original software. If you use this software
''    in a product, an acknowledgment in the product documentation would be
''    appreciated but is not required.
'' 2. Altered source versions must be plainly marked as such, and must not be
''    misrepresented as being the original software.
'' 3. This notice may not be removed or altered from any source distribution.

#pragma once

#inclib "SDL3_net"

#include once "SDL.bi"


extern "C"

type NET_Address as NET_Address_
type NET_DatagramSocket as NET_DatagramSocket_
type NET_Server as NET_Server_
type NET_StreamSocket as NET_StreamSocket_

#define SDL_NET_H_
const SDL_NET_MAJOR_VERSION as long = 3
const SDL_NET_MINOR_VERSION as long = 2
const SDL_NET_MICRO_VERSION as long = 0
#define SDL_NET_VERSION SDL_VERSIONNUM(SDL_NET_MAJOR_VERSION, SDL_NET_MINOR_VERSION, SDL_NET_MICRO_VERSION)
#define SDL_NET_VERSION_ATLEAST(X, Y, Z) (((SDL_NET_MAJOR_VERSION >= X) andalso ((SDL_NET_MAJOR_VERSION > X) orelse (SDL_NET_MINOR_VERSION >= Y))) andalso (((SDL_NET_MAJOR_VERSION > X) orelse (SDL_NET_MINOR_VERSION > Y)) orelse (SDL_NET_MICRO_VERSION >= Z)))
declare function NET_Version() as long

type NET_Status as long
enum
	NET_FAILURE = -1
	NET_WAITING = 0
	NET_SUCCESS = 1
end enum

declare function NET_Init() as boolean
declare sub NET_Quit()
declare function NET_ResolveHostname(byval host as const zstring ptr) as NET_Address ptr
declare function NET_WaitUntilResolved(byval address as NET_Address ptr, byval timeout as Sint32) as NET_Status
declare function NET_GetAddressStatus(byval address as NET_Address ptr) as NET_Status
declare function NET_GetAddressString(byval address as NET_Address ptr) as const zstring ptr
declare function NET_GetAddressBytes(byval address as NET_Address ptr, byval num_bytes as long ptr) as const any ptr
declare function NET_RefAddress(byval address as NET_Address ptr) as NET_Address ptr
declare sub NET_UnrefAddress(byval address as NET_Address ptr)
declare sub NET_SimulateAddressResolutionLoss(byval percent_loss as long)
declare function NET_CompareAddresses(byval a as const NET_Address ptr, byval b as const NET_Address ptr) as long
declare function NET_GetLocalAddresses(byval num_addresses as long ptr) as NET_Address ptr ptr
declare sub NET_FreeLocalAddresses(byval addresses as NET_Address ptr ptr)
declare function NET_CreateClient(byval address as NET_Address ptr, byval port as Uint16, byval props as SDL_PropertiesID) as NET_StreamSocket ptr
declare function NET_WaitUntilConnected(byval sock as NET_StreamSocket ptr, byval timeout as Sint32) as NET_Status
declare function NET_CreateServer(byval addr as NET_Address ptr, byval port as Uint16, byval props as SDL_PropertiesID) as NET_Server ptr
#define NET_PROP_SERVER_REUSEADDR_BOOLEAN "NET.server.reuseaddr"
declare function NET_AcceptClient(byval server as NET_Server ptr, byval client_stream as NET_StreamSocket ptr ptr) as boolean
declare sub NET_DestroyServer(byval server as NET_Server ptr)
declare function NET_GetStreamSocketAddress(byval sock as NET_StreamSocket ptr) as NET_Address ptr
declare function NET_GetConnectionStatus(byval sock as NET_StreamSocket ptr) as NET_Status
declare function NET_WriteToStreamSocket(byval sock as NET_StreamSocket ptr, byval buf as const any ptr, byval buflen as long) as boolean
declare function NET_GetStreamSocketPendingWrites(byval sock as NET_StreamSocket ptr) as long
declare function NET_WaitUntilStreamSocketDrained(byval sock as NET_StreamSocket ptr, byval timeout as Sint32) as long
declare function NET_ReadFromStreamSocket(byval sock as NET_StreamSocket ptr, byval buf as any ptr, byval buflen as long) as long
declare sub NET_SimulateStreamPacketLoss(byval sock as NET_StreamSocket ptr, byval percent_loss as long)
declare sub NET_DestroyStreamSocket(byval sock as NET_StreamSocket ptr)

type NET_Datagram
	addr as NET_Address ptr
	port as Uint16
	buf as Uint8 ptr
	buflen as long
end type

declare function NET_CreateDatagramSocket(byval addr as NET_Address ptr, byval port as Uint16, byval props as SDL_PropertiesID) as NET_DatagramSocket ptr
#define NET_PROP_DATAGRAM_SOCKET_REUSEADDR_BOOLEAN "NET.datagram_socket.reuseaddr"
#define NET_PROP_DATAGRAM_SOCKET_ALLOW_BROADCAST_BOOLEAN "NET.datagram_socket.allow_broadcast"
declare function NET_SendDatagram(byval sock as NET_DatagramSocket ptr, byval address as NET_Address ptr, byval port as Uint16, byval buf as const any ptr, byval buflen as long) as boolean
declare function NET_ReceiveDatagram(byval sock as NET_DatagramSocket ptr, byval dgram as NET_Datagram ptr ptr) as boolean
declare sub NET_DestroyDatagram(byval dgram as NET_Datagram ptr)
declare sub NET_SimulateDatagramPacketLoss(byval sock as NET_DatagramSocket ptr, byval percent_loss as long)
declare sub NET_DestroyDatagramSocket(byval sock as NET_DatagramSocket ptr)
declare function NET_WaitUntilInputAvailable(byval vsockets as any ptr ptr, byval numsockets as long, byval timeout as Sint32) as long

end extern

'' end of SDL_net.bi
