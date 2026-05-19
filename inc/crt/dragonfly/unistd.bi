''
''
'' unistd -- DragonFly CRT declarations
''
''
#ifndef __crt_dragonfly_unistd_bi__
#define __crt_dragonfly_unistd_bi__

#include once "crt/long.bi"
#include once "crt/stddef.bi"
#include once "crt/sys/types.bi"

#define _POSIX_VERSION 200112L
#define _POSIX2_VERSION 200112L

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
#endif

extern "c"
declare function access_ alias "access" (byval __name as zstring ptr, byval __type as long) as long
declare function close_ alias "close" (byval __fd as long) as long
declare function read_ alias "read" (byval __fd as long, byval __buf as any ptr, byval __nbytes as size_t) as ssize_t
declare function write_ alias "write" (byval __fd as long, byval __buf as any ptr, byval __n as size_t) as ssize_t
declare function pipe_ alias "pipe" (byval __pipedes as long ptr) as long
declare function sleep_ alias "sleep" (byval __seconds as ulong) as ulong
declare function chdir_ alias "chdir" (byval __path as zstring ptr) as long
declare function lseek (byval __fd as long, byval __offset as __off_t, byval __whence as long) as __off_t
declare function alarm (byval __seconds as ulong) as ulong
declare function usleep (byval __useconds as useconds_t) as long
declare function getcwd (byval __buf as zstring ptr, byval __size as size_t) as zstring ptr
declare function dup (byval __fd as long) as long
declare function dup2 (byval __fd as long, byval __fd2 as long) as long
declare sub _exit (byval __status as long)
declare function getpid () as __pid_t
declare function getppid () as __pid_t
declare function getuid () as __uid_t
declare function geteuid () as __uid_t
declare function getgid () as __gid_t
declare function getegid () as __gid_t
declare function fork () as __pid_t
declare function isatty (byval __fd as long) as long
declare function unlink (byval __name as zstring ptr) as long
declare function rmdir_ alias "rmdir" (byval __path as zstring ptr) as long
declare function gethostname (byval __name as zstring ptr, byval __len as size_t) as long
declare function fsync (byval __fd as long) as long
declare function truncate (byval __file as zstring ptr, byval __length as __off_t) as long
declare function ftruncate (byval __fd as long, byval __length as __off_t) as long
declare function sysconf (byval __name as long) as clong
end extern

#endif
