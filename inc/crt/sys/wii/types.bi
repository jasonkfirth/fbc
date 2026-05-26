''
'' sys/types -- Wii/newlib CRT type declarations
''
'' devkitPPC uses newlib.  These aliases mirror the public typedefs from
'' newlib's sys/types.h closely enough for FreeBASIC programs and tests to
'' compile against the Wii C runtime without depending on C header parsing.
''
#ifndef __crt_sys_wii_types_bi__
#define __crt_sys_wii_types_bi__

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

type __dev_t as short
type __uid_t as ushort
type __gid_t as ushort
type __ino_t as ushort
type __mode_t as ulong
type __nlink_t as ushort
type __off_t as clong
type __off64_t as longint
type __loff_t as __off64_t
type __pid_t as long
type __id_t as ulong
type __key_t as clong
type __clock_t as culong
type __time_t as longint
type __clockid_t as culong
type __timer_t as culong
type __blksize_t as clong
type __blkcnt_t as clong
type __fsblkcnt_t as ulongint
type __fsfilcnt_t as ulong
type __ssize_t as integer
type __useconds_t as culong
type __suseconds_t as clong
type __socklen_t as ulong

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
type loff_t as __loff_t

#ifndef ssize_t
type ssize_t as __ssize_t
#endif

#ifndef __crt_wii_time_types_defined
#define __crt_wii_time_types_defined
type clock_t as __clock_t
type time_t as __time_t
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

#endif

'' end of crt/sys/wii/types.bi
