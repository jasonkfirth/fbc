''
''
'' FreeBASIC FreeBSD CRT types
'' File: crt/sys/freebsd/types.bi
'' Purpose: Describe the native types used by FreeBSD CRT declarations.
'' Responsibilities: Preserve process, user, group, and file-offset widths.
'' This file intentionally does NOT contain function declarations.
''
''
''
''
''
#ifndef __crt_sys_freebsd_types_bi__
#define __crt_sys_freebsd_types_bi__

#include once "crt/stddef.bi"

type __clock_t as integer
type __time_t as integer

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

union __mstate_t
	as ubyte __mbstate8(0 to 127)
	as ulongint _mbstateL
end union

#endif

'' end of crt/sys/freebsd/types.bi
