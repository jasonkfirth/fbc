'' Project: FreeBASIC SDL3 examples
'' File: tag-tracks.bas
'' Purpose: Port upstream SDL3_mixer-3.2.4/examples/advanced/01-tag-tracks/tag-tracks.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example code creates a mixer, plays sounds, and tags them into groups.
''
'' We'll play a background music track, and tag the other sounds as "in-game".
'' So when you click the "pause" button, (what would be) your game sounds can
'' all pause at once, but the background music can continue on.
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"
#include once "SDL3/SDL_mixer.bi"

#define TAG_INGAME "in-game"

'' use the callbacks instead of main()
'' We will use this renderer to draw into this window every frame.
dim shared window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
dim shared renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
dim shared mixer as MIX_Mixer ptr = cptr(MIX_Mixer ptr, 0)
dim shared tracks(0 to 31) as MIX_Track ptr
dim shared next_play_ticks as Uint64 = 0
'' next time we will start a sound effect.
dim shared next_play_track as uinteger = 1
'' next track we'll use when we start a sound effect.
dim shared paused as boolean = false
type LoadedAudioRecord
	filename as const zstring ptr
	audio as MIX_Audio ptr
end type

'' whether our fake game is paused at the moment.
dim shared loaded_audio(0 to 3) as LoadedAudioRecord = {type<LoadedAudioRecord>(strptr("music.mp3"), cptr(MIX_Audio ptr, 0)), type<LoadedAudioRecord>(strptr("sword.wav"), cptr(MIX_Audio ptr, 0)), type<LoadedAudioRecord>(strptr("splash.wav"), cptr(MIX_Audio ptr, 0)), type<LoadedAudioRecord>(strptr("spring.wav"), cptr(MIX_Audio ptr, 0))}

