'' Project: FreeBASIC SDL3 examples
'' File: drawing-lines.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/pen/01-drawing-lines/drawing-lines.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example code reads pen/stylus input and draws lines. Darker lines
'' for harder pressure.
''
'' SDL can track multiple pens, but for simplicity here, this assumes any
'' pen input we see was from one device.
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"

'' use the callbacks instead of main()
'' We will use this renderer to draw into this window every frame.
dim shared window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
dim shared renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
dim shared render_target as SDL_Texture ptr = cptr(SDL_Texture ptr, 0)
dim shared pressure as single = 0.0f
dim shared previous_touch_x as single = (-1.0f)
dim shared previous_touch_y as single = (-1.0f)
dim shared tilt_x as single = 0.0f
dim shared tilt_y as single = 0.0f

declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

'' This function runs once at startup.
function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		dim w as long
		dim h as long
		SDL_SetAppMetadata(strptr("Example Pen Drawing Lines"), strptr("1.0"), strptr("com.example.pen-drawing-lines"))
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/pen/drawing-lines"), 640, 480, 0, @(window_), @(renderer)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't create window/renderer: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		'' we make a render target so we can draw lines to it and not have to record and redraw every pen stroke each frame.
		'' Instead rendering a frame for us is a single texture draw.
		'' make sure the render target matches output size (for hidpi displays, etc) so drawing matches the pen's position on a tablet display.
		SDL_GetRenderOutputSize(renderer, @(w), @(h))
		render_target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h)
		if (render_target = 0) then
			scope
				SDL_Log_(strptr("Couldn't create render target: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		'' just blank the render target to gray to start.
		SDL_SetRenderTarget(renderer, render_target)
		SDL_SetRenderDrawColor(renderer, 100, 100, 100, SDL_ALPHA_OPAQUE)
		SDL_RenderClear(renderer)
		SDL_SetRenderTarget(renderer, cptr(SDL_Texture ptr, 0))
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND)
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
		'' There are several events that track the specific stages of pen activity,
		'' but we're only going to look for motion and pressure, for simplicity.
		if (event->type = SDL_EVENT_PEN_MOTION) then
			scope
				'' you can check for when the pen is touching, but if pressure > 0.0f, it's definitely touching!
				if (pressure > 0.0f) then
					scope
						if (previous_touch_x >= 0.0f) then
							scope
								'' only draw if we're moving while touching
								'' draw with the alpha set to the pressure, so you effectively get a fainter line for lighter presses.
								SDL_SetRenderTarget(renderer, render_target)
								SDL_SetRenderDrawColorFloat(renderer, 0, 0, 0, pressure)
								SDL_RenderLine(renderer, previous_touch_x, previous_touch_y, event->pmotion.x, event->pmotion.y)
							end scope
						end if
						previous_touch_x = event->pmotion.x
						previous_touch_y = event->pmotion.y
					end scope
				else
					scope
						previous_touch_y = (-1.0f)
						previous_touch_x = previous_touch_y
					end scope
				end if
			end scope
		else
			if (event->type = SDL_EVENT_PEN_AXIS) then
				scope
					if (event->paxis.axis = SDL_PEN_AXIS_PRESSURE) then
						scope
							pressure = event->paxis.value
						end scope
					else
						if (event->paxis.axis = SDL_PEN_AXIS_XTILT) then
							scope
								tilt_x = event->paxis.value
							end scope
						else
							if (event->paxis.axis = SDL_PEN_AXIS_YTILT) then
								scope
									tilt_y = event->paxis.value
								end scope
							end if
						end if
					end if
				end scope
			end if
		end if
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once per frame, and is the heart of the program.
function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
	scope
		dim debug_text(0 to 1023) as byte
		'' make sure we're drawing to the window and not the render target
		SDL_SetRenderTarget(renderer, cptr(SDL_Texture ptr, 0))
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE)
		SDL_RenderClear(renderer)
		'' just in case.
		SDL_RenderTexture(renderer, render_target, cptr(const SDL_FRect ptr, 0), cptr(const SDL_FRect ptr, 0))
		SDL_snprintf(@debug_text(0), (sizeof(byte) * 1024), strptr("Tilt: %f %f"), cast(double, cast(double, tilt_x)), cast(double, cast(double, tilt_y)))
		SDL_RenderDebugText(renderer, 0, 8, @debug_text(0))
		SDL_RenderPresent(renderer)
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once at shutdown.
sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)
	scope
		SDL_DestroyTexture(render_target)
	end scope
end sub

#include once "callback-main.bi"

'' end of drawing-lines.bas
