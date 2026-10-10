'' Project: FreeBASIC SDL examples
'' File: timer.bas
'' Purpose:
''     Demonstrate an SDL1 timer with a shrinking interval.
'' Responsibilities:
''     Transfer timer signals through a semaphore and stop the timer.
'' This file intentionally does NOT contain:
''     Unsynchronized callback state or video.
''
' SDL_timer example adapted to freeBasic from:
' http://www.lugod.org/presentations/sdl-talk-2/18.html

#include  "SDL\SDL.bi"

	' SDL invokes timers on another thread. The semaphore transfers signals
	' to the main loop without an unsynchronized shared Integer.
	' FB-LINTER: DISABLE-NEXT-LINE FBL301
	dim shared flag as SDL_sem ptr

' callback function used by the SDL_SetTimer function
' note the use of SDLCALL (or cdecl) here, if not in place, the code may crash
' always check the SDL header files to see if SDLCALL is required for other
' any callback function used by SDL
function setflag SDLCALL (byval interval as Uint32) as Uint32
	SDL_SemPost(flag)
	' Stop before the shrinking interval falls below SDL1's 10 ms precision.
	if interval <= 20 then return 0
	Return interval \ 2
end function

	dim i as integer

	' initialise SDL with timer support on
	if SDL_Init(SDL_INIT_TIMER) <> 0 then
		print "SDL_Init: "; *SDL_GetError()
		end 1
	end if

	flag = SDL_CreateSemaphore(0)
	if flag = NULL then
		print "SDL_CreateSemaphore: "; *SDL_GetError()
		SDL_Quit
		end 1
	end if
	' Start at two seconds so the five-second example observes the callback.
	' note the use of the function pointer pointing to the setflag function defined
	' above
	if SDL_SetTimer(2000, @setflag) <> 0 then
		print "SDL_SetTimer: "; *SDL_GetError()
		SDL_DestroySemaphore(flag)
		SDL_Quit
		end 1
	end if

	' a little 5-second loop
	for i = 0 to 4
		' show a countdown
		print 5 - i

		' show if the flag was set during the last second (clear it, too)
		if SDL_SemTryWait(flag) = 0 then
			print "Flag was set!"
		end if

		SDL_Delay(1000)
	next

	' close up and quit
	SDL_SetTimer(0, NULL)
	' SDL_Quit joins the timer thread before its semaphore is destroyed.
	SDL_Quit
	SDL_DestroySemaphore(flag)

'' End of timer.bas
