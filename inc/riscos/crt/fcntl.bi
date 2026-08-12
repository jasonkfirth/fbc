''
'' FreeBASIC CRT declarations for GCCSDK UnixLib
'' ---------------------------------------------
''
'' File: crt/fcntl.bi
''
'' Purpose:
''
''     Describe UnixLib file-control flags, structures, and functions.
''
'' Responsibilities:
''
''     - define the numeric open() and fcntl() ABI constants
''     - reproduce UnixLib's flock memory layout
''     - declare fcntl(), open(), and creat()
''
'' This file intentionally does NOT contain:
''
''     - FreeBASIC file-number handling
''     - RISC OS native filetype conversion
''     - descriptor implementation details
''
'' ABI note:
''
''     These values come from GCCSDK UnixLib's fcntl.h.  They are BSD-derived
''     and must not be replaced with constants copied from Linux headers.
''

#ifndef __crt_fcntl_bi__
#define __crt_fcntl_bi__

#include once "crt/sys/types.bi"

'' ---------------------------------------------------------------------------
'' File access and status flags
'' ---------------------------------------------------------------------------

#define O_RDONLY &h0000
#define O_WRONLY &h0001
#define O_RDWR &h0002
#define O_ACCMODE &h0003
#define O_NONBLOCK &h0004
#define O_NDELAY O_NONBLOCK
#define O_APPEND &h0008
#define O_SHLOCK &h0010
#define O_EXLOCK &h0020
#define O_ASYNC &h0040
#define O_FSYNC &h0080
#define O_SYNC O_FSYNC
#define O_EXECCL &h0100
#define O_CLOEXEC O_EXECCL
#define O_CREAT &h0200
#define O_TRUNC &h0400
#define O_EXCL &h0800
#define O_NOCTTY &h10000
#define O_LARGEFILE &h20000
#define O_BINARY 0
#define O_TEXT 0

'' ---------------------------------------------------------------------------
'' fcntl() operations and record-lock layout
'' ---------------------------------------------------------------------------

#define F_DUPFD 0
#define F_GETFD 1
#define F_SETFD 2
#define F_GETFL 3
#define F_SETFL 4
#define F_GETOWN 5
#define F_SETOWN 6
#define F_GETLK 7
#define F_SETLK 8
#define F_SETLKW 9
#define F_GETUNL 10
#define F_SETUNL 11
#define FASYNC O_ASYNC
#define FCREAT O_CREAT
#define FEXCL O_EXCL
#define FTRUNC O_TRUNC
#define FNOCTTY O_NOCTTY
#define FFSYNC O_FSYNC
#define FAPPEND O_APPEND
#define FNONBLOCK O_NONBLOCK
#define FNDELAY O_NDELAY

'' UnixLib also spells these aliases FREAD, FWRITE, and FSYNC.  Those names
'' collide with existing FreeBASIC language tokens, so callers should use the
'' canonical O_RDONLY, O_WRONLY, and O_SYNC constants instead.
#define FD_CLOEXEC O_EXECCL
#define F_RDLCK 1
#define F_WRLCK 2
#define F_UNLCK 3

type flock
	l_type as short
	l_whence as short
	l_start as __off_t
	l_len as __off_t
	l_pid as short
end type

extern "c"
declare function fcntl (byval fd as long, byval command as long, ...) as long
declare function open_ alias "open" (byval file as const zstring ptr, byval flags as long, ...) as long
declare function creat (byval file as const zstring ptr, byval mode as __mode_t) as long
declare function lockf (byval fd as long, byval command as long, byval length as __off_t) as long
end extern

#endif

'' end of crt/fcntl.bi
