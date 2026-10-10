'' Project: FreeBASIC SDL3 examples
'' File: joystick-polling.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/input/01-joystick-polling/joystick-polling.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example code looks for the current joystick state once per frame,
'' and draws a visual representation of it.
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"

'' Joysticks are low-level interfaces: there's something with a bunch of
'' buttons, axes and hats, in no understood order or position. This is
'' a flexible interface, but you'll need to build some sort of configuration
'' UI to let people tell you what button, etc, does what. On top of this
'' interface, SDL offers the "gamepad" API, which works with lots of devices,
'' and knows how to map arbitrary buttons and such to look like an
'' Xbox/PlayStation/etc gamepad. This is easier, and better, for many games,
'' but isn't necessarily a good fit for complex apps and hardware. A flight
'' simulator, a realistic racing game, etc, might want the joystick interface
'' instead of gamepads.
'' SDL can handle multiple joysticks, but for simplicity, this program only
'' deals with the first stick it sees.
'' use the callbacks instead of main()
'' We will use this renderer to draw into this window every frame.
dim shared window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
dim shared renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
dim shared joystick as SDL_Joystick ptr = cptr(SDL_Joystick ptr, 0)
dim shared colors(0 to 63) as SDL_Color

declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

'' This function runs once at startup.
function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		dim i as long
		SDL_SetAppMetadata(strptr("Example Input Joystick Polling"), strptr("1.0"), strptr("com.example.input-joystick-polling"))
		if (SDL_Init((SDL_INIT_VIDEO or SDL_INIT_JOYSTICK)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/input/joystick-polling"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't create window/renderer: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		scope
			i = 0
			do while (i < (((sizeof(SDL_Color) * 64) \ sizeof((colors(0))))))
				scope
					colors(i).r = SDL_rand(255)
					colors(i).g = SDL_rand(255)
					colors(i).b = SDL_rand(255)
					colors(i).a = 255
				end scope
				i += 1
			loop
		end scope
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
		else
			if (event->type = SDL_EVENT_JOYSTICK_ADDED) then
				scope
					'' this event is sent for each hotplugged stick, but also each already-connected joystick during SDL_Init().
					if (joystick = cptr(SDL_Joystick ptr, (cptr(any ptr, 0)))) then
						scope
							'' we don't have a stick yet and one was added, open it!
							joystick = SDL_OpenJoystick(event->jdevice.which)
							if (joystick = 0) then
								scope
									SDL_Log_(strptr("Failed to open joystick ID %u: %s"), cast(ulong, cast(ulong, event->jdevice.which)), SDL_GetError())
								end scope
							end if
						end scope
					end if
				end scope
			else
				if (event->type = SDL_EVENT_JOYSTICK_REMOVED) then
					scope
						if (joystick andalso ((SDL_GetJoystickID(joystick) = event->jdevice.which))) then
							scope
								SDL_CloseJoystick(joystick)
								'' our joystick was unplugged.
								joystick = cptr(SDL_Joystick ptr, 0)
							end scope
						end if
					end scope
				end if
			end if
		end if
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once per frame, and is the heart of the program.
function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
	scope
		dim winw as long = 640
		dim winh as long = 480
		dim text as const zstring ptr = strptr("Plug in a joystick, please.")
		dim x as single
		dim y as single
		dim i as long
		if joystick then
			scope
				'' we have a stick opened?
				text = SDL_GetJoystickName(joystick)
			end scope
		end if
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255)
		SDL_RenderClear(renderer)
		SDL_GetWindowSize(window_, @(winw), @(winh))
		'' note that you can get input as events, instead of polling, which is
		'' better since it won't miss button presses if the system is lagging,
		'' but often times checking the current state per-frame is good enough,
		'' and maybe better if you'd rather _drop_ inputs due to lag.
		if joystick then
			scope
				'' we have a stick opened?
				dim size as single = 30.0f
				dim total as long
				'' draw axes as bars going across middle of screen. We don't know if it's an X or Y or whatever axis, so we can't do more than this.
				total = SDL_GetNumJoystickAxes(joystick)
				y = (((winh - ((total * size)))) / 2)
				x = ((cast(single, winw)) / 2.0f)
				scope
					i = 0
					do while (i < total)
						scope
							dim color_ as const SDL_Color ptr = @(colors((i mod (((sizeof(SDL_Color) * 64) \ sizeof((colors(0))))))))
							dim val_ as single = (((cast(single, SDL_GetJoystickAxis(joystick, i))) / 32767.0f))
							'' make it -1.0f to 1.0f
							dim dx as single = (x + ((val_ * x)))
							dim dst as SDL_FRect = type<SDL_FRect>(dx, y, (x - SDL_fabsf(dx)), size)
							SDL_SetRenderDrawColor(renderer, color_->r, color_->g, color_->b, color_->a)
							SDL_RenderFillRect(renderer, @(dst))
							y += size
						end scope
						i += 1
					loop
				end scope
				'' draw buttons as blocks across top of window. We only know the button numbers, but not where they are on the device.
				total = SDL_GetNumJoystickButtons(joystick)
				x = (((winw - ((total * size)))) / 2)
				scope
					i = 0
					do while (i < total)
						scope
							dim color_ as const SDL_Color ptr = @(colors((i mod (((sizeof(SDL_Color) * 64) \ sizeof((colors(0))))))))
							dim dst as SDL_FRect = type<SDL_FRect>(x, 0.0f, size, size)
							if SDL_GetJoystickButton(joystick, i) then
								scope
									SDL_SetRenderDrawColor(renderer, color_->r, color_->g, color_->b, color_->a)
								end scope
							else
								scope
									SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255)
								end scope
							end if
							SDL_RenderFillRect(renderer, @(dst))
							SDL_SetRenderDrawColor(renderer, 255, 255, 255, color_->a)
							SDL_RenderRect(renderer, @(dst))
							'' outline it
							x += size
						end scope
						i += 1
					loop
				end scope
				'' draw hats across the bottom of the screen.
				total = SDL_GetNumJoystickHats(joystick)
				x = (((((winw - ((total * ((size * 2.0f)))))) / 2.0f)) + ((size / 2.0f)))
				y = ((cast(single, winh)) - size)
				scope
					i = 0
					do while (i < total)
						scope
							dim color_ as const SDL_Color ptr = @(colors((i mod (((sizeof(SDL_Color) * 64) \ sizeof((colors(0))))))))
							dim thirdsize as single = (size / 3.0f)
							dim cross(0 to 1) as SDL_FRect = {type<SDL_FRect>(x, (y + thirdsize), size, thirdsize), type<SDL_FRect>((x + thirdsize), y, thirdsize, size)}
							dim hat as Uint8 = SDL_GetJoystickHat(joystick, i)
							SDL_SetRenderDrawColor(renderer, 90, 90, 90, 255)
							SDL_RenderFillRects(renderer, @cross(0), (((sizeof(SDL_FRect) * 2) \ sizeof((cross(0))))))
							SDL_SetRenderDrawColor(renderer, color_->r, color_->g, color_->b, color_->a)
							if (hat and SDL_HAT_UP) then
								scope
									dim dst as SDL_FRect = type<SDL_FRect>((x + thirdsize), y, thirdsize, thirdsize)
									SDL_RenderFillRect(renderer, @(dst))
								end scope
							end if
							if (hat and SDL_HAT_RIGHT) then
								scope
									dim dst as SDL_FRect = type<SDL_FRect>((x + ((thirdsize * 2))), (y + thirdsize), thirdsize, thirdsize)
									SDL_RenderFillRect(renderer, @(dst))
								end scope
							end if
							if (hat and SDL_HAT_DOWN) then
								scope
									dim dst as SDL_FRect = type<SDL_FRect>((x + thirdsize), (y + ((thirdsize * 2))), thirdsize, thirdsize)
									SDL_RenderFillRect(renderer, @(dst))
								end scope
							end if
							if (hat and SDL_HAT_LEFT) then
								scope
									dim dst as SDL_FRect = type<SDL_FRect>(x, (y + thirdsize), thirdsize, thirdsize)
									SDL_RenderFillRect(renderer, @(dst))
								end scope
							end if
							x += (size * 2)
						end scope
						i += 1
					loop
				end scope
			end scope
		end if
		x = ((((cast(single, winw)) - ((SDL_strlen(text) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE)))) / 2.0f)
		y = ((((cast(single, winh)) - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE)) / 2.0f)
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255)
		SDL_RenderDebugText(renderer, x, y, text)
		SDL_RenderPresent(renderer)
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once at shutdown.
sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)
	scope
		if joystick then
			scope
				SDL_CloseJoystick(joystick)
			end scope
		end if
	end scope
end sub

#include once "callback-main.bi"

'' end of joystick-polling.bas
