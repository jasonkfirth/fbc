''
''
'' errno -- header translated with help of SWIG FB wrapper
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __crt_errno_bi__
#define __crt_errno_bi__

#ifdef __FB_HAIKU__
	'' Haiku exposes POSIX errors through status_t values instead of the
	'' small positive numbers used by Linux.  These bases and mappings come
	'' from Haiku's public Errors.h ABI.
	#define __FB_HAIKU_GENERAL_ERROR_BASE cast(long, &h80000000)
	#define __FB_HAIKU_OS_ERROR_BASE (__FB_HAIKU_GENERAL_ERROR_BASE + &h1000)
	#define __FB_HAIKU_STORAGE_ERROR_BASE (__FB_HAIKU_GENERAL_ERROR_BASE + &h6000)
	#define __FB_HAIKU_POSIX_ERROR_BASE (__FB_HAIKU_GENERAL_ERROR_BASE + &h7000)

	#define EPERM (__FB_HAIKU_GENERAL_ERROR_BASE + 15)
	#define ENOFILE (__FB_HAIKU_STORAGE_ERROR_BASE + 3)
	#define ENOENT (__FB_HAIKU_STORAGE_ERROR_BASE + 3)
	#define ESRCH (__FB_HAIKU_POSIX_ERROR_BASE + 13)
	#define EINTR (__FB_HAIKU_GENERAL_ERROR_BASE + 10)
	#define EIO (__FB_HAIKU_GENERAL_ERROR_BASE + 1)
	#define ENXIO (__FB_HAIKU_POSIX_ERROR_BASE + 11)
	#define E2BIG (__FB_HAIKU_POSIX_ERROR_BASE + 1)
	#define ENOEXEC (__FB_HAIKU_OS_ERROR_BASE + &h302)
	#define EBADF (__FB_HAIKU_STORAGE_ERROR_BASE + 0)
	#define ECHILD (__FB_HAIKU_POSIX_ERROR_BASE + 2)
	#define EAGAIN (__FB_HAIKU_GENERAL_ERROR_BASE + 11)
	#define ENOMEM __FB_HAIKU_GENERAL_ERROR_BASE
	#define EACCES (__FB_HAIKU_GENERAL_ERROR_BASE + 2)
	#define EFAULT (__FB_HAIKU_OS_ERROR_BASE + &h301)
	#define EBUSY (__FB_HAIKU_GENERAL_ERROR_BASE + 14)
	#define EEXIST (__FB_HAIKU_STORAGE_ERROR_BASE + 2)
	#define EXDEV (__FB_HAIKU_STORAGE_ERROR_BASE + 11)
	#define ENODEV (__FB_HAIKU_POSIX_ERROR_BASE + 7)
	#define ENOTDIR (__FB_HAIKU_STORAGE_ERROR_BASE + 5)
	#define EISDIR (__FB_HAIKU_STORAGE_ERROR_BASE + 9)
	#define EINVAL (__FB_HAIKU_GENERAL_ERROR_BASE + 5)
	#define ENFILE (__FB_HAIKU_POSIX_ERROR_BASE + 6)
	#define EMFILE (__FB_HAIKU_STORAGE_ERROR_BASE + 10)
	#define ENOTTY (__FB_HAIKU_POSIX_ERROR_BASE + 10)
	#define EFBIG (__FB_HAIKU_POSIX_ERROR_BASE + 4)
	#define ENOSPC (__FB_HAIKU_STORAGE_ERROR_BASE + 7)
	#define ESPIPE (__FB_HAIKU_POSIX_ERROR_BASE + 12)
	#define EROFS (__FB_HAIKU_STORAGE_ERROR_BASE + 8)
	#define EMLINK (__FB_HAIKU_POSIX_ERROR_BASE + 5)
	#define EPIPE (__FB_HAIKU_STORAGE_ERROR_BASE + 13)
	#define EDOM (__FB_HAIKU_POSIX_ERROR_BASE + 16)
	#define ERANGE (__FB_HAIKU_POSIX_ERROR_BASE + 17)
	#define EDEADLOCK (__FB_HAIKU_POSIX_ERROR_BASE + 3)
	#define EDEADLK (__FB_HAIKU_POSIX_ERROR_BASE + 3)
	#define ENAMETOOLONG (__FB_HAIKU_STORAGE_ERROR_BASE + 4)
	#define ENOLCK (__FB_HAIKU_POSIX_ERROR_BASE + 8)
	#define ENOSYS (__FB_HAIKU_POSIX_ERROR_BASE + 9)
	#define ENOTEMPTY (__FB_HAIKU_STORAGE_ERROR_BASE + 6)
	#define EILSEQ (__FB_HAIKU_POSIX_ERROR_BASE + 38)
#elseif defined(__FB_DARWIN__)
        '' Darwin errno numbers are different from Linux, so use Darwin values here.
#else

        #define EPERM 1
        #define ENOFILE 2
        #define ENOENT 2
        #define ESRCH 3
        #define EINTR 4
        #define EIO 5
        #define ENXIO 6
        #define E2BIG 7
        #define ENOEXEC 8
        #define EBADF 9
        #define ECHILD 10
        #define EAGAIN 11
        #define ENOMEM 12
        #define EACCES 13
        #define EFAULT 14
        #define EBUSY 16
        #define EEXIST 17
        #define EXDEV 18
        #define ENODEV 19
        #define ENOTDIR 20
        #define EISDIR 21
        #define EINVAL 22
        #define ENFILE 23
        #define EMFILE 24
        #define ENOTTY 25
        #define EFBIG 27
        #define ENOSPC 28
        #define ESPIPE 29
        #define EROFS 30
        #define EMLINK 31
        #define EPIPE 32
        #define EDOM 33
        #define ERANGE 34
        #define EDEADLOCK 36
        #define EDEADLK 36
        #define ENAMETOOLONG 38
        #define ENOLCK 39
        #define ENOSYS 40
        #define ENOTEMPTY 41
        #define EILSEQ 42
#endif

extern "C"

#ifdef __FB_WIN32__
	declare function _errno() as long ptr
	#define errno (*_errno())
#elseif defined( __FB_LINUX__ )
	declare function __errno_location() as long ptr
	#define errno (*__errno_location())
#elseif defined( __FB_DARWIN__ )
        declare function __error() as long ptr
        #define errno (*__error())
#elseif defined( __FB_HAIKU__ )
	declare function _errnop() as long ptr
	#define errno (*_errnop())
#else
	extern errno as long
#endif

end extern

#endif
