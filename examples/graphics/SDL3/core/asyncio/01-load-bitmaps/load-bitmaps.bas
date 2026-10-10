'' Project: FreeBASIC SDL3 examples
'' File: load-bitmaps.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/asyncio/01-load-bitmaps/load-bitmaps.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example code loads a bitmap with asynchronous i/o and renders it.
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"

#define TOTAL_TEXTURES 4

'' use the callbacks instead of main()
'' We will use this renderer to draw into this window every frame.
dim shared window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
dim shared renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
dim shared queue as SDL_AsyncIOQueue ptr = cptr(SDL_AsyncIOQueue ptr, 0)
dim shared pngs(0 to 3) as const zstring ptr = {strptr("sample.png"), strptr("gamepad_front.png"), strptr("speaker.png"), strptr("icon2x.png")}
dim shared textures(0 to 3) as SDL_Texture ptr
dim shared texture_rects(0 to 3) as SDL_FRect = {type<SDL_FRect>(116, 156, 408, 167), type<SDL_FRect>(20, 200, 96, 60), type<SDL_FRect>(525, 180, 96, 96), type<SDL_FRect>(288, 375, 64, 64)}

declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

'' This function runs once at startup.
function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		dim i as long
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, strptr("Couldn't initialize SDL!"), SDL_GetError(), cptr(SDL_Window ptr, 0))
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/asyncio/load-bitmaps"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
			scope
				SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, strptr("Couldn't create window/renderer!"), SDL_GetError(), cptr(SDL_Window ptr, 0))
				return SDL_APP_FAILURE
			end scope
		end if
		SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX)
		queue = SDL_CreateAsyncIOQueue()
		if (queue = 0) then
			scope
				SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, strptr("Couldn't create async i/o queue!"), SDL_GetError(), cptr(SDL_Window ptr, 0))
				return SDL_APP_FAILURE
			end scope
		end if
		'' Load some .png files asynchronously from wherever the app is being run from, put them in the same queue.
		scope
			i = 0
			do while (i < (((sizeof(const zstring ptr) * 4) \ sizeof((pngs(0))))))
				scope
					dim path as zstring ptr = cptr(zstring ptr, 0)
					SDL_asprintf(@(path), strptr("%s%s"), SDL_GetBasePath(), pngs(i))
					'' allocate a string of the full file path
					'' you _should) check for failure, but we'll just go on without files here.
					SDL_LoadFileAsync(path, queue, cptr(any ptr, pngs(i)))
					'' attach the filename as app-specific data, so we can see it later.
					SDL_free(cptr(any ptr, path))
				end scope
				i += 1
			loop
		end scope
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs when a new event (mouse input, keypresses, etc) occurs.
function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
	scope
		if (event->type = SDL_EVENT_QUIT) then
			scope
				return SDL_APP_SUCCESS
			end scope
		end if
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once per frame, and is the heart of the program.
function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
	scope
		dim outcome as SDL_AsyncIOOutcome
		dim i as long
		if SDL_GetAsyncIOResult(queue, @(outcome)) then
			scope
				'' a .png file load has finished?
				if (outcome.result = SDL_ASYNCIO_COMPLETE) then
					scope
						'' this might be _any_ of the pngs; they might finish loading in any order.
						scope
							i = 0
							do while (i < (((sizeof(const zstring ptr) * 4) \ sizeof((pngs(0))))))
								scope
									'' this doesn't need a strcmp because we gave the pointer from this array to SDL_LoadFileAsync
									if (outcome.userdata = cptr(any ptr, pngs(i))) then
										scope
											exit do
										end scope
									end if
								end scope
								i += 1
							loop
						end scope
						if (i < (((sizeof(const zstring ptr) * 4) \ sizeof((pngs(0)))))) then
							scope
								'' (just in case.)
								dim surface as SDL_Surface ptr = SDL_LoadPNG_IO(SDL_IOFromConstMem(outcome.buffer, cast(uinteger, outcome.bytes_transferred)), true)
								if surface then
									scope
										'' the renderer is not multithreaded, so create the texture here once the data loads.
										textures(i) = SDL_CreateTextureFromSurface(renderer, surface)
										if (textures(i) = 0) then
											scope
												SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, strptr("Couldn't create texture!"), SDL_GetError(), cptr(SDL_Window ptr, 0))
												return SDL_APP_FAILURE
											end scope
										end if
										SDL_DestroySurface(surface)
									end scope
								end if
							end scope
						end if
					end scope
				end if
				SDL_free(outcome.buffer)
			end scope
		end if
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255)
		SDL_RenderClear(renderer)
		scope
			i = 0
			do while (i < (((sizeof(SDL_Texture ptr) * 4) \ sizeof((textures(0))))))
				scope
					SDL_RenderTexture(renderer, textures(i), cptr(const SDL_FRect ptr, 0), @(texture_rects(i)))
				end scope
				i += 1
			loop
		end scope
		SDL_RenderPresent(renderer)
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once at shutdown.
sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)
	scope
		dim i as long
		SDL_DestroyAsyncIOQueue(queue)
		scope
			i = 0
			do while (i < (((sizeof(SDL_Texture ptr) * 4) \ sizeof((textures(0))))))
				scope
					SDL_DestroyTexture(textures(i))
				end scope
				i += 1
			loop
		end scope
	end scope
end sub

#include once "callback-main.bi"

'' end of load-bitmaps.bas
