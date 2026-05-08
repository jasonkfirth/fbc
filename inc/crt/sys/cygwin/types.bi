''
''
'' sys\types -- Cygwin CRT type declarations
''
#ifndef __crt_sys_cygwin_types_bi__
#define __crt_sys_cygwin_types_bi__

#include once "crt/stddef.bi"
#include once "crt/long.bi"

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

type __blkcnt_t as longint
type __blksize_t as long
type __fsblkcnt_t as culong
type __fsfilcnt_t as culong
type __dev_t as ulong
type __uid_t as ulong
type __gid_t as ulong
type __id_t as ulong
type __ino_t as ulongint
type __mode_t as ulong

#ifdef __FB_64BIT__
type __off_t as clong
type __time_t as clong
#else
type __off_t as longint
type __time_t as longint
#endif

type __loff_t as longint
type __pid_t as long
type __key_t as longint
type __clock_t as culong
type __ssize_t as integer
type __clockid_t as culong
type __timer_t as culong
type __sa_family_t as ushort
type __socklen_t as long
type __nlink_t as ushort
type __suseconds_t as clong
type __useconds_t as culong
type __daddr_t as clong
type __caddr_t as byte ptr
type __intptr_t as integer

type loff_t as __loff_t
type ino_t as __ino_t
type dev_t as __dev_t
type gid_t as __gid_t
type mode_t as __mode_t
type nlink_t as __nlink_t
type uid_t as __uid_t
type off_t as __off_t
type pid_t as __pid_t
#ifndef ssize_t
type ssize_t as __ssize_t
#endif

#include once "crt/time.bi"
#include once "crt/stddef.bi"

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
type id_t as __id_t
type key_t as __key_t
type clockid_t as __clockid_t
type timer_t as __timer_t
type useconds_t as __useconds_t
type suseconds_t as __suseconds_t

#endif
