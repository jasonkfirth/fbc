'' Project: FreeBASIC SDL addon examples
'' File: example-common.bi
'' Purpose: Share bounded display loops and error reporting between addon examples.
'' Responsibilities: Select the SDL generation and leave each addon owning its resources.
'' This file intentionally does NOT contain: addon initialization or rendering.

#pragma once

#if SDL_ADDON_API = 3
	#include once "SDL3/SDL.bi"
#elseif SDL_ADDON_API = 2
	#include once "SDL2/SDL.bi"
#else
	#include once "SDL/SDL.bi"
#endif

private sub example_error(byref message as string)
	print message; ": "; *SDL_GetError()
end sub

private function example_done() as boolean
	static frames as ulong
	static limit as integer
	static initialized as boolean
	if not initialized then
		limit = valint(environ("FB_SDL_ADDON_FRAMES"))
		initialized = true
	end if
	dim event as SDL_Event
	while SDL_PollEvent(@event)
	#if SDL_ADDON_API = 3
		if event.type = SDL_EVENT_QUIT then return true
		if event.type = SDL_EVENT_KEY_DOWN then
	#else
		if event.type = SDL_QUIT_ then return true
		if event.type = SDL_KEYDOWN then
	#endif
		#if SDL_ADDON_API = 3
			if event.key.key = SDLK_ESCAPE then return true
		#else
			if event.key.keysym.sym = SDLK_ESCAPE then return true
		#endif
		end if
	wend
	frames += 1
	if limit > 0 andalso frames >= cuint(limit) then return true
	SDL_Delay(16)
	return false
end function

'' End of example-common.bi
