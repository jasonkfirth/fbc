'' Project: FreeBASIC SDL3 examples
'' File: showclipboard.bas
'' Purpose: Port upstream SDL3_image-3.4.8/examples/showclipboard.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' showclipboard:  A test application for the SDL image loading library.
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

declare function load_clipboard cdecl(byval window_ as SDL_Window ptr, byval renderer as SDL_Renderer ptr) as SDL_Texture ptr
declare sub draw_background cdecl(byval renderer as SDL_Renderer ptr)
declare function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long

function load_clipboard cdecl(byval window_ as SDL_Window ptr, byval renderer as SDL_Renderer ptr) as SDL_Texture ptr
	scope
		dim texture as SDL_Texture ptr = cptr(SDL_Texture ptr, 0)
		dim surface as SDL_Surface ptr = IMG_GetClipboardImage()
		if surface then
			scope
				dim text as zstring ptr = SDL_GetClipboardText()
				if (text andalso (*cptr(byte ptr, text))) then
					scope
						SDL_SetWindowTitle(window_, text)
					end scope
				else
					scope
						SDL_SetWindowTitle(window_, strptr("Copy an image and click here"))
					end scope
				end if
				SDL_free(cptr(any ptr, text))
				texture = SDL_CreateTextureFromSurface(renderer, surface)
				SDL_SetWindowTitle(window_, SDL_GetClipboardText())
				SDL_SetWindowSize(window_, surface->w, surface->h)
				SDL_SetRenderLogicalPresentation(renderer, surface->w, surface->h, SDL_LOGICAL_PRESENTATION_LETTERBOX)
				SDL_DestroySurface(surface)
			end scope
		end if
		return texture
	end scope
end function

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

function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long
	scope
		dim window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
		dim renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
		dim texture as SDL_Texture ptr = cptr(SDL_Texture ptr, 0)
		dim flags as Uint32 = 0
		dim i as long
		dim done as boolean = false
		dim event as SDL_Event
		dim result as long = 0
		'' Check command line usage
		scope
			i = 1
			do while argv[i]
				scope
					if (SDL_strcmp(argv[i], strptr("-fullscreen")) = 0) then
						scope
							SDL_HideCursor()
							flags or= SDL_WINDOW_FULLSCREEN
						end scope
					else
						scope
							SDL_Log_(strptr(!"Usage: %s [-fullscreen]\n"), argv[0])
							result = 1
							goto done
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
		if (SDL_CreateWindowAndRenderer(strptr(""), 640, 480, flags, @(window_), @(renderer)) = 0) then
			scope
				SDL_Log_(strptr(!"SDL_CreateWindowAndRenderer() failed: %s\n"), SDL_GetError())
				result = 2
				goto done
			end scope
		end if
		texture = load_clipboard(window_, renderer)
		do
			if ((done = 0)) = 0 then exit do
			scope
				do
					if (SDL_PollEvent(@(event))) = 0 then exit do
					scope
						select case event.type
							case SDL_EVENT_CLIPBOARD_UPDATE
								goto switch_case_6
							case SDL_EVENT_KEY_UP
								goto switch_case_7
							case SDL_EVENT_QUIT
								goto switch_case_8
							case else
								goto switch_case_9
						end select
						switch_case_6:
						scope
							texture = load_clipboard(window_, renderer)
							goto switch_done_10
						end scope
						switch_case_7:
						scope
							select case event.key.key
								case (27u), (113u)
									goto switch_case_11
								case else
									goto switch_done_12
							end select
							switch_case_11:
							scope
								done = true
								goto switch_done_12
							end scope
							switch_done_12:
							goto switch_done_10
						end scope
						switch_case_8:
						scope
							done = true
							goto switch_done_10
						end scope
						switch_case_9:
						scope
							goto switch_done_10
						end scope
						switch_done_10:
					end scope
				loop
				'' Draw a background pattern in case the image has transparency
				draw_background(renderer)
				'' Display the image
				if texture then
					scope
						SDL_RenderTexture(renderer, texture, cptr(const SDL_FRect ptr, 0), cptr(const SDL_FRect ptr, 0))
					end scope
				end if
				SDL_RenderPresent(renderer)
				SDL3_ExampleSmokeFrame()
				SDL_Delay(100)
			end scope
		loop
		if texture then
			scope
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

'' end of showclipboard.bas
