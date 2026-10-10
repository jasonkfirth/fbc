'' Project: FreeBASIC SDL3 examples
'' File: playsound.bas
'' Purpose: Port upstream SDL3_sound-3.2.0/examples/playsound.c.
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
#include once "smoke.bi"
#include once "SDL3/SDL_sound.bi"
#include once "crt.bi"

#define DEFAULT_DECODEBUF 16384
#define DEFAULT_AUDIOBUF 4096
#define PLAYSOUND_VER_MAJOR 3
#define PLAYSOUND_VER_MINOR 2
#define PLAYSOUND_VER_PATCH 0

''
'' This is a quick and dirty test of SDL_sound.
'' this is a console-only app
dim shared option_list(0 to 29) as const zstring ptr = { _
	strptr("--rate"), strptr("n       Playback at sample rate of n HZ."), strptr("--format"), strptr("fmt   Playback in fmt format (see below)."), strptr("--channels"), strptr("n   Playback on n channels (1 or 2)."), strptr("--decodebuf"), strptr("n  Buffer n decoded bytes at a time (default 16384)."), strptr("--audiobuf"), strptr("n   Buffer n samples to audio device (default 4096)."), strptr("--volume_level"), strptr("n     Playback volume_level multiplier (default 1.0)."), strptr("--window"), strptr("      Open a window for playback."), strptr("--version"), strptr("     Display version information and exit."), _
	strptr("--decoders"), strptr("    List supported data formats and exit."), strptr("--predecode"), strptr("   Decode entire sample before playback."), strptr("--loop"), strptr("n       Loop playback n times."), strptr("--seek"), strptr("list    List of seek points and playback durations."), strptr("--credits"), strptr("     Shameless promotion."), strptr("--help"), strptr("        Display this information and exit."), cptr(const zstring ptr, 0), cptr(const zstring ptr, 0) _
}
'' output_credits
dim shared done_flag as long = 0
type playsound_global_state
	decoded_ptr as Uint8 ptr
	decoded_bytes as Uint32
	predecode as long
	looping as long
	wants_volume_change as long
	volume_level as single
	total_seeks as Uint32
	seek_list as Uint32 ptr
	seek_index as Uint32
	bytes_before_next_seek as Sint32
end type

dim shared global_state as playsound_global_state

declare sub output_versions cdecl(byval argv0 as const zstring ptr)
declare sub output_decoders cdecl()
declare sub output_usage cdecl(byval argv0 as const zstring ptr)
declare sub output_credits cdecl()
declare function cvtMsToBytePos cdecl(byval info as SDL_AudioSpec ptr, byval ms as Uint32) as Uint32
declare sub do_seek cdecl(byval sample as Sound_Sample ptr)
declare function read_more_data cdecl(byval sample as Sound_Sample ptr) as long
declare sub memcpy_with_volume cdecl(byval sample as Sound_Sample ptr, byval dst as Uint8 ptr, byval src as Uint8 ptr, byval len_ as long)
declare sub audio_callback cdecl(byval userdata as any ptr, byval stream as Uint8 ptr, byval len_ as long)
declare sub sdl3_audio_callback cdecl(byval userdata as any ptr, byval stream as SDL_AudioStream ptr, byval additional_amount as long, byval total_amount as long)
declare function count_seek_list cdecl(byval list as const zstring ptr) as long
declare function parse_time_str cdecl(byval str_ as zstring ptr) as Uint32
declare sub parse_seek_list cdecl(byval _list as const zstring ptr)
declare function str_to_fmt cdecl(byval str_ as zstring ptr) as long
declare function valid_cmdline cdecl(byval argc as long, byval argv as zstring ptr ptr) as long
declare sub report_filename cdecl(byval filename as const zstring ptr)
declare function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long

sub output_versions cdecl(byval argv0 as const zstring ptr)
	dim compiled as long = (SDL_SOUND_VERSION)
	dim linked as long = Sound_Version()
	dim sdl_compiled as long = (SDL_VERSION)
	dim sdl_linked as long = SDL_GetVersion()
	fprintf(stdout, strptr(!"%s version %d.%d.%d\nCopyright 2001-2026 Ryan C. Gordon and others.\n\n Compiled against SDL_sound version %d.%d.%d,\n and linked against %d.%d.%d.\n Compiled against SDL version %d.%d.%d,\n and linked against %d.%d.%d.\n\n"), _
	        argv0, cast(long, 3), cast(long, 2), cast(long, 0), _
	        SDL_VERSIONNUM_MAJOR(compiled), SDL_VERSIONNUM_MINOR(compiled), SDL_VERSIONNUM_MICRO(compiled), _
	        SDL_VERSIONNUM_MAJOR(linked), SDL_VERSIONNUM_MINOR(linked), SDL_VERSIONNUM_MICRO(linked), _
	        SDL_VERSIONNUM_MAJOR(sdl_compiled), SDL_VERSIONNUM_MINOR(sdl_compiled), SDL_VERSIONNUM_MICRO(sdl_compiled), _
	        SDL_VERSIONNUM_MAJOR(sdl_linked), SDL_VERSIONNUM_MINOR(sdl_linked), SDL_VERSIONNUM_MICRO(sdl_linked))
