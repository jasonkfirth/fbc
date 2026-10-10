'' Project: FreeBASIC SDL3 examples
'' File: power.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/misc/01-power/power.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example code reports power status (plugged in, battery level, etc).
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"

'' use the callbacks instead of main()
'' We will use this renderer to draw into this window every frame.
dim shared window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
dim shared renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)

declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

'' This function runs once at startup.
function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		SDL_SetAppMetadata(strptr("Example Misc Power"), strptr("1.0"), strptr("com.example.misc-power"))
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/misc/power"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't create window/renderer: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX)
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
		dim frame as SDL_FRect = type<SDL_FRect>(100, 200, 440, 80)
		'' the percentage bar dimensions.
		'' Query for battery info
		dim seconds as long = 0
		dim percent as long = 0
		dim state as SDL_PowerState = SDL_GetPowerInfo(@(seconds), @(percent))
		'' We set up different drawing details for each power state, then
		'' run it all through the same drawing code.
		dim clearr as long = 0
		dim clearg as long = 0
		dim clearb as long = 0
		'' clear window to this color.
		dim textr as long = 255
		dim textg as long = 255
		dim textb as long = 255
		'' draw messages in this color.
		dim framer as long = 255
		dim frameg as long = 255
		dim frameb as long = 255
		'' draw a percentage bar frame in this color.
		dim barr as long = 0
		dim barg as long = 0
		dim barb as long = 0
		'' draw a percentage bar in this color.
		dim msg as const zstring ptr = cptr(const zstring ptr, 0)
		dim msg2 as const zstring ptr = cptr(const zstring ptr, 0)
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
				goto switch_case_2
		end select
		switch_case_1:
		scope
			msg2 = strptr("ERROR GETTING POWER STATE")
			msg = SDL_GetError()
			clearr = 255
			'' red background
			goto switch_done_7
		end scope
		switch_case_2:
		scope
			'' in case this does something unexpected later, treat it as unknown.
			msg = strptr("Power state is unknown.")
			clearg = 50
			clearb = clearg
			clearr = clearb
			'' grey background
			goto switch_done_7
		end scope
		switch_case_3:
		scope
			msg = strptr("Running on battery.")
			barr = 255
			'' draw in red
			goto switch_done_7
		end scope
		switch_case_4:
		scope
			msg = strptr("Plugged in, no battery available.")
			clearg = 50
			'' green background
			goto switch_done_7
		end scope
		switch_case_5:
		scope
			msg = strptr("Charging.")
			barg = 255
			barb = barg
			'' draw in cyan
			goto switch_done_7
		end scope
		switch_case_6:
		scope
			msg = strptr("Charged.")
			barg = 255
			'' draw in green
			goto switch_done_7
		end scope
		switch_done_7:
		SDL_SetRenderDrawColor(renderer, clearr, clearg, clearb, 255)
		SDL_RenderClear(renderer)
		if (percent >= 0) then
			scope
				dim x as single
				dim y as single
				dim pctrect as SDL_FRect
				dim remainstr(0 to 63) as byte
				dim msgbuf(0 to 127) as byte
				scope
				end scope
				SDL_memcpy(cptr(any ptr, (@(pctrect))), cptr(const any ptr, (@(frame))), sizeof(((*(@(frame))))))
				pctrect.w *= (percent / 100.0f)
				if (seconds < 0) then
					scope
						SDL_strlcpy(@remainstr(0), strptr("unknown time"), (sizeof(byte) * 64))
					end scope
				else
					scope
						dim hours as long
						dim minutes as long
						hours = (seconds \ ((60 * 60)))
						seconds -= (hours * ((60 * 60)))
						minutes = (seconds \ 60)
						seconds -= (minutes * 60)
						SDL_snprintf(@remainstr(0), (sizeof(byte) * 64), strptr("%02d:%02d:%02d"), cast(long, hours), cast(long, minutes), cast(long, seconds))
					end scope
				end if
				SDL_snprintf(@msgbuf(0), (sizeof(byte) * 128), strptr("Battery: %3d percent, %s remaining"), cast(long, percent), @remainstr(0))
				x = (frame.x + ((((frame.w - ((SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * SDL_strlen(@msgbuf(0)))))) / 2.0f)))
				y = ((frame.y + frame.h) + SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE)
				SDL_SetRenderDrawColor(renderer, barr, barg, barb, 255)
				'' draw percent bar.
				SDL_RenderFillRect(renderer, @(pctrect))
				SDL_SetRenderDrawColor(renderer, framer, frameg, frameb, 255)
				'' draw frame on top of bar.
				SDL_RenderRect(renderer, @(frame))
				SDL_SetRenderDrawColor(renderer, textr, textg, textb, 255)
				SDL_RenderDebugText(renderer, x, y, @msgbuf(0))
			end scope
		end if
		if msg then
			scope
				dim x as single = (frame.x + ((((frame.w - ((SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * SDL_strlen(msg))))) / 2.0f)))
				dim y as single = (frame.y - ((SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 2)))
				SDL_SetRenderDrawColor(renderer, textr, textg, textb, 255)
				SDL_RenderDebugText(renderer, x, y, msg)
			end scope
		end if
		if msg2 then
			scope
				dim x as single = (frame.x + ((((frame.w - ((SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * SDL_strlen(msg2))))) / 2.0f)))
				dim y as single = (frame.y - ((SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 4)))
				SDL_SetRenderDrawColor(renderer, textr, textg, textb, 255)
				SDL_RenderDebugText(renderer, x, y, msg2)
			end scope
		end if
		'' put the new rendering on the screen.
		SDL_RenderPresent(renderer)
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once at shutdown.
sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)
	scope
	end scope
end sub

#include once "callback-main.bi"

'' end of power.bas
