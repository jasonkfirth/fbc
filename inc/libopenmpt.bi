' Project: FreeBASIC binding for libopenmpt
' File: libopenmpt.bi
' Purpose:
'   Declare the stable C playback API exposed by libopenmpt.
' Responsibilities:
'   - Expose module creation, playback, duration, and cleanup functions.
'   - Keep C ABI types and calling conventions explicit for FreeBASIC callers.
' This file intentionally does not contain:
'   - The libopenmpt C++ API or its implementation details.
'   - File-system callbacks or convenience wrappers for a particular player.
' Based on the libopenmpt 0.8.4 public C headers. The C API and ABI are stable.
' Upstream license: BSD 3-Clause.

#pragma once

#inclib "openmpt"

#include once "crt/stddef.bi"
#include once "crt/stdint.bi"

extern "C"

' Opaque module handle. Its storage is owned by libopenmpt.
type openmpt_module as _openmpt_module

' Initial controls are a NULL/NULL-terminated array when supplied.
type openmpt_module_initial_ctl
	ctl as const zstring ptr
	value as const zstring ptr
end type

' Optional C callbacks used while constructing a module.
type openmpt_log_func as sub cdecl(byval message as const zstring ptr, byval user as any ptr)
type openmpt_error_func as function cdecl(byval error as long, byval user as any ptr) as long

' The version number is encoded as (major << 24) | (minor << 16) | patch.
declare function openmpt_get_library_version() as uint32_t

' The input buffer may be released after this function returns successfully.
declare function openmpt_module_create_from_memory2( _
	byval filedata as const any ptr, _
	byval filesize as size_t, _
	byval logfunc as openmpt_log_func, _
	byval loguser as any ptr, _
	byval errfunc as openmpt_error_func, _
	byval erruser as any ptr, _
	byval error as long ptr, _
	byval error_message as const zstring ptr ptr, _
	byval ctls as const openmpt_module_initial_ctl ptr _
) as openmpt_module ptr

declare sub openmpt_module_destroy(byval mod_ as openmpt_module ptr)

' Zero repeats means play once; -1 means repeat forever.
declare function openmpt_module_set_repeat_count( _
	byval mod_ as openmpt_module ptr, _
	byval repeat_count as int32_t _
) as long

' Returns an approximate duration in seconds for the selected song.
declare function openmpt_module_get_duration_seconds(byval mod_ as openmpt_module ptr) as double

' Render count stereo frames into interleaved signed 16-bit PCM.
' A return value of zero indicates the end of the song.
declare function openmpt_module_read_interleaved_stereo( _
	byval mod_ as openmpt_module ptr, _
	byval samplerate as int32_t, _
	byval count as size_t, _
	byval interleaved_stereo as int16_t ptr _
) as size_t

end extern

' end of libopenmpt.bi
