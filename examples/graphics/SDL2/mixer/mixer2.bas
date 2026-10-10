'' Project: FreeBASIC SDL addon examples
'' File: mixer2.bas
'' Purpose: Decode a WAV file with SDL2_mixer and play one bounded audio channel.
'' Responsibilities: Wait for playback and stop the channel before freeing its chunk.
'' This file intentionally does NOT contain: music streaming or application callbacks.

#define SDL_ADDON_API 2
#include once "../../SDL_common/example-common.bi"
#include once "SDL2/SDL_mixer.bi"

function main() as integer
	dim filename as string = command(1)
	if filename = "" then
		print "usage: mixer2 sound.wav"
		return 1
	end if
	if SDL_Init(SDL_INIT_AUDIO) <> 0 then return 1
	'' Eight kHz mono matches the controlled WAV fixture. SDL_mixer converts
	'' other input formats to this device format when loading the chunk.
	if Mix_OpenAudio(8000, AUDIO_S16SYS, 1, 256) <> 0 then
		example_error("audio device initialization failed")
		SDL_Quit()
		return 1
	end if
	dim exitStatus as integer = 1
	dim chunk as Mix_Chunk ptr = Mix_LoadWAV(strptr(filename))
	if chunk <> 0 then
		dim audioChannel as long = Mix_PlayChannel(-1, chunk, 0)
		if audioChannel >= 0 then
			dim started as Uint32 = SDL_GetTicks()
			'' Unsigned subtraction remains correct across the tick counter wrap.
			while Mix_Playing(audioChannel) <> 0 andalso SDL_GetTicks() - started < 5000u
				SDL_Delay(10)
			wend
			if Mix_Playing(audioChannel) = 0 then
				print "Mixed bytes: "; chunk->alen
				exitStatus = 0
			end if
			Mix_HaltChannel(audioChannel)
		end if
		Mix_FreeChunk(chunk)
	end if
	if exitStatus <> 0 then example_error("audio loading or playback failed")
	Mix_CloseAudio()
	SDL_Quit()
	return exitStatus
end function

end main()

'' End of mixer2.bas
