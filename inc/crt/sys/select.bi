''
''
'' sys\select -- header translated with help of SWIG FB wrapper
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __crt_sys_select_bi__
#define __crt_sys_select_bi__

#include once "crt/sys/types.bi"

#if defined(__FB_LINUX__) or defined(__FB_ANDROID__)
#include once "crt/sys/linux/select.bi"
#elseif defined(__FB_NUTTX__)
#include once "crt/sys/nuttx/select.bi"
#elseif defined(__FB_CYGWIN__)
#include once "crt/sys/linux/select.bi"
#elseif defined(__FB_DRAGONFLY__)
#include once "crt/sys/dragonfly/select.bi"
#elseif defined(__FB_OPENBSD__)
#include once "crt/sys/openbsd/select.bi"
#elseif defined(__FB_NETBSD__)
#include once "crt/sys/netbsd/select.bi"
#elseif defined(__FB_DARWIN__)
#include once "crt/sys/darwin/select.bi"
#else
#error Platform unsupported
#endif

type sigset_t as __sigset_t

#include once "crt/sys/time.bi"
#include once "crt/long.bi"

#ifndef suseconds_t
type suseconds_t as __suseconds_t
#endif

#if defined(__FB_NUTTX__)
type fd_set
	arr(0 to __SELECT_NUINT32-1) as uint32_t
end type
#elseif defined(__FB_DARWIN__)
'' Darwin fixes fd_set's storage unit at a signed 32-bit integer, including on
'' LP64 targets where C long and FreeBASIC Integer are 64 bits.
type __fd_mask as long

#define __NFDBITS 32
#define __FDELT(d) ((d) \ __NFDBITS)
#define __FDMASK(d) cast(__fd_mask, culng(1) shl ((d) mod __NFDBITS))
#define __FD_SETSIZE 1024

type fd_set
	fds_bits(0 to (__FD_SETSIZE \ __NFDBITS)-1) as __fd_mask
	#define __FDS_BITS(set) (set)->fds_bits
end type

#define FD_SETSIZE __FD_SETSIZE
type fd_mask as __fd_mask
#define NFDBITS __NFDBITS
#else
type __fd_mask as clong

#define __NFDBITS (8 * len(__fd_mask))
#define	__FDELT(d) ((d) \ __NFDBITS)
#define	__FDMASK(d) cast(__fd_mask, 1 shl ((d) mod __NFDBITS))
#define __FD_SETSIZE 1024

type fd_set
	___fds_bits(0 to (__FD_SETSIZE \ __NFDBITS)-1) as __fd_mask
	#define __FDS_BITS(set) (set)->___fds_bits
end type

#define	FD_SETSIZE __FD_SETSIZE

type fd_mask as __fd_mask
# define NFDBITS __NFDBITS
#endif

#ifdef __FB_DARWIN__
'' Darwin's SDK uses an inline helper so a descriptor expression is evaluated
'' once before the bounds check and bitmap lookup.  Keep the same behavior here
'' instead of expanding the expression several times in a macro.
private function __fb_darwin_fd_isset _
	( _
		byval descriptor as long, _
		byval descriptor_set as const fd_set ptr _
	) as long

	if( descriptor_set = 0 ) then
		return FALSE
	end if

	if( descriptor < 0 orelse descriptor >= FD_SETSIZE ) then
		return FALSE
	end if

	return ( __FDS_BITS(descriptor_set)(__FDELT(descriptor)) and _
		__FDMASK(descriptor) ) <> 0
end function

#define __FD_ISSET(d, set) __fb_darwin_fd_isset((d), (set))
#endif

#define	FD_SET_(fd, fdsetp) __FD_SET(fd, fdsetp)
#define	FD_CLR(fd, fdsetp) __FD_CLR(fd, fdsetp)
#define	FD_ISSET(fd, fdsetp) __FD_ISSET(fd, fdsetp)
#define	FD_ZERO(fdsetp) __FD_ZERO(fdsetp)

extern "C"
declare function select_ alias "select" (byval __nfds as long, byval __readfds as fd_set ptr, byval __writefds as fd_set ptr, byval __exceptfds as fd_set ptr, byval __timeout as timeval ptr) as long
#if defined(__FB_DARWIN__)
declare function pselect (byval __nfds as long, byval __readfds as fd_set ptr, byval __writefds as fd_set ptr, byval __exceptfds as fd_set ptr, byval __timeout as const timespec ptr, byval __sigmask as const sigset_t ptr) as long
#endif
end extern

#define selectsocket select_

#endif

'' end of crt/sys/select.bi