end sub

'' output_versions
sub output_decoders cdecl()
	dim rc as const Sound_DecoderInfo ptr ptr = Sound_AvailableDecoders()
	dim i as const Sound_DecoderInfo ptr ptr
	dim ext as const zstring ptr ptr
	fprintf(stdout, strptr(!"Supported sound formats:\n"))
	if cast(any ptr, rc) = 0 then
		fprintf(stdout, strptr(!" * Apparently, NONE!\n"))
	else
		scope
			i = rc
			do while ((*i) <> cptr(const Sound_DecoderInfo ptr, (cptr(any ptr, 0))))
				fprintf(stdout, strptr(!" * %s\n"), ((*i))->description)
				scope
					ext = ((*i))->extensions
					do while ((*ext) <> cptr(const zstring ptr, (cptr(any ptr, 0))))
						fprintf(stdout, strptr(!"   File extension \"%s\"\n"), (*ext))
						loop_continue_2:
						ext += 1
					loop
				end scope
				fprintf(stdout, strptr(!"   Written by %s.\n   %s\n\n"), ((*i))->author, ((*i))->url)
				i += 1
			loop
		end scope
	end if
	'' else
	fprintf(stdout, strptr(!"\n"))
end sub

'' output_decoders
sub output_usage cdecl(byval argv0 as const zstring ptr)
	dim i as const zstring ptr ptr = @option_list(0)
	fprintf(stderr, strptr(!"USAGE: %s [...options...] [soundFile1] ... [soundFileN]\n\n   Options:\n"), argv0)
	do
		if (((*i) <> cptr(const zstring ptr, (cptr(any ptr, 0))))) = 0 then exit do
		dim expression_value_4 as const zstring ptr ptr = i
		i += 1
		dim option_ as const zstring ptr = (*(expression_value_4))
		dim expression_value_5 as const zstring ptr ptr = i
		i += 1
		dim optiondesc as const zstring ptr = (*(expression_value_5))
		fprintf(stderr, strptr(!"     %s %s\n"), option_, optiondesc)
		loop_continue_3:
	loop
	'' while
	fprintf(stderr, strptr((!"\n   Valid arguments to the --format option are:\n     U8      Unsigned 8-bit.\n     S8      Signed 8-bit.\n     S16LSB  Signed 16-bit (least signific" & _
		!"ant byte first).\n     S16MSB  Signed 16-bit (most significant byte first).\n     S32LSB  Signed 32-bit (least significant byte first).\n     S32MSB  " & _
		!"Signed 32-bit (most significant byte first).\n     F32LSB  Float 32-bit (least significant byte first).\n     F32MSB  Float 32-bit (most significant b" & _
		!"yte first).\n\n   Valid arguments to the --seek options look like:\n     --seek \"mm:SS:ss;mm:SS:ss;mm:SS:ss\"\n     Where the first \"mm:SS:ss\" is t" & _
		!"he position, in minutes,\n     seconds and milliseconds to seek to at start of playback. The\n     next mm:SS:ss is how long to play audio from that p" & _
		!"oint.\n     The third mm:SS:ss is another seek after the duration of\n     playback has completed. If the final playback duration is\n     omitted, pl" & _
		!"ayback continues until the end of the file.\n     The \"mm\" and \"SS\" portions may be omitted. --loop\n     and --seek can coexist.\n\n")))
end sub

'' output_usage
sub output_credits cdecl()
	fprintf(stdout, strptr(!"playsound version %d.%d.%d\nCopyright 2001-2026 Ryan C. Gordon and others.\n\n    Written by Ryan C. Gordon, Torbj\&o303\&o266rn Andersson, Max Horn,\n     Tsuyoshi Iguchi, Tyler Montbriand, Darrell Walisser,\n     and a cast of thousands.\n\n    Website and source code: https://icculus.org/SDL_sound/\n\n"), cast(long, 3), cast(long, 2), cast(long, 0))
end sub

function cvtMsToBytePos cdecl(byval info as SDL_AudioSpec ptr, byval ms as Uint32) as Uint32
	'' "frames" == "sample frames"
	dim frames_per_ms as single = ((cast(single, info->freq)) / 1000.0f)
	dim frame_offset as Uint32 = cast(Uint32, ((frames_per_ms * (cast(single, ms)))))
	dim frame_size as Uint32 = (cast(Uint32, ((((info->format and 255)) \ 8))) * info->channels)
	return ((frame_offset * frame_size))
end function

