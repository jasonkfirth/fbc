'' Project: FreeBASIC SDL3 examples
'' File: sinewave.bas
'' Purpose: Port upstream SDL3_mixer-3.2.4/examples/basics/05-sinewave/sinewave.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example code creates a mixer, and plays a sinewave forever.
''
'' It's not super-useful to play a sinewave, but it is _something_
'' to play if you don't have anything else, and it's built-in to
'' SDL_mixer.
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"
#include once "SDL3/SDL_mixer.bi"

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
		dim mixer as MIX_Mixer ptr = cptr(MIX_Mixer ptr, 0)
		dim audio as MIX_Audio ptr = cptr(MIX_Audio ptr, 0)
		dim track as MIX_Track ptr = cptr(MIX_Track ptr, 0)
		SDL_SetAppMetadata(strptr("Example Load And Play"), strptr("1.0"), strptr("com.example.load-and-play"))
		'' this doesn't have to run very much, so give up tons of CPU time between iterations. Optional!
		SDL_SetHint(strptr(SDL_HINT_MAIN_CALLBACK_RATE), strptr("5"))
		'' we don't need video, but we'll make a window for smooth operation.
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/basic/load-and-play"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't create window/renderer: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (MIX_Init() = 0) then
			scope
				SDL_Log_(strptr("Couldn't init SDL_mixer library: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		'' Create a mixer on the default audio device. Don't care about the specific audio format.
		mixer = MIX_CreateMixerDevice((SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK), cptr(const SDL_AudioSpec ptr, 0))
		if (mixer = 0) then
			scope
				SDL_Log_(strptr("Couldn't create mixer on default device: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		audio = MIX_CreateSineWaveAudio(mixer, 300, 0.25f, (-1))
		'' -1: play forever. You can specify milliseconds otherwise to have a limit.
		if (audio = 0) then
			scope
				SDL_Log_(strptr("Couldn't generate sinewave: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		'' we need a track on the mixer to play the audio. Each track has audio assigned to it, and
		'' all playing tracks are mixed together for the final output.
		track = MIX_CreateTrack(mixer)
		if (track = 0) then
			scope
				SDL_Log_(strptr("Couldn't create a mixer track: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		MIX_SetTrackAudio(track, audio)
		'' start the audio playing!
		MIX_PlayTrack(track, 0)
		'' we don't save `mixer`, `audio`, or `track`; SDL_mixer will clean it up for us during MIX_Quit().
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
		'' draw a blank video frame to keep the OS happy
		SDL_RenderClear(renderer)
		SDL_RenderPresent(renderer)
		'' there's nothing for use to do here, the sinewave will play forever until the app is manually quit.
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once at shutdown.
sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)
	scope
		'' SDL will clean up the window/renderer for us, MIX_Quit() destroys any mixer objects we made.
		MIX_Quit()
	end scope
end sub

#include once "callback-main.bi"

'' end of sinewave.bas
