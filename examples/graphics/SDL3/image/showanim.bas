'' Project: FreeBASIC SDL3 examples
'' File: showanim.bas
'' Purpose: Port upstream SDL3_image-3.4.8/examples/showanim.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' showanim:  A test application for the SDL image loading library.
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
'' claim that you wrote the original software. If you use this software
'' in a product, an acknowledgment in the product documentation would be
'' appreciated but is not required.
'' 2. Altered source versions must be plainly marked as such, and must not be
'' misrepresented as being the original software.
'' 3. This notice may not be removed or altered from any source distribution.

#include once "SDL3/SDL.bi"
#include once "smoke.bi"
#include once "SDL3/SDL_image.bi"

declare sub draw_background cdecl(byval renderer as SDL_Renderer ptr)
declare function get_file_path cdecl(byval file_ as const zstring ptr) as const zstring ptr
declare function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long

'' Draw a Gimpish background pattern to show transparency in the image
sub draw_background cdecl(byval renderer as SDL_Renderer ptr)
	scope
		dim col(0 to 1) as SDL_Color = {type<SDL_Color>(102, 102, 102, 255), type<SDL_Color>(153, 153, 153, 255)}
		dim dx as long = 8
		dim dy_ as long = 8
		dim rect as SDL_FRect
		dim i as long
		dim x as long
		dim y as long
		dim w as long
		dim h as long
		SDL_GetCurrentRenderOutputSize(renderer, @(w), @(h))
		rect.w = cast(single, dx)
		rect.h = cast(single, dy_)
		scope
			y = 0
			do while (y < h)
				scope
					scope
						x = 0
						do while (x < w)
							scope
								'' use an 8x8 checkerboard pattern
								i = ((((((x xor y)) shr 3)) and 1))
								SDL_SetRenderDrawColor(renderer, col(i).r, col(i).g, col(i).b, col(i).a)
								rect.x = cast(single, x)
								rect.y = cast(single, y)
								SDL_RenderFillRect(renderer, @(rect))
							end scope
							x += dx
						loop
					end scope
				end scope
				y += dy_
			loop
		end scope
	end scope
end sub

function get_file_path cdecl(byval file_ as const zstring ptr) as const zstring ptr
	scope
		static path(0 to 4095) as byte
		if (((*file_) <> 47) andalso (SDL_GetPathInfo(file_, cptr(SDL_PathInfo ptr, 0)) = 0)) then
			scope
				SDL_snprintf(@path(0), (sizeof(byte) * 4096), strptr("%s%s"), SDL_GetBasePath(), file_)
				if SDL_GetPathInfo(@path(0), cptr(SDL_PathInfo ptr, 0)) then
					scope
						return @path(0)
					end scope
				end if
			end scope
		end if
		return file_
	end scope
end function

