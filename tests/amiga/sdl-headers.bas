'' Project: FreeBASIC Amiga library binding checks
'' File: sdl-headers.bas
'' Purpose: Verify that SDL2 uses the native C stream declaration.
'' Responsibilities: Check standalone time types and the native stdio RWops member.
'' This file intentionally does NOT require an installed SDL2 library.
#include once "crt/time.bi"
#ifdef __FB_AMIGA__
#assert sizeof(time_t) = 4
#endif
#include once "SDL2/SDL.bi"
sub SetStream(byval context as SDL_RWops ptr, byval stream as FILE ptr)
	context->hidden.stdio.fp = stream
end sub
'' end of sdl-headers.bas
