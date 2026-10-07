'' FreeBASIC AROS POSIX CRT declarations
'' File: aros/crt/unistd.bi
'' Purpose: Declare file and process interfaces exported by posixc.library.
'' Responsibilities: Preserve native-sized pid_t, off_t, and ssize_t values.
'' This file intentionally does NOT contain unimplemented POSIX functions.
''
'' AROS generates these exports from compiler/crt/posixc/posixc.conf.
'' Entries marked as skipped there, including fork and pread, are unavailable.
'' The normal SDK link includes posixc; -noposixc deliberately excludes it.

#ifndef __crt_aros_unistd_bi__
#define __crt_aros_unistd_bi__

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

extern "c"
declare function access_ alias "access" (byval path as const zstring ptr, byval mode as long) as long
declare function chdir_ alias "chdir" (byval path as const zstring ptr) as long
#ifndef __crt_close_declared__
#define __crt_close_declared__
declare function close_ alias "close" (byval fd as long) as long
#endif
declare function dup (byval fd as long) as long
declare function dup2 (byval fd as long, byval new_fd as long) as long
declare function execv (byval path as const zstring ptr, byval argv as zstring ptr ptr) as long
declare function execve (byval path as const zstring ptr, byval argv as zstring ptr ptr, byval envp as zstring ptr ptr) as long
declare function execvp (byval path as const zstring ptr, byval argv as zstring ptr ptr) as long
declare function fchdir (byval fd as long) as long
declare function fsync (byval fd as long) as long
declare function ftruncate (byval fd as long, byval length as off_t) as long
declare function getcwd (byval buffer as zstring ptr, byval length as size_t) as zstring ptr
declare function getegid () as gid_t
declare function geteuid () as uid_t
declare function getgid () as gid_t
declare function getgroups (byval count as long, byval groups as gid_t ptr) as long
declare function getpgid (byval pid as pid_t) as pid_t
declare function getpgrp () as pid_t
declare function getpid () as pid_t
declare function getppid () as pid_t
declare function getuid () as uid_t
declare function isatty (byval fd as long) as long
declare function link (byval source_path as const zstring ptr, byval target_path as const zstring ptr) as long
declare function lseek (byval fd as long, byval offset as off_t, byval whence as long) as off_t
declare function pathconf (byval path as const zstring ptr, byval selector as long) as clong
declare function pipe_ alias "pipe" (byval descriptors as long ptr) as long
declare function read_ alias "read" (byval fd as long, byval buffer as any ptr, byval count as size_t) as ssize_t
declare function readlink (byval path as const zstring ptr, byval buffer as zstring ptr, byval length as size_t) as ssize_t
declare function rmdir_ alias "rmdir" (byval path as const zstring ptr) as long
declare function setegid (byval gid as gid_t) as long
declare function seteuid (byval uid as uid_t) as long
declare function setgid (byval gid as gid_t) as long
declare function setsid () as pid_t
declare function setuid (byval uid as uid_t) as long
declare function sleep_ alias "sleep" (byval seconds as ulong) as ulong
declare function symlink (byval source_path as const zstring ptr, byval target_path as const zstring ptr) as long
declare sub sync ()
declare function sysconf (byval selector as long) as clong
declare function truncate_ alias "truncate" (byval path as const zstring ptr, byval length as off_t) as long
declare function unlink (byval path as const zstring ptr) as long
declare function write_ alias "write" (byval fd as long, byval buffer as const any ptr, byval count as size_t) as ssize_t
end extern

#endif

'' end of aros/crt/unistd.bi
