'' Project: FreeBASIC SDL addon examples
'' File: display2-common.bi
'' Purpose: Own the SDL2 window and renderer used by the small drawing examples.
'' Responsibilities: Check window creation and release resources in dependency order.
'' This file intentionally does NOT contain: addon resources or drawing commands.

#pragma once
#include once "example-common.bi"

private function display2_open(byval title as const zstring ptr, _
	byref win as SDL_Window ptr, byref renderer as SDL_Renderer ptr) as boolean
	if SDL_Init(SDL_INIT_VIDEO) <> 0 then
		example_error("SDL_Init failed")
		return false
	end if
	if SDL_CreateWindowAndRenderer(640, 480, 0, @win, @renderer) <> 0 then
		example_error("window creation failed")
		if renderer <> 0 then SDL_DestroyRenderer(renderer)
		if win <> 0 then SDL_DestroyWindow(win)
		SDL_Quit()
		return false
	end if
	SDL_SetWindowTitle(win, title)
	return true
end function

private sub display2_close(byval win as SDL_Window ptr, byval renderer as SDL_Renderer ptr)
	SDL_DestroyRenderer(renderer)
	SDL_DestroyWindow(win)
	SDL_Quit()
end sub

'' End of display2-common.bi
