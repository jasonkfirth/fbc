''
'' Project: FreeBASIC FreeBSD CRT bindings
'' File: crt/sys/freebsd/select.bi
'' Purpose: Implement the FreeBSD fd_set helper macros used by select().
'' Responsibilities: Set, clear, test, and zero native descriptor bitmaps.
'' This file intentionally does NOT contain: fd_set layout or libc prototypes.
''

#ifndef __crt_sys_freebsd_select_bi__
#define __crt_sys_freebsd_select_bi__

#include once "crt/bits/sigset.bi"

#macro __FD_ZERO(set)
	scope
		dim as fd_set ptr __arr = (set)
		for __i as integer = 0 to (sizeof(fd_set) \ sizeof(__fd_mask))-1
			__FDS_BITS(__arr)(__i) = 0
		next
	end scope
#endmacro

#define __FD_SET(d, set) __FDS_BITS(set) __FDELT(d) or= __FDMASK(d)
#define __FD_CLR(d, set) __FDS_BITS(set) __FDELT(d) and= not __FDMASK(d)
#define __FD_ISSET(d, set) ((__FDS_BITS(set) __FDELT(d) and __FDMASK(d)) <> 0)

#endif

'' end of crt/sys/freebsd/select.bi
