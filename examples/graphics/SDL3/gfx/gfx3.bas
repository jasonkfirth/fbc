'' Project: FreeBASIC SDL addon examples
'' File: gfx3.bas
'' Purpose: Exercise SDL3_gfx primitives, fonts, image filtering, and rotation.
'' Responsibilities: Check CPU transforms, display primitives, and release the renderer.
'' This file intentionally does NOT contain: custom shaders or application-owned threads.

#define SDL_ADDON_API 3
#include once "../../SDL_common/example-common.bi"
#include once "SDL3/SDL3_gfxPrimitives.bi"
#include once "SDL3/SDL3_framerate.bi"
#include once "SDL3/SDL3_imageFilter.bi"
#include once "SDL3/SDL3_rotozoom.bi"

function main() as integer
	dim bytes(0 to 3) as Uint8 = {1, 2, 3, 4}
	dim result(0 to 3) as Uint8
	if SDL_imageFilterAddByte(@bytes(0), @result(0), 4, 5) <> 0 then return 1
	for i as integer = 0 to 3
		if result(i) <> bytes(i) + 5 then return 1
	next
	if not SDL_Init(SDL_INIT_VIDEO) then
		example_error("SDL_Init failed")
		return 1
	end if
	dim win as SDL_Window ptr
	dim renderer as SDL_Renderer ptr
	if not SDL_CreateWindowAndRenderer("SDL3 gfx", 640, 480, 0, @win, @renderer) then
		example_error("Window creation failed")
		SDL_Quit()
		return 1
	end if
	dim surface as SDL_Surface ptr = SDL_CreateSurface(16, 16, SDL_PIXELFORMAT_RGBA32)
	if surface = 0 then
		SDL_DestroyRenderer(renderer)
		SDL_DestroyWindow(win)
		SDL_Quit()
		return 1
	end if
	dim rotated as SDL_Surface ptr = rotozoomSurface(surface, 45.0, 2.0, 1)
	if rotated = 0 then
		SDL_DestroySurface(surface)
		SDL_DestroyRenderer(renderer)
		SDL_DestroyWindow(win)
		SDL_Quit()
		return 1
	end if
	SDL_DestroySurface(rotated)
	SDL_DestroySurface(surface)
	dim manager as FPSmanager
	SDL_initFramerate(@manager)
	if SDL_setFramerate(@manager, 60) <> 0 then
		SDL_DestroyRenderer(renderer)
		SDL_DestroyWindow(win)
		SDL_Quit()
		return 1
	end if
	do
		SDL_SetRenderDrawColor(renderer, 15, 20, 30, 255)
		SDL_RenderClear(renderer)
		circleRGBA(renderer, 320, 220, 110, 255, 220, 0, 255)
		boxRGBA(renderer, 80, 80, 180, 180, 40, 180, 240, 255)
		thickLineRGBA(renderer, 100, 350, 540, 350, 8, 230, 80, 80, 255)
		stringRGBA(renderer, 200, 410, strptr("FreeBASIC SDL3_gfx"), 255, 255, 255, 255)
		SDL_RenderPresent(renderer)
		SDL_framerateDelay(@manager)
	loop until example_done()
	SDL_DestroyRenderer(renderer)
	SDL_DestroyWindow(win)
	SDL_Quit()
	return 0
end function

end main()

'' End of gfx3.bas
