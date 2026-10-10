'' Project: FreeBASIC SDL addon examples
'' File: rtf-common.bi
'' Purpose: Load and render the same RTF document through each supported SDL generation.
'' Responsibilities: Retain the callback engine, render the document, and free it before TTF.
'' This file intentionally does NOT contain: an RTF parser or an alternate font backend.

#pragma once
#include once "example-common.bi"
#include once "rtf-font-engine.bi"

function main() as integer
	dim filename as string = command(1)
	rtf_fontfile = command(2)
	if filename = "" or rtf_fontfile = "" then
		print "usage: rtf file.rtf font.ttf"
		return 1
	end if
	dim result as integer = 1
	dim context as RTF_Context ptr
	dim engine as RTF_FontEngine
	dim rect as SDL_Rect = (20, 20, 600, 360)
	engine.version = RTF_FONT_ENGINE_VERSION
	engine.CreateFont = @rtf_create_font
	engine.GetLineSpacing = @rtf_line_spacing
	engine.GetCharacterOffsets = @rtf_offsets
	engine.RenderText = @rtf_render_text
	engine.FreeFont = @rtf_free_font
	#if SDL_ADDON_API = 3
		if not SDL_Init(SDL_INIT_VIDEO) then return 1
	#else
		if SDL_Init(SDL_INIT_VIDEO) <> 0 then return 1
	#endif
	#if SDL_ADDON_API = 1
		dim video as SDL_Surface ptr = SDL_SetVideoMode(640, 400, 32, 0)
		if video = 0 then
			SDL_Quit()
			return 1
		end if
	#else
		dim win as SDL_Window ptr
		dim renderer as SDL_Renderer ptr
		#if SDL_ADDON_API = 3
			if not SDL_CreateWindowAndRenderer("SDL RTF", 640, 400, 0, @win, @renderer) then
				SDL_Quit()
				return 1
			end if
		#else
			win = SDL_CreateWindow("SDL RTF", 0, 0, 640, 400, 0)
			if win = 0 then
				SDL_Quit()
				return 1
			end if
			renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE)
			if renderer = 0 then
				SDL_DestroyWindow(win)
				SDL_Quit()
				return 1
			end if
		#endif
	#endif
	#if SDL_ADDON_API = 3
		if not TTF_Init() then goto cleanup
	#else
		if TTF_Init() <> 0 then goto cleanup
	#endif
	#if SDL_ADDON_API = 1
		context = RTF_CreateContext(@engine)
	#else
		context = RTF_CreateContext(renderer, @engine)
	#endif
	if context = 0 then goto cleanup
	#if SDL_ADDON_API = 3
		if not RTF_Load(context, strptr(filename)) then goto cleanup
	#else
		if RTF_Load(context, strptr(filename)) <> 0 then goto cleanup
	#endif
	if RTF_GetHeight(context, 600) <= 0 then goto cleanup
	do
		#if SDL_ADDON_API = 1
			SDL_FillRect(video, 0, SDL_MapRGB(video->format, 240, 240, 240))
			RTF_Render(context, video, @rect, 0)
			SDL_Flip(video)
		#else
			SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255)
			SDL_RenderClear(renderer)
			RTF_Render(context, @rect, 0)
			SDL_RenderPresent(renderer)
		#endif
	loop until example_done()
	result = 0
cleanup:
	if result <> 0 then example_error("RTF rendering failed")
	if context <> 0 then RTF_FreeContext(context)
	TTF_Quit()
	#if SDL_ADDON_API <> 1
		SDL_DestroyRenderer(renderer)
		SDL_DestroyWindow(win)
	#endif
	SDL_Quit()
	return result
end function

'' End of rtf-common.bi
