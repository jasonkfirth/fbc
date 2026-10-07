' TEST_MODE : COMPILE_ONLY_OK

'' FreeBASIC CRT platform coverage
'' File: platform-headers.bas
'' Purpose: Check standard CRT headers and native process/file scalar widths.
'' Responsibilities: Compile with each supported target's installed include order.
'' This file intentionally does NOT contain native runtime qualification.

#include once "crt.bi"
#include once "crt/stdint.bi"
#include once "crt/limits.bi"
#include once "crt/locale.bi"
#include once "crt/setjmp.bi"
#include once "crt/stdarg.bi"

#assert sizeof(size_t) = sizeof(any ptr)
#assert sizeof(ptrdiff_t) = sizeof(any ptr)
#assert sizeof(int32_t) = 4
#assert sizeof(int64_t) = 8

'' Wii and Xbox provide the standard C runtime, without a Unix process model.
'' Process bindings are checked where the SDK supplies an actual interface.
#if not defined(__FB_WII__) and not defined(__FB_XBOX__)

	#include once "crt/unistd.bi"
	#if defined(__FB_AROS__)
		#assert sizeof(typeof(getpid())) = sizeof(any ptr)
	#else
		#assert sizeof(typeof(getpid())) = 4
	#endif

	'' Force the external declaration into generated C so calling-convention
	'' and symbol checks can detect a declaration that only parses correctly.
	dim as typeof(getpid()) process_id = getpid()

	#if defined(__FB_FREEBSD__) or defined(__FB_NETBSD__) or defined(__FB_OPENBSD__)
		#assert sizeof(off_t) = 8
		#assert sizeof(uid_t) = 4
		#assert sizeof(gid_t) = 4
	#endif
	#if defined(__FB_FREEBSD__) or defined(__FB_NETBSD__) or defined(__FB_OPENBSD__) or defined(__FB_SOLARIS__) or defined(__FB_AROS__) or defined(__FB_AMIGA__)
		#assert sizeof(ssize_t) = sizeof(any ptr)
		'' Reading zero bytes exercises the ABI without consuming input.
		dim bytes_read as ssize_t = read_(STDIN_FILENO, 0, 0)
		dim position as off_t = lseek(-1, 0, SEEK_CUR)
	#endif

#endif

'' end of platform-headers.bas
