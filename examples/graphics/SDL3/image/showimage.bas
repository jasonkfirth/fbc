'' Project: FreeBASIC SDL3 examples
'' File: showimage.bas
'' Purpose: Port upstream SDL3_image-3.4.8/examples/showimage.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' showimage:  A test application for the SDL image loading library.
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

#define SDL_PROP_SURFACE_FLIP_NUMBER "SDL.surface.flip"

declare sub draw_background cdecl(byval renderer as SDL_Renderer ptr)
declare function get_file_path cdecl(byval file_ as const zstring ptr) as const zstring ptr
declare sub set_cursor cdecl(byval cursor_file as const zstring ptr)
declare function load_image cdecl(byval renderer as SDL_Renderer ptr, byval file_ as const zstring ptr, byval tonemap as const zstring ptr, byval flip_ as SDL_FlipMode ptr, byval rotation as single ptr) as SDL_Texture ptr
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

sub set_cursor cdecl(byval cursor_file as const zstring ptr)
	scope
		dim anim as IMG_Animation ptr = IMG_LoadAnimation(get_file_path(cursor_file))
		if anim then
			scope
				dim cursor as SDL_Cursor ptr = IMG_CreateAnimatedCursor(anim, 0, 0)
				if cursor then
					scope
						SDL_SetCursor(cursor)
					end scope
				else
					scope
						SDL_Log_(strptr("Couldn't create cursor with %s: %s"), cursor_file, SDL_GetError())
					end scope
				end if
				IMG_FreeAnimation(anim)
			end scope
		end if
	end scope
end sub

function load_image cdecl(byval renderer as SDL_Renderer ptr, byval file_ as const zstring ptr, byval tonemap as const zstring ptr, byval flip_ as SDL_FlipMode ptr, byval rotation as single ptr) as SDL_Texture ptr
	scope
		dim texture as SDL_Texture ptr = cptr(SDL_Texture ptr, 0)
		dim surface as SDL_Surface ptr = IMG_Load(get_file_path(file_))
		if (surface = 0) then
			scope
				SDL_Log_(strptr(!"Couldn't load %s: %s\n"), file_, SDL_GetError())
				return cptr(SDL_Texture ptr, 0)
			end scope
		end if
		if tonemap then
			scope
				dim temp as SDL_Surface ptr
				'' Use the tonemap operator to convert to SDR output
				SDL_SetStringProperty(SDL_GetSurfaceProperties(surface), strptr(SDL_PROP_SURFACE_TONEMAP_OPERATOR_STRING), tonemap)
				temp = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32)
				SDL_DestroySurface(surface)
				if (temp = 0) then
					scope
						SDL_Log_(strptr(!"Couldn't convert surface: %s\n"), SDL_GetError())
						return cptr(SDL_Texture ptr, 0)
					end scope
				end if
				surface = temp
			end scope
		end if
		(*flip_) = SDL_GetNumberProperty(SDL_GetSurfaceProperties(surface), strptr(SDL_PROP_SURFACE_FLIP_NUMBER), SDL_FLIP_NONE)
		(*rotation) = SDL_GetFloatProperty(SDL_GetSurfaceProperties(surface), strptr(SDL_PROP_SURFACE_ROTATION_FLOAT), 0.0f)
		texture = SDL_CreateTextureFromSurface(renderer, surface)
		SDL_DestroySurface(surface)
		return texture
	end scope
end function

