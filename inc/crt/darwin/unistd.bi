''
''
'' unistd -- Darwin CRT declarations
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __crt_darwin_unistd_bi__
#define __crt_darwin_unistd_bi__

#include once "crt/long.bi"
#include once "crt/stddef.bi"
#include once "crt/sys/types.bi"

#define _POSIX_VERSION 200112L
#define _POSIX2_VERSION 200112L
#define _POSIX2_C_BIND 200112L
#define _POSIX2_C_DEV 200112L
#define _POSIX2_SW_DEV 200112L
#define _POSIX2_LOCALEDEF 200112L
#define _XOPEN_VERSION 600
#define _XOPEN_XCU_VERSION 4

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

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
#define L_SET SEEK_SET
#define L_INCR SEEK_CUR
#define L_XTND SEEK_END
#endif

#ifndef F_ULOCK
#define F_ULOCK 0
#define F_LOCK 1
#define F_TLOCK 2
#define F_TEST 3
#endif

extern "c"
declare function access_ alias "access" (byval __path as const zstring ptr, byval __mode as long) as long
#ifndef __crt_close_declared__
#define __crt_close_declared__
'' sys/socket.bi also exposes the POSIX close() function.
declare function close_ alias "close" (byval __fd as long) as long
#endif
declare function read_ alias "read" (byval __fd as long, byval __buffer as any ptr, byval __bytes as size_t) as ssize_t
declare function write_ alias "write" (byval __fd as long, byval __buffer as const any ptr, byval __bytes as size_t) as ssize_t
declare function pipe_ alias "pipe" (byval __fds as long ptr) as long
declare function sleep_ alias "sleep" (byval __seconds as ulong) as ulong
declare function chdir_ alias "chdir" (byval __path as const zstring ptr) as long
declare function lseek (byval __fd as long, byval __offset as off_t, byval __whence as long) as off_t
declare function alarm (byval __seconds as ulong) as ulong
declare function ualarm (byval __value as useconds_t, byval __interval as useconds_t) as useconds_t
declare function usleep (byval __microseconds as useconds_t) as long
declare function pause () as long
declare function chown (byval __path as const zstring ptr, byval __owner as uid_t, byval __group as gid_t) as long
declare function fchown (byval __fd as long, byval __owner as uid_t, byval __group as gid_t) as long
declare function lchown (byval __path as const zstring ptr, byval __owner as uid_t, byval __group as gid_t) as long
declare function fchdir (byval __fd as long) as long
declare function getcwd (byval __buffer as zstring ptr, byval __size as size_t) as zstring ptr
declare function getwd (byval __buffer as zstring ptr) as zstring ptr
declare function dup (byval __fd as long) as long
declare function dup2 (byval __fd as long, byval __new_fd as long) as long
declare function execve (byval __path as const zstring ptr, byval __argv as byte ptr ptr, byval __envp as byte ptr ptr) as long
declare function execv (byval __path as const zstring ptr, byval __argv as byte ptr ptr) as long
declare function execle (byval __path as const zstring ptr, byval __arg0 as const zstring ptr, ...) as long
declare function execl (byval __path as const zstring ptr, byval __arg0 as const zstring ptr, ...) as long
declare function execvp (byval __file as const zstring ptr, byval __argv as byte ptr ptr) as long
declare function execlp (byval __file as const zstring ptr, byval __arg0 as const zstring ptr, ...) as long
declare function nice (byval __increment as long) as long
declare sub _exit (byval __status as long)
declare function pathconf (byval __path as const zstring ptr, byval __name as long) as clong
declare function fpathconf (byval __fd as long, byval __name as long) as clong
declare function sysconf (byval __name as long) as clong
declare function confstr (byval __name as long, byval __buffer as zstring ptr, byval __length as size_t) as size_t
declare function getpid () as pid_t
declare function getppid () as pid_t
declare function getpgrp () as pid_t
declare function getpgid (byval __pid as pid_t) as pid_t
declare function setpgid (byval __pid as pid_t, byval __pgid as pid_t) as long
declare function setpgrp () as pid_t
declare function setsid () as pid_t
declare function getuid () as uid_t
declare function geteuid () as uid_t
declare function getgid () as gid_t
declare function getegid () as gid_t
declare function getgroups (byval __count as long, byval __groups as gid_t ptr) as long
declare function setuid (byval __uid as uid_t) as long
declare function setreuid (byval __real_uid as uid_t, byval __effective_uid as uid_t) as long
declare function seteuid (byval __uid as uid_t) as long
declare function setgid (byval __gid as gid_t) as long
declare function setregid (byval __real_gid as gid_t, byval __effective_gid as gid_t) as long
declare function setegid (byval __gid as gid_t) as long
declare function fork () as pid_t
declare function vfork () as pid_t
declare function ttyname (byval __fd as long) as zstring ptr
declare function ttyname_r (byval __fd as long, byval __buffer as zstring ptr, byval __length as size_t) as long
declare function isatty (byval __fd as long) as long
declare function ttyslot () as long
declare function link (byval __from as const zstring ptr, byval __to as const zstring ptr) as long
declare function symlink (byval __from as const zstring ptr, byval __to as const zstring ptr) as long
declare function readlink (byval __path as const zstring ptr, byval __buffer as zstring ptr, byval __length as size_t) as ssize_t
declare function unlink (byval __path as const zstring ptr) as long
declare function rmdir_ alias "rmdir" (byval __path as const zstring ptr) as long
declare function tcgetpgrp (byval __fd as long) as pid_t
declare function tcsetpgrp (byval __fd as long, byval __pgrp as pid_t) as long
declare function getlogin () as zstring ptr
declare function getlogin_r (byval __buffer as zstring ptr, byval __length as size_t) as long
declare function setlogin (byval __name as const zstring ptr) as long
declare function gethostname (byval __name as zstring ptr, byval __length as size_t) as long
declare function sethostname (byval __name as const zstring ptr, byval __length as long) as long
declare sub sethostid (byval __identifier as clong)
declare function getdomainname (byval __name as zstring ptr, byval __length as long) as long
declare function setdomainname (byval __name as const zstring ptr, byval __length as long) as long
declare function revoke (byval __path as const zstring ptr) as long
declare function acct (byval __path as const zstring ptr) as long
declare function getusershell () as zstring ptr
declare sub endusershell ()
declare sub setusershell ()
declare function daemon (byval __no_chdir as long, byval __no_close as long) as long
declare function chroot (byval __path as const zstring ptr) as long
declare function getpass (byval __prompt as const zstring ptr) as zstring ptr
declare function fsync (byval __fd as long) as long
declare function gethostid () as clong
declare sub sync ()
declare function getpagesize () as long
declare function getdtablesize () as long
declare function truncate (byval __path as const zstring ptr, byval __length as off_t) as long
declare function ftruncate (byval __fd as long, byval __length as off_t) as long
declare function brk (byval __address as const any ptr) as any ptr
declare function sbrk (byval __increment as long) as any ptr
declare function syscall (byval __number as long, ...) as long

'' Darwin exposes process environment storage through this accessor so that
'' executables remain compatible with dyld's indirection model.
declare function _NSGetEnviron () as byte ptr ptr ptr
end extern

#endif

'' end of crt/darwin/unistd.bi
