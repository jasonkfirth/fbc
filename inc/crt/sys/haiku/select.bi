'' Project: FreeBASIC Haiku CRT bindings
'' File: crt/sys/haiku/select.bi
'' Purpose: Provide safe fd_set helpers for Haiku's POSIX select API.
'' Responsibilities: Bound descriptor accesses and clear Haiku fd_set storage.
'' This file intentionally does not define fd_set layout or select prototypes.

#ifndef __crt_sys_haiku_select_bi__
#define __crt_sys_haiku_select_bi__

#include once "crt/bits/sigset.bi"

#macro __FD_ZERO(set)
	scope
		dim as fd_set ptr __array = (set)
		if( __array <> 0 ) then
			for __index as integer = 0 to (sizeof(fd_set) \ sizeof(__fd_mask))-1
				__FDS_BITS(__array)(__index) = 0
			next
		end if
	end scope
#endmacro

#macro __FD_SET(descriptor, set)
	scope
		dim as integer __fd = (descriptor)
		dim as fd_set ptr __array = (set)
		if( __array <> 0 andalso __fd >= 0 andalso __fd < FD_SETSIZE ) then
			__FDS_BITS(__array)(__FDELT(__fd)) or= __FDMASK(__fd)
		end if
	end scope
#endmacro

#macro __FD_CLR(descriptor, set)
	scope
		dim as integer __fd = (descriptor)
		dim as fd_set ptr __array = (set)
		if( __array <> 0 andalso __fd >= 0 andalso __fd < FD_SETSIZE ) then
			__FDS_BITS(__array)(__FDELT(__fd)) and= not __FDMASK(__fd)
		end if
	end scope
#endmacro

#endif

'' end of crt/sys/haiku/select.bi
