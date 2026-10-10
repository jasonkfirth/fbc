'' Project: FreeBASIC SDL3 examples
'' File: gamepad-polling.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/input/03-gamepad-polling/gamepad-polling.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example code looks for the current gamepad state once per frame,
'' and draws a visual representation of it. See 01-joystick-polling for the
'' equivalent example code for the lower-level joystick API.
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

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
'' SDL can handle multiple gamepads, but for simplicity, this program only
'' deals with the first gamepad it sees.
'' use the callbacks instead of main()
'' We will use this renderer to draw into this window every frame.
dim shared window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
dim shared renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
dim shared texture as SDL_Texture ptr = cptr(SDL_Texture ptr, 0)
dim shared gamepad as SDL_Gamepad ptr = cptr(SDL_Gamepad ptr, 0)

declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

'' This function runs once at startup.
function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		dim png_path as zstring ptr = cptr(zstring ptr, 0)
		dim surface as SDL_Surface ptr = cptr(SDL_Surface ptr, 0)
		SDL_SetAppMetadata(strptr("Example Input Gamepad Polling"), strptr("1.0"), strptr("com.example.input-gamepad-polling"))
		if (SDL_Init((SDL_INIT_VIDEO or SDL_INIT_GAMEPAD)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/input/gamepad-polling"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't create window/renderer: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_STRETCH) = 0) then
			scope
				return SDL_APP_FAILURE
			end scope
		end if
		'' Textures are pixel data that we upload to the video hardware for fast drawing. Lots of 2D
		'' engines refer to these as "sprites." We'll do a static texture (upload once, draw many
		'' times) with data from a bitmap file.
		'' SDL_Surface is pixel data the CPU can access. SDL_Texture is pixel data the GPU can access.
		'' Load a .png into a surface, move it to a texture from there.
		SDL_asprintf(@(png_path), strptr("%sgamepad_front.png"), SDL_GetBasePath())
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
		else
			if (event->type = SDL_EVENT_GAMEPAD_ADDED) then
				scope
					'' this event is sent for each hotplugged gamepad, but also each already-connected gamepad during SDL_Init().
					if (gamepad = cptr(SDL_Gamepad ptr, (cptr(any ptr, 0)))) then
						scope
							'' we don't have a stick yet and one was added, open it!
							gamepad = SDL_OpenGamepad(event->gdevice.which)
							if (gamepad = 0) then
								scope
									SDL_Log_(strptr("Failed to open gamepad ID %u: %s"), cast(ulong, cast(ulong, event->gdevice.which)), SDL_GetError())
								end scope
							end if
						end scope
					end if
				end scope
			else
				if (event->type = SDL_EVENT_GAMEPAD_REMOVED) then
					scope
						if (gamepad andalso ((SDL_GetGamepadID(gamepad) = event->gdevice.which))) then
							scope
								SDL_CloseGamepad(gamepad)
								'' our controller was unplugged.
								gamepad = cptr(SDL_Gamepad ptr, 0)
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
		dim text as const zstring ptr = strptr("Plug in a gamepad, please.")
		static leftthumblast as Uint64 = 4294967295u
		static rightthumblast as Uint64 = 4294967295u
		dim now as Uint64 = SDL_GetTicks()
		dim axis_x as Sint16
		dim axis_y as Sint16
		dim x as single
		dim y as single
		dim i as long
		if gamepad then
			scope
				'' we have a stick opened?
				text = SDL_GetGamepadName(gamepad)
			end scope
		end if
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255)
		'' white
		SDL_RenderClear(renderer)
		'' note that you can get input as events, instead of polling, which is
		'' better since it won't miss button presses if the system is lagging,
		'' but often times checking the current state per-frame is good enough,
		'' and maybe better if you'd rather _drop_ inputs due to lag.
		if gamepad then
			scope
				'' we have a stick opened?
				'' where to draw the buttons
				dim buttons(0 to 15) as SDL_FRect = {type<SDL_FRect>(497, 266, 38, 38), type<SDL_FRect>(550, 217, 38, 38), type<SDL_FRect>(445, 221, 38, 38), type<SDL_FRect>(499, 173, 38, 38), type<SDL_FRect>(235, 228, 32, 29), type<SDL_FRect>(287, 195, 69, 69), type<SDL_FRect>(377, 228, 32, 29), type<SDL_FRect>(91, 234, 63, 63), type<SDL_FRect>(381, 354, 63, 63), type<SDL_FRect>(74, 73, 102, 29), type<SDL_FRect>(468, 73, 102, 29), type<SDL_FRect>(207, 316, 32, 32), type<SDL_FRect>(207, 384, 32, 32), type<SDL_FRect>(173, 351, 32, 32), type<SDL_FRect>(242, 351, 32, 32), type<SDL_FRect>(310, 286, 23, 27)}
				SDL_RenderTexture(renderer, texture, cptr(const SDL_FRect ptr, 0), cptr(const SDL_FRect ptr, 0))
				'' draw the gamepad picture to the whole window.
				'' draw green boxes over buttons that are currently pressed.
				SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255)
				'' green
				scope
					i = 0
					do while (i < (((sizeof(SDL_FRect) * 16) \ sizeof((buttons(0))))))
						scope
							if SDL_GetGamepadButton(gamepad, cast(SDL_GamepadButton, i)) then
								scope
									SDL_RenderFillRect(renderer, @(buttons(i)))
								end scope
							end if
						end scope
						i += 1
					loop
				end scope
				'' draw axes in blue.
				SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255)
				'' blue
				'' left thumb axis.
				axis_x = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX)
				axis_y = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY)
				if (((SDL_abs(axis_x) > 1000)) orelse ((SDL_abs(axis_y) > 1000))) then
					scope
						'' zero means centered, but it might be a little off zero...
						leftthumblast = now
					end scope
				end if
				if (((now - leftthumblast)) < 500) then
					scope
						'' draw if there was movement in the last half-second.
						dim box as SDL_FRect = type<SDL_FRect>((107 + ((((axis_x / 32767.0f)) * 30.0f))), (252 + ((((axis_y / 32767.0f)) * 30.0f))), 30, 30)
						SDL_RenderFillRect(renderer, @(box))
					end scope
				end if
				'' right thumb axis.
				axis_x = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTX)
				axis_y = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTY)
				if (((SDL_abs(axis_x) > 1000)) orelse ((SDL_abs(axis_y) > 1000))) then
					scope
						'' zero means centered, but it might be a little off zero...
						rightthumblast = now
					end scope
				end if
				if (((now - rightthumblast)) < 500) then
					scope
						'' draw if there was movement in the last half-second.
						dim box as SDL_FRect = type<SDL_FRect>((397 + ((((axis_x / 32767.0f)) * 30.0f))), (370 + ((((axis_y / 32767.0f)) * 30.0f))), 30, 30)
						SDL_RenderFillRect(renderer, @(box))
					end scope
				end if
				'' left trigger.
				axis_y = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER)
				if (axis_y > 1000) then
					scope
						'' zero means unpressed, but it might be a little off zero...
						dim height as single = ((((axis_y / 32767.0f)) * 65.0f))
						dim box as SDL_FRect = type<SDL_FRect>(127, (1 + ((65.0f - height))), 37, height)
						SDL_RenderFillRect(renderer, @(box))
					end scope
				end if
				'' right trigger.
				axis_y = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)
				if (axis_y > 1000) then
					scope
						'' zero means unpressed, but it might be a little off zero...
						dim height as single = ((((axis_y / 32767.0f)) * 65.0f))
						dim box as SDL_FRect = type<SDL_FRect>(481, (1 + ((65.0f - height))), 37, height)
						SDL_RenderFillRect(renderer, @(box))
					end scope
				end if
			end scope
		end if
		x = ((((cast(single, 640)) - ((SDL_strlen(text) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE)))) / 2.0f)
		if gamepad then
			scope
				y = cast(single, ((480 - ((SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE + 2)))))
			end scope
		else
			scope
				y = ((((cast(single, 480)) - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE)) / 2.0f)
			end scope
		end if
		SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255)
		'' blue
		SDL_RenderDebugText(renderer, x, y, text)
		SDL_RenderPresent(renderer)
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once at shutdown.
sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)
	scope
		SDL_DestroyTexture(texture)
		SDL_CloseGamepad(gamepad)
	end scope
end sub

#include once "callback-main.bi"

'' end of gamepad-polling.bas
