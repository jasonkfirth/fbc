''
''
'' sys\types -- DragonFly CRT types
''
''
#ifndef __crt_sys_dragonfly_types_bi__
#define __crt_sys_dragonfly_types_bi__

#include once "crt/long.bi"
#include once "crt/stddef.bi"

type __int8_t as byte
type __uint8_t as ubyte
type __int16_t as short
type __uint16_t as ushort
type __int32_t as long
type __uint32_t as ulong
type __int64_t as longint
type __uint64_t as ulongint

type __clock_t as culong
type __clockid_t as culong
type __off_t as longint
type __pid_t as long
type __suseconds_t as clong
type __useconds_t as ulong
type __ssize_t as integer
type __time_t as integer
type __timer_t as long
type __socklen_t as ulong
type __sa_family_t as ubyte
type ___wchar_t as long
type __wchar_t as ___wchar_t
type __wint_t as long

type __dev_t as ulong
type __ino_t as ulongint
type __mode_t as ushort
type __nlink_t as ulong
type __uid_t as ulong
type __gid_t as ulong
type __blksize_t as longint
type __blkcnt_t as longint
type __fsblkcnt_t as ulongint
type __fsfilcnt_t as ulongint
type __id_t as longint
type __caddr_t as byte ptr
type __intptr_t as integer
type __register_t as integer

union __mbstate_t
	as ubyte __mbstate8(0 to 127)
	as longint __mbstateL
end union

type off_t as __off_t
type pid_t as __pid_t
type uid_t as __uid_t
type gid_t as __gid_t
type dev_t as __dev_t
type ino_t as __ino_t
type mode_t as __mode_t
type nlink_t as __nlink_t
type blksize_t as __blksize_t
type blkcnt_t as __blkcnt_t
type fsblkcnt_t as __fsblkcnt_t
type fsfilcnt_t as __fsfilcnt_t
type id_t as __id_t
type suseconds_t as __suseconds_t
type useconds_t as __useconds_t
type caddr_t as __caddr_t
type intptr_t as __intptr_t

#ifndef ssize_t
type ssize_t as __ssize_t
#endif

#ifndef socklen_t
type socklen_t as __socklen_t
#endif

type int8_t as byte
type int16_t as short
type int32_t as long
type int64_t as longint
type u_int8_t as ubyte
type u_int16_t as ushort
type u_int32_t as ulong
type u_int64_t as ulongint
type register_t as integer

#endif
