'' Project: FreeBASIC SDL3 examples
'' File: simple-http-get.bas
'' Purpose: Port upstream SDL3_net-3.2.0/examples/simple-http-get.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This is just for demonstration purposes! A real HTTP solution would
'' be WAY more complicated, support HTTPS, cookies, etc. Use curl or
'' wget for real stuff, not this.
''
'' All this to say: don't use this for anything serious!

#include once "SDL3/SDL.bi"
#include once "SDL3/SDL_net.bi"
#include once "crt.bi"

declare function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long

function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long
	scope
		'' Port 80 is the HTTP default. An optional port also permits local
		'' test servers without requiring privileged sockets.
		dim server_port as Uint16 = 80
		if (NET_Init() = 0) then
			scope
				SDL_Log_(strptr("NET_Init() failed: %s"), SDL_GetError())
				return 1
			end scope
		end if
		scope
			dim i as long = 1
			do while (i < argc)
				scope
					if SDL_strcmp(argv[i], "--port") = 0 then
						if i + 1 >= argc then
							SDL_Log_("--port requires a value from 1 through 65535")
							NET_Quit()
							return 1
						end if
						dim port_value as long = SDL_atoi(argv[i + 1])
						if port_value < 1 orelse port_value > 65535 then
							SDL_Log_("Invalid HTTP port")
							NET_Quit()
							return 1
						end if
						server_port = port_value
						i += 2
						continue do
					end if
					SDL_Log_(strptr("Looking up %s ..."), argv[i])
					dim addr as NET_Address ptr = NET_ResolveHostname(argv[i])
					if (NET_WaitUntilResolved(addr, (-1)) = NET_FAILURE) then
						scope
							SDL_Log_(strptr("Failed to lookup %s: %s"), argv[i], SDL_GetError())
						end scope
					else
						scope
							SDL_Log_(strptr("%s is %s"), argv[i], NET_GetAddressString(addr))
							dim req as zstring ptr = cptr(zstring ptr, 0)
							SDL_asprintf(@(req), strptr(!"GET / HTTP/1.0\r\nHost: %s\r\n\r\n"), argv[i])
							dim sock as NET_StreamSocket ptr = iif(req, NET_CreateClient(addr, server_port, 0), cptr(NET_StreamSocket ptr, 0))
							if (req = 0) then
								scope
									SDL_Log_(strptr("Out of memory!"))
								end scope
							else
								if (sock = 0) then
									scope
										SDL_Log_(strptr(!"Failed to create stream socket to %s: %s\n"), argv[i], SDL_GetError())
									end scope
								else
									if (NET_WaitUntilConnected(sock, (-1)) = NET_FAILURE) then
										scope
											SDL_Log_(strptr("Failed to connect to %s: %s"), argv[i], SDL_GetError())
										end scope
									else
										if (NET_WriteToStreamSocket(sock, cptr(const any ptr, req), cast(long, SDL_strlen(req))) = 0) then
											scope
												SDL_Log_(strptr("Failed to write to %s: %s"), argv[i], SDL_GetError())
											end scope
										else
											if (NET_WaitUntilStreamSocketDrained(sock, (-1)) < 0) then
												scope
													SDL_Log_(strptr("Failed to finish write to %s: %s"), argv[i], SDL_GetError())
												end scope
											else
												scope
													dim buf(0 to 511) as byte
													dim br as long
													do
														br = NET_ReadFromStreamSocket(sock, cptr(any ptr, @buf(0)), (sizeof(byte) * 512))
														if (((br) >= 0)) = 0 then exit do
														scope
															fwrite(cptr(const any ptr, @buf(0)), 1, br, stdout)
														end scope
													loop
													printf(strptr(!"\n\n\n%s\n\n\n"), SDL_GetError())
													fflush(stdout)
												end scope
											end if
										end if
									end if
								end if
							end if
							if sock then
								scope
									NET_DestroyStreamSocket(sock)
								end scope
							end if
							SDL_free(cptr(any ptr, req))
						end scope
					end if
				end scope
				i += 1
			loop
		end scope
		NET_Quit()
		return 0
	end scope
end function

end SDL_RunApp(__FB_ARGC__, __FB_ARGV__, @example_main, 0)

'' end of simple-http-get.bas
