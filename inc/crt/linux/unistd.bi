''
''
'' unistd -- header translated with help of SWIG FB wrapper
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __crt_linux_unistd_bi__
#define __crt_linux_unistd_bi__

#define _POSIX_VERSION 200112L
#define _POSIX2_VERSION 200112L
#define _POSIX2_C_BIND 200112L
#define _POSIX2_C_DEV 200112L
#define _POSIX2_SW_DEV 200112L
#define _POSIX2_LOCALEDEF 200112L
#define _XOPEN_VERSION 4
#define _XOPEN_XCU_VERSION 4
#define _XOPEN_XPG2 1
#define _XOPEN_XPG3 1
#define _XOPEN_XPG4 1
#define _XOPEN_UNIX 1
#define _XOPEN_CRYPT 1
#define _XOPEN_ENH_I18N 1
#define _XOPEN_LEGACY 1

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#include once "crt/sys/types.bi"

#ifndef ssize_t
type ssize_t as __ssize_t
#endif

#include once "crt/stddef.bi"

type intptr_t as __intptr_t

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
#define L_SET 0
#define L_INCR 1
#define L_XTND 2
#endif

#ifndef F_ULOCK
#define F_ULOCK 0
#define F_LOCK 1
#define F_TLOCK 2
#define F_TEST 3
#endif

extern "c"
declare function access_ alias "access" (byval __name as zstring ptr, byval __type as integer) as integer
declare function close_ alias "close" (byval __fd as integer) as integer
declare function read_ alias "read" (byval __fd as integer, byval __buf as any ptr, byval __nbytes as size_t) as ssize_t
declare function write_ alias "write" (byval __fd as integer, byval __buf as any ptr, byval __n as size_t) as ssize_t
declare function pipe_ alias "pipe" (byval __pipedes as integer ptr) as integer
declare function sleep_ alias "sleep" (byval __seconds as uinteger) as uinteger
declare function chdir_ alias "chdir" (byval __path as zstring ptr) as integer
declare function lseek (byval __fd as integer, byval __offset as __off_t, byval __whence as integer) as __off_t
declare function alarm (byval __seconds as uinteger) as uinteger
declare function ualarm (byval __value as __useconds_t, byval __interval as __useconds_t) as __useconds_t
declare function usleep (byval __useconds as __useconds_t) as integer
declare function pause () as integer
declare function chown (byval __file as zstring ptr, byval __owner as __uid_t, byval __group as __gid_t) as integer
declare function fchown (byval __fd as integer, byval __owner as __uid_t, byval __group as __gid_t) as integer
declare function lchown (byval __file as zstring ptr, byval __owner as __uid_t, byval __group as __gid_t) as integer
declare function fchdir (byval __fd as integer) as integer
declare function getcwd (byval __buf as zstring ptr, byval __size as size_t) as zstring ptr
declare function getwd (byval __buf as zstring ptr) as zstring ptr
declare function dup (byval __fd as integer) as integer
declare function dup2 (byval __fd as integer, byval __fd2 as integer) as integer
declare function execve (byval __path as zstring ptr, byval __argv as byte ptr ptr, byval __envp as byte ptr ptr) as integer
declare function execv (byval __path as zstring ptr, byval __argv as byte ptr ptr) as integer
declare function execle (byval __path as zstring ptr, byval __arg as zstring ptr, ...) as integer
declare function execl (byval __path as zstring ptr, byval __arg as zstring ptr, ...) as integer
declare function execvp (byval __file as zstring ptr, byval __argv as byte ptr ptr) as integer
declare function execlp (byval __file as zstring ptr, byval __arg as zstring ptr, ...) as integer
declare function nice (byval __inc as integer) as integer
declare sub _exit (byval __status as integer)
declare function pathconf (byval __path as zstring ptr, byval __name as integer) as integer
declare function fpathconf (byval __fd as integer, byval __name as integer) as integer
declare function sysconf (byval __name as integer) as integer
declare function confstr (byval __name as integer, byval __buf as zstring ptr, byval __len as size_t) as size_t
declare function getpid () as __pid_t
declare function getppid () as __pid_t
declare function getpgrp () as __pid_t
declare function __getpgid (byval __pid as __pid_t) as __pid_t
declare function setpgid (byval __pid as __pid_t, byval __pgid as __pid_t) as integer
declare function setpgrp () as integer
declare function setsid () as __pid_t
declare function getuid () as __uid_t
declare function geteuid () as __uid_t
declare function getgid () as __gid_t
declare function getegid () as __gid_t
declare function getgroups (byval __size as integer, byval __list as __gid_t ptr) as integer
declare function setuid (byval __uid as __uid_t) as integer
declare function setreuid (byval __ruid as __uid_t, byval __euid as __uid_t) as integer
declare function seteuid (byval __uid as __uid_t) as integer
declare function setgid (byval __gid as __gid_t) as integer
declare function setregid (byval __rgid as __gid_t, byval __egid as __gid_t) as integer
declare function setegid (byval __gid as __gid_t) as integer
declare function fork () as __pid_t
declare function vfork () as __pid_t
declare function ttyname (byval __fd as integer) as zstring ptr
declare function ttyname_r (byval __fd as integer, byval __buf as zstring ptr, byval __buflen as size_t) as integer
declare function isatty (byval __fd as integer) as integer
declare function ttyslot () as integer
declare function link (byval __from as zstring ptr, byval __to as zstring ptr) as integer
declare function symlink (byval __from as zstring ptr, byval __to as zstring ptr) as integer
declare function readlink (byval __path as zstring ptr, byval __buf as zstring ptr, byval __len as size_t) as integer
declare function unlink (byval __name as zstring ptr) as integer
declare function rmdir_ alias "rmdir" (byval __path as zstring ptr) as integer
declare function tcgetpgrp (byval __fd as integer) as __pid_t
declare function tcsetpgrp (byval __fd as integer, byval __pgrp_id as __pid_t) as integer
declare function getlogin () as zstring ptr
declare function setlogin (byval __name as zstring ptr) as integer
declare function gethostname (byval __name as zstring ptr, byval __len as size_t) as integer
declare function sethostname (byval __name as zstring ptr, byval __len as size_t) as integer
declare function sethostid (byval __id as integer) as integer
declare function getdomainname (byval __name as zstring ptr, byval __len as size_t) as integer
declare function setdomainname (byval __name as zstring ptr, byval __len as size_t) as integer
declare function vhangup () as integer
declare function revoke (byval __file as zstring ptr) as integer
declare function profil (byval __sample_buffer as ushort ptr, byval __size as size_t, byval __offset as size_t, byval __scale as uinteger) as integer
declare function acct (byval __name as zstring ptr) as integer
declare function getusershell () as zstring ptr
declare sub endusershell ()
declare sub setusershell ()
declare function daemon (byval __nochdir as integer, byval __noclose as integer) as integer
declare function chroot (byval __path as zstring ptr) as integer
declare function getpass (byval __prompt as zstring ptr) as zstring ptr
declare function fsync (byval __fd as integer) as integer
declare function gethostid () as integer
declare sub sync ()
declare function getpagesize () as integer
declare function getdtablesize () as integer
declare function truncate (byval __file as zstring ptr, byval __length as __off_t) as integer
declare function ftruncate (byval __fd as integer, byval __length as __off_t) as integer
declare function brk (byval __addr as any ptr) as integer
declare function sbrk (byval __delta as intptr_t) as any ptr
declare function syscall (byval __sysno as integer, ...) as integer
declare function fdatasync (byval __fildes as integer) as integer
end extern

extern __environ alias "__environ" as byte ptr ptr

#endif
