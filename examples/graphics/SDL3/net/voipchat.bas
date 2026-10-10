'' Project: FreeBASIC SDL3 examples
'' File: voipchat.bas
'' Purpose: Port upstream SDL3_net-3.2.0/examples/voipchat.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.

#include once "SDL3/SDL.bi"
#include once "smoke.bi"
#include once "SDL3/SDL_net.bi"

type Voice
	stream as SDL_AudioStream ptr
	addr as NET_Address ptr
	port as Uint16
	idnum as Uint64
	last_seen as Uint64
	last_packetnum as Uint64
	prev as Voice ptr
	next_ as Voice ptr
end type

dim shared sock as NET_DatagramSocket ptr = cptr(NET_DatagramSocket ptr, 0)
'' you talk over this, client or server.
dim shared server_addr as NET_Address ptr = cptr(NET_Address ptr, 0)
'' address of the server you're talking to, NULL if you _are_ the server.
dim shared server_port as Uint16 = 3025
dim shared max_datagram as long = 0
dim shared voices as Voice ptr = cptr(Voice ptr, 0)
dim shared next_idnum as Uint64 = 0
dim shared window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
dim shared renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
dim shared audio_device as SDL_AudioDeviceID = 0
dim shared capture_device as SDL_AudioDeviceID = 0
dim shared capture_stream as SDL_AudioStream ptr = cptr(SDL_AudioStream ptr, 0)
dim shared audio_spec as SDL_AudioSpec = type<SDL_AudioSpec>(SDL_AUDIO_S16LE, 1, 8000)
dim shared scratch_area(0 to 511) as Uint64
dim shared extra as long = cast(long, ((sizeof(Uint64) * 2)))

declare function FindVoiceByAddr cdecl(byval addr as const NET_Address ptr, byval port as Uint16) as Voice ptr
declare function FindVoiceByIdNum cdecl(byval idnum as Uint64) as Voice ptr
declare sub ClearOldVoices cdecl(byval now as Uint64)
declare sub SendClientAudioToServer cdecl()
declare sub mainloop cdecl()
declare sub print_usage cdecl(byval prog as const zstring ptr)
declare sub run_voipchat cdecl(byval argc as long, byval argv as zstring ptr ptr)
declare function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long

function FindVoiceByAddr cdecl(byval addr as const NET_Address ptr, byval port as Uint16) as Voice ptr
	scope
		dim i as Voice ptr
		scope
			i = voices
			do while (i <> cptr(Voice ptr, (cptr(any ptr, 0))))
				scope
					if (((i->port = port)) andalso ((NET_CompareAddresses(i->addr, addr) = 0))) then
						scope
							return i
						end scope
					end if
				end scope
				i = i->next_
			loop
		end scope
		return cptr(Voice ptr, 0)
	end scope
end function

function FindVoiceByIdNum cdecl(byval idnum as Uint64) as Voice ptr
	scope
		dim i as Voice ptr
		scope
			i = voices
			do while (i <> cptr(Voice ptr, (cptr(any ptr, 0))))
				scope
					if (i->idnum = idnum) then
						scope
							return i
						end scope
					end if
				end scope
				i = i->next_
			loop
		end scope
		return cptr(Voice ptr, 0)
	end scope
end function

sub ClearOldVoices cdecl(byval now as Uint64)
	scope
		dim i as Voice ptr
		dim next_ as Voice ptr
		scope
			i = voices
			do while (i <> cptr(Voice ptr, (cptr(any ptr, 0))))
				scope
					next_ = i->next_
					if ((now = 0) orelse ((((now - i->last_seen)) > 60000))) then
						scope
							'' nothing new in 60+ seconds? (or shutting down?)
							if ((i->stream = 0) orelse ((SDL_GetAudioStreamAvailable(i->stream) = 0))) then
								scope
									'' they'll get a reprieve if data is still playing out
									SDL_Log_(strptr("Destroying voice #%" SDL_PRIu64), cast(Uint64, i->idnum))
									SDL_DestroyAudioStream(i->stream)
									NET_UnrefAddress(i->addr)
									if i->prev then
										scope
											i->prev->next_ = next_
										end scope
									else
										scope
											voices = next_
										end scope
									end if
									if next_ then
										scope
											next_->prev = i->prev
										end scope
									end if
									SDL_free(cptr(any ptr, i))
								end scope
							end if
						end scope
					end if
				end scope
				i = next_
			loop
		end scope
	end scope
