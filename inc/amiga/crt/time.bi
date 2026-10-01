'' FreeBASIC CRT declarations for classic Amiga newlib
'' ---------------------------------------------------
''
'' File: amiga/crt/time.bi
'' Purpose: Describe the pinned SDK's clock and calendar ABI.
'' Responsibilities: Declare 32-bit time types and the nine-field C tm record.
'' This file intentionally does NOT contain Unix clock IDs or time setters.

#ifndef __crt_time_bi__
#define __crt_time_bi__

#include once "crt/stddef.bi"

#define CLOCKS_PER_SEC 1000l
type clock_t as ulong
type time_t as clong

type tm
	tm_sec as long
	tm_min as long
	tm_hour as long
	tm_mday as long
	tm_mon as long
	tm_year as long
	tm_wday as long
	tm_yday as long
	tm_isdst as long
end type

extern "c"
declare function clock () as clock_t
declare function time_ alias "time" (byval timer as time_t ptr = NULL) as time_t
declare function difftime (byval later as time_t, byval earlier as time_t) as double
declare function mktime (byval value as tm ptr) as time_t
declare function asctime (byval value as const tm ptr) as zstring ptr
declare function ctime (byval timer as const time_t ptr) as zstring ptr
declare function gmtime (byval timer as const time_t ptr) as tm ptr
declare function localtime (byval timer as const time_t ptr) as tm ptr
declare function strftime (byval buffer as zstring ptr, byval bytes as size_t, byval format as const zstring ptr, byval value as const tm ptr) as size_t
declare function wcsftime (byval buffer as wchar_t ptr, byval characters as size_t, byval format as const wchar_t ptr, byval value as const tm ptr) as size_t
end extern

#endif

'' end of amiga/crt/time.bi
