''
''
'' sys\time -- DragonFly CRT declarations
''
''
#ifndef __crt_sys_dragonfly_time_bi__
#define __crt_sys_dragonfly_time_bi__

#include once "crt/sys/types.bi"

type timeval
	tv_sec as __time_t
	tv_usec as __suseconds_t
end type

type itimerval
	it_interval as timeval
	it_value as timeval
end type

#include once "crt/sys/select.bi"

extern "c"
declare function getitimer (byval as long, byval as itimerval ptr) as long
declare function gettimeofday (byval tv as timeval ptr, byval tz as any ptr) as long
declare function setitimer (byval as long, byval as itimerval ptr, byval as itimerval ptr) as long
declare function utimes (byval as zstring ptr, byval as timeval ptr) as long
end extern

#endif
