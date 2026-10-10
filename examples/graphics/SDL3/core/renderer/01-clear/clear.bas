'' Project: FreeBASIC SDL3 examples
'' File: clear.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/renderer/01-clear/clear.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example code creates an SDL window and renderer, and then clears the
'' window to a different color every frame, so you'll effectively get a window
'' that's smoothly fading between colors.
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
		SDL_SetAppMetadata(strptr("Example Renderer Clear"), strptr("1.0"), strptr("com.example.renderer-clear"))
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/renderer/clear"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
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
		dim now as double = ((cast(double, SDL_GetTicks())) / 1000.0)
		'' convert from milliseconds to seconds.
		'' choose the color for the frame we will draw. The sine wave trick makes it fade between colors smoothly.
		dim red as single = cast(single, ((0.5 + (0.5 * SDL_sin(now)))))
		dim green as single = cast(single, ((0.5 + (0.5 * SDL_sin((now + ((SDL_PI_D * 2) / 3)))))))
		dim blue as single = cast(single, ((0.5 + (0.5 * SDL_sin((now + ((SDL_PI_D * 4) / 3)))))))
		SDL_SetRenderDrawColorFloat(renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT)
		'' new color, full alpha.
		'' clear the window to the draw color.
		SDL_RenderClear(renderer)
		'' put the newly-cleared rendering on the screen.
		SDL_RenderPresent(renderer)
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once at shutdown.
sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)
	scope
	end scope
end sub

#include once "callback-main.bi"

'' end of clear.bas
