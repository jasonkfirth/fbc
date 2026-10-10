'' Project: FreeBASIC SDL3 examples
'' File: streaming-textures.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/renderer/07-streaming-textures/streaming-textures.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example creates an SDL window and renderer, and then draws a streaming
'' texture to it every frame.
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"

#define TEXTURE_SIZE 150
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

'' use the callbacks instead of main()
'' We will use this renderer to draw into this window every frame.
dim shared window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
dim shared renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
dim shared texture as SDL_Texture ptr = cptr(SDL_Texture ptr, 0)

declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

'' This function runs once at startup.
function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		SDL_SetAppMetadata(strptr("Example Renderer Streaming Textures"), strptr("1.0"), strptr("com.example.renderer-streaming-textures"))
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/renderer/streaming-textures"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't create window/renderer: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX)
		texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 150, 150)
		if (texture = 0) then
			scope
				SDL_Log_(strptr("Couldn't create streaming texture: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs when a new event (mouse input, keypresses, etc) occurs.
function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
	scope
		if (event->type = SDL_EVENT_QUIT) then
			scope
				return SDL_APP_SUCCESS
			end scope
		end if
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once per frame, and is the heart of the program.
function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
	scope
		dim dst_rect as SDL_FRect
		dim now as Uint64 = SDL_GetTicks()
		dim surface as SDL_Surface ptr = cptr(SDL_Surface ptr, 0)
		'' we'll have some color move around over a few seconds.
		dim direction as single = iif(((((now mod 2000)) >= 1000)), 1.0f, (-1.0f))
		dim scale as single = (((cast(single, (((cast(long, ((now mod 1000)))) - 500))) / 500.0f)) * direction)
		'' To update a streaming texture, you need to lock it first. This gets you access to the pixels.
		'' Note that this is considered a _write-only_ operation: the buffer you get from locking
		'' might not actually have the existing contents of the texture, and you have to write to every
		'' locked pixel!
		'' You can use SDL_LockTexture() to get an array of raw pixels, but we're going to use
		'' SDL_LockTextureToSurface() here, because it wraps that array in a temporary SDL_Surface,
		'' letting us use the surface drawing functions instead of lighting up individual pixels.
		if SDL_LockTextureToSurface(texture, cptr(const SDL_Rect ptr, 0), @(surface)) then
			scope
				dim r as SDL_Rect
				SDL_FillSurfaceRect(surface, cptr(const SDL_Rect ptr, 0), SDL_MapRGB(SDL_GetPixelFormatDetails(surface->format), cptr(const SDL_Palette ptr, 0), 0, 0, 0))
				'' make the whole surface black
				r.w = 150
				r.h = (150 \ 10)
				r.x = 0
				r.y = cast(long, (((cast(single, ((150 - r.h)))) * ((((scale + 1.0f)) / 2.0f)))))
				SDL_FillSurfaceRect(surface, @(r), SDL_MapRGB(SDL_GetPixelFormatDetails(surface->format), cptr(const SDL_Palette ptr, 0), 0, 255, 0))
				'' make a strip of the surface green
				SDL_UnlockTexture(texture)
			end scope
		end if
		'' as you can see from this, rendering draws over whatever was drawn before it.
		SDL_SetRenderDrawColor(renderer, 66, 66, 66, SDL_ALPHA_OPAQUE)
		'' grey, full alpha
		SDL_RenderClear(renderer)
		'' start with a blank canvas.
		'' Just draw the static texture a few times. You can think of it like a
		'' stamp, there isn't a limit to the number of times you can draw with it.
		'' Center this one. It'll draw the latest version of the texture we drew while it was locked.
		dst_rect.x = ((cast(single, ((640 - 150)))) / 2.0f)
		dst_rect.y = ((cast(single, ((480 - 150)))) / 2.0f)
		dst_rect.h = cast(single, 150)
		dst_rect.w = dst_rect.h
		SDL_RenderTexture(renderer, texture, cptr(const SDL_FRect ptr, 0), @(dst_rect))
		SDL_RenderPresent(renderer)
		'' put it all on the screen!
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once at shutdown.
sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)
	scope
		SDL_DestroyTexture(texture)
	end scope
end sub

#include once "callback-main.bi"

'' end of streaming-textures.bas