function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long
	scope
		dim window_ as SDL_Window ptr
		dim renderer as SDL_Renderer ptr
		dim anim as IMG_Animation ptr
		dim textures as SDL_Texture ptr ptr
		dim flags as Uint32
		dim i as long
		dim j as long
		dim w as long
		dim h as long
		dim done as long
		dim once_ as long = 0
		dim played as long = 0
		dim loop_count as long = 0
		dim current_frame as long
		dim delay as long
		dim event as SDL_Event
		dim saveFile as const zstring ptr = cptr(const zstring ptr, 0)
		'' Check command line usage
		if (argv[1] = 0) then
			scope
				SDL_Log_(strptr(!"Usage: %s [-fullscreen] [-save file] <image_file> ...\n"), argv[0])
				return (1)
			end scope
		end if
		flags = SDL_WINDOW_HIDDEN
		scope
			i = 1
			do while argv[i]
				scope
					if (SDL_strcmp(argv[i], strptr("-fullscreen")) = 0) then
						scope
							SDL_HideCursor()
							flags or= SDL_WINDOW_FULLSCREEN
						end scope
					end if
				end scope
				i += 1
			loop
		end scope
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				SDL_Log_(strptr(!"SDL_Init(SDL_INIT_VIDEO) failed: %s\n"), SDL_GetError())
				return (2)
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("animation demo"), 0, 0, flags, @(window_), @(renderer)) = 0) then
			scope
				SDL_Log_(strptr(!"SDL_CreateWindowAndRenderer() failed: %s\n"), SDL_GetError())
				return (2)
			end scope
		end if
		scope
			i = 1
			do while argv[i]
				scope
					if (SDL_strcmp(argv[i], strptr("-fullscreen")) = 0) then
						scope
							goto loop_continue_4
						end scope
					end if
					if (SDL_strcmp(argv[i], strptr("-once")) = 0) then
						scope
							once_ = 1
							goto loop_continue_4
						end scope
					end if
					if ((SDL_strcmp(argv[i], strptr("-save")) = 0) andalso argv[(i + 1)]) then
						scope
							i += 1
							saveFile = argv[i]
							goto loop_continue_4
						end scope
					end if
					'' Open the image file
					anim = IMG_LoadAnimation(get_file_path(argv[i]))
					if (anim = 0) then
						scope
							SDL_Log_(strptr(!"Couldn't load %s: %s\n"), argv[i], SDL_GetError())
							goto loop_continue_4
						end scope
					end if
					loop_count = cast(long, SDL_GetNumberProperty(SDL_GetSurfaceProperties(anim->frames[0]), strptr(IMG_PROP_METADATA_LOOP_COUNT_NUMBER), (-1)))
					w = anim->w
					h = anim->h
					if saveFile then
						scope
							if (IMG_SaveAnimation(anim, saveFile) = 0) then
								scope
									SDL_Log_(strptr("Couldn't save animation: %s"), SDL_GetError())
								end scope
							end if
						end scope
					end if
					textures = cptr(SDL_Texture ptr ptr, SDL_calloc(anim->count, sizeof(((*textures)))))
					if (textures = 0) then
						scope
							SDL_Log_(strptr(!"Couldn't allocate textures\n"))
							IMG_FreeAnimation(anim)
							goto loop_continue_4
						end scope
					end if
					scope
						j = 0
						do while (j < anim->count)
							scope
								textures[j] = SDL_CreateTextureFromSurface(renderer, anim->frames[j])
							end scope
							j += 1
						loop
					end scope
					played = 0
					current_frame = 0
					'' Show the window
					SDL_SetWindowTitle(window_, argv[i])
					SDL_SetWindowSize(window_, w, h)
					SDL_ShowWindow(window_)
					done = 0
					do
						if ((done = 0)) = 0 then exit do
						scope
							do
								if (SDL_PollEvent(@(event))) = 0 then exit do
								scope
									select case event.type
										case SDL_EVENT_KEY_UP
											goto switch_case_8
										case SDL_EVENT_MOUSE_BUTTON_DOWN
											goto switch_case_9
										case SDL_EVENT_QUIT
											goto switch_case_10
										case else
											goto switch_case_11
									end select
									switch_case_8:
									scope
										select case event.key.key
											case (1073741904u)
												goto switch_case_13
											case (1073741903u)
												goto switch_case_14
											case (27u), (113u)
												goto switch_case_15
											case (32u), (9u)
												goto switch_case_16
											case else
												goto switch_case_17
										end select
										switch_case_13:
										scope
											if (i > 1) then
												scope
													i -= 2
													done = 1
												end scope
											end if
											goto switch_done_18
										end scope
										switch_case_14:
										scope
											if argv[(i + 1)] then
												scope
													done = 1
												end scope
											end if
											goto switch_done_18
										end scope
										switch_case_15:
										scope
											argv[(i + 1)] = cptr(zstring ptr, 0)
										end scope
										switch_case_16:
										scope
											done = 1
											goto switch_done_18
										end scope
										switch_case_17:
										scope
											goto switch_done_18
										end scope
										switch_done_18:
										goto switch_done_12
									end scope
									switch_case_9:
									scope
										done = 1
										goto switch_done_12
									end scope
									switch_case_10:
									scope
										argv[(i + 1)] = cptr(zstring ptr, 0)
										done = 1
										goto switch_done_12
									end scope
									switch_case_11:
									scope
										goto switch_done_12
									end scope
									switch_done_12:
								end scope
							loop
							'' Draw a background pattern in case the image has transparency
							draw_background(renderer)
							'' Display the image
							SDL_RenderTexture(renderer, textures[current_frame], cptr(const SDL_FRect ptr, 0), cptr(const SDL_FRect ptr, 0))
							SDL_RenderPresent(renderer)
							SDL3_ExampleSmokeFrame()
							if anim->delays[current_frame] then
								scope
									delay = anim->delays[current_frame]
								end scope
							else
								scope
									delay = 100
								end scope
							end if
							SDL_Delay(delay)
							if (current_frame < ((anim->count - 1))) then
								scope
									current_frame += 1
								end scope
							else
								scope
									if (played <> ((loop_count - 1))) then
										scope
											played += 1
											current_frame = 0
										end scope
									end if
									if once_ then
										scope
											exit do
										end scope
									end if
								end scope
							end if
						end scope
					loop
					scope
						j = 0
						do while (j < anim->count)
							scope
								SDL_DestroyTexture(textures[j])
							end scope
							j += 1
						loop
					end scope
					SDL_free(cptr(any ptr, textures))
					IMG_FreeAnimation(anim)
				end scope
				loop_continue_4:
				i += 1
			loop
		end scope
		SDL_DestroyRenderer(renderer)
		SDL_DestroyWindow(window_)
		'' We're done!
		SDL_Quit()
		return (0)
	end scope
end function

end SDL_RunApp(__FB_ARGC__, __FB_ARGV__, @example_main, 0)

'' end of showanim.bas