declare function load_audio cdecl(byval fname as const zstring ptr) as MIX_Audio ptr
declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare sub draw_centered_text cdecl(byval renderer as SDL_Renderer ptr, byval rw as long, byval y as long ptr, byval str_ as const zstring ptr)
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
		dim options as SDL_PropertiesID = 0
		dim i as long
		SDL_SetAppMetadata(strptr("Example Tagging Tracks"), strptr("1.0"), strptr("com.example.tagging-tracks"))
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/advanced/tagging-tracks"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
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
		scope
			i = 0
			do while (i < cast(long, (((sizeof(LoadedAudioRecord) * 4) \ sizeof((loaded_audio(0)))))))
				scope
					loaded_audio(i).audio = load_audio(loaded_audio(i).filename)
					if (loaded_audio(i).audio = 0) then
						scope
							return SDL_APP_FAILURE
						end scope
					end if
				end scope
				i += 1
			loop
		end scope
		'' we need tracks on the mixer to play the audio. Each track has audio
		'' assigned to it, and all playing tracks are mixed together for the final
		'' output.
		scope
			i = 0
			do while (i < cast(long, (((sizeof(MIX_Track ptr) * 32) \ sizeof((tracks(0)))))))
				scope
					tracks(i) = MIX_CreateTrack(mixer)
					if (tracks(i) = 0) then
						scope
							SDL_Log_(strptr("Couldn't create a mixer track: %s"), SDL_GetError())
							return SDL_APP_FAILURE
						end scope
					end if
					if (i > 0) then
						scope
							'' everything but the background music is tagged as in-game for this example.
							if (MIX_TagTrack(tracks(i), strptr("in-game")) = 0) then
								scope
									SDL_Log_(strptr("Couldn't tag mixer track #%d: %s"), cast(long, i), SDL_GetError())
									return SDL_APP_FAILURE
								end scope
							end if
						end scope
					end if
				end scope
				i += 1
			loop
		end scope
		'' Put the music (first thing we loaded) on track[0], for simplicity here.
		MIX_SetTrackAudio(tracks(0), loaded_audio(0).audio)
		options = SDL_CreateProperties()
		if (options = 0) then
			scope
				SDL_Log_(strptr("Couldn't create play options: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		SDL_SetNumberProperty(options, strptr(MIX_PROP_PLAY_LOOPS_NUMBER), (-1))
		'' loop forever.
		'' start the music playing! it loops forever.
		MIX_PlayTrack(tracks(0), options)
		SDL_DestroyProperties(options)
		'' MIX_PlayTrack makes a copy of the options, so this can go away.
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
		else
			if (((event->type = SDL_EVENT_KEY_DOWN)) andalso ((event->key.key = 32u))) then
				scope
					paused = cast(boolean, (paused = 0))
					if paused then
						scope
							MIX_PauseTag(mixer, strptr("in-game"))
						end scope
					else
						scope
							MIX_ResumeTag(mixer, strptr("in-game"))
						end scope
					end if
				end scope
			end if
		end if
		return SDL_APP_CONTINUE
	end scope
end function

sub draw_centered_text cdecl(byval renderer as SDL_Renderer ptr, byval rw as long, byval y as long ptr, byval str_ as const zstring ptr)
	scope
		dim x as long = (((rw - (((cast(long, SDL_strlen(str_))) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE)))) \ 2)
		SDL_RenderDebugText(renderer, cast(single, x), cast(single, (*y)), str_)
		(*y) += (SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 2)
	end scope
end sub

'' This function runs once per frame, and is the heart of the program.
function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
	scope
		dim now as Uint64 = SDL_GetTicks()
		dim rw as long
		dim rh as long
		dim y as long
		if ((paused = 0) andalso ((now >= next_play_ticks))) then
			scope
				'' simulate video game sounds by starting a new one every now and then, with some randomness.
				dim track as MIX_Track ptr = tracks(next_play_track)
				'' these sounds are short enough that the tracks will finish playing before we reuse them
				MIX_SetTrackAudio(track, loaded_audio((SDL_rand(((((sizeof(LoadedAudioRecord) * 4) \ sizeof((loaded_audio(0))))) - 1)) + 1)).audio)
				'' pick a random not-music audio sound.
				MIX_PlayTrack(track, 0)
				next_play_track += 1
				if (next_play_track >= (((sizeof(MIX_Track ptr) * 32) \ sizeof((tracks(0)))))) then
					scope
						next_play_track = 1
					end scope
				end if
				next_play_ticks = (now + SDL_rand(1000))
			end scope
		end if
		SDL_GetCurrentRenderOutputSize(renderer, @(rw), @(rh))
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255)
		'' clear to black
		SDL_RenderClear(renderer)
		y = (SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 8)
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255)
		'' white text
		draw_centered_text(renderer, rw, @(y), strptr("PRETEND THIS IS A VIDEO GAME."))
		draw_centered_text(renderer, rw, @(y), strptr("THERE ARE SOUND EFFECTS AND BACKGROUND MUSIC."))
		draw_centered_text(renderer, rw, @(y), strptr(!"THE EFFECTS ARE ON TRACKS TAGGED AS \"in-game\"."))
		draw_centered_text(renderer, rw, @(y), strptr("PRESS SPACE TO PAUSE/UNPAUSE THE GAME."))
		draw_centered_text(renderer, rw, @(y), strptr("THE IN-GAME SOUNDS WILL PAUSE."))
		draw_centered_text(renderer, rw, @(y), strptr("THE MUSIC TRACK, NOT TAGGED, WILL NOT."))
		y += (SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 8)
		if paused then
			scope
				SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255)
				'' red text
				draw_centered_text(renderer, rw, @(y), strptr("[ CURRENTLY PAUSED ]"))
			end scope
		else
			scope
				SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255)
				'' green text
				draw_centered_text(renderer, rw, @(y), strptr("[ CURRENTLY UNPAUSED ]"))
			end scope
		end if
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

'' end of tag-tracks.bas
