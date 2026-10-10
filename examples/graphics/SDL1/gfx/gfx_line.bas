'' Project: FreeBASIC SDL examples
'' File: gfx_line.bas
'' Purpose:
''     Draw random lines using SDL1_gfx.
'' Responsibilities:
''     Create the video surface, draw lines, and handle exit events.
'' This file intentionally does NOT contain:
''     Direct pixel access or audio.
''
''
'' SDL_gfx library example: line drawing
''

#include "SDL\SDL.bi"
#include "SDL\SDL_gfx_primitives.bi"

const SCR_WIDTH  = 320*1
const SCR_HEIGHT = 240*1
const SCR_BPP	 = 16

declare sub doRender (byval video as SDL_Surface ptr)

	dim result as unsigned integer
	dim video as SDL_Surface ptr
	dim event as SDL_Event

	randomize timer

	result = SDL_Init(SDL_INIT_VIDEO)
	if result <> 0 then
		end 1
	end if

	video = SDL_SetVideoMode( SCR_WIDTH, SCR_HEIGHT, SCR_BPP, 0 ) 'or SDL_FULLSCREEN
	if video = 0 then
		SDL_Quit
		end 1
	end if

	do

		doRender video

		SDL_Flip video

		SDL_PumpEvents
	loop until( (SDL_PollEvent( @event ) <> 0) and ((event.type = SDL_KEYDOWN) or (event.type = SDL_MOUSEBUTTONDOWN) or (event.type = SDL_QUIT_)) )

	SDL_Quit


sub doRender( byval video as SDL_Surface ptr )
	dim x1 as integer, y1 as integer
	dim x2 as integer, y2 as integer
	dim r as uinteger, g as uinteger, b as uinteger
	dim i as integer

	for i = 1 to 1000
		x1 = rnd * (SCR_WIDTH-1)
		y1 = rnd * (SCR_HEIGHT-1)
		x2 = rnd * (SCR_WIDTH-1)
		y2 = rnd * (SCR_HEIGHT-1)
		r = rnd * 255
		g = rnd * 255
		b = rnd * 255

		lineRGBA video, x1, y1, x2, y2, r, g, b, 255
	next i

end sub

'' End of gfx_line.bas
