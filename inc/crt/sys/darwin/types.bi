''
''
'' sys\types -- Darwin CRT type declarations
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __crt_sys_darwin_types_bi__
#define __crt_sys_darwin_types_bi__

#include once "crt/stddef.bi"
#include once "crt/long.bi"

'' Darwin's machine/_types.h uses these exact-width types on both Intel and
'' ARM.  Keep the internal names available because the other CRT bindings use
'' them when spelling system structures and function prototypes.
type __u_char as ubyte
type __u_short as ushort
type __u_int as ulong
type __u_long as culong
type __int8_t as byte
type __uint8_t as ubyte
type __int16_t as short
type __uint16_t as ushort
type __int32_t as long
type __uint32_t as ulong
type __int64_t as longint
type __uint64_t as ulongint

type __quad_t as longint
type __u_quad_t as ulongint

'' These definitions follow sys/_types.h and the individual sys/_types
'' headers in the macOS SDK.  Several intentionally differ from Linux even
'' when their total storage happens to be the same on a 64-bit target.
type __blkcnt_t as longint
type __blksize_t as long
type __dev_t as long
type __fsblkcnt_t as ulong
type __fsfilcnt_t as ulong
type __gid_t as ulong
type __id_t as ulong
type __ino_t as ulongint
type __mode_t as ushort
type __nlink_t as ushort
type __off_t as longint
type __pid_t as long
type __uid_t as ulong
type __useconds_t as ulong
type __suseconds_t as long
type __clock_t as culong
type __time_t as clong
type __ssize_t as clong
type __socklen_t as ulong
type __sa_family_t as ubyte
type __key_t as long
type __daddr_t as long
type __caddr_t as byte ptr
type __intptr_t as clong
#ifndef __sigset_t
type __sigset_t as ulong
#endif
type __wchar_t as long
type __wint_t as long

'' Darwin deliberately makes mbstate_t opaque and reserves 128 bytes.  The
'' 64-bit member supplies the alignment required by the SDK definition.
union __mbstate_t
	as ubyte __mbstate8(0 to 127)
	as longint __mbstateL
end union

type blkcnt_t as __blkcnt_t
type blksize_t as __blksize_t
type dev_t as __dev_t
type fsblkcnt_t as __fsblkcnt_t
type fsfilcnt_t as __fsfilcnt_t
type gid_t as __gid_t
type id_t as __id_t
type ino_t as __ino_t
type mode_t as __mode_t
type nlink_t as __nlink_t
type off_t as __off_t
type pid_t as __pid_t
type uid_t as __uid_t
type useconds_t as __useconds_t
type suseconds_t as __suseconds_t
type key_t as __key_t
type caddr_t as __caddr_t

#ifndef ssize_t
type ssize_t as __ssize_t
#endif

#ifndef socklen_t
type socklen_t as __socklen_t
#endif

#ifndef intptr_t
type intptr_t as __intptr_t
#endif

#ifndef int8_t
type int8_t as byte
type int16_t as short
type int32_t as long
type int64_t as longint
#endif

type u_int8_t as ubyte
type u_int16_t as ushort
type u_int32_t as ulong
type u_int64_t as ulongint
type register_t as longint

#endif

'' end of crt/sys/darwin/types.bi
