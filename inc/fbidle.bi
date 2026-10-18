'' FreeBASIC runtime: fbidle.bi
'' Purpose: Expose a small cooperative polling loop with bounded waits.
'' Responsibilities: ABI, callback ownership and status constants.
'' Threads, network clients and operating-system background jobs belong elsewhere.
#ifndef __FB_IDLE_BI__
#define __FB_IDLE_BI__
#if __FB_LANG__ <> "fb"
	#error fbidle.bi requires the fb dialect
#endif
#include once "fbservices.bi"
namespace FB
	'' Zero-initialize before first use. Only its owner thread may access this
	'' state. Keep callbacks short; use a worker for blocking network requests.
	type IDLE_STATE
		deadline as double
		previous as double
		interval_ms as ulong
		running as long
		busy as long
		reserved as long
	end type
end namespace
namespace FBC
	extern "rtlib"
		declare function idleInit alias "fb_IdleInit" _
			( byref state as FB.IDLE_STATE, byval interval_ms as ulong ) as long
		'' The callback returns nonzero to continue, zero to stop. It runs on
		'' this thread, at most once per step, after at most maximum_wait_ms.
		declare function idleStep alias "fb_IdleStep" _
			( byref state as FB.IDLE_STATE, _
			  byval callback as function( byval user as any ptr ) as long, _
			  byval user as any ptr = 0, byval maximum_wait_ms as long = 1000 ) as long
		declare function idleStop alias "fb_IdleStop" _
			( byref state as FB.IDLE_STATE ) as long
	end extern
end namespace
#endif
'' end of fbidle.bi
