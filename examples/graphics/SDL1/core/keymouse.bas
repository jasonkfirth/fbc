'' Project: FreeBASIC SDL examples
'' File: keymouse.bas
'' Purpose:
''     Display SDL1 keyboard and mouse events.
'' Responsibilities:
''     Initialize video, handle input, and shut down SDL.
'' This file intentionally does NOT contain:
''     Rendering or audio playback.
''
''
'' SDL events test
'' by marzec
''

#include once "SDL/SDL.bi"

declare sub doInit ( )
declare sub doQuit ( )
declare sub doMain ( )
declare sub exitError ( byref msg as string )

'' Setup stores the video surface for the module's SDL procedures.
'' FB-LINTER: DISABLE-NEXT-LINE FBL301
dim shared video as SDL_Surface ptr

doInit
doMain
doQuit



sub doInit ( )

	if(SDL_Init( SDL_INIT_VIDEO )) then exitError "couldn't init SDL"

	video = SDL_SetVideoMode ( 320, 200, 24, SDL_HWSURFACE )
	if( video = 0 ) then exitError "couldn't init videomode"

end sub

sub doMain ( )

	dim quit as Integer
	dim event as SDL_Event
	quit = 1

	print "-----------------------------------------------"
	print "                  SDL TEST"
	print "-----------------------------------------------"

	while( quit = 1)

		while( SDL_PollEvent ( @event ) )
		  print "event happened, type:" + str(event.type)
			select case event.type
				case SDL_KEYDOWN:
					print "key event, pressed key:" + str(event.key.keysym.sym)
					print
				case SDL_MOUSEBUTTONDOWN, SDL_QUIT_:
					print "mousebutton pressed, quitting"
					quit = 0
			end select

		wend
		SDL_Delay(1)

	wend

	print "leaving doMain"
end sub

sub doQuit ( )

	print "quiting"
	SDL_Quit

end sub

sub exitError ( byref msg as string )
	print "error: " + msg
	SDL_Quit
	'' Initialization failed; returning would let the caller continue with no SDL video surface.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL-CF-005
	end 1
end sub

'' End of keymouse.bas
