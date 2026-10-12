'' Project: FreeBASIC Haiku CRT bindings
'' File: crt/sys/haiku/time.bi
'' Purpose: Declare Haiku timeval structures and system time calls.
'' Responsibilities: Match the Haiku POSIX sys/time.h interfaces.
'' This file intentionally does not declare select or clock functions.

#ifndef __crt_sys_haiku_time_bi__
#define __crt_sys_haiku_time_bi__

#include once "crt/sys/types.bi"

type timeval
	tv_sec as time_t
	tv_usec as suseconds_t
end type

type timezone
	tz_minuteswest as long
	tz_dsttime as long
end type

type itimerval
	it_interval as timeval
	it_value as timeval
end type

#define ITIMER_REAL 1
#define ITIMER_VIRTUAL 2
#define ITIMER_PROF 3

extern "C"
declare function getitimer(byval which as long, byval value as itimerval ptr) as long
declare function setitimer(byval which as long, byval value as const itimerval ptr, byval old_value as itimerval ptr) as long
declare function gettimeofday(byval value as timeval ptr, byval timezone as any ptr) as long
declare function utimes(byval path as const zstring ptr, byval times as timeval ptr) as long
end extern

#endif

'' end of crt/sys/haiku/time.bi