'' cvtMsToBytePos
sub do_seek cdecl(byval sample as Sound_Sample ptr)
	dim seek_list as Uint32 ptr = global_state.seek_list
	dim seek_index as Uint32 = global_state.seek_index
	dim total_seeks as Uint32 = global_state.total_seeks
	fprintf(stdout, strptr(!"Seeking to %.2d:%.2d:%.4d...\n"), cast(long, ((((seek_list[seek_index] / 1000)) / 60))), cast(long, ((((seek_list[seek_index] / 1000)) mod 60))), cast(long, (((seek_list[seek_index] mod 1000)))))
	if global_state.predecode then
		dim pos_ as Uint32 = cvtMsToBytePos(@(sample->desired), seek_list[seek_index])
		if (pos_ > sample->buffer_size) then
			fprintf(stderr, strptr(!"Seek past end of predecoded buffer.\n"))
			done_flag = 1
		else
			global_state.decoded_ptr = (((cptr(Uint8 ptr, sample->buffer)) + pos_))
			global_state.decoded_bytes = (sample->buffer_size - pos_)
		end if
	else
		if (Sound_Seek(sample, seek_list[seek_index]) = 0) then
			fprintf(stderr, strptr(!"Sound_Seek() failed: %s\n"), Sound_GetError())
			done_flag = 1
		end if
	end if
	'' else
	seek_index += 1
	if (seek_index >= total_seeks) then
		global_state.bytes_before_next_seek = (-1)
	else
		global_state.bytes_before_next_seek = cvtMsToBytePos(@(sample->desired), seek_list[seek_index])
		seek_index += 1
	end if
	'' else
	global_state.seek_index = seek_index
end sub

'' do_seek
'' This updates (decoded_bytes) and (decoded_ptr) with more audio data,
'' taking into account potential looping, seeking and predecoding.
function read_more_data cdecl(byval sample as Sound_Sample ptr) as long
	if done_flag then
		global_state.decoded_bytes = 0
		return (0)
	end if
	'' if
	if (((global_state.bytes_before_next_seek >= 0)) andalso ((global_state.decoded_bytes > cast(Uint32, global_state.bytes_before_next_seek)))) then
		global_state.decoded_bytes = global_state.bytes_before_next_seek
	end if
	'' if
	if (global_state.decoded_bytes > 0) then
		'' don't need more data; just return.
		return (global_state.decoded_bytes)
	end if
	'' Need more audio data. See if we're supposed to seek...
	if (((global_state.bytes_before_next_seek = 0)) andalso ((global_state.seek_index < global_state.total_seeks))) then
		do_seek(sample)
		'' do it, baby!
		return (read_more_data(sample))
	end if
	'' if
	'' See if there's more to be read...
	if (((global_state.bytes_before_next_seek <> 0)) andalso ((((sample->flags and ((SOUND_SAMPLEFLAG_ERROR or SOUND_SAMPLEFLAG_EOF)))) = 0))) then
		global_state.decoded_bytes = Sound_Decode(sample)
		if (sample->flags and SOUND_SAMPLEFLAG_ERROR) then
			fprintf(stderr, strptr(!"Error in decoding sound file!\n  reason: [%s].\n"), Sound_GetError())
		end if
		'' if
		global_state.decoded_ptr = cptr(Uint8 ptr, sample->buffer)
		return (read_more_data(sample))
	end if
	'' if
	'' No more to be read from stream, but we may want to loop the sample.
	if (global_state.looping = 0) then
		return (0)
	end if
	global_state.looping -= 1
	global_state.seek_index = 0
	global_state.bytes_before_next_seek = iif(((global_state.total_seeks > 0)), 0, (-1))
	'' we just need to point predecoded samples to the start of the buffer.
	if global_state.predecode then
		global_state.decoded_bytes = sample->buffer_size
		global_state.decoded_ptr = cptr(Uint8 ptr, sample->buffer)
	else
		Sound_Rewind(sample)
	end if
	'' else
	return (read_more_data(sample))
end function

