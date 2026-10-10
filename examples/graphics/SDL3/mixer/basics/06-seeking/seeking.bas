'' Project: FreeBASIC SDL3 examples
'' File: seeking.bas
'' Purpose: Port upstream SDL3_mixer-3.2.4/examples/basics/06-seeking/seeking.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example code creates a mixer, loads a single sound, and plays it on
'' loop, letting the user seek around in playback with a slider.
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"
#include once "SDL3/SDL_mixer.bi"

#define KNOB_WIDTH 30

'' use the callbacks instead of main()
'' We will use this renderer to draw into this window every frame.
dim shared window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
dim shared renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
dim shared mixer as MIX_Mixer ptr = cptr(MIX_Mixer ptr, 0)
dim shared audio as MIX_Audio ptr = cptr(MIX_Audio ptr, 0)
dim shared track as MIX_Track ptr = cptr(MIX_Track ptr, 0)
dim shared progressbar as SDL_FRect = type<SDL_FRect>(120, 225, 400, 75)

declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare sub seek_audio cdecl(byval x as single, byval y as single)
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

'' This function runs once at startup.
function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		dim path as zstring ptr = cptr(zstring ptr, 0)
		dim options as SDL_PropertiesID = 0
		SDL_SetAppMetadata(strptr("Example Seeking"), strptr("1.0"), strptr("com.example.seeking"))
		'' this doesn't have to run very much, so give up tons of CPU time between iterations. Optional!
		SDL_SetHint(strptr(SDL_HINT_MAIN_CALLBACK_RATE), strptr("5"))
		'' we don't need video, but we'll make a window for smooth operation.
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/basic/seeking"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
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
		'' load a sound file
		SDL_asprintf(@(path), strptr("%smusic.mp3"), SDL_GetBasePath())
		'' allocate a string of the full file path
		audio = MIX_LoadAudio(mixer, path, false)
		if (audio = 0) then
			scope
				SDL_Log_(strptr("Couldn't load %s: %s"), path, SDL_GetError())
				SDL_free(cptr(any ptr, path))
				return SDL_APP_FAILURE
			end scope
		end if
		SDL_free(cptr(any ptr, path))
		'' done with this, the file is loaded.
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
		options = SDL_CreateProperties()
		if (options = 0) then
			scope
				SDL_Log_(strptr("Couldn't create play options: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		'' note these all use MILLISECONDS, since the track is generic, but you can use equivalent FRAMES properties, for frame-perfect mixing.
		SDL_SetNumberProperty(options, strptr(MIX_PROP_PLAY_LOOPS_NUMBER), (-1))
		'' loop forever
		MIX_PlayTrack(track, options)
		SDL_DestroyProperties(options)
		'' MIX_PlayTrack makes a copy of the options, so this can go away.
		return SDL_APP_CONTINUE
	end scope
end function

sub seek_audio cdecl(byval x as single, byval y as single)
	scope
		dim pt as SDL_FPoint = type<SDL_FPoint>(x, y)
		if SDL_PointInRectFloat(@(pt), @(progressbar)) then
			scope
				'' seek to a new position in the track, based on where the mouse clicked/dragged in the progress bar.
				dim pct as single = (((pt.x - progressbar.x)) / progressbar.w)
				MIX_SetTrackPlaybackPosition(track, cast(Sint64, ((cast(single, MIX_GetAudioDuration(audio)) * pct))))
			end scope
		end if
	end scope
end sub

'' This function runs when a new event (mouse input, keypresses, etc) occurs.
function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
	scope
		if (event->type = SDL_EVENT_QUIT) then
			scope
				return SDL_APP_SUCCESS
			end scope
		else
			if (event->type = SDL_EVENT_MOUSE_BUTTON_DOWN) then
				scope
					if (event->button.button = 1) then
						scope
							seek_audio(event->button.x, event->button.y)
						end scope
					end if
				end scope
			else
				if (event->type = SDL_EVENT_MOUSE_MOTION) then
					scope
						if (event->motion.state and (SDL_BUTTON_LMASK)) then
							scope
								seek_audio(event->motion.x, event->motion.y)
							end scope
						end if
					end scope
				end if
			end if
		end if
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once per frame, and is the heart of the program.
function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
	scope
		dim knob as SDL_FRect = type<SDL_FRect>(progressbar.x, progressbar.y, 30, progressbar.h)
		knob.x += (((progressbar.w - knob.w)) * (((cast(single, MIX_GetTrackPlaybackPosition(track))) / (cast(single, MIX_GetAudioDuration(audio))))))
		SDL_SetRenderDrawColor(renderer, 0, 0, 100, 255)
		SDL_RenderClear(renderer)
		SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255)
		SDL_RenderFillRect(renderer, @(progressbar))
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255)
		SDL_RenderFillRect(renderer, @(knob))
		SDL_RenderPresent(renderer)
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

'' end of seeking.bas
