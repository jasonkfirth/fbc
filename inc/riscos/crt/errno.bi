''
'' FreeBASIC CRT declarations for GCCSDK UnixLib
'' ---------------------------------------------
''
'' File: crt/errno.bi
''
'' Purpose:
''
''     Define the complete UnixLib errno interface for RISC OS.
''
'' Responsibilities:
''
''     - define UnixLib's ABI error numbers
''     - expose UnixLib's process errno variable
''     - remain independent of Linux errno numbering
''
'' This file intentionally does NOT contain:
''
''     - error-message formatting
''     - FreeBASIC runtime error numbers
''     - declarations for other operating systems
''

#ifndef __crt_errno_bi__
#define __crt_errno_bi__

'' GCCSDK UnixLib follows the classic BSD numbering through its socket
'' range, followed by UnixLib-specific errors.
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
#define EDEADLOCK 11
#define EDEADLK 11
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
#define EAGAIN 35
#define ENAMETOOLONG 63
#define ENOTEMPTY 66
#define ENOLCK 79
#define ENOSYS 87
#define EILSEQ 90


extern "C"
	extern errno as long

end extern

#endif

'' end of crt/errno.bi
