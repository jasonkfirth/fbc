'' Project: FreeBASIC SDL3 examples
'' File: lines.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/renderer/03-lines/lines.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example creates an SDL window and renderer, and then draws some lines
'' to it every frame.
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"

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
		SDL_SetAppMetadata(strptr("Example Renderer Lines"), strptr("1.0"), strptr("com.example.renderer-lines"))
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/renderer/lines"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
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
		dim i as long
		'' Lines (line segments, really) are drawn in terms of points: a set of
		'' X and Y coordinates, one set for each end of the line.
		'' (0, 0) is the top left of the window, and larger numbers go down
		'' and to the right. This isn't how geometry works, but this is pretty
		'' standard in 2D graphics.
		static line_points(0 to 8) as SDL_FPoint = {type<SDL_FPoint>(100, 354), type<SDL_FPoint>(220, 230), type<SDL_FPoint>(140, 230), type<SDL_FPoint>(320, 100), type<SDL_FPoint>(500, 230), type<SDL_FPoint>(420, 230), type<SDL_FPoint>(540, 354), type<SDL_FPoint>(400, 354), type<SDL_FPoint>(100, 354)}
		'' as you can see from this, rendering draws over whatever was drawn before it.
		SDL_SetRenderDrawColor(renderer, 100, 100, 100, SDL_ALPHA_OPAQUE)
		'' grey, full alpha
		SDL_RenderClear(renderer)
		'' start with a blank canvas.
		'' You can draw lines, one at a time, like these brown ones...
		SDL_SetRenderDrawColor(renderer, 127, 49, 32, SDL_ALPHA_OPAQUE)
		SDL_RenderLine(renderer, 240, 450, 400, 450)
		SDL_RenderLine(renderer, 240, 356, 400, 356)
		SDL_RenderLine(renderer, 240, 356, 240, 450)
		SDL_RenderLine(renderer, 400, 356, 400, 450)
		'' You can also draw a series of connected lines in a single batch...
		SDL_SetRenderDrawColor(renderer, 0, 255, 0, SDL_ALPHA_OPAQUE)
		SDL_RenderLines(renderer, @line_points(0), (((sizeof(SDL_FPoint) * 9) \ sizeof((line_points(0))))))
		'' here's a bunch of lines drawn out from a center point in a circle.
		'' we randomize the color of each line, so it functions as animation.
		scope
			i = 0
			do while (i < 360)
				scope
					dim size as single = 30.0f
					dim x as single = 320.0f
					dim y as single = (95.0f - ((size / 2.0f)))
					dim r as single = (cast(single, i) * ((SDL_PI_F / 180.0f)))
					SDL_SetRenderDrawColor(renderer, SDL_rand(256), SDL_rand(256), SDL_rand(256), SDL_ALPHA_OPAQUE)
					SDL_RenderLine(renderer, x, y, (x + (SDL_cosf(r) * size)), (y + (SDL_sinf(r) * size)))
				end scope
				i += 1
			loop
		end scope
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

'' end of lines.bas
