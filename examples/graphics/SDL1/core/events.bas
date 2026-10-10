'' Project: FreeBASIC SDL examples
'' File: events.bas
'' Purpose:
''     Demonstrate SDL1 event filtering and event handling.
'' Responsibilities:
''     Filter one quit request and handle input on the main loop.
'' This file intentionally does NOT contain:
''     Rendering or application-created event threads.
''
' SDL_events example adapted to freeBasic from:
' http://www.libsdl.org/cgi/docwiki.cgi/Event_20Examples
'
' Demonstrates the filtering and handling of events.

#include  "SDL\SDL.bi"

' This function may run in a separate event thread
' note the use of SDLCALL (or cdecl) for this callback function
function FilterEvents SDLCALL (byval event as const SDL_Event ptr) as long
	static boycott as integer

	' This quit event signals the closing of the window
	if ((event->type = SDL_QUIT_) and boycott = 0) then
		print "Quit event filtered out -- try again."
		boycott = 1
		Return 0
	end if

	if (event->type = SDL_MOUSEMOTION) then
		print "Mouse moved to ("; event->motion.x; ","; event->motion.y; ")"
		Return 0
	end if

	Return 1
end function

	dim event as SDL_Event

	' Initialize the SDL library (starts the event loop)
	if (SDL_Init(SDL_INIT_VIDEO) < 0) then
		print "Couldn't initialize SDL: "; *SDL_GetError()
		end 1
	end if

	' Ignore key events
	SDL_EventState(SDL_KEYDOWN, SDL_IGNORE)
	SDL_EventState(SDL_KEYUP, SDL_IGNORE)

	' Filter quit and mouse motion events
	' note the function pointer pointing to the function FilterEvents
	SDL_SetEventFilter(@FilterEvents)

	' The mouse isn't much use unless we have a display for reference
	if (SDL_SetVideoMode(320, 240, 8, 0) = NULL) then
		print "Couldn't set 320x240x8 video mode: "; *SDL_GetError()
		SDL_Quit
		end 1
	end if

	' Loop waiting for ESC+Mouse_Button
	' Windows headers already use FAILED() for HRESULT checks.
	dim exitStatus as integer = 1
	do while (SDL_WaitEvent(@event) > 0)
		select case (event.type)
		case SDL_ACTIVEEVENT:
			if (event.active.state and SDL_APPACTIVE) then
				if (event.active.gain) then
					print "App activated"
				else
					print "App iconified"
				end if
			end if

		case SDL_MOUSEBUTTONDOWN:
			dim keys as Uint8 ptr

			keys = SDL_GetKeyState(NULL) + SDLK_ESCAPE
			if (*keys = SDL_PRESSED) then
				print "Bye bye..."
				exitStatus = 0
				exit do
			end if
			print "Mouse button pressed"

		case SDL_QUIT_:
			print "Quit requested, quitting."
			exitStatus = 0
			exit do
		end select
	loop

	' SDL_WaitEvent returns zero on error, not a negative value.
	if exitStatus then
		print "SDL_WaitEvent error: "; *SDL_GetError()
	end if
	SDL_Quit
	end exitStatus

'' End of events.bas
