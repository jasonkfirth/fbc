'' Project: FreeBASIC SDL examples
'' File: pixel.bas
'' Purpose:
''     Plot pixels directly into a 32-bit SDL1 video surface.
'' Responsibilities:
''     Lock the surface, honor its pixel layout, and handle exit events.
'' This file intentionally does NOT contain:
''     Support for other pixel depths or audio.
''
''
'' SDL pixel ploting example
''



#include  "SDL\SDL.bi"

const SCR_WIDTH  = 320*1
const SCR_HEIGHT = 240*1

declare sub doRender (byval video as SDL_Surface ptr)

	dim result as unsigned integer
	dim video as SDL_Surface ptr
	dim event as SDL_Event

	result = SDL_Init(SDL_INIT_VIDEO)
	if result <> 0 then
		end 1
	end if

	video = SDL_SetVideoMode( SCR_WIDTH, SCR_HEIGHT, 32, 0 ) 'or SDL_FULLSCREEN
	if video = 0 then
		SDL_Quit
		end 1
	end if
	if video->format->BytesPerPixel <> len(Uint32) then
		print "This example requires a 32-bit video surface"
		SDL_Quit
		end 1
	end if

	Randomize Timer

	do

		doRender video

		SDL_Flip video

		SDL_PumpEvents
	loop until( (SDL_PollEvent( @event ) <> 0) and ((event.type = SDL_KEYDOWN) or (event.type = SDL_MOUSEBUTTONDOWN) or (event.type = SDL_QUIT_)) )

	SDL_Quit


sub doRender( byval video as SDL_Surface ptr )
	dim buffer as Uint32 ptr
	dim x as integer, y as integer
	dim c as Uint32
	dim i as integer

	if SDL_LockSurface( video ) <> 0 then
		print "SDL_LockSurface: "; *SDL_GetError()
		SDL_Quit
		end 1
	end if
	if video->pixels = NULL or video->pitch < video->w * len(Uint32) then
		print "Invalid 32-bit surface layout"
		SDL_UnlockSurface( video )
		SDL_Quit
		end 1
	end if

	for i = 1 to 1000
		x = int(rnd * video->w)
		y = int(rnd * video->h)
		c = SDL_MapRGB(video->format, rnd * 255, rnd * 255, rnd * 255)

		'' SDL pixels have a fixed 32-bit layout. FreeBASIC Integer and
		'' UInteger grow to 64 bits on a 64-bit target, unlike Uint32.
		buffer = video->pixels + y * video->pitch + x * len(Uint32)

		*buffer = c
	next i

	SDL_UnlockSurface( video )

end sub

'' End of pixel.bas
