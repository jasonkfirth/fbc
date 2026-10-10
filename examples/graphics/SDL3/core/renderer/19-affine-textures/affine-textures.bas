'' Project: FreeBASIC SDL3 examples
'' File: affine-textures.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/renderer/19-affine-textures/affine-textures.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' affine-textures.c ...

#include once "SDL3/SDL.bi"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

'' This example creates an SDL window and renderer, and then draws a cube
'' using affine-transformed textures every frame.
''
'' This code is public domain. Feel free to use it for any purpose!
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
		SDL_SetAppMetadata(strptr("Example Renderer Affine Textures"), strptr("1.0"), strptr("com.example.renderer-affine-textures"))
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/renderer/affine-textures"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
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
		dim x0 as single = (0.5f * 640)
		dim y0 as single = (0.5f * 480)
		dim px as single = ((iif((((640) < (480))), (640), (480))) / SDL_sqrtf(3.0f))
		dim now as Uint64 = SDL_GetTicks()
		dim rad as single = (((((cast(single, (cast(long, ((now mod 2000)))))) / 2000.0f)) * SDL_PI_F) * 2)
		dim cos_ as single = SDL_cosf(rad)
		dim sin_ as single = SDL_sinf(rad)
		dim k(0 to 2) as single = {(3.0f / SDL_sqrtf(50.0f)), (4.0f / SDL_sqrtf(50.0f)), (5.0f / SDL_sqrtf(50.0f))}
		dim mat(0 to 8) as single = {(cos_ + ((((1.0f - cos_)) * k(0)) * k(0))), (((-sin_) * k(2)) + ((((1.0f - cos_)) * k(0)) * k(1))), ((sin_ * k(1)) + ((((1.0f - cos_)) * k(0)) * k(2))), ((sin_ * k(2)) + ((((1.0f - cos_)) * k(0)) * k(1))), (cos_ + ((((1.0f - cos_)) * k(1)) * k(1))), (((-sin_) * k(0)) + ((((1.0f - cos_)) * k(1)) * k(2))), (((-sin_) * k(1)) + ((((1.0f - cos_)) * k(0)) * k(2))), ((sin_ * k(0)) + ((((1.0f - cos_)) * k(1)) * k(2))), (cos_ + ((((1.0f - cos_)) * k(2)) * k(2)))}
		dim corners(0 to 15) as single
		dim i as long
		scope
			i = 0
			do while (i < 8)
				scope
					dim x as single = iif(((i and 1)), (-0.5f), 0.5f)
					dim y as single = iif(((i and 2)), (-0.5f), 0.5f)
					dim z as single = iif(((i and 4)), (-0.5f), 0.5f)
					corners((0 + (2 * i))) = (((mat(0) * x) + (mat(1) * y)) + (mat(2) * z))
					corners((1 + (2 * i))) = (((mat(3) * x) + (mat(4) * y)) + (mat(5) * z))
				end scope
				i += 1
			loop
		end scope
		SDL_SetRenderDrawColor(renderer, 66, 135, 245, SDL_ALPHA_OPAQUE)
		'' light blue background.
		SDL_RenderClear(renderer)
		scope
			i = 1
			do while (i < 7)
				scope
					dim direction_index as long = (3 and (iif(((i and 4)), (not i), i)))
					dim odd as long = ((((i and 1)) xor ((((i and 2)) shr 1))) xor ((((i and 4)) shr 2)))
					if (0 < ((iif(odd, 1.0f, (-1.0f))) * mat((5 + direction_index)))) then
						goto loop_continue_2
					end if
					dim origin_index as long = ((1 shl ((((direction_index - 1)) mod 3))))
					dim right_index as long = (((1 shl ((((direction_index + odd)) mod 3)))) or origin_index)
					dim down_index as long = (((1 shl ((((direction_index + ((odd xor 1)))) mod 3)))) or origin_index)
					if (odd = 0) then
						scope
							origin_index xor= 7
							right_index xor= 7
							down_index xor= 7
						end scope
					end if
					dim origin as SDL_FPoint
					dim right_samples as SDL_FPoint
					dim down as SDL_FPoint
					origin.x = (x0 + (px * corners((0 + (2 * origin_index)))))
					origin.y = (y0 + (px * corners((1 + (2 * origin_index)))))
					right_samples.x = (x0 + (px * corners((0 + (2 * right_index)))))
					right_samples.y = (y0 + (px * corners((1 + (2 * right_index)))))
					down.x = (x0 + (px * corners((0 + (2 * down_index)))))
					down.y = (y0 + (px * corners((1 + (2 * down_index)))))
					SDL_RenderTextureAffine(renderer, texture, cptr(const SDL_FRect ptr, 0), @(origin), @(right_samples), @(down))
				end scope
				loop_continue_2:
				i += 1
			loop
		end scope
		SDL_RenderPresent(renderer)
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

'' end of affine-textures.bas
