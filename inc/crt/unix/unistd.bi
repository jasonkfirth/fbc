'' FreeBASIC BSD, Solaris, and Android CRT declarations
'' File: crt/unix/unistd.bi
'' Purpose: Declare the common POSIX process and file interfaces.
'' Responsibilities: Use each target's native scalar types and C exports.
'' This file intentionally does NOT contain OS-specific configuration selectors.
''
'' Sources: FreeBSD, NetBSD, OpenBSD, illumos, and Bionic unistd.h headers.
'' Configuration constants and legacy setpgrp signatures differ between these
'' systems. Keep those extensions in platform bindings instead of borrowing
'' Linux enums or treating all Unix interfaces as the same ABI.

#ifndef __crt_unix_unistd_bi__
#define __crt_unix_unistd_bi__

#include once "crt/long.bi"
#include once "crt/sys/types.bi"

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
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

'' ---------------------------------------------------------------------------
'' File descriptors and path operations
'' ---------------------------------------------------------------------------

extern "c"
declare function access_ alias "access" (byval path as const zstring ptr, byval mode as long) as long
#ifndef __crt_close_declared__
#define __crt_close_declared__
declare function close_ alias "close" (byval fd as long) as long
#endif
declare function read_ alias "read" (byval fd as long, byval buffer as any ptr, byval count as size_t) as ssize_t
declare function write_ alias "write" (byval fd as long, byval buffer as const any ptr, byval count as size_t) as ssize_t
declare function pread (byval fd as long, byval buffer as any ptr, byval count as size_t, byval offset as off_t) as ssize_t
declare function pwrite (byval fd as long, byval buffer as const any ptr, byval count as size_t, byval offset as off_t) as ssize_t
declare function pipe_ alias "pipe" (byval descriptors as long ptr) as long
declare function dup (byval fd as long) as long
declare function dup2 (byval fd as long, byval new_fd as long) as long
declare function lseek (byval fd as long, byval offset as off_t, byval whence as long) as off_t
declare function ftruncate (byval fd as long, byval length as off_t) as long
declare function truncate_ alias "truncate" (byval path as const zstring ptr, byval length as off_t) as long
declare function fsync (byval fd as long) as long
declare function chdir_ alias "chdir" (byval path as const zstring ptr) as long
declare function fchdir (byval fd as long) as long
declare function getcwd (byval buffer as zstring ptr, byval length as size_t) as zstring ptr
declare function unlink (byval path as const zstring ptr) as long
declare function rmdir_ alias "rmdir" (byval path as const zstring ptr) as long
declare function link (byval source_path as const zstring ptr, byval target_path as const zstring ptr) as long
declare function symlink (byval source_path as const zstring ptr, byval target_path as const zstring ptr) as long
declare function readlink (byval path as const zstring ptr, byval buffer as zstring ptr, byval length as size_t) as ssize_t
declare function isatty (byval fd as long) as long
declare function sysconf (byval selector as long) as clong
declare function pathconf (byval path as const zstring ptr, byval selector as long) as clong
declare function fpathconf (byval fd as long, byval selector as long) as clong

'' ---------------------------------------------------------------------------
'' Process identity and lifetime
'' ---------------------------------------------------------------------------

declare function getpid () as pid_t
declare function getppid () as pid_t
declare function getpgrp () as pid_t
declare function getpgid (byval pid as pid_t) as pid_t
declare function getsid (byval pid as pid_t) as pid_t
declare function setpgid (byval pid as pid_t, byval group as pid_t) as long
declare function setsid () as pid_t
declare function fork () as pid_t
declare function execv (byval path as const zstring ptr, byval argv as zstring ptr ptr) as long
declare function execvp (byval path as const zstring ptr, byval argv as zstring ptr ptr) as long
declare function execve (byval path as const zstring ptr, byval argv as zstring ptr ptr, byval envp as zstring ptr ptr) as long
declare sub _exit (byval status as long)
declare function getuid () as uid_t
declare function geteuid () as uid_t
declare function getgid () as gid_t
declare function getegid () as gid_t
declare function setuid (byval uid as uid_t) as long
declare function seteuid (byval uid as uid_t) as long
declare function setgid (byval gid as gid_t) as long
declare function setegid (byval gid as gid_t) as long
declare function getgroups (byval count as long, byval groups as gid_t ptr) as long
declare function sleep_ alias "sleep" (byval seconds as ulong) as ulong
declare function usleep (byval microseconds as ulong) as long
declare function alarm (byval seconds as ulong) as ulong
declare function pause () as long
end extern

#endif

'' end of crt/unix/unistd.bi
