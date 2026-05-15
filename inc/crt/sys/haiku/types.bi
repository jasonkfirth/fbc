''
''
'' sys\types -- Haiku CRT type declarations
''
''
#ifndef __crt_sys_haiku_types_bi__
#define __crt_sys_haiku_types_bi__

#include once "crt/stddef.bi"
#include once "crt/long.bi"

type __clock_t as long
#if defined(__FB_64BIT__) or not defined(__FB_X86__)
type __time_t as longint
#else
type __time_t as long
#endif

type __blkcnt_t as longint
type __blksize_t as long
type __dev_t as long
type __fsblkcnt_t as longint
type __fsfilcnt_t as longint
type __gid_t as ulong
type __id_t as long
type __ino_t as longint
type __mode_t as ulong
type __nlink_t as long
type __off_t as longint
type __pid_t as long
type __uid_t as ulong
type __useconds_t as ulong
type __suseconds_t as long
type __ssize_t as integer
type __clockid_t as long
type __timer_t as any ptr

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
type register_t as integer

#include once "crt/time.bi"
#include once "crt/stddef.bi"

#endif