end sub

sub SendClientAudioToServer cdecl()
	scope
		dim br as long = SDL_GetAudioStreamData(capture_stream, cptr(any ptr, (@scratch_area(0) + ((extra \ sizeof(Uint64))))), (max_datagram - extra))
		if (br > 0) then
			scope
				next_idnum += 1
				scratch_area(0) = (0)
				'' just being nice and leaving space in the buffer for the server to replace.
				scratch_area(1) = (next_idnum)
				SDL_Log_(strptr("CLIENT: Sending %d new bytes to server at %s:%d..."), cast(long, (br + extra)), NET_GetAddressString(server_addr), cast(long, cast(long, server_port)))
				if (NET_SendDatagram(sock, server_addr, server_port, cptr(const any ptr, @scratch_area(0)), (br + extra)) = 0) then
					scope
						SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, strptr("NET_SendDatagram failed: %s"), SDL_GetError())
					end scope
				end if
			end scope
		end if
	end scope
end sub

sub mainloop cdecl()
	scope
		dim is_client as boolean = cast(boolean, iif(((server_addr <> cptr(NET_Address ptr, (cptr(any ptr, 0))))), 1, 0))
		dim done as boolean = false
		dim last_send_ticks as Uint64 = 0
		if is_client then
			scope
				SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255)
			end scope
		end if
		do
			if ((done = 0)) = 0 then exit do
			scope
				dim activity as boolean = false
				dim now as Uint64 = SDL_GetTicks()
				dim event as SDL_Event
				dim dgram as NET_Datagram ptr = cptr(NET_Datagram ptr, 0)
				do
					if (((NET_ReceiveDatagram(sock, @(dgram))) andalso ((dgram <> cptr(NET_Datagram ptr, (cptr(any ptr, 0))))))) = 0 then exit do
					scope
						SDL_Log_(strptr("%s: got %d-byte datagram from %s:%d"), iif(is_client, strptr("CLIENT"), strptr("SERVER")), cast(long, cast(long, dgram->buflen)), NET_GetAddressString(dgram->addr), cast(long, cast(long, dgram->port)))
						activity = true
						if (is_client = 0) then
							scope
								'' we're the server?
								dim voice_value as Voice ptr = FindVoiceByAddr(dgram->addr, dgram->port)
								dim i as Voice ptr
								if (voice_value = 0) then
									scope
										SDL_Log_(strptr("SERVER: Creating voice idnum=%" SDL_PRIu64 " from %s:%d"), cast(Uint64, (next_idnum + 1)), NET_GetAddressString(dgram->addr), cast(long, cast(long, dgram->port)))
										voice_value = cptr(Voice ptr, SDL_calloc(1, sizeof(Voice)))
										voice_value->addr = NET_RefAddress(dgram->addr)
										voice_value->port = dgram->port
										next_idnum += 1
										voice_value->idnum = next_idnum
										if voices then
											scope
												voice_value->next_ = voices
												voices->prev = voice_value
											end scope
										end if
										voices = voice_value
									end scope
								end if
								voice_value->last_seen = now
								'' send this new voice data to all recent speakers.
								if (dgram->buflen > extra) then
									scope
										'' ignore it if too small, might just be a keepalive packet.
										(*(cptr(Uint64 ptr, dgram->buf))) = (voice_value->idnum)
										'' the client leaves space to fill this in for convenience.
										scope
											i = voices
											do while (i <> cptr(Voice ptr, (cptr(any ptr, 0))))
												scope
													if (((voice_value->port <> i->port)) orelse ((NET_CompareAddresses(voice_value->addr, i->addr) <> 0))) then
														scope
															'' don't send client's own voice back to them.
															SDL_Log_(strptr("SERVER: sending %d-byte datagram to %s:%d"), cast(long, cast(long, dgram->buflen)), NET_GetAddressString(i->addr), cast(long, cast(long, i->port)))
															NET_SendDatagram(sock, i->addr, i->port, cptr(const any ptr, dgram->buf), dgram->buflen)
														end scope
													end if
												end scope
												i = i->next_
											loop
										end scope
									end scope
								end if
							end scope
						else
							scope
								'' we're the client.
								if (((dgram->port <> server_port)) orelse ((NET_CompareAddresses(dgram->addr, server_addr) <> 0))) then
									scope
										SDL_Log_(strptr("CLIENT: Got packet from non-server address %s:%d. Ignoring."), NET_GetAddressString(dgram->addr), cast(long, cast(long, dgram->port)))
									end scope
								else
									if (dgram->buflen < extra) then
										scope
											SDL_Log_(strptr("CLIENT: Got bogus packet from the server. Ignoring."))
										end scope
									else
										scope
											dim idnum as Uint64 = ((cptr(const Uint64 ptr, dgram->buf))[0])
											dim packetnum as Uint64 = ((cptr(const Uint64 ptr, dgram->buf))[1])
											dim voice_value as Voice ptr = FindVoiceByIdNum(idnum)
											if (voice_value = 0) then
												scope
													SDL_Log_(strptr("CLIENT: Creating voice idnum=#%" SDL_PRIu64), cast(Uint64, idnum))
													voice_value = cptr(Voice ptr, SDL_calloc(1, sizeof(Voice)))
													if audio_device then
														scope
															voice_value->stream = SDL_CreateAudioStream(@(audio_spec), @(audio_spec))
															if voice_value->stream then
																scope
																	SDL_BindAudioStream(audio_device, voice_value->stream)
																end scope
															end if
														end scope
													end if
													voice_value->idnum = idnum
													if voices then
														scope
															voice_value->next_ = voices
															voices->prev = voice_value
														end scope
													end if
													voices = voice_value
												end scope
											end if
											voice_value->last_seen = now
											if (packetnum > voice_value->last_packetnum) then
												scope
													'' if packet arrived out of order, don't queue it for playing.
													voice_value->last_packetnum = packetnum
													SDL_PutAudioStreamData(voice_value->stream, cptr(const any ptr, (dgram->buf + extra)), (dgram->buflen - extra))
													SDL_FlushAudioStream(voice_value->stream)
												end scope
											end if
										end scope
									end if
								end if
							end scope
						end if
						NET_DestroyDatagram(dgram)
					end scope
				loop
				do
					if (SDL_PollEvent(@(event))) = 0 then exit do
					scope
						activity = true
						select case event.type
							case SDL_EVENT_QUIT
								goto switch_case_8
							case SDL_EVENT_MOUSE_BUTTON_DOWN
								goto switch_case_9
							case SDL_EVENT_MOUSE_BUTTON_UP
								goto switch_case_10
							case else
								goto switch_done_11
						end select
						switch_case_8:
						scope
							done = true
							goto switch_done_11
						end scope
						switch_case_9:
						scope
								if ((is_client andalso (capture_stream <> 0)) andalso ((event.button.button = SDL_BUTTON_LEFT))) then
								scope
									if SDL_BindAudioStream(capture_device, capture_stream) then
										scope
											SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255)
										end scope
									end if
								end scope
							end if
							goto switch_done_11
						end scope
						switch_case_10:
						scope
								if ((is_client andalso (capture_stream <> 0)) andalso ((event.button.button = SDL_BUTTON_LEFT))) then
								scope
									SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255)
									'' red when not recording
									SDL_UnbindAudioStream(capture_stream)
									SDL_FlushAudioStream(capture_stream)
									do
										if ((SDL_GetAudioStreamAvailable(capture_stream) > 0)) = 0 then exit do
										scope
											SendClientAudioToServer()
											last_send_ticks = now
										end scope
									loop
								end scope
							end if
							goto switch_done_11
						end scope
						switch_done_11:
					end scope
				loop
				if is_client then
					scope
						if capture_stream then
							scope
								do
									if ((SDL_GetAudioStreamAvailable(capture_stream) > max_datagram)) = 0 then exit do
									scope
										SendClientAudioToServer()
										last_send_ticks = now
										activity = true
									end scope
								loop
							end scope
						end if
						if ((last_send_ticks = 0) orelse ((((now - last_send_ticks)) > 5000))) then
							scope
								'' send a keepalive packet if we haven't transmitted for a bit.
								next_idnum += 1
								scratch_area(0) = (0)
								scratch_area(1) = (next_idnum)
								SDL_Log_(strptr("CLIENT: Sending %d keepalive bytes to server at %s:%d..."), cast(long, extra), NET_GetAddressString(server_addr), cast(long, cast(long, server_port)))
								NET_SendDatagram(sock, server_addr, server_port, cptr(const any ptr, @scratch_area(0)), extra)
								last_send_ticks = now
							end scope
						end if
					end scope
				end if
				ClearOldVoices(now)
				if (activity = 0) then
					scope
						SDL_Delay(10)
					end scope
				end if
				SDL_RenderClear(renderer)
				SDL_RenderPresent(renderer)
				SDL3_ExampleSmokeFrame()
			end scope
		loop
	end scope
