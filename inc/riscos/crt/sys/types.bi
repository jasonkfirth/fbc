''
'' FreeBASIC CRT primitive types for GCCSDK UnixLib
'' ------------------------------------------------
''
'' File: crt/sys/types.bi
''
'' Purpose:
''
''     Reproduce the primitive C typedefs used by the UnixLib ABI.
''
'' Responsibilities:
''
''     - map UnixLib internal integer and identifier types to FreeBASIC types
''     - expose the public POSIX aliases used by the shared CRT headers
''     - preserve UnixLib's 32-bit long and pointer data model
''
'' This file intentionally does NOT contain:
''
''     - structures owned by individual C headers
''     - compiler target selection
''     - large-file redirection macros
''
'' ABI note:
''
''     GCCSDK's RISC OS target is a 32-bit ABI.  __ssize_t, __off_t, time_t,
''     and intptr_t are signed 32-bit long values; the quad types are 64-bit.
''

#ifndef __crt_sys_types_bi__
#define __crt_sys_types_bi__

#include once "crt/stddef.bi"
#include once "crt/long.bi"

'' ---------------------------------------------------------------------------
'' UnixLib internal types
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

type __dev_t as long
type __uid_t as ulong
type __gid_t as ulong
type __ino_t as ulong
type __ino64_t as __quad_t
type __mode_t as ulong
type __nlink_t as ushort
type __off_t as clong
type __off64_t as __quad_t
type __pid_t as long
type __ssize_t as clong
type __fsid_t as __u_quad_t
type __clock_t as clong
type __rlim_t as clong
type __rlim64_t as __quad_t
type __id_t as ulong
type __time_t as clong
type __useconds_t as ulong
type __suseconds_t as clong
type __daddr_t as clong
type __key_t as clong
type __swblk_t as clong
type __clockid_t as long
type __timer_t as long
type __tcflag_t as culong
type __cc_t as ubyte
type __speed_t as clong
type __sig_atomic_t as long
type __sigset_t as culong
type __ipc_pid_t as ushort
type __blksize_t as ulong
type __blkcnt_t as clong
type __blkcnt64_t as __quad_t
type __fsblkcnt_t as ulong
type __fsblkcnt64_t as __u_quad_t
type __fsfilcnt_t as culong
type __fsfilcnt64_t as __u_quad_t
type __qaddr_t as __quad_t ptr
type __caddr_t as byte ptr
type __t_scalar_t as long
type __t_uscalar_t as ulong
type __intptr_t as clong
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
type useconds_t as __useconds_t
type suseconds_t as __suseconds_t
type clockid_t as __clockid_t
type timer_t as __timer_t
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
type register_t as long

type blkcnt_t as __blkcnt_t
type blksize_t as __blksize_t
type fsblkcnt_t as __fsblkcnt_t
type fsfilcnt_t as __fsfilcnt_t
type fsid_t as __fsid_t

#endif

'' end of crt/sys/types.bi
