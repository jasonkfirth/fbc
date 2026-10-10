'' Project: FreeBASIC SDL addon examples
'' File: image2.bas
'' Purpose: Decode a PNG with SDL2_image and display it through an SDL2 texture.
'' Responsibilities: Check the decoder, present its output, and release owned resources.
'' This file intentionally does NOT contain: image encoding or background loading.

#define SDL_ADDON_API 2
#include once "../../SDL_common/display2-common.bi"
#include once "SDL2/SDL_image.bi"

function main() as integer
	dim filename as string = command(1)
	if filename = "" then
		print "usage: image2 image.png"
		return 1
	end if
	dim win as SDL_Window ptr
	dim renderer as SDL_Renderer ptr
	if not display2_open("SDL2 image", win, renderer) then return 1
	dim exitStatus as integer = 1
	dim surface as SDL_Surface ptr
	dim texture as SDL_Texture ptr
	do
		if (IMG_Init(IMG_INIT_PNG) and IMG_INIT_PNG) = 0 then exit do
		surface = IMG_Load(strptr(filename))
		if surface = 0 then exit do
		texture = SDL_CreateTextureFromSurface(renderer, surface)
		if texture = 0 then exit do
		dim presented as boolean = true
		do
			SDL_SetRenderDrawColor(renderer, 15, 20, 30, 255)
			SDL_RenderClear(renderer)
			if SDL_RenderCopy(renderer, texture, 0, 0) <> 0 then
				presented = false
				exit do
			end if
			SDL_RenderPresent(renderer)
		loop until example_done()
		if not presented then exit do
		exitStatus = 0
	loop while false
	if exitStatus <> 0 then example_error("image loading failed")
	if texture <> 0 then SDL_DestroyTexture(texture)
	if surface <> 0 then SDL_FreeSurface(surface)
	IMG_Quit()
	display2_close(win, renderer)
	return exitStatus
end function

end main()

'' End of image2.bas
