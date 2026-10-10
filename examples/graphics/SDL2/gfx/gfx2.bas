'' Project: FreeBASIC SDL addon examples
'' File: gfx2.bas
'' Purpose: Exercise SDL2_gfx drawing, font, frame-rate, filter, and rotation APIs.
'' Responsibilities: Check CPU transforms and present a scene using the SDL2 renderer.
'' This file intentionally does NOT contain: custom shaders or application threads.

#define SDL_ADDON_API 2
#include once "../../SDL_common/display2-common.bi"
#include once "SDL2/SDL2_gfx_primitives.bi"
#include once "SDL2/SDL2_gfx_framerate.bi"
#include once "SDL2/SDL2_gfx_imageFilter.bi"
#include once "SDL2/SDL2_gfx_rotozoom.bi"

function main() as integer
	dim bytes(0 to 3) as Uint8 = {1, 2, 3, 4}
	dim result(0 to 3) as Uint8
	if SDL_imageFilterAddByte(@bytes(0), @result(0), 4, 5) <> 0 then return 1
	for i as integer = 0 to 3
		if result(i) <> bytes(i) + 5 then return 1
	next
	dim win as SDL_Window ptr
	dim renderer as SDL_Renderer ptr
	if not display2_open("SDL2 gfx", win, renderer) then return 1
	dim exitStatus as integer = 1
	dim surface as SDL_Surface ptr = SDL_CreateRGBSurfaceWithFormat(0, 16, 16, 32, SDL_PIXELFORMAT_RGBA32)
	dim rotated as SDL_Surface ptr
	do
		if surface = 0 then exit do
		rotated = rotozoomSurface(surface, 45.0, 2.0, 1)
		if rotated = 0 then exit do
		dim manager as FPSmanager
		SDL_initFramerate(@manager)
		if SDL_setFramerate(@manager, 60) <> 0 then exit do
		do
			SDL_SetRenderDrawColor(renderer, 15, 20, 30, 255)
			SDL_RenderClear(renderer)
			circleRGBA(renderer, 320, 220, 110, 255, 220, 0, 255)
			boxRGBA(renderer, 80, 80, 180, 180, 40, 180, 240, 255)
			thickLineRGBA(renderer, 100, 350, 540, 350, 8, 230, 80, 80, 255)
			stringRGBA(renderer, 200, 410, "FreeBASIC SDL2_gfx", 255, 255, 255, 255)
			SDL_RenderPresent(renderer)
			SDL_framerateDelay(@manager)
		loop until example_done()
		exitStatus = 0
	loop while false
	if exitStatus <> 0 then example_error("gfx transform or drawing failed")
	if rotated <> 0 then SDL_FreeSurface(rotated)
	if surface <> 0 then SDL_FreeSurface(surface)
	display2_close(win, renderer)
	return exitStatus
end function

end main()

'' End of gfx2.bas
