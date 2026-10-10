'' Project: FreeBASIC SDL3 examples
'' File: primitives.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/renderer/02-primitives/primitives.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example creates an SDL window and renderer, and then draws some lines,
'' rectangles and points to it every frame.
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"

'' use the callbacks instead of main()
'' We will use this renderer to draw into this window every frame.
dim shared window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
dim shared renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
dim shared points(0 to 499) as SDL_FPoint

declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

'' This function runs once at startup.
function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		dim i as long
		SDL_SetAppMetadata(strptr("Example Renderer Primitives"), strptr("1.0"), strptr("com.example.renderer-primitives"))
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/renderer/primitives"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't create window/renderer: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX)
		'' set up some random points
		scope
			i = 0
			do while (i < (((sizeof(SDL_FPoint) * 500) \ sizeof((points(0))))))
				scope
					points(i).x = (((SDL_randf() * 440.0f)) + 100.0f)
					points(i).y = (((SDL_randf() * 280.0f)) + 100.0f)
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
		dim rect as SDL_FRect
		'' as you can see from this, rendering draws over whatever was drawn before it.
		SDL_SetRenderDrawColor(renderer, 33, 33, 33, SDL_ALPHA_OPAQUE)
		'' dark gray, full alpha
		SDL_RenderClear(renderer)
		'' start with a blank canvas.
		'' draw a filled rectangle in the middle of the canvas.
		SDL_SetRenderDrawColor(renderer, 0, 0, 255, SDL_ALPHA_OPAQUE)
		'' blue, full alpha
		rect.y = 100
		rect.x = rect.y
		rect.w = 440
		rect.h = 280
		SDL_RenderFillRect(renderer, @(rect))
		'' draw some points across the canvas.
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE)
		'' red, full alpha
		SDL_RenderPoints(renderer, @points(0), (((sizeof(SDL_FPoint) * 500) \ sizeof((points(0))))))
		'' draw a unfilled rectangle in-set a little bit.
		SDL_SetRenderDrawColor(renderer, 0, 255, 0, SDL_ALPHA_OPAQUE)
		'' green, full alpha
		rect.x += 30
		rect.y += 30
		rect.w -= 60
		rect.h -= 60
		SDL_RenderRect(renderer, @(rect))
		'' draw two lines in an X across the whole canvas.
		SDL_SetRenderDrawColor(renderer, 255, 255, 0, SDL_ALPHA_OPAQUE)
		'' yellow, full alpha
		SDL_RenderLine(renderer, 0, 0, 640, 480)
		SDL_RenderLine(renderer, 0, 480, 640, 0)
		SDL_RenderPresent(renderer)
		'' put it all on the screen!
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once at shutdown.
sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)
	scope
	end scope
end sub

#include once "callback-main.bi"

'' end of primitives.bas
