'' Project: FreeBASIC SDL addon examples
'' File: fontcache-common.bi
'' Purpose: Exercise text loading, measurement, and rendering through SDL_FontCache.
'' Responsibilities: Keep the font inside its render backend's lifetime and release it.
'' This file intentionally does NOT contain: direct glyph uploads or worker threads.

#pragma once
#include once "example-common.bi"
#if SDL_ADDON_API = 1
	#include once "SDL/SDL_FontCache.bi"
#else
	#include once "SDL2/SDL_FontCache.bi"
#endif

function main() as integer
	dim fontfile as string = command(1)
	if fontfile = "" then
		print "usage: fontcache font.ttf"
		return 1
	end if
	dim result as integer = 1
	dim font as FC_Font ptr
	#if SDL_ADDON_API = 1
		dim target as GPU_Target ptr = GPU_Init(640, 300, 0)
		if target = 0 then return 1
	#else
		dim win as SDL_Window ptr
		dim target as SDL_Renderer ptr
		if SDL_Init(SDL_INIT_VIDEO) <> 0 then return 1
		win = SDL_CreateWindow("SDL2 FontCache", 0, 0, 640, 300, 0)
		if win = 0 then
			SDL_Quit()
			return 1
		end if
		target = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE)
		if target = 0 then
			SDL_DestroyWindow(win)
			SDL_Quit()
			return 1
		end if
	#endif
	if TTF_Init() <> 0 then goto cleanup
	font = FC_CreateFont()
	if font = 0 then goto cleanup
	#if SDL_ADDON_API = 1
		if FC_LoadFont(font, strptr(fontfile), 28, FC_MakeColor(255, 240, 120, 255), TTF_STYLE_NORMAL) = 0 then goto cleanup
	#else
		if FC_LoadFont(font, target, strptr(fontfile), 28, FC_MakeColor(255, 240, 120, 255), TTF_STYLE_NORMAL) = 0 then goto cleanup
	#endif
	if FC_GetWidth(font, strptr("FreeBASIC FontCache")) = 0 then goto cleanup
	do
		#if SDL_ADDON_API = 1
			GPU_ClearRGBA(target, 15, 25, 40, 255)
		#else
			SDL_SetRenderDrawColor(target, 15, 25, 40, 255)
			SDL_RenderClear(target)
		#endif
		FC_Draw(font, target, 60.0, 100.0, strptr("FreeBASIC FontCache"))
		#if SDL_ADDON_API = 1
			GPU_Flip(target)
		#else
			SDL_RenderPresent(target)
		#endif
	loop until example_done()
	result = 0
cleanup:
	if result <> 0 then example_error("FontCache failed")
	if font <> 0 then FC_FreeFont(font)
	TTF_Quit()
	#if SDL_ADDON_API = 1
		GPU_Quit()
	#else
		SDL_DestroyRenderer(target)
		SDL_DestroyWindow(win)
		SDL_Quit()
	#endif
	return result
end function

'' End of fontcache-common.bi
