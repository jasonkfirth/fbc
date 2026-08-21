''
'' FreeBASIC CRT declarations for AROS POSIXC time services
'' --------------------------------------------------------
''
'' File: crt/time.bi
''
'' Purpose:
''
''     Define the AROS clock, calendar, and POSIX time interface.
''
'' Responsibilities:
''
''     - define AROS clock_t, time_t, timespec, and tm with ABI-correct fields
''     - declare ISO C time functions supplied by stdc.library
''     - declare implemented POSIX extensions supplied by posixc.library
''
'' This file intentionally does NOT contain:
''
''     - timezone database management
''     - declarations for other operating systems
''     - architecture-specific baseline flags
''
#ifndef __crt_time_bi__
#define __crt_time_bi__

#include once "crt/long.bi"
#include once "crt/stddef.bi"
#include once "crt/sys/types.bi"

#define CLOCKS_PER_SEC 50l
#define TIME_UTC 1

#define CLOCK_MONOTONIC 0
#define CLOCK_PROCESS_CPUTIME_ID 1
#define CLOCK_REALTIME 2
#define CLOCK_THREAD_CPUTIME_ID 3
#define TIMER_ABSTIME &h01

type clock_t as __clock_t
type time_t as __time_t

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
declare function timespec_get (byval value as timespec ptr, byval base as long) as long

declare function asctime_r (byval value as const tm ptr, byval buffer as zstring ptr) as zstring ptr
declare function ctime_r (byval timer as const time_t ptr, byval buffer as zstring ptr) as zstring ptr
declare function gmtime_r (byval timer as const time_t ptr, byval result as tm ptr) as tm ptr
declare function localtime_r (byval timer as const time_t ptr, byval result as tm ptr) as tm ptr
declare sub tzset ()
declare function clock_gettime (byval clock_id as clockid_t, byval value as timespec ptr) as long
declare function nanosleep (byval requested as const timespec ptr, byval remaining as timespec ptr) as long
declare function strptime (byval source as const zstring ptr, byval format as const zstring ptr, byval value as tm ptr) as zstring ptr
end extern

extern daylight as long
extern timezone as clong
extern tzname(0 to 1) as zstring ptr

#endif

'' end of crt/time.bi