'' read_more_data
sub memcpy_with_volume cdecl(byval sample as Sound_Sample ptr, byval dst as Uint8 ptr, byval src as Uint8 ptr, byval len_ as long)
	dim i as long
	dim s16src as Sint16 ptr = cptr(Sint16 ptr, 0)
	dim s16dst as Sint16 ptr = cptr(Sint16 ptr, 0)
	dim s32src as Sint32 ptr = cptr(Sint32 ptr, 0)
	dim s32dst as Sint32 ptr = cptr(Sint32 ptr, 0)
	union FloatswapperRecord
		f as single
		ui32 as Uint32
	end union

	dim floatswapper as FloatswapperRecord
	dim f32dst as single ptr = cptr(single ptr, 0)
	dim f32src as single ptr = cptr(single ptr, 0)
	dim volume_level as single = global_state.volume_level
	if (global_state.wants_volume_change = 0) then
		SDL_memcpy(cptr(any ptr, dst), cptr(const any ptr, src), len_)
		exit sub
	end if
	'' if
	'' !!! FIXME: This would be more efficient with a lookup table.
	select case sample->desired.format
		case (SDL_AUDIO_U8)
			goto switch_case_6
		case (SDL_AUDIO_S8)
			goto switch_case_7
		case (SDL_AUDIO_S16LE)
			goto switch_case_8
		case (SDL_AUDIO_S16BE)
			goto switch_case_9
		case (SDL_AUDIO_S32LE)
			goto switch_case_10
		case (SDL_AUDIO_S32BE)
			goto switch_case_11
		case (SDL_AUDIO_F32LE)
			goto switch_case_12
		case (SDL_AUDIO_F32BE)
			goto switch_case_13
		case else
			goto switch_case_14
	end select
	switch_case_6:
	scope
		scope
			i = 0
			do while (i < len_)
				(*dst) = cast(Uint8, (((cast(single, ((*src)))) * volume_level)))
				i += 1
				src += 1
				dst += 1
			loop
		end scope
		goto switch_done_15
	end scope
	switch_case_7:
	scope
		scope
			i = 0
			do while (i < len_)
				(*dst) = cast(Sint8, (((cast(single, ((*src)))) * volume_level)))
				i += 1
				src += 1
				dst += 1
			loop
		end scope
		goto switch_done_15
	end scope
	switch_case_8:
	scope
		s16src = cptr(Sint16 ptr, src)
		s16dst = cptr(Sint16 ptr, dst)
		scope
			i = 0
			do while (i < len_)
				(*s16dst) = cast(Sint16, (((cast(single, (((*s16src))))) * volume_level)))
				(*s16dst) = ((*s16dst))
				i += sizeof(Sint16)
				s16src += 1
				s16dst += 1
			loop
		end scope
		'' for
		goto switch_done_15
	end scope
	switch_case_9:
	scope
		s16src = cptr(Sint16 ptr, src)
		s16dst = cptr(Sint16 ptr, dst)
		scope
			i = 0
			do while (i < len_)
				(*s16dst) = cast(Sint16, (((cast(single, (SDL_Swap16((*s16src))))) * volume_level)))
				(*s16dst) = SDL_Swap16((*s16dst))
				i += sizeof(Sint16)
				s16src += 1
				s16dst += 1
			loop
		end scope
		'' for
		goto switch_done_15
	end scope
	switch_case_10:
	scope
		s32src = cptr(Sint32 ptr, src)
		s32dst = cptr(Sint32 ptr, dst)
		scope
			i = 0
			do while (i < len_)
				(*s32dst) = cast(Sint32, (((cast(double, (((*s32src))))) * cast(double, volume_level))))
				(*s32dst) = ((*s32dst))
				i += sizeof(Sint32)
				s32src += 1
				s32dst += 1
			loop
		end scope
		'' for
		goto switch_done_15
	end scope
	switch_case_11:
	scope
		s32src = cptr(Sint32 ptr, src)
		s32dst = cptr(Sint32 ptr, dst)
		scope
			i = 0
			do while (i < len_)
				(*s32dst) = cast(Sint32, (((cast(double, (SDL_Swap32((*s32src))))) * cast(double, volume_level))))
				(*s32dst) = SDL_Swap32((*s32dst))
				i += sizeof(Sint32)
				s32src += 1
				s32dst += 1
			loop
		end scope
		'' for
		goto switch_done_15
	end scope
	switch_case_12:
	scope
		f32src = cptr(single ptr, src)
		f32dst = cptr(single ptr, dst)
		scope
			i = 0
			do while (i < len_)
				floatswapper.f = (*f32src)
				floatswapper.ui32 = (floatswapper.ui32)
				floatswapper.f *= volume_level
				floatswapper.ui32 = (floatswapper.ui32)
				(*f32dst) = floatswapper.f
				i += sizeof(single)
				f32src += 1
				f32dst += 1
			loop
		end scope
		'' for
		goto switch_done_15
	end scope
	switch_case_13:
	scope
		f32src = cptr(single ptr, src)
		f32dst = cptr(single ptr, dst)
		scope
			i = 0
			do while (i < len_)
				floatswapper.f = (*f32src)
				floatswapper.ui32 = SDL_Swap32(floatswapper.ui32)
				floatswapper.f *= volume_level
				floatswapper.ui32 = SDL_Swap32(floatswapper.ui32)
				(*f32dst) = floatswapper.f
				i += sizeof(single)
				f32src += 1
				f32dst += 1
			loop
		end scope
		'' for
		goto switch_done_15
	end scope
	switch_case_14:
	scope
		SDL_memcpy(cptr(any ptr, dst), cptr(const any ptr, src), len_)
		'' oh well.
		goto switch_done_15
	end scope
	switch_done_15:
end sub

