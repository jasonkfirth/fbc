'' Project: FreeBASIC SDL addon examples
'' File: net2.bas
'' Purpose: Exchange a UDP packet through loopback with SDL2_net.
'' Responsibilities: Use an ephemeral local port, check packet data, and close the socket.
'' This file intentionally does NOT contain: public networking or listening threads.

#define SDL_ADDON_API 2
#include once "../../SDL_common/example-common.bi"
#include once "SDL2/SDL_net.bi"

function main() as integer
	if SDL_Init(0) <> 0 then return 1
	if SDLNet_Init() <> 0 then
		SDL_Quit()
		return 1
	end if
	dim exitStatus as integer = 1
	dim sock as UDPsocket = SDLNet_UDP_Open(0)
	dim packet as UDPpacket ptr = SDLNet_AllocPacket(4)
	do
		if sock = 0 or packet = 0 then exit do
		'' Channel -1 returns the socket's own address. Its port is stored in
		'' network byte order, whereas ResolveHost accepts a host-order port.
		dim localAddress as IPaddress ptr = SDLNet_UDP_GetPeerAddress(sock, -1)
		if localAddress = 0 then exit do
		if SDLNet_ResolveHost(@packet->address, "127.0.0.1", SDL_SwapBE16(localAddress->port)) <> 0 then exit do
		packet->len = 4
		for i as integer = 0 to 3
			packet->data[i] = 40 + i
		next
		if SDLNet_UDP_Send(sock, -1, packet) <> 1 then exit do
		packet->len = 0
		dim received as long
		dim started as Uint32 = SDL_GetTicks()
		do
			received = SDLNet_UDP_Recv(sock, packet)
			if received <> 0 then exit do
			SDL_Delay(10)
		loop while SDL_GetTicks() - started < 5000u
		if received <> 1 or packet->len <> 4 then exit do
		dim validPacket as boolean = true
		for i as integer = 0 to 3
			if packet->data[i] <> 40 + i then validPacket = false
		next
		if not validPacket then exit do
		print "Loopback bytes: "; packet->len
		exitStatus = 0
	loop while false
	if exitStatus <> 0 then example_error("UDP packet exchange failed")
	if packet <> 0 then SDLNet_FreePacket(packet)
	if sock <> 0 then SDLNet_UDP_Close(sock)
	SDLNet_Quit()
	SDL_Quit()
	return exitStatus
end function

end main()

'' End of net2.bas
