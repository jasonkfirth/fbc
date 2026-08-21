''
'' FreeBASIC CRT primitive types for AROS POSIXC
'' ---------------------------------------------
''
'' File: crt/sys/types.bi
''
'' Purpose:
''
''     Reproduce the primitive C typedefs used by the AROS POSIXC ABI.
''
'' Responsibilities:
''
''     - map fixed-width and pointer-width AROS types to FreeBASIC types
''     - expose the POSIX aliases consumed by shared CRT declarations
''     - preserve the distinct 32-bit and 64-bit AROS data models
''
'' This file intentionally does NOT contain:
''
''     - structures owned by individual C headers
''     - compiler target selection
''     - m68k instruction or floating-point baseline policy
''
'' ABI note:
''
''     AROS keeps time_t and inode numbers at 32 bits on all three supported
''     ports.  dev_t, off_t, pid_t, ssize_t, and filesystem counters follow
''     the native pointer width.
''

#ifndef __crt_sys_types_bi__
#define __crt_sys_types_bi__

#include once "crt/stddef.bi"
#include once "crt/long.bi"

'' ---------------------------------------------------------------------------
'' AROS internal types
'' ---------------------------------------------------------------------------

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

type __dev_t as uinteger
type __uid_t as ulong
type __gid_t as ulong
type __ino_t as long
type __ino64_t as longint
type __mode_t as ushort
type __nlink_t as ushort
type __off_t as integer
type __off64_t as longint
type __pid_t as integer
type __ssize_t as integer
type __clock_t as culong
type __id_t as ulong
type __time_t as long
type __useconds_t as ulong
type __suseconds_t as long
type __key_t as long
type __clockid_t as long
type __timer_t as long
type __blksize_t as integer
type __blkcnt_t as integer
type __blkcnt64_t as longint
type __fsblkcnt_t as uinteger
type __fsblkcnt64_t as ulongint
type __fsfilcnt_t as uinteger
type __fsfilcnt64_t as ulongint
type __qaddr_t as __quad_t ptr
type __caddr_t as byte ptr
type __intptr_t as integer
type __socklen_t as ulong

'' ---------------------------------------------------------------------------
'' Public POSIX and fixed-width aliases
'' ---------------------------------------------------------------------------

type ino_t as __ino_t
type dev_t as __dev_t
type gid_t as __gid_t
type mode_t as __mode_t
type nlink_t as __nlink_t
type uid_t as __uid_t
type off_t as __off_t
type pid_t as __pid_t
type id_t as __id_t
type key_t as __key_t
type useconds_t as __useconds_t
type suseconds_t as __suseconds_t
type clockid_t as __clockid_t
type timer_t as __timer_t
type socklen_t as __socklen_t
#ifndef ssize_t
type ssize_t as __ssize_t
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

type blkcnt_t as __blkcnt_t
type blksize_t as __blksize_t
type fsblkcnt_t as __fsblkcnt_t
type fsfilcnt_t as __fsfilcnt_t

#endif

'' end of crt/sys/types.bi
