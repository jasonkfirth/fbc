''
''
'' sys\types -- OpenBSD CRT types
''
''
#ifndef __crt_sys_openbsd_types_bi__
#define __crt_sys_openbsd_types_bi__

#include once "crt/long.bi"

type __clock_t as integer
type __time_t as integer
type __suseconds_t as integer
type __ssize_t as integer
type __off_t as integer
type __pid_t as long
type __socklen_t as ulong
type __sa_family_t as ubyte
type __wchar_t as long
type __wint_t as long

type __dev_t as ulong
type __ino_t as ulongint
type __mode_t as ulong
type __nlink_t as ulong
type __uid_t as ulong
type __gid_t as ulong
type __blksize_t as long
type __blkcnt_t as longint

union __mbstate_t
	as ubyte __mbstate8(0 to 127)
	as ulongint _mbstateL
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

#ifndef ssize_t
type ssize_t as __ssize_t
#endif

#ifndef socklen_t
type socklen_t as __socklen_t
#endif

type suseconds_t as __suseconds_t

#endif
