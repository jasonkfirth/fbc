'' Project: FreeBASIC SDL3 examples
'' File: cliprect.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/renderer/15-cliprect/cliprect.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example creates an SDL window and renderer, and then draws a scene
'' to it every frame, while sliding around a clipping rectangle.
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define CLIPRECT_SIZE 250
#define CLIPRECT_SPEED 200

'' use the callbacks instead of main()
'' pixels per second
'' We will use this renderer to draw into this window every frame.
dim shared window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
dim shared renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
dim shared texture as SDL_Texture ptr = cptr(SDL_Texture ptr, 0)
dim shared cliprect_position as SDL_FPoint
dim shared cliprect_direction as SDL_FPoint
dim shared last_time as Uint64 = 0

declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

'' A lot of this program is examples/renderer/02-primitives, so we have a good
'' visual that we can slide a clip rect around. The actual new magic in here
'' is the SDL_SetRenderClipRect() function.
'' This function runs once at startup.
function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		dim surface as SDL_Surface ptr = cptr(SDL_Surface ptr, 0)
		dim png_path as zstring ptr = cptr(zstring ptr, 0)
		SDL_SetAppMetadata(strptr("Example Renderer Clipping Rectangle"), strptr("1.0"), strptr("com.example.renderer-cliprect"))
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/renderer/cliprect"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't create window/renderer: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX)
		cliprect_direction.y = 1.0f
		cliprect_direction.x = cliprect_direction.y
		last_time = SDL_GetTicks()
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
		dim cliprect as SDL_Rect = type<SDL_Rect>(cast(long, SDL_roundf(cliprect_position.x)), cast(long, SDL_roundf(cliprect_position.y)), 250, 250)
		dim now as Uint64 = SDL_GetTicks()
		dim elapsed as single = ((cast(single, ((now - last_time)))) / 1000.0f)
		'' seconds since last iteration
		dim distance as single = (elapsed * 200)
		'' Set a new clipping rectangle position
		cliprect_position.x += (distance * cliprect_direction.x)
		if (cliprect_position.x < (-250)) then
			scope
				cliprect_position.x = (-250)
				cliprect_direction.x = 1.0f
			end scope
		else
			if (cliprect_position.x >= 640) then
				scope
					cliprect_position.x = (640 - 1)
					cliprect_direction.x = (-1.0f)
				end scope
			end if
		end if
		cliprect_position.y += (distance * cliprect_direction.y)
		if (cliprect_position.y < (-250)) then
			scope
				cliprect_position.y = (-250)
				cliprect_direction.y = 1.0f
			end scope
		else
			if (cliprect_position.y >= 480) then
				scope
					cliprect_position.y = (480 - 1)
					cliprect_direction.y = (-1.0f)
				end scope
			end if
		end if
		SDL_SetRenderClipRect(renderer, @(cliprect))
		last_time = now
		'' okay, now draw!
		'' Note that SDL_RenderClear is _not_ affected by the clipping rectangle!
		SDL_SetRenderDrawColor(renderer, 33, 33, 33, SDL_ALPHA_OPAQUE)
		'' grey, full alpha
		SDL_RenderClear(renderer)
		'' start with a blank canvas.
		'' stretch the texture across the entire window. Only the piece in the
		'' clipping rectangle will actually render, though!
		SDL_RenderTexture(renderer, texture, cptr(const SDL_FRect ptr, 0), cptr(const SDL_FRect ptr, 0))
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

'' end of cliprect.bas
