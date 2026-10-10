'' Project: FreeBASIC SDL addon examples
'' File: pango1.bas
'' Purpose: Render styled UTF-8 text through SDL_Pango and an SDL1 surface.
'' Responsibilities: Initialize text layout, blit its result, and release the context.
'' This file intentionally does NOT contain: direct Pango access or font discovery.

#define SDL_ADDON_API 1
#include once "../../SDL_common/example-common.bi"
#include once "SDL/SDL_Pango.bi"

function main() as integer
	if SDL_Init(SDL_INIT_VIDEO) <> 0 then return 1
	dim video as SDL_Surface ptr = SDL_SetVideoMode(640, 300, 32, 0)
	if video = 0 then
		SDL_Quit()
		return 1
	end if
	if SDLPango_Init() <> 0 then
		SDL_Quit()
		return 1
	end if
	dim context as SDLPango_Context ptr = SDLPango_CreateContext()
	if context = 0 then
		SDL_Quit()
		return 1
	end if
	SDLPango_SetDefaultColor(context, MATRIX_BLACK_BACK)
	SDLPango_SetMinimumSize(context, 600, 200)
	SDLPango_SetMarkup(context, strptr("<span size='xx-large' foreground='yellow'>FreeBASIC SDL_Pango</span>"), -1)
	dim image as SDL_Surface ptr = SDLPango_CreateSurfaceDraw(context)
	if image = 0 then
		SDLPango_FreeContext(context)
		SDL_Quit()
		return 1
	end if
	dim dest as SDL_Rect = (20, 40, 0, 0)
	SDL_FillRect(video, 0, SDL_MapRGB(video->format, 0, 0, 0))
	SDL_BlitSurface(image, 0, video, @dest)
	SDL_Flip(video)
	do
	loop until example_done()
	SDL_FreeSurface(image)
	SDLPango_FreeContext(context)
	SDL_Quit()
	return 0
end function

end main()

'' End of pango1.bas
