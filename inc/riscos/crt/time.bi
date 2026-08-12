''
'' FreeBASIC CRT declarations for GCCSDK UnixLib
'' ---------------------------------------------
''
'' File: crt/time.bi
''
'' Purpose:
''
''     Define the complete UnixLib clock and calendar interface.
''
'' Responsibilities:
''
''     - define UnixLib clock, calendar, and broken-down time types
''     - declare standard time functions
''     - declare UnixLib reentrant and BSD time extensions
''
'' This file intentionally does NOT contain:
''
''     - RISC OS five-byte time conversions
''     - timezone database management
''     - declarations for other operating systems
''
#ifndef __crt_time_bi__
#define __crt_time_bi__

#include once "crt/long.bi"
#include once "crt/stddef.bi"
#include once "crt/sys/types.bi"

#define CLOCKS_PER_SEC 100l

type clock_t as clong
type time_t as clong

type timespec
	tv_sec as time_t
	tv_nsec as clong
end type

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
	tm_gmtoff as clong
	tm_zone as const zstring ptr
end type

extern "c"
declare function gmtime_r (byval timer as const time_t ptr, byval result as tm ptr) as tm ptr
declare function localtime_r (byval timer as const time_t ptr, byval result as tm ptr) as tm ptr
declare function asctime_r (byval value as const tm ptr, byval buffer as zstring ptr) as zstring ptr
declare function ctime_r (byval timer as const time_t ptr, byval buffer as zstring ptr) as zstring ptr
declare function timegm (byval value as tm ptr) as time_t
declare function timelocal (byval value as tm ptr) as time_t
declare function dysize (byval year as long) as long
end extern
extern "c"
declare function clock () as clock_t
declare function time_ alias "time" (byval as time_t ptr = NULL) as time_t
declare function difftime (byval as time_t, byval as time_t) as double
declare function mktime (byval as tm ptr) as time_t
declare function asctime (byval as tm ptr) as zstring ptr
declare function ctime (byval as time_t ptr) as zstring ptr
declare function gmtime (byval as time_t ptr) as tm ptr
declare function localtime (byval as time_t ptr) as tm ptr
declare function strftime (byval as zstring ptr, byval as size_t, byval as zstring ptr, byval as tm ptr) as size_t
declare function wcsftime (byval as wchar_t ptr, byval as size_t, byval as wchar_t ptr, byval as tm ptr) as size_t
end extern

#endif

'' end of crt/time.bi
