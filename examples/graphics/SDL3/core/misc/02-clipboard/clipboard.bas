'' Project: FreeBASIC SDL3 examples
'' File: clipboard.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/misc/02-clipboard/clipboard.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example code lets the user copy and paste with the system clipboard.
''
'' This only handles text, but SDL supports other data types, too.
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"

'' use the callbacks instead of main()
'' We will use this renderer to draw into this window every frame.
dim shared window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
dim shared renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
dim shared copybuttonstr as const zstring ptr = strptr("Click here to copy!")
dim shared pastebuttonstr as const zstring ptr = strptr("Click here to paste!")
dim shared currenttimerect as SDL_FRect
dim shared copybuttonrect as SDL_FRect
dim shared pastetextrect as SDL_FRect
dim shared pastebuttonrect as SDL_FRect
dim shared copy_pressed as boolean = false
dim shared paste_pressed as boolean = false
dim shared current_time(0 to 63) as byte
dim shared pasted_str as zstring ptr = cptr(zstring ptr, 0)

declare sub CalculateCurrentTimeString cdecl()
declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare sub RenderPastedText cdecl()
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

sub CalculateCurrentTimeString cdecl()
	scope
		dim ticks as SDL_Time = 0
		dim dt as SDL_DateTime
		if ((SDL_GetCurrentTime(@(ticks)) = 0) orelse (SDL_TimeToDateTime(ticks, @(dt), true) = 0)) then
			scope
				SDL_snprintf(@current_time(0), (sizeof(byte) * 64), strptr("(Don't know the current time, sorry.)"))
			end scope
		else
			scope
				static month(0 to 11) as const zstring ptr = {strptr("January"), strptr("February"), strptr("March"), strptr("April"), strptr("May"), strptr("June"), strptr("July"), strptr("August"), strptr("September"), strptr("October"), strptr("November"), strptr("December")}
				static day(0 to 6) as const zstring ptr = {strptr("Sunday"), strptr("Monday"), strptr("Tuesday"), strptr("Wednesday"), strptr("Thursday"), strptr("Friday"), strptr("Saturday")}
				SDL_snprintf(@current_time(0), (sizeof(byte) * 64), strptr("%s, %s %d, %d   %02d:%02d:%02d"), day(dt.day_of_week), month((dt.month - 1)), cast(long, dt.day), cast(long, dt.year), cast(long, dt.hour), cast(long, dt.minute), cast(long, dt.second))
			end scope
		end if
	end scope
end sub

'' This function runs once at startup.
function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		SDL_SetAppMetadata(strptr("Example Misc Clipboard"), strptr("1.0"), strptr("com.example.misc-clipboard"))
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/misc/clipboard"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't create window/renderer: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX)
		CalculateCurrentTimeString()
		'' set up the locations where we'll draw stuff.
		currenttimerect.x = 30
		currenttimerect.y = 10
		currenttimerect.w = 390
		currenttimerect.h = (SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE + 10)
		copybuttonrect.x = ((currenttimerect.x + currenttimerect.w) + 30)
		copybuttonrect.y = currenttimerect.y
		copybuttonrect.w = cast(single, ((((SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * SDL_strlen(copybuttonstr))) + 10)))
		copybuttonrect.h = currenttimerect.h
		pastetextrect.x = 10
		pastetextrect.y = ((currenttimerect.y + currenttimerect.h) + 10)
		pastetextrect.w = 620
		pastetextrect.h = (((((480 - pastetextrect.y)) - copybuttonrect.h)) - 20)
		pastebuttonrect.w = cast(single, ((((SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * SDL_strlen(pastebuttonstr))) + 10)))
		pastebuttonrect.x = (((640 - pastebuttonrect.w)) / 2.0f)
		pastebuttonrect.y = ((pastetextrect.y + pastetextrect.h) + 10)
		pastebuttonrect.h = copybuttonrect.h
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs when a new event (mouse input, keypresses, etc) occurs.
function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
	scope
		SDL_ConvertEventToRenderCoordinates(renderer, event)
		if (event->type = SDL_EVENT_QUIT) then
			scope
				return SDL_APP_SUCCESS
			end scope
		else
			if (event->type = SDL_EVENT_MOUSE_BUTTON_DOWN) then
				scope
					if (event->button.button = SDL_BUTTON_LEFT) then
						scope
							dim p as SDL_FPoint = type<SDL_FPoint>(event->button.x, event->button.y)
							copy_pressed = SDL_PointInRectFloat(@(p), @(copybuttonrect))
							paste_pressed = SDL_PointInRectFloat(@(p), @(pastebuttonrect))
						end scope
					end if
				end scope
			else
				if (event->type = SDL_EVENT_MOUSE_BUTTON_UP) then
					scope
						if (event->button.button = SDL_BUTTON_LEFT) then
							scope
								dim p as SDL_FPoint = type<SDL_FPoint>(event->button.x, event->button.y)
								if (copy_pressed andalso SDL_PointInRectFloat(@(p), @(copybuttonrect))) then
									scope
										SDL_SetClipboardText(@current_time(0))
									end scope
								else
									if (paste_pressed andalso SDL_PointInRectFloat(@(p), @(pastebuttonrect))) then
										scope
											SDL_free(cptr(any ptr, pasted_str))
											pasted_str = SDL_GetClipboardText()
										end scope
									end if
								end if
								paste_pressed = false
								copy_pressed = paste_pressed
							end scope
						end if
					end scope
				end if
			end if
		end if
		return SDL_APP_CONTINUE
	end scope
