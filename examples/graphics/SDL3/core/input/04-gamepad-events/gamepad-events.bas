'' Project: FreeBASIC SDL3 examples
'' File: gamepad-events.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/input/04-gamepad-events/gamepad-events.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example code looks for gamepad input in the event handler, and
'' reports any changes as a flood of info.
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"

#define MOTION_EVENT_COOLDOWN 40

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
'' use the callbacks instead of main()
'' We will use this renderer to draw into this window every frame.
dim shared window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
dim shared renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
dim shared colors(0 to 63) as SDL_Color
type EventMessage
	str_ as zstring ptr
	color_ as SDL_Color
	start_ticks as Uint64
	next_ as EventMessage ptr
end type

dim shared messages as EventMessage
dim shared messages_tail as EventMessage ptr = @(messages)

declare function battery_state_string cdecl(byval state as SDL_PowerState) as const zstring ptr
declare sub add_message cdecl(byval jid as SDL_JoystickID, byval fmt as const zstring ptr, ...)
declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

function battery_state_string cdecl(byval state as SDL_PowerState) as const zstring ptr
	scope
		select case state
			case SDL_POWERSTATE_ERROR
				goto switch_case_1
			case SDL_POWERSTATE_UNKNOWN
				goto switch_case_2
			case SDL_POWERSTATE_ON_BATTERY
				goto switch_case_3
			case SDL_POWERSTATE_NO_BATTERY
				goto switch_case_4
			case SDL_POWERSTATE_CHARGING
				goto switch_case_5
			case SDL_POWERSTATE_CHARGED
				goto switch_case_6
			case else
				goto switch_case_7
		end select
		switch_case_1:
		scope
			return strptr("ERROR")
		end scope
		switch_case_2:
		scope
			return strptr("UNKNOWN")
		end scope
		switch_case_3:
		scope
			return strptr("ON BATTERY")
		end scope
		switch_case_4:
		scope
			return strptr("NO BATTERY")
		end scope
		switch_case_5:
		scope
			return strptr("CHARGING")
		end scope
		switch_case_6:
		scope
			return strptr("CHARGED")
		end scope
		switch_case_7:
		scope
			goto switch_done_8
		end scope
		switch_done_8:
		return strptr("UNKNOWN")
	end scope
end function

sub add_message cdecl(byval jid as SDL_JoystickID, byval fmt as const zstring ptr, ...)
	scope
		dim color_ as const SDL_Color ptr = @(colors(((cast(uinteger, jid)) mod (((sizeof(SDL_Color) * 64) \ sizeof((colors(0))))))))
		dim msg as EventMessage ptr = cptr(EventMessage ptr, 0)
		dim str_ as zstring ptr = cptr(zstring ptr, 0)
		dim ap as va_list
		msg = cptr(EventMessage ptr, SDL_calloc(1, sizeof(((*msg)))))
		if (msg = 0) then
			scope
				exit sub
			end scope
		end if
		cva_start(ap, fmt)
		SDL_vasprintf(@(str_), fmt, ap)
		cva_end(ap)
		if (str_ = 0) then
			scope
				SDL_free(cptr(any ptr, msg))
				exit sub
			end scope
		end if
		msg->str_ = str_
		scope
		end scope
		SDL_memcpy(cptr(any ptr, (@(msg->color_))), cptr(const any ptr, (color_)), sizeof(((*(color_)))))
		msg->start_ticks = SDL_GetTicks()
		msg->next_ = cptr(EventMessage ptr, 0)
		messages_tail->next_ = msg
		messages_tail = msg
	end scope
end sub