'' memcpy_with_volume
sub audio_callback cdecl(byval userdata as any ptr, byval stream as Uint8 ptr, byval len_ as long)
	dim sample as Sound_Sample ptr = cptr(Sound_Sample ptr, userdata)
	dim bw as long = 0
	'' bytes written to stream this time through the callback
	do
		if ((bw < len_)) = 0 then exit do
		dim cpysize as long
		'' bytes to copy on this iteration of the loop.
		if (read_more_data(sample) = 0) then
			'' ...there isn't any more data to read!
			SDL_memset(cptr(any ptr, (stream + bw)), 0, (len_ - bw))
			done_flag = 1
			exit sub
		end if
		'' if
		'' decoded_bytes and decoder_ptr are updated as necessary...
		cpysize = (len_ - bw)
		if (cpysize > cast(Sint32, global_state.decoded_bytes)) then
			cpysize = cast(Sint32, global_state.decoded_bytes)
		end if
		if (cpysize > 0) then
			memcpy_with_volume(sample, (stream + bw), cptr(Uint8 ptr, global_state.decoded_ptr), cpysize)
			bw += cpysize
			global_state.decoded_ptr += cpysize
			global_state.decoded_bytes -= cpysize
			if (global_state.bytes_before_next_seek >= 0) then
				global_state.bytes_before_next_seek -= cpysize
			end if
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

function count_seek_list cdecl(byval list as const zstring ptr) as long
	dim ptr_ as const zstring ptr
	dim retval as long = 0
	scope
		ptr_ = list
		do while (ptr_ <> cptr(const zstring ptr, (cptr(any ptr, 0))))
			retval += 1
			ptr_ = SDL_strchr((ptr_ + 1), 59)
		loop
	end scope
	return (retval)
end function

'' count_seek_list
function parse_time_str cdecl(byval str_ as zstring ptr) as Uint32
	dim minutes as Uint32 = 0
	dim seconds as Uint32 = 0
	dim ms as Uint32 = 0
	dim ptr_ as zstring ptr = SDL_strchr(str_, 58)
	if (ptr_ <> cptr(zstring ptr, (cptr(any ptr, 0)))) then
		dim ptr2 as zstring ptr
		(*cptr(byte ptr, ptr_)) = 0
		ptr2 = SDL_strchr((ptr_ + 1), 58)
		if (ptr2 <> cptr(zstring ptr, (cptr(any ptr, 0)))) then
			(*cptr(byte ptr, ptr2)) = 0
			minutes = SDL_atoi(str_)
			str_ = (ptr_ + 1)
			ptr_ = ptr2
		end if
		'' if
		seconds = SDL_atoi(str_)
		str_ = (ptr_ + 1)
	end if
	'' if
	ms = SDL_atoi(str_)
	return ((((((((minutes * 60)) + seconds)) * 1000)) + ms))
end function

'' parse_time_str
sub parse_seek_list cdecl(byval _list as const zstring ptr)
	dim i as Uint32
	dim listlen as uinteger = (SDL_strlen(_list) + 1)
	dim list as zstring ptr = cptr(zstring ptr, SDL_malloc(listlen))
	dim save_list as zstring ptr = list
	if (list = cptr(zstring ptr, (cptr(any ptr, 0)))) then
		fprintf(stderr, strptr(!"malloc() failed. Skipping seek list.\n"))
		exit sub
	end if
	'' if
	SDL_strlcpy(list, _list, listlen)
	if (global_state.seek_list <> cptr(Uint32 ptr, (cptr(any ptr, 0)))) then
		SDL_free(cptr(any ptr, global_state.seek_list))
	end if
	global_state.total_seeks = count_seek_list(list)
	global_state.seek_list = cptr(Uint32 ptr, SDL_malloc((global_state.total_seeks * sizeof(Uint32))))
	if (global_state.seek_list = cptr(Uint32 ptr, (cptr(any ptr, 0)))) then
		fprintf(stderr, strptr(!"malloc() failed. Skipping seek list.\n"))
		global_state.total_seeks = 0
		exit sub
	end if
	'' if
	scope
		i = 0
		do while (i < global_state.total_seeks)
			dim ptr_ as zstring ptr = SDL_strchr(list, 59)
			if (ptr_ <> cptr(zstring ptr, (cptr(any ptr, 0)))) then
				(*cptr(byte ptr, ptr_)) = 0
			end if
			global_state.seek_list[i] = parse_time_str(list)
			list = (ptr_ + 1)
			i += 1
		loop
	end scope
	'' for
	global_state.bytes_before_next_seek = 0
	SDL_free(cptr(any ptr, save_list))
end sub

