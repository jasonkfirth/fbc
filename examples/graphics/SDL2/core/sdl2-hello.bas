'' Project: FreeBASIC SDL examples
'' File: sdl2-hello.bas
'' Purpose:
''     Display an image and rendered font using SDL2.
'' Responsibilities:
''     Own the window, renderer, textures, and font through cleanup.
'' This file intentionally does NOT contain:
''     SDL1 APIs or animation.
''
#include "SDL2/SDL.bi"
#include "SDL2/SDL_image.bi"
#include "SDL2/SDL_ttf.bi"

dim sdlversion as SDL_version
SDL_GetVersion(@sdlversion)
print "SDL2 version = "; SDL_VERSIONNUM(sdlversion.major, sdlversion.minor, sdlversion.patch)

var sdlimageversion = IMG_Linked_Version()
print "SDL2_image version = "; SDL_VERSIONNUM(sdlimageversion->major, sdlimageversion->minor, sdlimageversion->patch)

var sdlttfversion = TTF_Linked_Version()
print "SDL2_ttf version = "; SDL_VERSIONNUM(sdlttfversion->major, sdlttfversion->minor, sdlttfversion->patch)

function main() as integer
	dim win as SDL_Window ptr
	dim renderer as SDL_Renderer ptr
	dim surfaceHorse as SDL_Surface ptr, surfaceHello as SDL_Surface ptr
	dim textureHorse as SDL_Texture ptr, textureHello as SDL_Texture ptr
	dim font as TTF_Font ptr
	dim rcHorse as SDL_Rect, rcHello as SDL_Rect
	dim event as SDL_Event
	dim done as integer
	dim result as integer = 1
	const TEXT = "Hello!"

	if SDL_Init(SDL_INIT_VIDEO) <> 0 then
		print "SDL_Init: "; *SDL_GetError()
		return 1
	end if
	'' The built-in TGA decoder needs no additional IMG_Init codec flags.
	IMG_Init(0)
	if TTF_Init() <> 0 then
		print "TTF_Init: "; *TTF_GetError()
		goto cleanup
	end if

	win = SDL_CreateWindow("SDL2 test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0)
	if win = NULL then
		print "SDL_CreateWindow: "; *SDL_GetError()
		goto cleanup
	end if
	renderer = SDL_CreateRenderer(win, -1, 0)
	if renderer = NULL then
		print "SDL_CreateRenderer: "; *SDL_GetError()
		goto cleanup
	end if

	'' Load the image into a texture, retaining its actual size for rendering.
	surfaceHorse = IMG_Load("data/horse.tga")
	if surfaceHorse = NULL then
		print "IMG_Load: "; *IMG_GetError()
		goto cleanup
	end if
	rcHorse.w = surfaceHorse->w
	rcHorse.h = surfaceHorse->h
	textureHorse = SDL_CreateTextureFromSurface(renderer, surfaceHorse)
	if textureHorse = NULL then
		print "SDL_CreateTextureFromSurface: "; *SDL_GetError()
		goto cleanup
	end if
	SDL_FreeSurface(surfaceHorse) : surfaceHorse = NULL

	'' Draw text into a texture. SDL_Color includes an alpha byte in SDL2.
	font = TTF_OpenFont("data/Vera.ttf", 36)
	if font = NULL then
		print "TTF_OpenFont: "; *TTF_GetError()
		goto cleanup
	end if
	surfaceHello = TTF_RenderText_Solid(font, TEXT, type<SDL_Color>(255, 0, 0, 255))
	if surfaceHello = NULL then
		print "TTF_RenderText_Solid: "; *TTF_GetError()
		goto cleanup
	end if
	textureHello = SDL_CreateTextureFromSurface(renderer, surfaceHello)
	if textureHello = NULL then
		print "SDL_CreateTextureFromSurface: "; *SDL_GetError()
		goto cleanup
	end if
	SDL_FreeSurface(surfaceHello) : surfaceHello = NULL

	'' Measure the text so SDL_RenderCopy draws it without scaling.
	if TTF_SizeText(font, TEXT, @rcHello.w, @rcHello.h) <> 0 then
		print "TTF_SizeText: "; *TTF_GetError()
		goto cleanup
	end if
	rcHello.x = 100
	rcHello.y = 100

	do
		if SDL_RenderClear(renderer) <> 0 or _
		   SDL_RenderCopy(renderer, textureHorse, NULL, @rcHorse) <> 0 or _
		   SDL_RenderCopy(renderer, textureHello, NULL, @rcHello) <> 0 then
			print "Rendering failed: "; *SDL_GetError()
			goto cleanup
		end if
		SDL_RenderPresent(renderer)

		if SDL_WaitEvent(@event) = 0 then
			print "SDL_WaitEvent: "; *SDL_GetError()
			goto cleanup
		end if
		select case event.type
		case SDL_QUIT_
			done = 1
		case SDL_KEYDOWN
			if event.key.keysym.sym = SDLK_ESCAPE then done = 1
		end select
	loop until done
	result = 0

cleanup:
	'' Textures belong to this renderer; release them before the renderer.
	if surfaceHello <> NULL then SDL_FreeSurface(surfaceHello)
	if surfaceHorse <> NULL then SDL_FreeSurface(surfaceHorse)
	if textureHello <> NULL then SDL_DestroyTexture(textureHello)
	if textureHorse <> NULL then SDL_DestroyTexture(textureHorse)
	if font <> NULL then TTF_CloseFont(font)
	if renderer <> NULL then SDL_DestroyRenderer(renderer)
	if win <> NULL then SDL_DestroyWindow(win)
	TTF_Quit()
	IMG_Quit()
	SDL_Quit()
	return result
end function

end main()

'' End of sdl2-hello.bas
