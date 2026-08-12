''
'' FreeBASIC CRT declarations for GCCSDK UnixLib
'' ---------------------------------------------
''
'' File: crt/unistd.bi
''
'' Purpose:
''
''     Describe the UnixLib unistd ABI used by RISC OS programs.
''
'' Responsibilities:
''
''     - define UnixLib POSIX feature and file descriptor constants
''     - declare the process, file, terminal, and host functions UnixLib exports
''     - expose UnixLib's process environment symbol to FreeBASIC code
''
'' This file intentionally does NOT contain:
''
''     - Linux-only declarations
''     - RISC OS SWI wrappers
''     - socket declarations owned by crt/sys/socket.bi
''
'' ABI notes:
''
''     UnixLib implements the 1990 POSIX interface and selected BSD/X/Open
''     extensions.  Its setpgrp() takes pid and process-group arguments; this
''     differs from the no-argument function exposed by modern Linux libc.
''

#ifndef __crt_unistd_bi__
#define __crt_unistd_bi__

#include once "crt/long.bi"
#include once "crt/stddef.bi"
#include once "crt/sys/types.bi"

'' ---------------------------------------------------------------------------
'' UnixLib feature levels and standard descriptors
'' ---------------------------------------------------------------------------

#define _POSIX_VERSION 199009L
#define _POSIX2_C_VERSION 199912L
#define _POSIX2_C_BIND 1
#define _POSIX2_C_DEV 1
#define _POSIX2_SW_DEV 1

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#ifndef intptr_t
type intptr_t as __intptr_t
#endif

#ifndef socklen_t
type socklen_t as __socklen_t
#endif

#ifndef R_OK
#define R_OK 4
#define W_OK 2
#define X_OK 1
#define F_OK 0
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#ifndef F_ULOCK
#define F_ULOCK 0
#define F_LOCK 1
#define F_TLOCK 2
#define F_TEST 3
#endif

'' ---------------------------------------------------------------------------
'' pathconf(), sysconf(), and confstr() selectors
'' ---------------------------------------------------------------------------

enum
	_PC_LINK_MAX
	_PC_MAX_CANON
	_PC_MAX_INPUT
	_PC_NAME_MAX
	_PC_PATH_MAX
	_PC_PIPE_BUF
	_PC_CHOWN_RESTRICTED
	_PC_NO_TRUNC
	_PC_VDISABLE
end enum

enum
	_SC_ARG_MAX
	_SC_CHILD_MAX
	_SC_CLK_TCK
	_SC_NGROUPS_MAX
	_SC_OPEN_MAX
	_SC_STREAM_MAX
	_SC_TZNAME_MAX
	_SC_JOB_CONTROL
	_SC_SAVED_IDS
	_SC_VERSION
	_SC_PAGESIZE
	_SC_BC_BASE_MAX
	_SC_BC_DIM_MAX
	_SC_BC_SCALE_MAX
	_SC_BC_STRING_MAX
	_SC_COLL_WEIGHTS_MAX
	_SC_EQUIV_CLASS_MAX
	_SC_EXPR_NEST_MAX
	_SC_LINE_MAX
	_SC_RE_DUP_MAX
	_SC_2_VERSION
	_SC_2_C_BIND
	_SC_2_C_DEV
	_SC_2_FORT_DEV
	_SC_2_FORT_RUN
	_SC_2_SW_DEV
	_SC_2_LOCALEDEF
	_SC_PAGE_SIZE = _SC_PAGESIZE
	_SC_PHYS_PAGES
	_SC_NPROCESSORS_ONLN
end enum

#define _CS_PATH 0

'' ---------------------------------------------------------------------------
'' UnixLib function declarations
'' ---------------------------------------------------------------------------

