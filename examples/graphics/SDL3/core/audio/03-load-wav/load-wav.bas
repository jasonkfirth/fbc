'' Project: FreeBASIC SDL3 examples
'' File: load-wav.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/audio/03-load-wav/load-wav.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example code creates a simple audio stream for playing sound, and
'' loads a .wav file that is pushed through the stream in a loop.
''
'' This code is public domain. Feel free to use it for any purpose!
''
'' The .wav file is a sample from Will Provost's song, The Living Proof,
'' used with permission.
''
'' From the album The Living Proof
'' Publisher: 5 Guys Named Will
'' Copyright 1996 Will Provost
'' https://itunes.apple.com/us/album/the-living-proof/id4153978
'' http://www.amazon.com/The-Living-Proof-Will-Provost/dp/B00004R8RH

#include once "SDL3/SDL.bi"

'' use the callbacks instead of main()
'' We will use this renderer to draw into this window every frame.
dim shared window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
dim shared renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
dim shared stream as SDL_AudioStream ptr = cptr(SDL_AudioStream ptr, 0)
dim shared wav_data as Uint8 ptr = cptr(Uint8 ptr, 0)
dim shared wav_data_len as Uint32 = 0

declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

'' This function runs once at startup.
function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		dim spec as SDL_AudioSpec
		dim wav_path as zstring ptr = cptr(zstring ptr, 0)
		SDL_SetAppMetadata(strptr("Example Audio Load Wave"), strptr("1.0"), strptr("com.example.audio-load-wav"))
		if (SDL_Init((SDL_INIT_VIDEO or SDL_INIT_AUDIO)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		'' we don't _need_ a window for audio-only things but it's good policy to have one.
		if (SDL_CreateWindowAndRenderer(strptr("examples/audio/load-wav"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't create window/renderer: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX)
		'' Load the .wav file from wherever the app is being run from.
		SDL_asprintf(@(wav_path), strptr("%ssample.wav"), SDL_GetBasePath())
		'' allocate a string of the full file path
		if (SDL_LoadWAV(wav_path, @(spec), @(wav_data), @(wav_data_len)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't load .wav file: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		SDL_free(cptr(any ptr, wav_path))
		'' done with this string.
		'' Create our audio stream in the same format as the .wav file. It'll convert to what the audio hardware wants.
		stream = SDL_OpenAudioDeviceStream((SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK), @(spec), cptr(SDL_AudioStreamCallback, 0), (cptr(any ptr, 0)))
		if (stream = 0) then
			scope
				SDL_Log_(strptr("Couldn't create audio stream: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		'' SDL_OpenAudioDeviceStream starts the device paused. You have to tell it to start!
		SDL_ResumeAudioStreamDevice(stream)
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
		'' see if we need to feed the audio stream more data yet.
		'' We're being lazy here, but if there's less than the entire wav file left to play,
		'' just shove a whole copy of it into the queue, so we always have _tons_ of
		'' data queued for playback.
		if (SDL_GetAudioStreamQueued(stream) < cast(long, wav_data_len)) then
			scope
				'' feed more data to the stream. It will queue at the end, and trickle out as the hardware needs more data.
				SDL_PutAudioStreamData(stream, cptr(const any ptr, wav_data), wav_data_len)
			end scope
		end if
		'' we're not doing anything with the renderer, so just blank it out.
		SDL_RenderClear(renderer)
		SDL_RenderPresent(renderer)
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once at shutdown.
sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)
	scope
		SDL_free(cptr(any ptr, wav_data))
	end scope
end sub

#include once "callback-main.bi"

'' end of load-wav.bas
