'' Project: FreeBASIC SDL3 examples
'' File: playsound_simple.bas
'' Purpose: Port upstream SDL3_sound-3.2.0/examples/playsound_simple.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
''
'' SDL_sound; An abstract sound format decoding API.
''
'' Please see the file LICENSE.txt in the source's root directory.
''
'' This file written by Ryan C. Gordon.

#include once "SDL3/SDL.bi"
#include once "SDL3/SDL_sound.bi"
#include once "crt.bi"

type PlaysoundAudioCallbackData
	sample as Sound_Sample ptr
	devformat as SDL_AudioSpec
	decoded_ptr as Uint8 ptr
	decoded_bytes as Uint32
end type

'' This variable is flipped to non-zero when the audio callback has
'' finished playing the whole file.

'' The device callback runs on SDL's audio thread. Atomics publish completion
'' without relying on a volatile C integer or a BASIC data race.
dim shared global_done_flag as SDL_AtomicInt

declare sub audio_callback cdecl(byval userdata as any ptr, byval stream as Uint8 ptr, byval len_ as long)
declare sub sdl3_audio_callback cdecl(byval userdata as any ptr, byval stream as SDL_AudioStream ptr, byval additional_amount as long, byval total_amount as long)
declare sub playOneSoundFile cdecl(byval fname as const zstring ptr)
declare function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long

'' The audio callback. SDL calls this frequently to feed the audio device.
'' We decode the audio file being played in here in small chunks and feed
'' the device as necessary. Other solutions may want to predecode more
'' (or all) of the file, since this needs to run fast and frequently,
'' but since we're only sitting here and waiting for the file to play,
'' the only real requirement is that we can decode a given audio file
'' faster than realtime, which isn't really a problem with any modern format
'' on even pretty old hardware at this point.
sub audio_callback cdecl(byval userdata as any ptr, byval stream as Uint8 ptr, byval len_ as long)
	dim data_ as PlaysoundAudioCallbackData ptr = cptr(PlaysoundAudioCallbackData ptr, userdata)
	dim sample as Sound_Sample ptr = data_->sample
	dim bw as long = 0
	'' bytes written to stream this time through the callback
	do
		if ((bw < len_)) = 0 then exit do
		dim cpysize as long
		'' bytes to copy on this iteration of the loop.
		if (data_->decoded_bytes = 0) then
			'' need more data!
			'' if there wasn't previously an error or EOF, read more.
			if (((((sample->flags and SOUND_SAMPLEFLAG_ERROR)) = 0)) andalso ((((sample->flags and SOUND_SAMPLEFLAG_EOF)) = 0))) then
				data_->decoded_bytes = Sound_Decode(sample)
				data_->decoded_ptr = cptr(Uint8 ptr, sample->buffer)
			end if
			'' if
			if (data_->decoded_bytes = 0) then
				'' ...there isn't any more data to read!
				SDL_memset(cptr(any ptr, (stream + bw)), 0, (len_ - bw))
				'' write silence.
				SDL_SetAtomicInt(@global_done_flag, 1)
				exit sub
			end if
		end if
		'' if
		'' we have data decoded and ready to write to the device...
		cpysize = (len_ - bw)
		'' len - bw == amount device still wants.
		if (cpysize > cast(Sint32, data_->decoded_bytes)) then
			cpysize = cast(Sint32, data_->decoded_bytes)
		end if
		'' clamp to what we have left.
		'' if it's 0, next iteration will decode more or decide we're done.
		if (cpysize > 0) then
			'' write this iteration's data to the device.
			SDL_memcpy(cptr(any ptr, (stream + bw)), cptr(const any ptr, cptr(Uint8 ptr, data_->decoded_ptr)), cpysize)
			'' update state for next iteration or callback
			bw += cpysize
			data_->decoded_ptr += cpysize
			data_->decoded_bytes -= cpysize
		end if
	loop
end sub

'' audio_callback
sub sdl3_audio_callback cdecl(byval userdata as any ptr, byval stream as SDL_AudioStream ptr, byval additional_amount as long, byval total_amount as long)
	if (additional_amount > 0) then
		dim data_ as Uint8 ptr = cptr(Uint8 ptr, SDL_malloc((sizeof(Uint8) * (additional_amount))))
		if data_ then
			audio_callback(userdata, data_, additional_amount)
			SDL_PutAudioStreamData(stream, cptr(const any ptr, data_), additional_amount)
			SDL_free(cptr(any ptr, data_))
		end if
	end if