'' parse_seek_list
function str_to_fmt cdecl(byval str_ as zstring ptr) as long
	if (SDL_strcmp(str_, strptr("U8")) = 0) then
		return SDL_AUDIO_U8
	end if
	if (SDL_strcmp(str_, strptr("S8")) = 0) then
		return SDL_AUDIO_S8
	end if
	if (SDL_strcmp(str_, strptr("S16LSB")) = 0) then
		return SDL_AUDIO_S16LE
	end if
	if (SDL_strcmp(str_, strptr("S16MSB")) = 0) then
		return SDL_AUDIO_S16BE
	end if
	if (SDL_strcmp(str_, strptr("S32LSB")) = 0) then
		return SDL_AUDIO_S32LE
	end if
	if (SDL_strcmp(str_, strptr("S32MSB")) = 0) then
		return SDL_AUDIO_S32BE
	end if
	if (SDL_strcmp(str_, strptr("F32LSB")) = 0) then
		return SDL_AUDIO_F32LE
	end if
	if (SDL_strcmp(str_, strptr("F32MSB")) = 0) then
		return SDL_AUDIO_F32BE
	end if
	return 0
end function

'' str_to_fmt
function valid_cmdline cdecl(byval argc as long, byval argv as zstring ptr ptr) as long
	dim i as long
	if (argc < 2) then
		output_usage(argv[0])
		return (0)
	end if
	'' if
	'' Make sure all command line options are valid.
	scope
		i = 1
		do while (i < argc)
			dim opts as const zstring ptr ptr = @option_list(0)
			if (SDL_strncmp(argv[i], strptr("--"), 2) <> 0) then
				'' not an option; skip it.
				goto loop_continue_27
			end if
			do
				if (((*opts) <> cptr(const zstring ptr, (cptr(any ptr, 0))))) = 0 then exit do
				dim expression_value_29 as const zstring ptr ptr = opts
				opts += 1
				if (SDL_strcmp(argv[i], (*(expression_value_29))) = 0) then
					exit do
				end if
				opts += 1
			loop
			'' else
			if ((*opts) = cptr(const zstring ptr, (cptr(any ptr, 0)))) then
				fprintf(stderr, strptr(!"unknown option: \"%s\"\n"), argv[i])
				return (0)
			end if
			loop_continue_27:
			i += 1
		loop
	end scope
	'' for
	return (1)
end function

'' valid_cmdline
sub report_filename cdecl(byval filename as const zstring ptr)
	dim icon as const zstring ptr = strptr("playsound")
	dim ptr_ as zstring ptr = cptr(zstring ptr, 0)
	ptr_ = SDL_strrchr(filename, 47)
	if (ptr_ <> cptr(zstring ptr, (cptr(any ptr, 0)))) then
		filename = (ptr_ + 1)
	end if
	ptr_ = SDL_strrchr(filename, 92)
	if (ptr_ <> cptr(zstring ptr, (cptr(any ptr, 0)))) then
		filename = (ptr_ + 1)
	end if
	'' SDL2's PulseAudio backend picks up these hints.
	SDL_SetHint(strptr("SDL_AUDIO_DEVICE_APP_NAME"), icon)
	SDL_SetHint(strptr("SDL_AUDIO_DEVICE_STREAM_NAME"), filename)
	fprintf(stdout, strptr(!"%s: Now playing [%s]...\n"), icon, filename)
	fflush(stdout)
end sub

