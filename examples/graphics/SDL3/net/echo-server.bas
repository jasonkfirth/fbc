'' Project: FreeBASIC SDL3 examples
'' File: echo-server.bas
'' Purpose: Port upstream SDL3_net-3.2.0/examples/echo-server.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.

#include once "SDL3/SDL.bi"
#include once "SDL3/SDL_net.bi"

declare function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long

function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long
	scope
		dim interface as const zstring ptr = cptr(const zstring ptr, 0)
		dim server_port as Uint16 = 2382
		dim simulate_failure as long = 0
		scope
			dim i as long = 1
			do while (i < argc)
				scope
					dim arg as const zstring ptr = argv[i]
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
								interface = arg
							end scope
						end if
					end if
				end scope
				i += 1
			loop
		end scope
		if (NET_Init() = 0) then
			scope
				SDL_Log_(strptr(!"NET_Init failed: %s\n"), SDL_GetError())
				SDL_Quit()
				return 1
			end scope
		end if
		if interface then
			scope
				SDL_Log_(strptr("Attempting to listen on interface '%s', port %d"), interface, cast(long, cast(long, server_port)))
			end scope
		else
			scope
				SDL_Log_(strptr("Attempting to listen on all interfaces, port %d"), cast(long, cast(long, server_port)))
			end scope
		end if
		simulate_failure = (iif((((simulate_failure) < (0))), (0), (iif((((simulate_failure) > (100))), (100), (simulate_failure)))))
		if simulate_failure then
			scope
				SDL_Log_(strptr("Simulating failure at %d percent"), cast(long, simulate_failure))
			end scope
		end if
		dim server_addr as NET_Address ptr = cptr(NET_Address ptr, 0)
		if interface then
			scope
				server_addr = NET_ResolveHostname(interface)
				if ((server_addr = 0) orelse ((NET_WaitUntilResolved(server_addr, (-1)) <> NET_SUCCESS))) then
					scope
						SDL_Log_(strptr("Failed to resolve interface for '%s': %s"), interface, SDL_GetError())
						if server_addr then
							scope
								NET_UnrefAddress(server_addr)
							end scope
						end if
						NET_Quit()
						SDL_Quit()
						return 1
					end scope
				else
					scope
						SDL_Log_(strptr("Interface '%s' resolves to '%s' ..."), interface, NET_GetAddressString(server_addr))
					end scope
				end if
			end scope
		end if
		dim server as NET_Server ptr = NET_CreateServer(server_addr, server_port, 0)
		if (server = 0) then
			scope
				SDL_Log_(strptr("Failed to create server: %s"), SDL_GetError())
			end scope
		else
			scope
				SDL_Log_(strptr("Server is ready! Connect to port %d and send text!"), cast(long, cast(long, server_port)))
				dim num_vsockets as long = 1
				dim vsockets(0 to 127) as any ptr
				SDL_memset(cptr(any ptr, @vsockets(0)), 0, (sizeof(any ptr) * 128))
				vsockets(0) = cptr(any ptr, server)
				do
					if ((NET_WaitUntilInputAvailable(@vsockets(0), num_vsockets, (-1)) >= 0)) = 0 then exit do
					scope
						dim streamsocket as NET_StreamSocket ptr = cptr(NET_StreamSocket ptr, 0)
						if (NET_AcceptClient(server, @(streamsocket)) = 0) then
							scope
								SDL_Log_(strptr("NET_AcceptClient failed: %s"), SDL_GetError())
								exit do
							end scope
						else
							if streamsocket then
								scope
									'' new connection!
									SDL_Log_(strptr("New connection from %s!"), NET_GetAddressString(NET_GetStreamSocketAddress(streamsocket)))
									if (num_vsockets >= cast(long, (((((sizeof(any ptr) * 128) \ sizeof((vsockets(0))))) - 1)))) then
										scope
											SDL_Log_(strptr("  (too many connections, though, so dropping immediately.)"))
											NET_DestroyStreamSocket(streamsocket)
										end scope
									else
										scope
											if simulate_failure then
												scope
													NET_SimulateStreamPacketLoss(streamsocket, simulate_failure)
												end scope
											end if
											dim expression_value_3 as long = num_vsockets
											num_vsockets += 1
											vsockets(expression_value_3) = cptr(any ptr, streamsocket)
										end scope
									end if
								end scope
							end if
						end if
						'' see if anything has new stuff.
						dim buffer(0 to 1023) as byte
						scope
							dim i as long = 1
							do while (i < num_vsockets)
								scope
									dim kill_socket as boolean = false
									streamsocket = cptr(NET_StreamSocket ptr, vsockets(i))
									dim br as long = NET_ReadFromStreamSocket(streamsocket, cptr(any ptr, @buffer(0)), (sizeof(byte) * 1024))
									if (br < 0) then
										scope
											'' uhoh, socket failed!
											kill_socket = true
										end scope
									else
										if (br > 0) then
											scope
												dim addrstr as const zstring ptr = NET_GetAddressString(NET_GetStreamSocketAddress(streamsocket))
												SDL_Log_(strptr("Got %d more bytes from '%s'"), cast(long, br), addrstr)
												if (NET_WriteToStreamSocket(streamsocket, cptr(const any ptr, @buffer(0)), br) = 0) then
													scope
														SDL_Log_(strptr("Failed to echo data back to '%s': %s"), addrstr, SDL_GetError())
														kill_socket = true
													end scope
												end if
											end scope
										end if
									end if
									if kill_socket then
										scope
											SDL_Log_(strptr("Dropping connection to '%s'"), NET_GetAddressString(NET_GetStreamSocketAddress(streamsocket)))
											NET_DestroyStreamSocket(streamsocket)
											vsockets(i) = (cptr(any ptr, 0))
											if (i < ((num_vsockets - 1))) then
												scope
													SDL_memmove(cptr(any ptr, @(vsockets(i))), cptr(const any ptr, @(vsockets((i + 1)))), (sizeof((vsockets(0))) * ((((num_vsockets - i)) - 1))))
												end scope
											end if
											num_vsockets -= 1
											i -= 1
										end scope
									end if
								end scope
								i += 1
							loop
						end scope
					end scope
				loop
				SDL_Log_(strptr("Destroying server..."))
				NET_DestroyServer(server)
			end scope
		end if
		SDL_Log_(strptr("Shutting down..."))
		NET_Quit()
		SDL_Quit()
		return 0
	end scope
end function

end SDL_RunApp(__FB_ARGC__, __FB_ARGV__, @example_main, 0)

'' end of echo-server.bas
