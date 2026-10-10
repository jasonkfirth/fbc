'' Project: FreeBASIC SDL addon examples
'' File: sound-common.bi
'' Purpose: Decode and rewind a WAV file using SDL_sound.
'' Responsibilities: Validate decoded data and release the decoder's sample.
'' This file intentionally does NOT contain: mixing or asynchronous sample ownership.

#pragma once
#include once "example-common.bi"
#if SDL_ADDON_API = 1
	#include once "SDL/SDL_sound.bi"
#else
	#include once "SDL2/SDL_sound.bi"
#endif

function main() as integer
	dim filename as string = command(1)
	if filename = "" then
		print "usage: sound file.wav"
		return 1
	end if
	if SDL_Init(SDL_INIT_AUDIO) <> 0 then return 1
	if Sound_Init() = 0 then
		SDL_Quit()
		return 1
	end if
	dim sample as Sound_Sample ptr = Sound_NewSampleFromFile(strptr(filename), 0, 4096)
	if sample = 0 then
		Sound_Quit()
		SDL_Quit()
		return 1
	end if
	dim count as Uint32 = Sound_Decode(sample)
	if count = 0 or (sample->flags and SOUND_SAMPLEFLAG_ERROR) <> 0 then
		Sound_FreeSample(sample)
		Sound_Quit()
		SDL_Quit()
		return 1
	end if
	if Sound_Rewind(sample) = 0 then
		Sound_FreeSample(sample)
		Sound_Quit()
		SDL_Quit()
		return 1
	end if
	count = Sound_DecodeAll(sample)
	if count = 0 or (sample->flags and SOUND_SAMPLEFLAG_ERROR) <> 0 then
		Sound_FreeSample(sample)
		Sound_Quit()
		SDL_Quit()
		return 1
	end if
	print "Decoded bytes: "; count
	'' Decoding and rewind are also useful without opening a physical device.
	'' The local runner supplies SDL's dummy audio backend for this example.
	Sound_FreeSample(sample)
	Sound_Quit()
	SDL_Quit()
	return 0
end function

'' End of sound-common.bi