'' This function runs once at startup.
function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		dim i as long
		SDL_SetAppMetadata(strptr("Example Input Gamepad Events"), strptr("1.0"), strptr("com.example.input-gamepad-events"))
		if (SDL_Init((SDL_INIT_VIDEO or SDL_INIT_GAMEPAD)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/input/gamepad-events"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't create window/renderer: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		colors(0).a = 255
		colors(0).b = colors(0).a
		colors(0).g = colors(0).b
		colors(0).r = colors(0).g
		scope
			i = 1
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
		add_message(0, strptr("Please plug in a gamepad."))
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
					'' this event is sent for each hotplugged stick, but also each already-connected gamepad during SDL_Init().
					dim which as SDL_JoystickID = event->gdevice.which
					dim gamepad as SDL_Gamepad ptr = SDL_OpenGamepad(which)
					if (gamepad = 0) then
						scope
							add_message(which, strptr("Gamepad #%u add, but not opened: %s"), cast(ulong, cast(ulong, which)), SDL_GetError())
						end scope
					else
						scope
							dim mapping as zstring ptr = SDL_GetGamepadMapping(gamepad)
							add_message(which, strptr("Gamepad #%u ('%s') added"), cast(ulong, cast(ulong, which)), SDL_GetGamepadName(gamepad))
							if mapping then
								scope
									add_message(which, strptr("Gamepad #%u mapping: %s"), cast(ulong, cast(ulong, which)), mapping)
									SDL_free(cptr(any ptr, mapping))
								end scope
							end if
						end scope
					end if
				end scope
			else
				if (event->type = SDL_EVENT_GAMEPAD_REMOVED) then
					scope
						dim which as SDL_JoystickID = event->gdevice.which
						dim gamepad as SDL_Gamepad ptr = SDL_GetGamepadFromID(which)
						if gamepad then
							scope
								SDL_CloseGamepad(gamepad)
							end scope
						end if
						add_message(which, strptr("Gamepad #%u removed"), cast(ulong, cast(ulong, which)))
					end scope
				else
					if (event->type = SDL_EVENT_GAMEPAD_AXIS_MOTION) then
						scope
							static axis_motion_cooldown_time as Uint64 = 0
							'' these are spammy, only show every X milliseconds.
							dim now as Uint64 = SDL_GetTicks()
							if (now >= axis_motion_cooldown_time) then
								scope
									dim which as SDL_JoystickID = event->gaxis.which
									axis_motion_cooldown_time = (now + 40)
									add_message(which, strptr("Gamepad #%u axis %s -> %d"), cast(ulong, cast(ulong, which)), SDL_GetGamepadStringForAxis(cast(SDL_GamepadAxis, event->gaxis.axis)), cast(long, cast(long, event->gaxis.value)))
								end scope
							end if
						end scope
					else
						if (((event->type = SDL_EVENT_GAMEPAD_BUTTON_UP)) orelse ((event->type = SDL_EVENT_GAMEPAD_BUTTON_DOWN))) then
							scope
								dim which as SDL_JoystickID = event->gbutton.which
								add_message(which, strptr("Gamepad #%u button %s -> %s"), cast(ulong, cast(ulong, which)), SDL_GetGamepadStringForButton(cast(SDL_GamepadButton, event->gbutton.button)), iif(event->gbutton.down, strptr("PRESSED"), strptr("RELEASED")))
							end scope
						else
							if (event->type = SDL_EVENT_JOYSTICK_BATTERY_UPDATED) then
								scope
									dim which as SDL_JoystickID = event->jbattery.which
									if SDL_IsGamepad(which) then
										scope
											'' this is only reported for joysticks, so make sure this joystick is _actually_ a gamepad.
											add_message(which, strptr("Gamepad #%u battery -> %s - %d%%"), cast(ulong, cast(ulong, which)), battery_state_string(event->jbattery.state), cast(long, event->jbattery.percent))
										end scope
									end if
								end scope
							end if
						end if
					end if
				end if
			end if
		end if
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once per frame, and is the heart of the program.
function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
	scope
		dim now as Uint64 = SDL_GetTicks()
		dim msg_lifetime as single = 3500.0f
		'' milliseconds a message lives for.
		dim msg as EventMessage ptr = messages.next_
		dim prev_y as single = 0.0f
		dim winw as long = 640
		dim winh as long = 480
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255)
		SDL_RenderClear(renderer)
		SDL_GetWindowSize(window_, @(winw), @(winh))
		do
			if (msg) = 0 then exit do
			scope
				dim x as single
				dim y as single
				dim life_percent as single = ((cast(single, ((now - msg->start_ticks)))) / msg_lifetime)
				if (life_percent >= 1.0f) then
					scope
						'' msg is done.
						messages.next_ = msg->next_
						if (messages_tail = msg) then
							scope
								messages_tail = @(messages)
							end scope
						end if
						SDL_free(cptr(any ptr, msg->str_))
						SDL_free(cptr(any ptr, msg))
						msg = messages.next_
						goto loop_continue_10
					end scope
				end if
				x = ((((cast(single, winw)) - ((SDL_strlen(msg->str_) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE)))) / 2.0f)
				y = ((cast(single, winh)) * life_percent)
				if (((prev_y <> 0.0f)) andalso ((((prev_y - y)) < (cast(single, SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE))))) then
					scope
						msg->start_ticks = now
						exit do
					end scope
				end if
				SDL_SetRenderDrawColor(renderer, msg->color_.r, msg->color_.g, msg->color_.b, cast(Uint8, (((cast(single, msg->color_.a)) * ((1.0f - life_percent))))))
				SDL_RenderDebugText(renderer, x, y, msg->str_)
				prev_y = y
				msg = msg->next_
			end scope
			loop_continue_10:
		loop
		SDL_RenderPresent(renderer)
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once at shutdown.
sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)
	scope
		SDL_Quit()
	end scope
end sub

#include once "callback-main.bi"

'' end of gamepad-events.bas
