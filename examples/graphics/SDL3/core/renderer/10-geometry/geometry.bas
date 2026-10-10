'' Project: FreeBASIC SDL3 examples
'' File: geometry.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/renderer/10-geometry/geometry.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example creates an SDL window and renderer, and then draws some
'' geometry (arbitrary polygons) to it every frame.
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
		SDL_SetAppMetadata(strptr("Example Renderer Geometry"), strptr("1.0"), strptr("com.example.renderer-geometry"))
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/renderer/geometry"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
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
		dim now as Uint64 = SDL_GetTicks()
		'' we'll have the triangle grow and shrink over a few seconds.
		dim direction as single = iif(((((now mod 2000)) >= 1000)), 1.0f, (-1.0f))
		dim scale as single = (((cast(single, (((cast(long, ((now mod 1000)))) - 500))) / 500.0f)) * direction)
		dim size as single = (200.0f + ((200.0f * scale)))
		dim vertices(0 to 3) as SDL_Vertex
		dim i as long
		'' as you can see from this, rendering draws over whatever was drawn before it.
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE)
		'' black, full alpha
		SDL_RenderClear(renderer)
		'' start with a blank canvas.
		'' Draw a single triangle with a different color at each vertex. Center this one and make it grow and shrink.
		'' You always draw triangles with this, but you can string triangles together to form polygons.
		SDL_memset(cptr(any ptr, @vertices(0)), 0, (sizeof(SDL_Vertex) * 4))
		vertices(0).position.x = ((cast(single, 640)) / 2.0f)
		vertices(0).position.y = ((((cast(single, 480)) - size)) / 2.0f)
		vertices(0).color.r = 1.0f
		vertices(0).color.a = 1.0f
		vertices(1).position.x = ((((cast(single, 640)) + size)) / 2.0f)
		vertices(1).position.y = ((((cast(single, 480)) + size)) / 2.0f)
		vertices(1).color.g = 1.0f
		vertices(1).color.a = 1.0f
		vertices(2).position.x = ((((cast(single, 640)) - size)) / 2.0f)
		vertices(2).position.y = ((((cast(single, 480)) + size)) / 2.0f)
		vertices(2).color.b = 1.0f
		vertices(2).color.a = 1.0f
		SDL_RenderGeometry(renderer, cptr(SDL_Texture ptr, 0), @vertices(0), 3, cptr(const long ptr, 0), 0)
		'' you can also map a texture to the geometry! Texture coordinates go from 0.0f to 1.0f. That will be the location
		'' in the texture bound to this vertex.
		SDL_memset(cptr(any ptr, @vertices(0)), 0, (sizeof(SDL_Vertex) * 4))
		vertices(0).position.x = 10.0f
		vertices(0).position.y = 10.0f
		vertices(0).color.a = 1.0f
		vertices(0).color.b = vertices(0).color.a
		vertices(0).color.g = vertices(0).color.b
		vertices(0).color.r = vertices(0).color.g
		vertices(0).tex_coord.x = 0.0f
		vertices(0).tex_coord.y = 0.0f
		vertices(1).position.x = 150.0f
		vertices(1).position.y = 10.0f
		vertices(1).color.a = 1.0f
		vertices(1).color.b = vertices(1).color.a
		vertices(1).color.g = vertices(1).color.b
		vertices(1).color.r = vertices(1).color.g
		vertices(1).tex_coord.x = 1.0f
		vertices(1).tex_coord.y = 0.0f
		vertices(2).position.x = 10.0f
		vertices(2).position.y = 150.0f
		vertices(2).color.a = 1.0f
		vertices(2).color.b = vertices(2).color.a
		vertices(2).color.g = vertices(2).color.b
		vertices(2).color.r = vertices(2).color.g
		vertices(2).tex_coord.x = 0.0f
		vertices(2).tex_coord.y = 1.0f
		SDL_RenderGeometry(renderer, texture, @vertices(0), 3, cptr(const long ptr, 0), 0)
		'' Did that only draw half of the texture? You can do multiple triangles sharing some vertices,
		'' using indices, to get the whole thing on the screen:
		'' Let's just move this over so it doesn't overlap...
		scope
			i = 0
			do while (i < 3)
				scope
					vertices(i).position.x += 450
				end scope
				i += 1
			loop
		end scope
		'' we need one more vertex, since the two triangles can share two of them.
		vertices(3).position.x = 600.0f
		vertices(3).position.y = 150.0f
		vertices(3).color.a = 1.0f
		vertices(3).color.b = vertices(3).color.a
		vertices(3).color.g = vertices(3).color.b
		vertices(3).color.r = vertices(3).color.g
		vertices(3).tex_coord.x = 1.0f
		vertices(3).tex_coord.y = 1.0f
		'' And an index to tell it to reuse some of the vertices between triangles...
		scope
			'' 4 vertices, but 6 actual places they used. Indices need less bandwidth to transfer and can reorder vertices easily!
			dim indices(0 to 5) as long = {0, 1, 2, 1, 2, 3}
			SDL_RenderGeometry(renderer, texture, @vertices(0), 4, @indices(0), (((sizeof(long) * 6) \ sizeof((indices(0))))))
		end scope
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

'' end of geometry.bas
