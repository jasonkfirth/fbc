''
'' time -- Wii/newlib CRT time declarations
''
'' devkitPPC's newlib time.h uses a 1000 Hz clock and a simple ANSI tm
'' structure on Wii.  The common crt/time.bi wrapper declares the portable
'' time functions after this file defines the platform types.
''
#ifndef __crt_wii_time_bi__
#define __crt_wii_time_bi__

#include once "crt/long.bi"

#define CLOCKS_PER_SEC 1000ul
#define CLK_TCK CLOCKS_PER_SEC

#ifndef __crt_wii_time_types_defined
#define __crt_wii_time_types_defined
type clock_t as culong
type time_t as longint
#endif

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
end type

extern "c"
declare function gmtime_r (byval timer as time_t ptr, byval result as tm ptr) as tm ptr
declare function localtime_r (byval timer as time_t ptr, byval result as tm ptr) as tm ptr
declare function asctime_r (byval tblock as tm ptr, byval buf as zstring ptr) as zstring ptr
declare function ctime_r (byval timer as time_t ptr, byval buf as zstring ptr) as zstring ptr
end extern

#endif

'' end of crt/wii/time.bi
