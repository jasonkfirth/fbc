''
''
'' sys\time -- Darwin CRT declarations
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __crt_sys_darwin_time_bi__
#define __crt_sys_darwin_time_bi__

#include once "crt/sys/types.bi"

'' Unlike LP64 Linux, Darwin keeps suseconds_t as a signed 32-bit integer.
'' The structure still occupies 16 bytes because the target ABI rounds its
'' final size to the alignment required by time_t.
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
declare function getitimer (byval __which as long, byval __value as itimerval ptr) as long
declare function gettimeofday (byval __value as timeval ptr, byval __timezone as any ptr) as long
declare function setitimer (byval __which as long, byval __value as const itimerval ptr, byval __old_value as itimerval ptr) as long
declare function utimes (byval __path as const zstring ptr, byval __times as const timeval ptr) as long
end extern

#endif

'' end of crt/sys/darwin/time.bi