end sub

sub print_usage cdecl(byval prog as const zstring ptr)
	scope
		SDL_Log_(strptr("USAGE: %s <hostname|ip> [--help] [--server] [--port X] [--simulate-failure Y]"), prog)
	end scope
end sub

sub run_voipchat cdecl(byval argc as long, byval argv as zstring ptr ptr)
	scope
		dim hostname as const zstring ptr = cptr(const zstring ptr, 0)
		dim is_server as boolean = false
		dim simulate_failure as long = 0
		dim i as long
		dim socket_address as NET_Address ptr = cptr(NET_Address ptr, 0)
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
				else
					scope
						dim num_addresses as long
						dim addresses as NET_Address ptr ptr
						addresses = NET_GetLocalAddresses(@(num_addresses))
						if ((addresses = cptr(NET_Address ptr ptr, (cptr(any ptr, 0)))) orelse (num_addresses <= 0)) then
							scope
								SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, strptr("Failed to to get local addresses: %s"), SDL_GetError())
							end scope
						else
							scope
								socket_address = addresses[0]
								NET_RefAddress(socket_address)
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
				audio_device = SDL_OpenAudioDevice((SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK), @(audio_spec))
				if (audio_device = 0) then
					scope
						SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, strptr("CLIENT: Failed to open output audio device (%s), going on without sound playback!"), SDL_GetError())
					end scope
				end if
				capture_device = SDL_OpenAudioDevice((SDL_AUDIO_DEVICE_DEFAULT_RECORDING), @(audio_spec))
				if (capture_device = 0) then
					scope
						SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, strptr("CLIENT: Failed to open capture audio device (%s), going on without sound recording!"), SDL_GetError())
					end scope
				else
					scope
						capture_stream = SDL_CreateAudioStream(@(audio_spec), @(audio_spec))
						if (capture_stream = 0) then
							scope
								SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, strptr("CLIENT: Failed to create capture audio stream (%s), going on without sound recording!"), SDL_GetError())
								SDL_CloseAudioDevice(capture_device)
								capture_device = 0
							end scope
						end if
					end scope
				end if
			end scope
		end if
		'' server _must_ be on the requested port. Clients can take anything available, server will respond to where it sees it come from.
		sock = NET_CreateDatagramSocket(socket_address, iif(is_server, server_port, 0), 0)
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
				mainloop()
			end scope
		end if
		SDL_Log_(strptr("Shutting down..."))
		ClearOldVoices(0)
		SDL_DestroyAudioStream(capture_stream)
		SDL_CloseAudioDevice(audio_device)
		SDL_CloseAudioDevice(capture_device)
		capture_device = 0
		audio_device = capture_device
		NET_UnrefAddress(server_addr)
		server_addr = cptr(NET_Address ptr, 0)
		NET_DestroyDatagramSocket(sock)
		sock = cptr(NET_DatagramSocket ptr, 0)
	end scope
end sub

function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long
	scope
		if (SDL_Init((SDL_INIT_VIDEO or SDL_INIT_AUDIO)) = 0) then
			scope
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, strptr("SDL_Init failed: %s"), SDL_GetError())
				return 1
			end scope
		end if
		if (NET_Init() = 0) then
			scope
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, strptr("NET_Init failed: %s"), SDL_GetError())
				SDL_Quit()
				return 1
			end scope
		end if
		window_ = SDL_CreateWindow(strptr("SDL3_net voipchat example"), 640, 480, 0)
		renderer = SDL_CreateRenderer(window_, cptr(const zstring ptr, 0))
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255)
		max_datagram = (iif((((1200) < (cast(long, (sizeof(Uint64) * 512))))), (1200), (cast(long, (sizeof(Uint64) * 512)))))
		run_voipchat(argc, argv)
		SDL_DestroyRenderer(renderer)
		SDL_DestroyWindow(window_)
		NET_Quit()
		SDL_Quit()
		return 0
	end scope
end function

end SDL_RunApp(__FB_ARGC__, __FB_ARGV__, @example_main, 0)

'' end of voipchat.bas
