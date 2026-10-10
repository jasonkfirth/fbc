'' Project: FreeBASIC SDL3 examples
'' File: rectangles.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/renderer/05-rectangles/rectangles.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example creates an SDL window and renderer, and then draws some
'' rectangles to it every frame.
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

'' use the callbacks instead of main()
'' We will use this renderer to draw into this window every frame.
dim shared window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
dim shared renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)

declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

'' This function runs once at startup.
function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		SDL_SetAppMetadata(strptr("Example Renderer Rectangles"), strptr("1.0"), strptr("com.example.renderer-rectangles"))
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/renderer/rectangles"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't create window/renderer: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX)
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
		dim rects(0 to 15) as SDL_FRect
		dim now as Uint64 = SDL_GetTicks()
		dim i as long
		'' we'll have the rectangles grow and shrink over a few seconds.
		dim direction as single = iif(((((now mod 2000)) >= 1000)), 1.0f, (-1.0f))
		dim scale as single = (((cast(single, (((cast(long, ((now mod 1000)))) - 500))) / 500.0f)) * direction)
		'' as you can see from this, rendering draws over whatever was drawn before it.
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE)
		'' black, full alpha
		SDL_RenderClear(renderer)
		'' start with a blank canvas.
		'' Rectangles are comprised of set of X and Y coordinates, plus width and
		'' height. (0, 0) is the top left of the window, and larger numbers go
		'' down and to the right. This isn't how geometry works, but this is
		'' pretty standard in 2D graphics.
		'' Let's draw a single rectangle (square, really).
		rects(0).y = 100
		rects(0).x = rects(0).y
		rects(0).h = (100 + ((100 * scale)))
		rects(0).w = rects(0).h
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE)
		'' red, full alpha
		SDL_RenderRect(renderer, @(rects(0)))
		'' Now let's draw several rectangles with one function call.
		scope
			i = 0
			do while (i < 3)
				scope
					dim size as single = (((i + 1)) * 50.0f)
					rects(i).h = (size + ((size * scale)))
					rects(i).w = rects(i).h
					rects(i).x = (((640 - rects(i).w)) / 2)
					'' center it.
					rects(i).y = (((480 - rects(i).h)) / 2)
				end scope
				i += 1
			loop
		end scope
		SDL_SetRenderDrawColor(renderer, 0, 255, 0, SDL_ALPHA_OPAQUE)
		'' green, full alpha
		SDL_RenderRects(renderer, @rects(0), 3)
		'' draw three rectangles at once
		'' those were rectangle _outlines_, really. You can also draw _filled_ rectangles!
		rects(0).x = 400
		rects(0).y = 50
		rects(0).w = (100 + ((100 * scale)))
		rects(0).h = (50 + ((50 * scale)))
		SDL_SetRenderDrawColor(renderer, 0, 0, 255, SDL_ALPHA_OPAQUE)
		'' blue, full alpha
		SDL_RenderFillRect(renderer, @(rects(0)))
		'' ...and also fill a bunch of rectangles at once...
		scope
			i = 0
			do while (i < (((sizeof(SDL_FRect) * 16) \ sizeof((rects(0))))))
				scope
					dim w as single = ((cast(single, 640) / (((sizeof(SDL_FRect) * 16) \ sizeof((rects(0)))))))
					dim h as single = (i * 8.0f)
					rects(i).x = (i * w)
					rects(i).y = (480 - h)
					rects(i).w = w
					rects(i).h = h
				end scope
				i += 1
			loop
		end scope
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE)
		'' white, full alpha
		SDL_RenderFillRects(renderer, @rects(0), (((sizeof(SDL_FRect) * 16) \ sizeof((rects(0))))))
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

'' end of rectangles.bas