end sub

sub playOneSoundFile cdecl(byval fname as const zstring ptr)
	dim stream as SDL_AudioStream ptr
	dim data_ as PlaysoundAudioCallbackData
	dim spec as SDL_AudioSpec
	dim sample_frames as long
	SDL_memset(cptr(any ptr, @((data_))), 0, sizeof(((data_))))
	data_.sample = Sound_NewSampleFromFile(fname, cptr(const SDL_AudioSpec ptr, 0), 65536)
	if (data_.sample = cptr(Sound_Sample ptr, (cptr(any ptr, 0)))) then
		fprintf(stderr, strptr(!"Couldn't load '%s': %s.\n"), fname, Sound_GetError())
		exit sub
	end if
	'' Open device in format of the the sound to be played.
	'' We open and close the device for each sound file, so that SDL
	'' handles the data conversion to hardware format; this is the
	'' easy way out, but isn't practical for most apps. Usually you'll
	'' want to pick one format for all the data or one format for the
	'' audio device and convert the data when needed. This is a more
	'' complex issue than I can describe in a source code comment, though.
	data_.devformat.freq = data_.sample->actual.freq
	data_.devformat.format = data_.sample->actual.format
	data_.devformat.channels = data_.sample->actual.channels
	stream = SDL_OpenAudioDeviceStream((SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK), @(data_.devformat), @sdl3_audio_callback, cptr(any ptr, @(data_)))
	if (stream = cptr(SDL_AudioStream ptr, (cptr(any ptr, 0)))) then
		fprintf(stderr, strptr(!"Couldn't open audio device: %s.\n"), SDL_GetError())
		Sound_FreeSample(data_.sample)
		exit sub
	end if
	'' if
	printf(strptr(!"Now playing [%s]...\n"), fname)
	SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(stream))
	'' SDL audio device is "paused" right after opening.
	SDL_SetAtomicInt(@global_done_flag, 0)
	'' the audio callback will flip this flag.
	do
		if ((SDL_SetAtomicInt(@global_done_flag, 0))) = 0 then exit do
		SDL_Delay(10)
	loop
	'' just wait for the audio callback to finish.
	'' at this point, we've played the entire audio file.
	SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(stream))
	'' Sleep two buffers' worth of audio before closing, in order
	'' to allow the playback to finish. This isn't always enough;
	'' perhaps SDL needs a way to explicitly wait for device drain?
	'' Most apps don't have this issue, since they aren't explicitly
	'' closing the device as soon as a sound file is done playback.
	'' As an alternative for this app, you could also change the callback
	'' to write silence for a call or two before flipping global_done_flag.
	sample_frames = 0
	SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(stream), @(spec), @(sample_frames))
	SDL_Delay(((sample_frames * 1000) \ spec.freq))
	'' if there was an error, tell the user.
	if (data_.sample->flags and SOUND_SAMPLEFLAG_ERROR) then
		fprintf(stderr, strptr(!"Error decoding file: %s\n"), Sound_GetError())
	end if
	Sound_FreeSample(data_.sample)
	'' clean up SDL_Sound resources...
	SDL_CloseAudioDevice(SDL_GetAudioStreamDevice(stream))
end sub

'' playOneSoundFile
function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long
	dim i as long
	if (Sound_Init() = 0) then
		'' this calls SDL_Init(SDL_INIT_AUDIO) ...
		fprintf(stderr, strptr(!"Sound_Init() failed: %s.\n"), Sound_GetError())
		SDL_Quit()
		return (42)
	end if
	'' if
	scope
		i = 1
		do while (i < argc)
			'' each arg is an audio file to play.
			playOneSoundFile(argv[i])
			i += 1
		loop
	end scope
	'' Shutdown the libraries...
	Sound_Quit()
	SDL_Quit()
	return (0)
end function

end SDL_RunApp(__FB_ARGC__, __FB_ARGV__, @example_main, 0)

'' end of playsound_simple.bas
