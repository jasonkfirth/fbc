''
''
'' FreeBASIC FreeBSD CRT types
'' File: crt/sys/freebsd/types.bi
'' Purpose: Describe the native types used by FreeBSD CRT declarations.
'' Responsibilities: Preserve process, time, file-offset, and conversion-state widths.
'' This file intentionally does NOT contain function declarations.
''
''
''
''
''
#ifndef __crt_sys_freebsd_types_bi__
#define __crt_sys_freebsd_types_bi__

#include once "crt/stddef.bi"

#if defined(__FB_64BIT__)
type __clock_t as longint
type __time_t as longint
#else
type __clock_t as ulong
type __time_t as long
#endif
type __clockid_t as long
type __suseconds_t as clong

'' FreeBSD keeps process IDs at 32 bits and file offsets at 64 bits on both
'' x86 ABIs. INTEGER cannot describe either type across both targets.
type __pid_t as long
type __off_t as longint
type __uid_t as ulong
type __gid_t as ulong
type __useconds_t as ulong

type pid_t as __pid_t
type off_t as __off_t
type uid_t as __uid_t
type gid_t as __gid_t
type useconds_t as __useconds_t
type clockid_t as __clockid_t
type suseconds_t as __suseconds_t

'' FreeBSD exposes an opaque 128-byte mbstate_t. Keep its 64-bit alignment.
union __mbstate_t
	as ubyte __mbstate8(0 to 127)
	as ulongint _mbstateL
end union

#endif

'' end of crt/sys/freebsd/types.bi
