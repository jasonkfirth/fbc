'' Project: FreeBASIC SDL3 examples
'' File: play-multiple-sounds.bas
'' Purpose: Port upstream SDL3_mixer-3.2.4/examples/basics/03-play-multiple-sounds/play-multiple-sounds.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example code creates a mixer, loads two sounds, and mixes them.
''
'' SDL_mixer is, of course, a mixer, so here are two sounds mixing together!
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"
#include once "SDL3/SDL_mixer.bi"

'' use the callbacks instead of main()
'' We will use this renderer to draw into this window every frame.
dim shared window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
dim shared renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
dim shared mixer as MIX_Mixer ptr = cptr(MIX_Mixer ptr, 0)
dim shared track1 as MIX_Track ptr = cptr(MIX_Track ptr, 0)

declare function load_audio cdecl(byval fname as const zstring ptr) as MIX_Audio ptr
declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

function load_audio cdecl(byval fname as const zstring ptr) as MIX_Audio ptr
	scope
		dim path as zstring ptr = cptr(zstring ptr, 0)
		dim audio as MIX_Audio ptr
		SDL_asprintf(@(path), strptr("%s%s"), SDL_GetBasePath(), fname)
		'' allocate a string of the full file path
		audio = MIX_LoadAudio(mixer, path, false)
		if (audio = 0) then
			scope
				SDL_Log_(strptr("Couldn't load %s: %s"), path, SDL_GetError())
			end scope
		end if
		SDL_free(cptr(any ptr, path))
		'' done with this.
		return audio
	end scope
end function

'' This function runs once at startup.
function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		dim music_ as MIX_Audio ptr = cptr(MIX_Audio ptr, 0)
		dim sound_ as MIX_Audio ptr = cptr(MIX_Audio ptr, 0)
		dim track2 as MIX_Track ptr = cptr(MIX_Track ptr, 0)
		dim options as SDL_PropertiesID = 0
		SDL_SetAppMetadata(strptr("Example Play Multiple Sounds"), strptr("1.0"), strptr("com.example.play-multiple-sounds"))
		'' this doesn't have to run very much, so give up tons of CPU time between iterations. Optional!
		SDL_SetHint(strptr(SDL_HINT_MAIN_CALLBACK_RATE), strptr("5"))
		'' we don't need video, but we'll make a window for smooth operation.
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/basic/play-multiple-sounds"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
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
		'' load our audio files. Note that you can use any supported file format!
		music_ = load_audio(strptr("music.mp3"))
		if (music_ = 0) then
			scope
				return SDL_APP_FAILURE
			end scope
		end if
		sound_ = load_audio(strptr("sword.wav"))
		if (sound_ = 0) then
			scope
				return SDL_APP_FAILURE
			end scope
		end if
		'' we need a track on the mixer to play the audio. Each track has audio assigned to it, and
		'' all playing tracks are mixed together for the final output.
		track1 = MIX_CreateTrack(mixer)
		if (track1 = 0) then
			scope
				SDL_Log_(strptr("Couldn't create a mixer track: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		MIX_SetTrackAudio(track1, music_)
		track2 = MIX_CreateTrack(mixer)
		if (track2 = 0) then
			scope
				SDL_Log_(strptr("Couldn't create a mixer track: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		MIX_SetTrackAudio(track2, sound_)
		options = SDL_CreateProperties()
		if (options = 0) then
			scope
				SDL_Log_(strptr("Couldn't create play options: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		SDL_SetNumberProperty(options, strptr(MIX_PROP_PLAY_LOOPS_NUMBER), (-1))
		'' loop forever.
		'' start the audio playing! music plays through once, with the sound effect playing in a loop at the same time.
		MIX_PlayTrack(track1, 0)
		'' no extra options this time, so a zero for the second argument.
		MIX_PlayTrack(track2, options)
		SDL_DestroyProperties(options)
		'' we don't save `music`, `sound`, or `track2`; SDL_mixer will clean them up for us during MIX_Quit().
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
		'' when the music has finished playing, end the program.
		if (MIX_TrackPlaying(track1) = 0) then
			scope
				return SDL_APP_SUCCESS
			end scope
		end if
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

'' end of play-multiple-sounds.bas
