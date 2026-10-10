'' Project: FreeBASIC SDL examples
'' File: video_info.bas
'' Purpose:
''     Report the SDL1 video driver and its capabilities.
'' Responsibilities:
''     Query and validate driver information, then shut down SDL.
'' This file intentionally does NOT contain:
''     Window creation or graphics rendering.
''
'
' SDL_video example
'

#include  "SDL\SDL.bi"

	Dim Video As const SDL_VideoInfo Ptr

	' Startup SDL
	If ( SDL_Init( SDL_INIT_VIDEO ) = -1) Then
		Print "SDL_Init: "; *SDL_GetError()
		End 1
	End If

	Video = SDL_GetVideoInfo

	If Video <> 0 Then

		'' Keep the C string buffer alive while using the returned pointer.
		dim NameBuffer as zstring * 256
		dim driver as zstring ptr = SDL_VideoDriverName( NameBuffer, len( NameBuffer ) )
		if driver = NULL then
			Print "SDL_VideoDriverName: "; *SDL_GetError()
			SDL_Quit
			End 1
		end if

		Print "Device: "; *driver

		Print Using "VRAM: ###,### KB"; Video->video_mem

		If ( Video->hw_available ) Then Print "  + Hardware surfaces available"
		If ( Video->wm_available ) Then Print "  + Window manager available"
		If ( Video->blit_hw      ) Then Print "  + Hardware to hardware blits accelerated"
		If ( Video->blit_hw_CC   ) Then Print "  + Hardware to hardware colorkey blits accelerated"
		If ( Video->blit_hw_A    ) Then Print "  + Hardware to hardware alpha blits accelerated"
		If ( Video->blit_sw      ) Then Print "  + Software to hardware blits accelerated"
		If ( Video->blit_sw_CC   ) Then Print "  + Software to hardware colorkey blits accelerated"
		If ( Video->blit_sw_A    ) Then Print "  + Software to hardware alpha blits accelerated"
		If ( Video->blit_fill    ) Then Print "  + Color fills accelerated"

	Else
		Print "SDL_GetVideoInfo: "; *SDL_GetError()
		SDL_Quit
		End 1
	End If

	' shutdown SDL
	SDL_Quit

	sleep

'' End of video_info.bas
