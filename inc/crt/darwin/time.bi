''
''
'' time -- Darwin CRT declarations
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __crt_darwin_time_bi__
#define __crt_darwin_time_bi__

#include once "crt/long.bi"

'' macOS exposes the POSIX microsecond clock scale in its default SDK mode.
#define CLOCKS_PER_SEC 1000000

type clock_t as __clock_t
type time_t as __time_t

'' Apple declares clockid_t as an enum.  C enums use the C int ABI.
type clockid_t as long

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
	tm_zone as zstring ptr
end type

enum
	CLOCK_REALTIME = 0
	CLOCK_MONOTONIC_RAW = 4
	CLOCK_MONOTONIC_RAW_APPROX = 5
	CLOCK_MONOTONIC = 6
	CLOCK_UPTIME_RAW = 8
	CLOCK_UPTIME_RAW_APPROX = 9
	CLOCK_PROCESS_CPUTIME_ID = 12
	CLOCK_THREAD_CPUTIME_ID = 16
end enum

extern "C"

declare function gmtime_r (byval __timer as const time_t ptr, byval __tp as tm ptr) as tm ptr
declare function localtime_r (byval __timer as const time_t ptr, byval __tp as tm ptr) as tm ptr
declare function asctime_r (byval __tp as const tm ptr, byval __buf as zstring ptr) as zstring ptr
declare function ctime_r (byval __timer as const time_t ptr, byval __buf as zstring ptr) as zstring ptr
declare function timegm (byval __tp as tm ptr) as time_t
declare function timelocal (byval __tp as tm ptr) as time_t
declare function clock_getres (byval __clock_id as clockid_t, byval __res as timespec ptr) as long
declare function clock_gettime (byval __clock_id as clockid_t, byval __tp as timespec ptr) as long
declare function clock_gettime_nsec_np (byval __clock_id as clockid_t) as ulongint
declare function clock_settime (byval __clock_id as clockid_t, byval __tp as const timespec ptr) as long

extern tzname(0 to 1) as zstring ptr
extern daylight as long
extern timezone as clong

end extern

#endif

'' end of crt/darwin/time.bi
