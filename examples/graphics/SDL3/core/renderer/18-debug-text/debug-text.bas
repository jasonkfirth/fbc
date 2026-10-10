'' Project: FreeBASIC SDL3 examples
'' File: debug-text.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/renderer/18-debug-text/debug-text.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example creates an SDL window and renderer, and then draws some text
'' using SDL_RenderDebugText() every frame.
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
		SDL_SetAppMetadata(strptr("Example Renderer Debug Texture"), strptr("1.0"), strptr("com.example.renderer-debug-text"))
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/renderer/debug-text"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
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
		dim charsize as long = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE
		'' as you can see from this, rendering draws over whatever was drawn before it.
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE)
		'' black, full alpha
		SDL_RenderClear(renderer)
		'' start with a blank canvas.
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE)
		'' white, full alpha
		SDL_RenderDebugText(renderer, 272, 100, strptr("Hello world!"))
		SDL_RenderDebugText(renderer, 224, 150, strptr("This is some debug text."))
		SDL_SetRenderDrawColor(renderer, 51, 102, 255, SDL_ALPHA_OPAQUE)
		'' light blue, full alpha
		SDL_RenderDebugText(renderer, 184, 200, strptr("You can do it in different colors."))
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE)
		'' white, full alpha
		SDL_SetRenderScale(renderer, 4.0f, 4.0f)
		SDL_RenderDebugText(renderer, 14, 65, strptr("It can be scaled."))
		SDL_SetRenderScale(renderer, 1.0f, 1.0f)
		SDL_RenderDebugText(renderer, 64, 350, strptr(!"This only does ASCII chars. So this laughing emoji won't draw: \&o360\&o237\&o244\&o243"))
		SDL_RenderDebugTextFormat(renderer, ((cast(single, ((640 - ((charsize * 46))))) / 2)), 400, strptr("(This program has been running for %" SDL_PRIu64 " seconds.)"), cast(Uint64, (SDL_GetTicks() / 1000)))
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

'' end of debug-text.bas
