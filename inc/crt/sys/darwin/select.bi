''
''
'' sys\select -- Darwin fd_set helpers
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __crt_sys_darwin_select_bi__
#define __crt_sys_darwin_select_bi__

'' The SDK's inline helpers perform an availability-aware bounds check before
'' touching the bitmap.  FreeBASIC cannot reproduce that weak-link check in a
'' macro, so reject invalid descriptors locally and leave the set unchanged.
#macro __FD_ZERO(set)
	scope
		dim as fd_set ptr __arr = (set)
		for __i as integer = 0 to (sizeof(fd_set) \ sizeof(__fd_mask))-1
			__FDS_BITS(__arr)(__i) = 0
		next
	end scope
#endmacro

#macro __FD_SET(d, set)
	scope
		dim as integer __fd = (d)
		if( __fd >= 0 andalso __fd < FD_SETSIZE ) then
			__FDS_BITS(set)(__FDELT(__fd)) or= __FDMASK(__fd)
		end if
	end scope
#endmacro

#macro __FD_CLR(d, set)
	scope
		dim as integer __fd = (d)
		if( __fd >= 0 andalso __fd < FD_SETSIZE ) then
			__FDS_BITS(set)(__FDELT(__fd)) and= not __FDMASK(__fd)
		end if
	end scope
#endmacro

#endif

'' end of crt/sys/darwin/select.bi