end function

sub RenderPastedText cdecl()
	scope
		dim str_ as zstring ptr = pasted_str
		if str_ then
			scope
				dim x as single = (pastetextrect.x + 5)
				dim y as single = (pastetextrect.y + 5)
				dim w as single = (pastetextrect.w - 10)
				dim h as single = pastetextrect.h
				dim max_chars_per_line as uinteger = cast(uinteger, ((w / SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE)))
				dim newline as zstring ptr
				dim slen as uinteger
				dim ch as byte
				'' this doesn't wordwrap, or deal with Unicode....this is just a simple example app!
				do
					newline = SDL_strchr(str_, 10)
					if (((newline) <> cptr(zstring ptr, (cptr(any ptr, 0))))) = 0 then exit do
					scope
						dim ignore_cr as boolean = cast(boolean, ((((newline > str_)) andalso ((cptr(byte ptr, newline)[(-1)] = 13)))))
						if ignore_cr then
							scope
								cptr(byte ptr, newline)[(-1)] = 0
							end scope
						end if
						(*cptr(byte ptr, newline)) = 0
						slen = SDL_strlen(str_)
						'' length to end of line.
						slen = (iif((((slen) < (max_chars_per_line))), (slen), (max_chars_per_line)))
						ch = cptr(byte ptr, str_)[slen]
						cptr(byte ptr, str_)[slen] = 0
						SDL_RenderDebugText(renderer, x, y, str_)
						cptr(byte ptr, str_)[slen] = ch
						if ignore_cr then
							scope
								cptr(byte ptr, newline)[(-1)] = 13
							end scope
						end if
						(*cptr(byte ptr, newline)) = 10
						str_ = (newline + 1)
						y += ((SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE + 2))
						if (((h - y)) < SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) then
							scope
								exit do
							end scope
						end if
					end scope
				loop
				'' last text after newline, if there's room.
				if (((h - y)) >= SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) then
					scope
						slen = SDL_strlen(str_)
						'' length to end of line.
						slen = (iif((((slen) < (max_chars_per_line))), (slen), (max_chars_per_line)))
						ch = cptr(byte ptr, str_)[slen]
						cptr(byte ptr, str_)[slen] = 0
						SDL_RenderDebugText(renderer, x, y, str_)
						cptr(byte ptr, str_)[slen] = ch
					end scope
				end if
			end scope
		end if
	end scope
end sub

'' This function runs once per frame, and is the heart of the program.
function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
	scope
		dim x as single
		dim y as single
		CalculateCurrentTimeString()
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255)
		'' black
		SDL_RenderClear(renderer)
		'' draw a frame around the current time.
		SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255)
		SDL_RenderFillRect(renderer, @(currenttimerect))
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255)
		SDL_RenderRect(renderer, @(currenttimerect))
		'' draw the current time inside the frame.
		x = (currenttimerect.x + ((((currenttimerect.w - ((SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * SDL_strlen(@current_time(0)))))) / 2.0f)))
		y = (currenttimerect.y + 5)
		SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255)
		SDL_RenderDebugText(renderer, x, y, @current_time(0))
		'' draw a frame for the "copy the current time to the clipboard" button.
		if copy_pressed then
			scope
				SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255)
			end scope
		else
			scope
				SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255)
			end scope
		end if
		SDL_RenderFillRect(renderer, @(copybuttonrect))
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255)
		SDL_RenderRect(renderer, @(copybuttonrect))
		'' draw the "copy this text" button string.
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255)
		SDL_RenderDebugText(renderer, (copybuttonrect.x + 5), (copybuttonrect.y + 5), copybuttonstr)
		'' draw a frame for the pasted text area.
		SDL_SetRenderDrawColor(renderer, 0, 53, 25, 255)
		SDL_RenderFillRect(renderer, @(pastetextrect))
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255)
		SDL_RenderRect(renderer, @(pastetextrect))
		'' draw pasted text.
		SDL_SetRenderDrawColor(renderer, 0, 219, 107, 255)
		RenderPastedText()
		'' draw a frame for the "paste from the clipboard" button.
		if paste_pressed then
			scope
				SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255)
			end scope
		else
			scope
				SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255)
			end scope
		end if
		SDL_RenderFillRect(renderer, @(pastebuttonrect))
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255)
		SDL_RenderRect(renderer, @(pastebuttonrect))
		'' draw the "paste some text" button string.
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255)
		SDL_RenderDebugText(renderer, (pastebuttonrect.x + 5), (pastebuttonrect.y + 5), pastebuttonstr)
		'' put the new rendering on the screen.
		SDL_RenderPresent(renderer)
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once at shutdown.
sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)
	scope
		SDL_free(cptr(any ptr, pasted_str))
	end scope
end sub

#include once "callback-main.bi"

'' end of clipboard.bas
