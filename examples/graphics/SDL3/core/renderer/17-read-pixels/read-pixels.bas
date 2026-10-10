'' Project: FreeBASIC SDL3 examples
'' File: read-pixels.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/renderer/17-read-pixels/read-pixels.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example creates an SDL window and renderer, and draws a
'' rotating texture to it, reads back the rendered pixels, converts them to
'' black and white, and then draws the converted image to a corner of the
'' screen.
''
'' This isn't necessarily an efficient thing to do--in real life one might
'' want to do this sort of thing with a render target--but it's just a visual
'' example of how to use SDL_RenderReadPixels().
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
dim shared converted_texture as SDL_Texture ptr = cptr(SDL_Texture ptr, 0)
dim shared converted_texture_width as long = 0
dim shared converted_texture_height as long = 0

declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

'' This function runs once at startup.
function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		dim surface as SDL_Surface ptr = cptr(SDL_Surface ptr, 0)
		dim png_path as zstring ptr = cptr(zstring ptr, 0)
		SDL_SetAppMetadata(strptr("Example Renderer Read Pixels"), strptr("1.0"), strptr("com.example.renderer-read-pixels"))
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/renderer/read-pixels"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
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
		dim surface as SDL_Surface ptr
		dim center as SDL_FPoint
		dim dst_rect as SDL_FRect
		'' we'll have a texture rotate around over 2 seconds (2000 milliseconds). 360 degrees in a circle!
		dim rotation as single = ((((cast(single, (cast(long, ((now mod 2000)))))) / 2000.0f)) * 360.0f)
		'' as you can see from this, rendering draws over whatever was drawn before it.
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE)
		'' black, full alpha
		SDL_RenderClear(renderer)
		'' start with a blank canvas.
		'' Center this one, and draw it with some rotation so it spins!
		dst_rect.x = ((cast(single, ((640 - texture_width)))) / 2.0f)
		dst_rect.y = ((cast(single, ((480 - texture_height)))) / 2.0f)
		dst_rect.w = cast(single, texture_width)
		dst_rect.h = cast(single, texture_height)
		'' rotate it around the center of the texture; you can rotate it from a different point, too!
		center.x = (texture_width / 2.0f)
		center.y = (texture_height / 2.0f)
		SDL_RenderTextureRotated(renderer, texture, cptr(const SDL_FRect ptr, 0), @(dst_rect), cast(double, rotation), @(center), SDL_FLIP_NONE)
		'' this next whole thing is _super_ expensive. Seriously, don't do this in real life.
		'' Download the pixels of what has just been rendered. This has to wait for the GPU to finish rendering it and everything before it,
		'' and then make an expensive copy from the GPU to system RAM!
		surface = SDL_RenderReadPixels(renderer, cptr(const SDL_Rect ptr, 0))
		'' This is also expensive, but easier: convert the pixels to a format we want.
		if ((surface andalso ((surface->format <> SDL_PIXELFORMAT_RGBA8888))) andalso ((surface->format <> SDL_PIXELFORMAT_BGRA8888))) then
			scope
				dim converted as SDL_Surface ptr = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA8888)
				SDL_DestroySurface(surface)
				surface = converted
			end scope
		end if
		if surface then
			scope
				'' Rebuild converted_texture if the dimensions have changed (window resized, etc).
				if (((surface->w <> converted_texture_width)) orelse ((surface->h <> converted_texture_height))) then
					scope
						SDL_DestroyTexture(converted_texture)
						converted_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, surface->w, surface->h)
						if (converted_texture = 0) then
							scope
								SDL_Log_(strptr("Couldn't (re)create conversion texture: %s"), SDL_GetError())
								return SDL_APP_FAILURE
							end scope
						end if
						converted_texture_width = surface->w
						converted_texture_height = surface->h
					end scope
				end if
				'' Turn each pixel into either black or white. This is a lousy technique but it works here.
				'' In real life, something like Floyd-Steinberg dithering might work
				'' better: https://en.wikipedia.org/wiki/Floyd%E2%80%93Steinberg_dithering
				dim x as long
				dim y as long
				scope
					y = 0
					do while (y < surface->h)
						scope
							dim pixels as Uint32 ptr = cptr(Uint32 ptr, (((cptr(Uint8 ptr, surface->pixels)) + ((y * surface->pitch)))))
							scope
								x = 0
								do while (x < surface->w)
									scope
										dim p as Uint8 ptr = cptr(Uint8 ptr, (@(pixels[x])))
										dim average as Uint32 = (((((cast(Uint32, p[1])) + (cast(Uint32, p[2]))) + (cast(Uint32, p[3])))) / 3)
										if (average = 0) then
											scope
												p[3] = 255
												p[0] = p[3]
												p[2] = 0
												p[1] = p[2]
											end scope
										else
											scope
												p[3] = iif(((average > 50)), 255, 0)
												p[2] = p[3]
												p[1] = p[2]
											end scope
										end if
									end scope
									x += 1
								loop
							end scope
						end scope
						y += 1
					loop
				end scope
				'' upload the processed pixels back into a texture.
				SDL_UpdateTexture(converted_texture, cptr(const SDL_Rect ptr, 0), surface->pixels, surface->pitch)
				SDL_DestroySurface(surface)
				'' draw the texture to the top-left of the screen.
				dst_rect.y = 0.0f
				dst_rect.x = dst_rect.y
				dst_rect.w = ((cast(single, 640)) / 4.0f)
				dst_rect.h = ((cast(single, 480)) / 4.0f)
				SDL_RenderTexture(renderer, converted_texture, cptr(const SDL_FRect ptr, 0), @(dst_rect))
			end scope
		end if
		SDL_RenderPresent(renderer)
		'' put it all on the screen!
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once at shutdown.
sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)
	scope
		SDL_DestroyTexture(converted_texture)
		SDL_DestroyTexture(texture)
	end scope
end sub

#include once "callback-main.bi"

'' end of read-pixels.bas