extern "c"
declare function access_ alias "access" (byval path as const zstring ptr, byval mode as long) as long
#ifndef __crt_close_declared__
#define __crt_close_declared__
declare function close_ alias "close" (byval fd as long) as long
#endif
declare function read_ alias "read" (byval fd as long, byval buffer as any ptr, byval bytes as size_t) as ssize_t
declare function write_ alias "write" (byval fd as long, byval buffer as const any ptr, byval bytes as size_t) as ssize_t
declare function pipe_ alias "pipe" (byval descriptors as long ptr) as long
declare function sleep_ alias "sleep" (byval seconds as ulong) as ulong
declare function chdir_ alias "chdir" (byval path as const zstring ptr) as long
declare function lseek (byval fd as long, byval offset as off_t, byval whence as long) as off_t
declare function alarm (byval seconds as ulong) as ulong
declare function ualarm (byval value as __useconds_t, byval interval as __useconds_t) as __useconds_t
declare function usleep (byval microseconds as __useconds_t) as long
declare function pause () as long
declare function chown (byval path as const zstring ptr, byval owner as uid_t, byval group as gid_t) as long
declare function fchown (byval fd as long, byval owner as uid_t, byval group as gid_t) as long
declare function lchown (byval path as const zstring ptr, byval owner as uid_t, byval group as gid_t) as long
declare function fchdir (byval fd as long) as long
declare function getcwd (byval buffer as zstring ptr, byval length as size_t) as zstring ptr
declare function getwd (byval buffer as zstring ptr) as zstring ptr
declare function dup (byval fd as long) as long
declare function dup2 (byval fd as long, byval new_fd as long) as long
declare function execve (byval path as const zstring ptr, byval argv as byte ptr ptr, byval envp as byte ptr ptr) as long
declare function execv (byval path as const zstring ptr, byval argv as byte ptr ptr) as long
declare function execle (byval path as const zstring ptr, byval first_arg as const zstring ptr, ...) as long
declare function execl (byval path as const zstring ptr, byval first_arg as const zstring ptr, ...) as long
declare function execvp (byval file as const zstring ptr, byval argv as byte ptr ptr) as long
declare function execlp (byval file as const zstring ptr, byval first_arg as const zstring ptr, ...) as long
declare sub _exit (byval status as long)
declare function pathconf (byval path as const zstring ptr, byval selector as long) as clong
declare function fpathconf (byval fd as long, byval selector as long) as clong
declare function sysconf (byval selector as long) as clong
declare function confstr (byval selector as long, byval buffer as zstring ptr, byval length as size_t) as size_t
declare function getpid () as pid_t
declare function getppid () as pid_t
declare function getpgrp () as pid_t
declare function setpgrp (byval pid as pid_t, byval process_group as pid_t) as long
declare function setpgid (byval pid as pid_t, byval process_group as pid_t) as long
declare function setsid () as pid_t
declare function getuid () as uid_t
declare function geteuid () as uid_t
declare function getgid () as gid_t
declare function getegid () as gid_t
declare function setuid (byval uid as uid_t) as long
declare function seteuid (byval uid as uid_t) as long
declare function setgid (byval gid as gid_t) as long
declare function setegid (byval gid as gid_t) as long
declare function fork () as pid_t
declare function vfork () as pid_t
declare function ttyname (byval fd as long) as zstring ptr
declare function ttyname_r (byval fd as long, byval buffer as zstring ptr, byval length as size_t) as long
declare function isatty (byval fd as long) as long
declare function ispipe (byval fd as long) as long
declare function link (byval old_path as const zstring ptr, byval new_path as const zstring ptr) as long
declare function symlink (byval target as const zstring ptr, byval link_path as const zstring ptr) as long
declare function readlink (byval path as const zstring ptr, byval buffer as zstring ptr, byval length as size_t) as ssize_t
declare function unlink (byval path as const zstring ptr) as long
declare function rmdir_ alias "rmdir" (byval path as const zstring ptr) as long
declare function tcgetpgrp (byval fd as long) as pid_t
declare function tcsetpgrp (byval fd as long, byval process_group as pid_t) as long
declare function getlogin () as zstring ptr
declare function brk (byval address as any ptr) as long
declare function sbrk (byval increment as intptr_t) as any ptr
declare function gethostname (byval name as zstring ptr, byval length as size_t) as long
declare function sethostname (byval name as const zstring ptr, byval length as size_t) as long
declare function getdomainname (byval name as zstring ptr, byval length as size_t) as long
declare function setdomainname (byval name as const zstring ptr, byval length as size_t) as long
declare sub sync ()
declare function fsync (byval fd as long) as long
declare function truncate (byval path as const zstring ptr, byval length as off_t) as long
declare function ftruncate (byval fd as long, byval length as off_t) as long
declare function getpagesize () as long
declare function getdtablesize () as long
declare function chroot (byval path as const zstring ptr) as long
declare function getpass (byval prompt as const zstring ptr) as zstring ptr
declare function nice (byval increment as long) as long
end extern

'' UnixLib exports the ISO/POSIX symbol directly.  glibc instead exposes the
'' internal spelling used by crt/linux/unistd.bi.
extern __environ alias "environ" as byte ptr ptr

#endif

'' end of crt/unistd.bi
