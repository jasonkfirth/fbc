'' Project: FreeBASIC SDL addon examples
'' File: ttf2.bas
'' Purpose: Render TrueType text with SDL2_ttf and display the resulting surface.
'' Responsibilities: Keep the font alive during rendering and release each resource.
'' This file intentionally does NOT contain: font discovery or text editing.

#define SDL_ADDON_API 2
#include once "../../SDL_common/display2-common.bi"
#include once "SDL2/SDL_ttf.bi"

function main() as integer
	dim filename as string = command(1)
	if filename = "" then
		print "usage: ttf2 font.ttf"
		return 1
	end if
	dim win as SDL_Window ptr
	dim renderer as SDL_Renderer ptr
	if not display2_open("SDL2 TrueType", win, renderer) then return 1
	dim exitStatus as integer = 1
	dim font as TTF_Font ptr
	dim surface as SDL_Surface ptr
	dim texture as SDL_Texture ptr
	do
		if TTF_Init() <> 0 then exit do
		font = TTF_OpenFont(strptr(filename), 28)
		if font = 0 then exit do
		dim foreground as SDL_Color = (240, 220, 40, 255)
		surface = TTF_RenderUTF8_Blended(font, "FreeBASIC SDL2_ttf", foreground)
		if surface = 0 then exit do
		texture = SDL_CreateTextureFromSurface(renderer, surface)
		if texture = 0 then exit do
		dim destination as SDL_Rect = (40, 200, surface->w, surface->h)
		dim presented as boolean = true
		do
			SDL_SetRenderDrawColor(renderer, 15, 20, 30, 255)
			SDL_RenderClear(renderer)
			if SDL_RenderCopy(renderer, texture, 0, @destination) <> 0 then
				presented = false
				exit do
			end if
			SDL_RenderPresent(renderer)
		loop until example_done()
		if not presented then exit do
		exitStatus = 0
	loop while false
	if exitStatus <> 0 then example_error("font rendering failed")
	if texture <> 0 then SDL_DestroyTexture(texture)
	if surface <> 0 then SDL_FreeSurface(surface)
	if font <> 0 then TTF_CloseFont(font)
	TTF_Quit()
	display2_close(win, renderer)
	return exitStatus
end function

end main()

'' End of ttf2.bas
