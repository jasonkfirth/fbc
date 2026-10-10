'' Project: FreeBASIC SDL3 examples
'' File: viewport.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/renderer/14-viewport/viewport.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example creates an SDL window and renderer, and then draws some
'' textures to it every frame, adjusting the viewport.
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

'' use the callbacks instead of main()
'' We will use this renderer to draw into this window every frame.
dim shared window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
dim shared renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
dim shared texture as SDL_Texture ptr = cptr(SDL_Texture ptr, 0)
dim shared texture_width as long = 0
dim shared texture_height as long = 0

declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

'' This function runs once at startup.
function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		dim surface as SDL_Surface ptr = cptr(SDL_Surface ptr, 0)
		dim png_path as zstring ptr = cptr(zstring ptr, 0)
		SDL_SetAppMetadata(strptr("Example Renderer Viewport"), strptr("1.0"), strptr("com.example.renderer-viewport"))
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/renderer/viewport"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't create window/renderer: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX)
		'' Textures are pixel data that we upload to the video hardware for fast drawing. Lots of 2D
		'' engines refer to these as "sprites." We'll do a static texture (upload once, draw many
		'' times) with data from a bitmap file.
		'' SDL_Surface is pixel data the CPU can access. SDL_Texture is pixel data the GPU can access.
		'' Load a .png into a surface, move it to a texture from there.
		SDL_asprintf(@(png_path), strptr("%ssample.png"), SDL_GetBasePath())
		'' allocate a string of the full file path
		surface = SDL_LoadPNG(png_path)
		if (surface = 0) then
			scope
				SDL_Log_(strptr("Couldn't load bitmap: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		SDL_free(cptr(any ptr, png_path))
		'' done with this, the file is loaded.
		texture_width = surface->w
		texture_height = surface->h
		texture = SDL_CreateTextureFromSurface(renderer, surface)
		if (texture = 0) then
			scope
				SDL_Log_(strptr("Couldn't create static texture: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		SDL_DestroySurface(surface)
		'' done with this, the texture has a copy of the pixels now.
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
		dim dst_rect as SDL_FRect = type<SDL_FRect>(0, 0, cast(single, texture_width), cast(single, texture_height))
		dim viewport as SDL_Rect
		'' Setting a viewport has the effect of limiting the area that rendering
		'' can happen, and making coordinate (0, 0) live somewhere else in the
		'' window. It does _not_ scale rendering to fit the viewport.
		'' as you can see from this, rendering draws over whatever was drawn before it.
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE)
		'' black, full alpha
		SDL_RenderClear(renderer)
		'' start with a blank canvas.
		'' Draw once with the whole window as the viewport.
		viewport.x = 0
		viewport.y = 0
		viewport.w = (640 \ 2)
		viewport.h = (480 \ 2)
		SDL_SetRenderViewport(renderer, cptr(const SDL_Rect ptr, 0))
		'' NULL means "use the whole window"
		SDL_RenderTexture(renderer, texture, cptr(const SDL_FRect ptr, 0), @(dst_rect))
		'' top right quarter of the window.
		viewport.x = (640 \ 2)
		viewport.y = (480 \ 2)
		viewport.w = (640 \ 2)
		viewport.h = (480 \ 2)
		SDL_SetRenderViewport(renderer, @(viewport))
		SDL_RenderTexture(renderer, texture, cptr(const SDL_FRect ptr, 0), @(dst_rect))
		'' bottom 20% of the window. Note it clips the width!
		viewport.x = 0
		viewport.y = (480 - ((480 \ 5)))
		viewport.w = (640 \ 5)
		viewport.h = (480 \ 5)
		SDL_SetRenderViewport(renderer, @(viewport))
		SDL_RenderTexture(renderer, texture, cptr(const SDL_FRect ptr, 0), @(dst_rect))
		'' what happens if you try to draw above the viewport? It should clip!
		viewport.x = 100
		viewport.y = 200
		viewport.w = 640
		viewport.h = 480
		SDL_SetRenderViewport(renderer, @(viewport))
		dst_rect.y = (-50)
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

'' end of viewport.bas