function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long
	scope
		dim window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
		dim renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
		dim texture as SDL_Texture ptr = cptr(SDL_Texture ptr, 0)
		dim flip_ as SDL_FlipMode = SDL_FLIP_NONE
		dim rotation as single = 0.0f
		dim flags as Uint32
		dim i as long
		dim done as long = 0
		dim quit as long = 0
		dim event as SDL_Event
		dim tonemap as const zstring ptr = cptr(const zstring ptr, 0)
		dim saveFile as const zstring ptr = cptr(const zstring ptr, 0)
		dim attempted as long = 0
		dim result as long = 0
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
				result = 2
				goto done
			end scope
		end if
		window_ = SDL_CreateWindow(strptr(""), 0, 0, flags)
		if (window_ = 0) then
			scope
				SDL_Log_(strptr(!"SDL_CreateWindow() failed: %s\n"), SDL_GetError())
				result = 2
				goto done
			end scope
		end if
		if SDL_GetBooleanProperty(SDL_GetDisplayProperties(SDL_GetPrimaryDisplay()), strptr(SDL_PROP_DISPLAY_HDR_ENABLED_BOOLEAN), false) then
			scope
				dim props as SDL_PropertiesID = SDL_CreateProperties()
				SDL_SetPointerProperty(props, strptr(SDL_PROP_RENDERER_CREATE_WINDOW_POINTER), cptr(any ptr, window_))
				SDL_SetNumberProperty(props, strptr(SDL_PROP_RENDERER_CREATE_OUTPUT_COLORSPACE_NUMBER), SDL_COLORSPACE_SRGB_LINEAR)
				renderer = SDL_CreateRendererWithProperties(props)
				SDL_DestroyProperties(props)
			end scope
		end if
		if (renderer = 0) then
			scope
				renderer = SDL_CreateRenderer(window_, cptr(const zstring ptr, 0))
			end scope
		end if
		if (renderer = 0) then
			scope
				SDL_Log_(strptr(!"SDL_CreateRenderer() failed: %s\n"), SDL_GetError())
				result = 2
				goto done
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
					if (SDL_strcmp(argv[i], strptr("-quit")) = 0) then
						scope
							quit = 1
							goto loop_continue_4
						end scope
					end if
					if ((SDL_strcmp(argv[i], strptr("-tonemap")) = 0) andalso argv[(i + 1)]) then
						scope
							i += 1
							tonemap = argv[i]
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
					if ((SDL_strcmp(argv[i], strptr("-cursor")) = 0) andalso argv[(i + 1)]) then
						scope
							i += 1
							set_cursor(argv[i])
							goto loop_continue_4
						end scope
					end if
					'' Open the image file
					attempted += 1
					texture = load_image(renderer, argv[i], tonemap, @(flip_), @(rotation))
					if (texture = 0) then
						scope
							goto loop_continue_4
						end scope
					end if
					'' Save the image file, if desired
					if saveFile then
						scope
							dim surface as SDL_Surface ptr = IMG_Load(get_file_path(argv[i]))
							if surface then
								scope
									if (IMG_Save(surface, saveFile) = 0) then
										scope
											SDL_Log_(strptr(!"Couldn't save %s: %s\n"), saveFile, SDL_GetError())
											result = 3
										end scope
									end if
								end scope
							else
								scope
									SDL_Log_(strptr(!"Couldn't load %s: %s\n"), argv[i], SDL_GetError())
									result = 3
								end scope
							end if
						end scope
					end if
					'' Show the window
					SDL_SetWindowTitle(window_, argv[i])
					if ((rotation = 90.0f) orelse (rotation = 270.0f)) then
						scope
							SDL_SetWindowSize(window_, texture->h, texture->w)
						end scope
					else
						scope
							SDL_SetWindowSize(window_, texture->w, texture->h)
						end scope
					end if
					SDL_ShowWindow(window_)
					done = quit
					do
						if ((done = 0)) = 0 then exit do
						scope
							do
								if (SDL_PollEvent(@(event))) = 0 then exit do
								scope
									select case event.type
										case SDL_EVENT_KEY_UP
											goto switch_case_7
										case SDL_EVENT_MOUSE_BUTTON_DOWN
											goto switch_case_8
										case SDL_EVENT_QUIT
											goto switch_case_9
										case else
											goto switch_case_10
									end select
									switch_case_7:
									scope
										select case event.key.key
											case (1073741904u)
												goto switch_case_12
											case (1073741903u)
												goto switch_case_13
											case (27u), (113u)
												goto switch_case_14
											case (32u), (9u)
												goto switch_case_15
											case else
												goto switch_case_16
										end select
										switch_case_12:
										scope
											if (i > 1) then
												scope
													i -= 2
													done = 1
												end scope
											end if
											goto switch_done_17
										end scope
										switch_case_13:
										scope
											if argv[(i + 1)] then
												scope
													done = 1
												end scope
											end if
											goto switch_done_17
										end scope
										switch_case_14:
										scope
											argv[(i + 1)] = cptr(zstring ptr, 0)
										end scope
										switch_case_15:
										scope
											done = 1
											goto switch_done_17
										end scope
										switch_case_16:
										scope
											goto switch_done_17
										end scope
										switch_done_17:
										goto switch_done_11
									end scope
									switch_case_8:
									scope
										done = 1
										goto switch_done_11
									end scope
									switch_case_9:
									scope
										argv[(i + 1)] = cptr(zstring ptr, 0)
										done = 1
										goto switch_done_11
									end scope
									switch_case_10:
									scope
										goto switch_done_11
									end scope
									switch_done_11:
								end scope
							loop
							'' Draw a background pattern in case the image has transparency
							draw_background(renderer)
							'' Display the image
							dim dst as SDL_FRect
							if ((rotation = 90.0f) orelse (rotation = 270.0f)) then
								scope
									'' Use a pre-rotated destination rectangle
									dst.x = ((-((texture->w - texture->h))) / 2.0f)
									dst.y = (((texture->w - texture->h)) / 2.0f)
									dst.w = cast(single, texture->w)
									dst.h = cast(single, texture->h)
								end scope
							else
								scope
									dst.x = 0.0f
									dst.y = 0.0f
									dst.w = cast(single, texture->w)
									dst.h = cast(single, texture->h)
								end scope
							end if
							SDL_RenderTextureRotated(renderer, texture, cptr(const SDL_FRect ptr, 0), @(dst), cast(double, rotation), cptr(const SDL_FPoint ptr, 0), flip_)
							SDL_RenderPresent(renderer)
							SDL3_ExampleSmokeFrame()
							SDL_Delay(100)
						end scope
					loop
					SDL_DestroyTexture(texture)
					texture = cptr(SDL_Texture ptr, 0)
				end scope
				loop_continue_4:
				i += 1
			loop
		end scope
		if ((attempted = 0) andalso (quit = 0)) then
			scope
				'' Show the window if needed
				SDL_SetWindowTitle(window_, strptr("showimage"))
				SDL_SetWindowSize(window_, 640, 480)
				SDL_ShowWindow(window_)
				do
					if ((done = 0)) = 0 then exit do
					scope
						do
							if (SDL_PollEvent(@(event))) = 0 then exit do
							scope
								select case event.type
									case SDL_EVENT_DROP_FILE
										goto switch_case_20
									case SDL_EVENT_KEY_UP
										goto switch_case_21
									case SDL_EVENT_MOUSE_BUTTON_DOWN
										goto switch_case_22
									case SDL_EVENT_QUIT
										goto switch_case_23
									case else
										goto switch_case_24
								end select
								switch_case_20:
								scope
									scope
										dim file_ as const zstring ptr = event.drop.data
										SDL_DestroyTexture(texture)
										SDL_Log_(strptr(!"Loading %s\n"), file_)
										texture = load_image(renderer, file_, tonemap, @(flip_), @(rotation))
										if (texture = 0) then
											scope
												goto switch_done_25
											end scope
										end if
										SDL_SetWindowTitle(window_, file_)
										if ((rotation = 90.0f) orelse (rotation = 270.0f)) then
											scope
												SDL_SetWindowSize(window_, texture->h, texture->w)
											end scope
										else
											scope
												SDL_SetWindowSize(window_, texture->w, texture->h)
											end scope
										end if
									end scope
									goto switch_done_25
								end scope
								switch_case_21:
								scope
									select case event.key.key
										case (27u), (113u)
											goto switch_case_26
										case else
											goto switch_done_27
									end select
									switch_case_26:
									scope
										done = 1
										goto switch_done_27
									end scope
									switch_done_27:
									goto switch_done_25
								end scope
								switch_case_22:
								scope
									done = 1
									goto switch_done_25
								end scope
								switch_case_23:
								scope
									done = 1
									goto switch_done_25
								end scope
								switch_case_24:
								scope
									goto switch_done_25
								end scope
								switch_done_25:
							end scope
						loop
						'' Draw a background pattern in case the image has transparency
						draw_background(renderer)
						'' Display the image
						dim dst as SDL_FRect
						if ((rotation = 90.0f) orelse (rotation = 270.0f)) then
							scope
								'' Use a pre-rotated destination rectangle
								dst.x = ((-((texture->w - texture->h))) / 2.0f)
								dst.y = (((texture->w - texture->h)) / 2.0f)
								dst.w = cast(single, texture->w)
								dst.h = cast(single, texture->h)
							end scope
						else
							scope
								dst.x = 0.0f
								dst.y = 0.0f
								dst.w = cast(single, texture->w)
								dst.h = cast(single, texture->h)
							end scope
						end if
						SDL_RenderTextureRotated(renderer, texture, cptr(const SDL_FRect ptr, 0), @(dst), cast(double, rotation), cptr(const SDL_FPoint ptr, 0), flip_)
						SDL_RenderPresent(renderer)
						SDL3_ExampleSmokeFrame()
						SDL_Delay(100)
					end scope
				loop
				SDL_DestroyTexture(texture)
			end scope
		end if
		'' We're done!
		done:
		SDL_Quit()
		return result
	end scope
end function

end SDL_RunApp(__FB_ARGC__, __FB_ARGV__, @example_main, 0)

'' end of showimage.bas
