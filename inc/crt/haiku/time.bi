''
''
'' time -- Haiku CRT declarations
''
''
#ifndef __crt_haiku_time_bi__
#define __crt_haiku_time_bi__

#include once "crt/long.bi"

#define CLOCKS_PER_SEC 1000000l
#define CLK_TCK CLOCKS_PER_SEC
#define TIME_UTC 1

#define CLOCK_MONOTONIC 0
#define CLOCK_REALTIME -1
#define CLOCK_PROCESS_CPUTIME_ID -2
#define CLOCK_THREAD_CPUTIME_ID -3
#define TIMER_ABSTIME 1

type clock_t as __clock_t
type time_t as __time_t

type timespec
	tv_sec as time_t
	tv_nsec as clong
end type

type itimerspec
	it_interval as timespec
	it_value as timespec
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
	tm_gmtoff as long
	tm_zone as zstring ptr
end type

extern "C"

declare function gmtime_r (byval timer_ as time_t ptr, byval tm_ as tm ptr) as tm ptr
declare function localtime_r (byval timer_ as time_t ptr, byval tm_ as tm ptr) as tm ptr
declare function asctime_r (byval tm_ as tm ptr, byval buffer as zstring ptr) as zstring ptr
declare function ctime_r (byval timer_ as time_t ptr, byval buffer as zstring ptr) as zstring ptr
declare function nanosleep (byval rqtp as timespec ptr, byval rmtp as timespec ptr) as long
declare function clock_getres (byval clock_id as clockid_t, byval resolution as timespec ptr) as long
declare function clock_gettime (byval clock_id as clockid_t, byval time_ as timespec ptr) as long
declare function clock_settime (byval clock_id as clockid_t, byval time_ as const timespec ptr) as long
declare function clock_nanosleep (byval clock_id as clockid_t, byval flags as long, byval time_ as timespec ptr, byval remaining as timespec ptr) as long
declare function clock_getcpuclockid (byval pid as pid_t, byval clock_id as clockid_t ptr) as long
declare function timer_create (byval clock_id as clockid_t, byval event as any ptr, byval timer_id as timer_t ptr) as long
declare function timer_delete (byval timer_id as timer_t) as long
declare function timer_gettime (byval timer_id as timer_t, byval value as itimerspec ptr) as long
declare function timer_settime (byval timer_id as timer_t, byval flags as long, byval value as const itimerspec ptr, byval old_value as itimerspec ptr) as long
declare function timer_getoverrun (byval timer_id as timer_t) as long
declare function timespec_get (byval ts as timespec ptr, byval base as long) as long
declare function stime (byval t as const time_t ptr) as long
declare sub tzset ()

extern tzname as zstring ptr ptr
extern daylight as long
extern timezone as clong

end extern

#endif
