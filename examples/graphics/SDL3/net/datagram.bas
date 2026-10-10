'' Project: FreeBASIC SDL3 examples
'' File: datagram.bas
'' Purpose: Port upstream SDL3_net-3.2.0/examples/datagram.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.

#include once "SDL3/SDL.bi"
#include once "SDL3/SDL_net.bi"

dim shared sock as NET_DatagramSocket ptr = cptr(NET_DatagramSocket ptr, 0)
'' you talk over this, client or server.
dim shared server_addr as NET_Address ptr = cptr(NET_Address ptr, 0)
'' address of the server you're talking to, NULL if you _are_ the server.
dim shared server_port as Uint16 = 9781

declare sub print_usage cdecl(byval prog as const zstring ptr)
declare sub run_datagram cdecl(byval argc as long, byval argv as zstring ptr ptr)
declare function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long

sub print_usage cdecl(byval prog as const zstring ptr)
	scope
		SDL_Log_(strptr("USAGE: %s <hostname|ip|-> [--help] [--server] [--port X] [--simulate-failure Y]"), prog)
	end scope
end sub

sub run_datagram cdecl(byval argc as long, byval argv as zstring ptr ptr)
	scope
		dim hostname as const zstring ptr = cptr(const zstring ptr, 0)
		dim is_server as boolean = false
		dim simulate_failure as long = 0
		dim i as long
		dim socket_address as NET_Address ptr = cptr(NET_Address ptr, 0)
		dim props as SDL_PropertiesID = SDL_CreateProperties()
		scope
			i = 1
			do while (i < argc)
				scope
					dim arg as const zstring ptr = argv[i]
					if (SDL_strcmp(arg, strptr("--help")) = 0) then
						scope
							print_usage(argv[0])
							exit sub
						end scope
					else
						if (SDL_strcmp(arg, strptr("--server")) = 0) then
							scope
								is_server = true
							end scope
						else
							if (((SDL_strcmp(arg, strptr("--port")) = 0)) andalso ((i < ((argc - 1))))) then
								scope
									i += 1
									server_port = cast(Uint16, SDL_atoi(argv[i]))
								end scope
							else
								if (((SDL_strcmp(arg, strptr("--simulate-failure")) = 0)) andalso ((i < ((argc - 1))))) then
									scope
										i += 1
										simulate_failure = cast(long, SDL_atoi(argv[i]))
									end scope
								else
									scope
										hostname = arg
									end scope
								end if
							end if
						end if
					end if
				end scope
				i += 1
			loop
		end scope
		'' A loopback interface has no broadcast address. Request broadcast
		'' support only for the client's explicit '-' broadcast mode.
		if not is_server andalso hostname <> 0 then
			if SDL_strcmp(hostname, "-") = 0 then
				SDL_SetBooleanProperty(props, NET_PROP_DATAGRAM_SOCKET_ALLOW_BROADCAST_BOOLEAN, true)
			end if
		end if
		simulate_failure = (iif((((simulate_failure) < (0))), (0), (iif((((simulate_failure) > (100))), (100), (simulate_failure)))))
		if simulate_failure then
			scope
				SDL_Log_(strptr("Simulating failure at %d percent"), cast(long, simulate_failure))
			end scope
		end if
		if ((is_server = 0) andalso (hostname = 0)) then
			scope
				print_usage(argv[0])
				exit sub
			end scope
		end if
		if is_server then
			scope
				if hostname then
					scope
						SDL_Log_(strptr("SERVER: Resolving binding hostname '%s' ..."), hostname)
						socket_address = NET_ResolveHostname(hostname)
						if socket_address then
							scope
								if (NET_WaitUntilResolved(socket_address, (-1)) = NET_FAILURE) then
									scope
										NET_UnrefAddress(socket_address)
										socket_address = cptr(NET_Address ptr, 0)
									end scope
								end if
							end scope
						end if
					end scope
				end if
				if socket_address then
					scope
						SDL_Log_(strptr("SERVER: Listening on %s:%d."), NET_GetAddressString(socket_address), cast(long, server_port))
					end scope
				else
					scope
						SDL_Log_(strptr("SERVER: Listening on port %d"), cast(long, server_port))
					end scope
				end if
			end scope
		else
			scope
				if (SDL_strcmp(hostname, strptr("-")) = 0) then
					scope
						SDL_Log_(strptr("CLIENT: Broadcasting instead of unicasting to a specific address"))
					end scope
				else
					scope
						SDL_Log_(strptr("CLIENT: Resolving server hostname '%s' ..."), hostname)
						server_addr = NET_ResolveHostname(hostname)
						if server_addr then
							scope
								if (NET_WaitUntilResolved(server_addr, (-1)) = NET_FAILURE) then
									scope
										NET_UnrefAddress(server_addr)
										server_addr = cptr(NET_Address ptr, 0)
									end scope
								end if
							end scope
						end if
						if (server_addr = 0) then
							scope
								SDL_Log_(strptr("CLIENT: Failed! %s"), SDL_GetError())
								SDL_Log_(strptr("CLIENT: Giving up."))
								exit sub
							end scope
						end if
						SDL_Log_(strptr("CLIENT: Server is at %s:%d."), NET_GetAddressString(server_addr), cast(long, cast(long, server_port)))
					end scope
				end if
			end scope
		end if
		'' server _must_ be on the requested port. Clients can take anything available, server will respond to where it sees it come from.
		sock = NET_CreateDatagramSocket(socket_address, iif(is_server, server_port, 0), props)
		SDL_DestroyProperties(props)
		NET_UnrefAddress(socket_address)
		if (sock = 0) then
			scope
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, strptr("Failed to create datagram socket: %s"), SDL_GetError())
			end scope
		else
			scope
				if simulate_failure then
					scope
						NET_SimulateDatagramPacketLoss(sock, simulate_failure)
					end scope
				end if
				if (is_server = 0) then
					scope
						dim buf(0 to 127) as Uint8
						SDL_memset(cptr(any ptr, @buf(0)), 0, (sizeof(Uint8) * 128))
						SDL_Log_(strptr("CLIENT: %s %d bytes..."), iif(server_addr, strptr("Sending"), strptr("Broadcasting")), cast(long, cast(long, (sizeof(Uint8) * 128))))
						NET_SendDatagram(sock, server_addr, server_port, cptr(const any ptr, @buf(0)), (sizeof(Uint8) * 128))
					end scope
				else
					scope
						do
							if ((NET_WaitUntilInputAvailable(cptr(any ptr ptr, @(sock)), 1, (-1)) >= 0)) = 0 then exit do
							scope
								dim dgram as NET_Datagram ptr = cptr(NET_Datagram ptr, 0)
								if (NET_ReceiveDatagram(sock, @(dgram)) andalso ((dgram <> cptr(NET_Datagram ptr, (cptr(any ptr, 0)))))) then
									scope
										SDL_Log_(strptr("SERVER: got %d-byte datagram from %s:%d"), cast(long, cast(long, dgram->buflen)), NET_GetAddressString(dgram->addr), cast(long, cast(long, dgram->port)))
										NET_DestroyDatagram(dgram)
									end scope
								end if
							end scope
						loop
					end scope
				end if
			end scope
		end if
		SDL_Log_(strptr("Shutting down..."))
		NET_UnrefAddress(server_addr)
		server_addr = cptr(NET_Address ptr, 0)
		NET_DestroyDatagramSocket(sock)
		sock = cptr(NET_DatagramSocket ptr, 0)
	end scope
end sub

function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long
	scope
		if (NET_Init() = 0) then
			scope
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, strptr("NET_Init failed: %s"), SDL_GetError())
				SDL_Quit()
				return 1
			end scope
		end if
		run_datagram(argc, argv)
		NET_Quit()
		return 0
	end scope
end function

end SDL_RunApp(__FB_ARGC__, __FB_ARGV__, @example_main, 0)

'' end of datagram.bas
