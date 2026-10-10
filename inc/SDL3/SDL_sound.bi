'' Project: FreeBASIC SDL3 bindings
'' File: SDL_sound.bi
'' Purpose: Declare the SDL3_sound-3.2.0 C interface for FreeBASIC.
'' Responsibilities: Preserve public types, constants, and calling conventions.
'' This file intentionally does NOT contain: the upstream library implementation.
''
'' Translated from the upstream headers; this is an altered source version.
'' Regenerate with build_scripts/generate-sdl3-bindings.py.
''
'' Copyright (c) 2001-2026 Ryan C. Gordon <icculus@icculus.org> and others.
''
'' This software is provided 'as-is', without any express or implied
'' warranty.  In no event will the authors be held liable for any damages
'' arising from the use of this software.
''
'' Permission is granted to anyone to use this software for any purpose,
'' including commercial applications, and to alter it and redistribute it
'' freely, subject to the following restrictions:
''
'' 1. The origin of this software must not be misrepresented; you must not
''    claim that you wrote the original software. If you use this software
''    in a product, an acknowledgment in the product documentation would be
''    appreciated but is not required.
'' 2. Altered source versions must be plainly marked as such, and must not be
''    misrepresented as being the original software.
'' 3. This notice may not be removed or altered from any source distribution.

#pragma once

#inclib "SDL3_sound"

#include once "SDL.bi"


extern "C"



#define SDL_SOUND_H_
const SDL_SOUND_MAJOR_VERSION as long = 3
const SDL_SOUND_MINOR_VERSION as long = 2
const SDL_SOUND_MICRO_VERSION as long = 0
#define SDL_SOUND_VERSION SDL_VERSIONNUM(SDL_SOUND_MAJOR_VERSION, SDL_SOUND_MINOR_VERSION, SDL_SOUND_MICRO_VERSION)
#define SDL_SOUND_VERSION_ATLEAST(X, Y, Z) (((SDL_SOUND_MAJOR_VERSION >= X) andalso ((SDL_SOUND_MAJOR_VERSION > X) orelse (SDL_SOUND_MINOR_VERSION >= Y))) andalso (((SDL_SOUND_MAJOR_VERSION > X) orelse (SDL_SOUND_MINOR_VERSION > Y)) orelse (SDL_SOUND_MICRO_VERSION >= Z)))
declare function Sound_Version() as long

type Sound_SampleFlags as ulong
enum
	SOUND_SAMPLEFLAG_NONE = 0
	SOUND_SAMPLEFLAG_CANSEEK = 1
	SOUND_SAMPLEFLAG_EOF = 1u shl 29
	SOUND_SAMPLEFLAG_ERROR = 1u shl 30
	SOUND_SAMPLEFLAG_EAGAIN = 1u shl 31
end enum

type Sound_DecoderInfo
	extensions as const zstring ptr ptr
	description as const zstring ptr
	author as const zstring ptr
	url as const zstring ptr
end type

type Sound_Sample
	opaque as any ptr
	decoder as const Sound_DecoderInfo ptr
	desired as SDL_AudioSpec
	actual as SDL_AudioSpec
	buffer as any ptr
	buffer_size as Uint32
	flags as Sound_SampleFlags
end type

declare function Sound_Init() as long
declare function Sound_Quit() as long
declare function Sound_AvailableDecoders() as const Sound_DecoderInfo ptr ptr
declare function Sound_GetError() as const zstring ptr
declare sub Sound_ClearError()
declare function Sound_NewSample(byval io as SDL_IOStream ptr, byval ext as const zstring ptr, byval desired as const SDL_AudioSpec ptr, byval bufferSize as Uint32) as Sound_Sample ptr
declare function Sound_NewSampleFromMem(byval data as const Uint8 ptr, byval size as Uint32, byval ext as const zstring ptr, byval desired as const SDL_AudioSpec ptr, byval bufferSize as Uint32) as Sound_Sample ptr
declare function Sound_NewSampleFromFile(byval filename as const zstring ptr, byval desired as const SDL_AudioSpec ptr, byval bufferSize as Uint32) as Sound_Sample ptr
declare sub Sound_FreeSample(byval sample as Sound_Sample ptr)
declare function Sound_GetDuration(byval sample as Sound_Sample ptr) as Sint32
declare function Sound_SetBufferSize(byval sample as Sound_Sample ptr, byval new_size as Uint32) as long
declare function Sound_SetDesiredFormat(byval sample as Sound_Sample ptr, byval desired as const SDL_AudioSpec ptr) as long
declare function Sound_Decode(byval sample as Sound_Sample ptr) as Uint32
declare function Sound_DecodeAll(byval sample as Sound_Sample ptr) as Uint32
declare function Sound_Rewind(byval sample as Sound_Sample ptr) as long
declare function Sound_Seek(byval sample as Sound_Sample ptr, byval ms as Uint32) as long

end extern

'' end of SDL_sound.bi