'' report_filename
function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long
	dim sound_desired as SDL_AudioSpec
	dim sdl_desired as SDL_AudioSpec
	dim audio_buffersize as Uint32 = 4096
	dim decode_buffersize as Uint32 = 16384
	dim sample as Sound_Sample ptr
	dim use_specific_audiofmt as long = 0
	dim i as long
	dim delay as long
	dim new_sample as long = 1
	dim window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
	dim renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
	dim sdl_init_flags as Uint32 = 16u
	if (valid_cmdline(argc, argv) = 0) then
		return (42)
	end if
	'' Handle some command lines upfront.
	scope
		i = 1
		do while (i < argc)
			if (SDL_strncmp(argv[i], strptr("--"), 2) <> 0) then
				goto loop_continue_30
			end if
			if (SDL_strcmp(argv[i], strptr("--version")) = 0) then
				output_versions(argv[0])
				return (42)
			end if
			'' if
			if (SDL_strcmp(argv[i], strptr("--credits")) = 0) then
				output_credits()
				return (42)
			else
				'' if
				if (SDL_strcmp(argv[i], strptr("--help")) = 0) then
					output_usage(argv[0])
					return (42)
				else
					'' if
					if (SDL_strcmp(argv[i], strptr("--decoders")) = 0) then
						if (Sound_Init() = 0) then
							fprintf(stderr, strptr(!"Sound_Init() failed!\n  reason: [%s].\n"), Sound_GetError())
							SDL_Quit()
							return (42)
						end if
						'' if
						output_decoders()
						Sound_Quit()
						return (0)
					else
						'' if
						if (SDL_strcmp(argv[i], strptr("--window")) = 0) then
							sdl_init_flags or= 32u
						end if
					end if
				end if
			end if
			loop_continue_30:
			i += 1
		loop
	end scope
	'' for
	if (SDL_Init(sdl_init_flags) = 0) then
		fprintf(stderr, strptr(!"SDL_Init() failed!\n  reason: [%s].\n"), SDL_GetError())
		return (42)
	end if
	'' if
	if (Sound_Init() = 0) then
		fprintf(stderr, strptr(!"Sound_Init() failed!\n  reason: [%s].\n"), Sound_GetError())
		SDL_Quit()
		return (42)
	end if
	if (sdl_init_flags and 32u) then
		window_ = SDL_CreateWindow(strptr("playsound"), 320, 240, 0)
		if (window_ = cptr(SDL_Window ptr, (cptr(any ptr, 0)))) then
			fprintf(stderr, strptr(!"SDL_CreateWindow() failed!\n  reason: [%s].\nGoing on without a window.\n"), SDL_GetError())
		else
			'' some video targets need renderers or they won't work. Make one just in case.
			renderer = SDL_CreateRenderer(window_, cptr(const zstring ptr, 0))
			if (renderer = 0) then
				fprintf(stderr, strptr(!"SDL_CreateRenderer() failed!\n  reason: [%s].\nGoing on without a renderer.\n"), SDL_GetError())
			end if
		end if
	end if
	SDL_memset(cptr(any ptr, @(sound_desired)), 0, sizeof(SDL_AudioSpec))
	scope
		i = 1
		do while (i < argc)
			dim filename as const zstring ptr = cptr(const zstring ptr, 0)
			dim stream as SDL_AudioStream ptr
			'' set variables back to defaults for next file...
			if new_sample then
				if (global_state.seek_list <> cptr(Uint32 ptr, (cptr(any ptr, 0)))) then
					SDL_free(cptr(any ptr, global_state.seek_list))
				end if
				SDL_memset(cptr(any ptr, @(global_state)), 0, sizeof((global_state)))
				SDL_memset(cptr(any ptr, @(sdl_desired)), 0, sizeof(SDL_AudioSpec))
				global_state.volume_level = cast(single, 1.0)
				global_state.bytes_before_next_seek = (-1)
				audio_buffersize = 4096
				decode_buffersize = 16384
				new_sample = 0
			end if
			'' if
			if ((SDL_strcmp(argv[i], strptr("--rate")) = 0) andalso (argc > (i + 1))) then
				dim r as Sint32
				use_specific_audiofmt = 1
				i += 1
				r = SDL_atoi(argv[i])
				if (r <= 0) then
					fprintf(stderr, strptr(!"Bad argument to --rate!\n"))
					return (42)
				end if
				'' if
				sound_desired.freq = cast(Uint32, r)
			else
				'' else if
				if ((SDL_strcmp(argv[i], strptr("--format")) = 0) andalso (argc > (i + 1))) then
					use_specific_audiofmt = 1
					i += 1
					sound_desired.format = str_to_fmt(argv[i])
					if (sound_desired.format = 0) then
						fprintf(stderr, strptr(!"Bad argument to --format! Try one of:\nU8, S8, S16LSB, S16MSB, S32LSB, S32MSB, F32LSB, L32MSB\n"))
						return (42)
					end if
				else
					'' else if
					if ((SDL_strcmp(argv[i], strptr("--channels")) = 0) andalso (argc > (i + 1))) then
						use_specific_audiofmt = 1
						i += 1
						sound_desired.channels = SDL_atoi(argv[i])
						if ((sound_desired.channels < 1) orelse (sound_desired.channels > 2)) then
							fprintf(stderr, strptr(!"Bad argument to --channels! Try 1 (mono) or 2 (stereo).\n"))
							return (42)
						end if
					else
						'' else if
						if ((SDL_strcmp(argv[i], strptr("--audiobuf")) = 0) andalso (argc > (i + 1))) then
							i += 1
							audio_buffersize = SDL_atoi(argv[i])
						else
							'' else if
							if ((SDL_strcmp(argv[i], strptr("--decodebuf")) = 0) andalso (argc > (i + 1))) then
								i += 1
								decode_buffersize = SDL_atoi(argv[i])
							else
								'' else if
								if ((SDL_strcmp(argv[i], strptr("--volume_level")) = 0) andalso (argc > (i + 1))) then
									i += 1
									global_state.volume_level = cast(single, SDL_atof(argv[i]))
									if (global_state.volume_level <> 1.0f) then
										global_state.wants_volume_change = 1
									end if
								else
									'' else if
									if (SDL_strcmp(argv[i], strptr("--predecode")) = 0) then
										global_state.predecode = 1
									else
										'' else if
										if ((SDL_strcmp(argv[i], strptr("--loop")) = 0) andalso (argc > (i + 1))) then
											i += 1
											global_state.looping = SDL_atoi(argv[i])
										else
											'' else if
											if (SDL_strcmp(argv[i], strptr("--seek")) = 0) then
												i += 1
												parse_seek_list(argv[i])
											else
												'' else if
												if (SDL_strncmp(argv[i], strptr("--"), 2) = 0) then
												else
													filename = argv[i]
													sample = Sound_NewSampleFromFile(filename, iif(use_specific_audiofmt, @(sound_desired), cptr(SDL_AudioSpec ptr, 0)), decode_buffersize)
												end if
											end if
										end if
									end if
								end if
							end if
						end if
					end if
				end if
			end if
			'' else
			if (filename = cptr(const zstring ptr, (cptr(any ptr, 0)))) then
				'' still parsing command line stuff?
				goto loop_continue_31
			end if
			new_sample = 1
			if (sample = cptr(Sound_Sample ptr, (cptr(any ptr, 0)))) then
				fprintf(stderr, strptr(!"Couldn't load \"%s\"!\n  reason: [%s].\n"), filename, Sound_GetError())
				goto loop_continue_31
			end if
			'' if
			if (global_state.total_seeks > 0) then
				if (((global_state.predecode = 0)) andalso ((((sample->flags and SOUND_SAMPLEFLAG_CANSEEK)) = 0))) then
					fprintf(stderr, strptr(!"Want seeks, but sample cannot handle it!\n"))
					Sound_FreeSample(sample)
					goto loop_continue_31
				end if
			end if
			'' if
			'' Unless explicitly specified, pick the format from the sound
			'' to be played.
			if use_specific_audiofmt then
				sdl_desired.freq = sample->desired.freq
				sdl_desired.format = sample->desired.format
				sdl_desired.channels = sample->desired.channels
			else
				sdl_desired.freq = sample->actual.freq
				sdl_desired.format = sample->actual.format
				sdl_desired.channels = sample->actual.channels
			end if
			'' else
			'' grr, SDL_CloseAudio() calls SDL_QuitSubSystem internally.
			if (SDL_WasInit(16u) = 0) then
				if (SDL_Init(16u) = 0) then
					fprintf(stderr, strptr(!"SDL_Init() failed!\n  reason: [%s].\n"), SDL_GetError())
					Sound_Quit()
					SDL_Quit()
					return (42)
				end if
			end if
			'' if
			report_filename(filename)
			stream = SDL_OpenAudioDeviceStream((cast(SDL_AudioDeviceID, 4294967295u)), @(sdl_desired), @sdl3_audio_callback, cptr(any ptr, sample))
			if (stream = cptr(SDL_AudioStream ptr, (cptr(any ptr, 0)))) then
				fprintf(stderr, strptr(!"Couldn't open audio device!\n  reason: [%s].\n"), SDL_GetError())
				Sound_Quit()
				SDL_Quit()
				return (42)
			end if
			'' if
			if global_state.predecode then
				fprintf(stdout, strptr("  predecoding..."))
				fflush(stdout)
				global_state.decoded_bytes = Sound_DecodeAll(sample)
				global_state.decoded_ptr = cptr(Uint8 ptr, sample->buffer)
				if (sample->flags and SOUND_SAMPLEFLAG_ERROR) then
					'' Uint32 has the same width on all targets; C long does not.
					fprintf(stderr, strptr(!"Couldn't fully decode \"%s\"!\n  reason: [%s].\n  (playing first %u bytes of decoded data...)\n"), filename, Sound_GetError(), global_state.decoded_bytes)
				else
					fprintf(stdout, strptr(!"done.\n"))
				end if
				'' else
				fflush(stdout)
			end if
			'' if
			SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(stream))
			done_flag = 0
			'' the audio callback will flip this flag.
			do
				if ((done_flag = 0)) = 0 then exit do
				if window_ then
					dim event as SDL_Event
					SDL_PollEvent(@(event))
					if ((event.type = SDL_EVENT_KEY_DOWN) orelse (event.type = SDL_EVENT_QUIT)) then
						done_flag = 1
					end if
					'' some video targets need renderers or they won't work. Just clear and present here.
					if renderer then
						SDL_RenderClear(renderer)
						SDL_RenderPresent(renderer)
						SDL3_ExampleSmokeFrame()
					end if
				end if
				SDL_Delay(10)
			loop
			'' while
			SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(stream))
			'' Sleep two buffers' worth of audio before closing, in order
			'' to allow the playback to finish. This isn't always enough;
			'' perhaps SDL needs a way to explicitly wait for device drain?
			delay = (((2 * 1000) * audio_buffersize) / sdl_desired.freq)
			SDL_Delay(delay)
			SDL_CloseAudioDevice(SDL_GetAudioStreamDevice(stream))
			'' reopen with next sample's format if possible
			Sound_FreeSample(sample)
			if (done_flag < 0) then
				exit do
			end if
			loop_continue_31:
			i += 1
		loop
	end scope
	'' for
	SDL_DestroyRenderer(renderer)
	SDL_DestroyWindow(window_)
	Sound_Quit()
	SDL_Quit()
	return (iif(((done_flag < 0)), 1, 0))
end function

end SDL_RunApp(__FB_ARGC__, __FB_ARGV__, @example_main, 0)

'' end of playsound.bas
