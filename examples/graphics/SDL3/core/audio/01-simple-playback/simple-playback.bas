'' Project: FreeBASIC SDL3 examples
'' File: simple-playback.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/audio/01-simple-playback/simple-playback.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example code creates a simple audio stream for playing sound, and
'' generates a sine wave sound effect for it to play as time goes on. This
'' is the simplest way to get up and running with procedural sound.
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"

'' use the callbacks instead of main()
'' We will use this renderer to draw into this window every frame.
dim shared window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
dim shared renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
dim shared stream as SDL_AudioStream ptr = cptr(SDL_AudioStream ptr, 0)
dim shared current_sine_sample as long = 0

declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

'' This function runs once at startup.
function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		dim spec as SDL_AudioSpec
		SDL_SetAppMetadata(strptr("Example Audio Simple Playback"), strptr("1.0"), strptr("com.example.audio-simple-playback"))
		if (SDL_Init((SDL_INIT_VIDEO or SDL_INIT_AUDIO)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		'' we don't _need_ a window for audio-only things but it's good policy to have one.
		if (SDL_CreateWindowAndRenderer(strptr("examples/audio/simple-playback"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't create window/renderer: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX)
		'' We're just playing a single thing here, so we'll use the simplified option.
		'' We are always going to feed audio in as mono, float32 data at 8000Hz.
		'' The stream will convert it to whatever the hardware wants on the other side.
		spec.channels = 1
		spec.format = SDL_AUDIO_F32
		spec.freq = 8000
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
		'' We're being lazy here, but if there's less than half a second queued, generate more.
		'' A sine wave is unchanging audio--easy to stream--but for video games, you'll want
		'' to generate significantly _less_ audio ahead of time!
		dim minimum_audio as long = (((8000 * sizeof(single))) \ 2)
		'' 8000 float samples per second. Half of that.
		if (SDL_GetAudioStreamQueued(stream) < minimum_audio) then
			scope
				static samples(0 to 511) as single
				'' this will feed 512 samples each frame until we get to our maximum.
				dim i as long
				'' generate a 440Hz pure tone
				scope
					i = 0
					do while (i < (((sizeof(single) * 512) \ sizeof((samples(0))))))
						scope
							dim freq as long = 440
							dim phase as single = ((current_sine_sample * freq) / 8000.0f)
							samples(i) = SDL_sinf(((phase * 2) * SDL_PI_F))
							current_sine_sample += 1
						end scope
						i += 1
					loop
				end scope
				'' wrapping around to avoid floating-point errors
				current_sine_sample mod= 8000
				'' feed the new data to the stream. It will queue at the end, and trickle out as the hardware needs more data.
				SDL_PutAudioStreamData(stream, cptr(const any ptr, @samples(0)), (sizeof(single) * 512))
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
	end scope
end sub

#include once "callback-main.bi"

'' end of simple-playback.bas
