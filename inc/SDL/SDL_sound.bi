'' Project: FreeBASIC SDL addon bindings
'' File: SDL_sound.bi
'' Purpose: Declare the pinned SDL_sound C interface.
'' Responsibilities: Preserve public types, callbacks, and header helpers.
'' This file intentionally does NOT contain: the external library implementation.
'' Translated from upstream; this is an altered source version.
''
'' * \file SDL_sound.h

#pragma once

#inclib "SDL_sound"
#include once "SDL.bi"


extern "C"

#define _INCLUDE_SDL_SOUND_H_
#define SNDDECLSPEC
const SOUND_VER_MAJOR = 1
const SOUND_VER_MINOR = 0
const SOUND_VER_PATCH = 3

type Sound_SampleFlags as ulong
enum
	SOUND_SAMPLEFLAG_NONE = 0
	SOUND_SAMPLEFLAG_CANSEEK = 1
	SOUND_SAMPLEFLAG_EOF = 1 shl 29
	SOUND_SAMPLEFLAG_ERROR = 1 shl 30
	SOUND_SAMPLEFLAG_EAGAIN = 1u shl 31
end enum

type Sound_AudioInfo
	format as Uint16
	channels as Uint8
	rate as Uint32
end type

type Sound_DecoderInfo
	extensions as const zstring ptr ptr
	description as const zstring ptr
	author as const zstring ptr
	url as const zstring ptr
end type

type Sound_Sample
	opaque as any ptr
	decoder as const Sound_DecoderInfo ptr
	desired as Sound_AudioInfo
	actual as Sound_AudioInfo
	buffer as any ptr
	buffer_size as Uint32
	flags as Sound_SampleFlags
end type

type Sound_Version
	major as long
	minor as long
	patch as long
end type

#macro SOUND_VERSION_(x)
	scope
		(x)->major = SOUND_VER_MAJOR
		(x)->minor = SOUND_VER_MINOR
		(x)->patch = SOUND_VER_PATCH
	end scope
#endmacro
declare sub Sound_GetLinkedVersion(byval ver as Sound_Version ptr)
declare function Sound_Init() as long
declare function Sound_Quit() as long
declare function Sound_AvailableDecoders() as const Sound_DecoderInfo ptr ptr
declare function Sound_GetError() as const zstring ptr
declare sub Sound_ClearError()
declare function Sound_NewSample(byval rw as SDL_RWops ptr, byval ext as const zstring ptr, byval desired as Sound_AudioInfo ptr, byval bufferSize as Uint32) as Sound_Sample ptr
declare function Sound_NewSampleFromFile(byval fname as const zstring ptr, byval desired as Sound_AudioInfo ptr, byval bufferSize as Uint32) as Sound_Sample ptr
declare sub Sound_FreeSample(byval sample as Sound_Sample ptr)
declare function Sound_SetBufferSize(byval sample as Sound_Sample ptr, byval new_size as Uint32) as long
declare function Sound_Decode(byval sample as Sound_Sample ptr) as Uint32
declare function Sound_DecodeAll(byval sample as Sound_Sample ptr) as Uint32
declare function Sound_Rewind(byval sample as Sound_Sample ptr) as long
declare function Sound_Seek(byval sample as Sound_Sample ptr, byval ms as Uint32) as long

end extern

'' End of SDL_sound.bi
