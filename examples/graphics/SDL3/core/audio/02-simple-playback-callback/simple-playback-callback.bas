'' Project: FreeBASIC SDL3 examples
'' File: simple-playback-callback.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/audio/02-simple-playback-callback/simple-playback-callback.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example code creates a simple audio stream for playing sound, and
'' generates a sine wave sound effect for it to play as time goes on. Unlike
'' the previous example, this uses a callback to generate sound.
''
'' This might be the path of least resistance if you're moving an SDL2
'' program's audio code to SDL3.
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"

'' use the callbacks instead of main()
'' We will use this renderer to draw into this window every frame.
dim shared window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
dim shared renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
dim shared stream as SDL_AudioStream ptr = cptr(SDL_AudioStream ptr, 0)
dim shared current_sine_sample as long = 0

declare sub FeedTheAudioStreamMore cdecl(byval userdata as any ptr, byval astream as SDL_AudioStream ptr, byval additional_amount as long, byval total_amount as long)
declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

'' this function will be called (usually in a background thread) when the audio stream is consuming data.
sub FeedTheAudioStreamMore cdecl(byval userdata as any ptr, byval astream as SDL_AudioStream ptr, byval additional_amount as long, byval total_amount as long)
	scope
		'' total_amount is how much data the audio stream is eating right now, additional_amount is how much more it needs
		'' than what it currently has queued (which might be zero!). You can supply any amount of data here; it will take what
		'' it needs and use the extra later. If you don't give it enough, it will take everything and then feed silence to the
		'' hardware for the rest. Ideally, though, we always give it what it needs and no extra, so we aren't buffering more
		'' than necessary.
		additional_amount \= sizeof(single)
		'' convert from bytes to samples
		do
			if ((additional_amount > 0)) = 0 then exit do
			scope
				dim samples(0 to 127) as single
				'' this will feed 128 samples each iteration until we have enough.
				dim total as long = (iif((((additional_amount) < ((((sizeof(single) * 128) \ sizeof((samples(0)))))))), (additional_amount), ((((sizeof(single) * 128) \ sizeof((samples(0))))))))
				dim i as long
				'' generate a 440Hz pure tone
				scope
					i = 0
					do while (i < total)
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
				SDL_PutAudioStreamData(astream, cptr(const any ptr, @samples(0)), (total * sizeof(single)))
				additional_amount -= total
			end scope
		loop
	end scope
end sub

'' This function runs once at startup.
function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		dim spec as SDL_AudioSpec
		SDL_SetAppMetadata(strptr("Example Simple Audio Playback Callback"), strptr("1.0"), strptr("com.example.audio-simple-playback-callback"))
		if (SDL_Init((SDL_INIT_VIDEO or SDL_INIT_AUDIO)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		'' we don't _need_ a window for audio-only things but it's good policy to have one.
		if (SDL_CreateWindowAndRenderer(strptr("examples/audio/simple-playback-callback"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
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
		stream = SDL_OpenAudioDeviceStream((SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK), @(spec), @FeedTheAudioStreamMore, (cptr(any ptr, 0)))
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
		'' we're not doing anything with the renderer, so just blank it out.
		SDL_RenderClear(renderer)
		SDL_RenderPresent(renderer)
		'' all the work of feeding the audio stream is happening in a callback in a background thread.
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once at shutdown.
sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)
	scope
	end scope
end sub

#include once "callback-main.bi"

'' end of simple-playback-callback.bas
