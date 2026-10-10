'' Project: FreeBASIC SDL3 examples
'' File: multiple-streams.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/audio/04-multiple-streams/multiple-streams.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example code loads two .wav files, puts them in audio streams and
'' binds them for playback, repeating both sounds on loop. This shows several
'' streams mixing into a single playback device.
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"

'' use the callbacks instead of main()
'' We will use this renderer to draw into this window every frame.
dim shared window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
dim shared renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
dim shared audio_device as SDL_AudioDeviceID = 0
type Sound
	wav_data as Uint8 ptr
	wav_data_len as Uint32
	stream as SDL_AudioStream ptr
end type

dim shared sounds(0 to 1) as Sound

declare function init_sound cdecl(byval fname as const zstring ptr, byval sound_ as Sound ptr) as boolean
declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

function init_sound cdecl(byval fname as const zstring ptr, byval sound_ as Sound ptr) as boolean
	scope
		dim retval as boolean = false
		dim spec as SDL_AudioSpec
		dim wav_path as zstring ptr = cptr(zstring ptr, 0)
		'' Load the .wav files from wherever the app is being run from.
		SDL_asprintf(@(wav_path), strptr("%s%s"), SDL_GetBasePath(), fname)
		'' allocate a string of the full file path
		if (SDL_LoadWAV(wav_path, @(spec), @(sound_->wav_data), @(sound_->wav_data_len)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't load .wav file: %s"), SDL_GetError())
				return false
			end scope
		end if
		'' Create an audio stream. Set the source format to the wav's format (what
		'' we'll input), leave the dest format NULL here (it'll change to what the
		'' device wants once we bind it).
		sound_->stream = SDL_CreateAudioStream(@(spec), cptr(const SDL_AudioSpec ptr, 0))
		if (sound_->stream = 0) then
			scope
				SDL_Log_(strptr("Couldn't create audio stream: %s"), SDL_GetError())
			end scope
		else
			if (SDL_BindAudioStream(audio_device, sound_->stream) = 0) then
				scope
					'' once bound, it'll start playing when there is data available!
					SDL_Log_(strptr("Failed to bind '%s' stream to device: %s"), fname, SDL_GetError())
				end scope
			else
				scope
					retval = true
				end scope
			end if
		end if
		SDL_free(cptr(any ptr, wav_path))
		'' done with this string.
		return retval
	end scope
end function

'' This function runs once at startup.
function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		SDL_SetAppMetadata(strptr("Example Audio Multiple Streams"), strptr("1.0"), strptr("com.example.audio-multiple-streams"))
		if (SDL_Init((SDL_INIT_VIDEO or SDL_INIT_AUDIO)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/audio/multiple-streams"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't create window/renderer: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX)
		'' open the default audio device in whatever format it prefers; our audio streams will adjust to it.
		audio_device = SDL_OpenAudioDevice((SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK), cptr(const SDL_AudioSpec ptr, 0))
		if (audio_device = 0) then
			scope
				SDL_Log_(strptr("Couldn't open audio device: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (init_sound(strptr("sample.wav"), @(sounds(0))) = 0) then
			scope
				return SDL_APP_FAILURE
			end scope
		else
			if (init_sound(strptr("sword.wav"), @(sounds(1))) = 0) then
				scope
					return SDL_APP_FAILURE
				end scope
			end if
		end if
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
		dim i as long
		scope
			i = 0
			do while (i < (((sizeof(Sound) * 2) \ sizeof((sounds(0))))))
				scope
					'' If less than a full copy of the audio is queued for playback, put another copy in there.
					'' This is overkill, but easy when lots of RAM is cheap. One could be more careful and
					'' queue less at a time, as long as the stream doesn't run dry.
					if (SDL_GetAudioStreamQueued(sounds(i).stream) < (cast(long, sounds(i).wav_data_len))) then
						scope
							SDL_PutAudioStreamData(sounds(i).stream, cptr(const any ptr, sounds(i).wav_data), cast(long, sounds(i).wav_data_len))
						end scope
					end if
				end scope
				i += 1
			loop
		end scope
		'' just blank the screen.
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255)
		SDL_RenderClear(renderer)
		SDL_RenderPresent(renderer)
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once at shutdown.
sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)
	scope
		dim i as long
		SDL_CloseAudioDevice(audio_device)
		scope
			i = 0
			do while (i < (((sizeof(Sound) * 2) \ sizeof((sounds(0))))))
				scope
					if sounds(i).stream then
						scope
							SDL_DestroyAudioStream(sounds(i).stream)
						end scope
					end if
					SDL_free(cptr(any ptr, sounds(i).wav_data))
				end scope
				i += 1
			loop
		end scope
	end scope
end sub

#include once "callback-main.bi"

'' end of multiple-streams.bas
