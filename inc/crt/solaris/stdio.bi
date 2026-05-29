''
'' stdio -- Solaris/illumos C runtime declarations
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
#ifndef __crt_solaris_stdio_bi__
#define __crt_solaris_stdio_bi__

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2
#define BUFSIZ 1024
#define FILENAME_MAX 1024
#define FOPEN_MAX 20
#define P_tmpdir "/tmp"
#define L_tmpnam 25
#define TMP_MAX 17576
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

type FILE
#if defined(__FB_64BIT__)
	__pad(0 to 15) as clong
#else
	_cnt as long
	_ptr as ubyte ptr
	_base as ubyte ptr
	_flag as ubyte
	_magic as ubyte
	__flags as ushort
#endif
end type

extern _iob(0 to 2) alias "_iob" as FILE
#define stdin (@_iob(STDIN_FILENO))
#define stdout (@_iob(STDOUT_FILENO))
#define stderr (@_iob(STDERR_FILENO))

type fpos_t as longint

extern "c"
declare function snprintf (byval s as zstring ptr, byval n as size_t, byval format as zstring ptr, ...) as long
declare function vsnprintf (byval s as zstring ptr, byval n as size_t, byval format as zstring ptr, byval arg as va_list) as long
declare function popen (byval as zstring ptr, byval as zstring ptr) as FILE ptr
declare function pclose (byval as FILE ptr) as long
declare function getw (byval as FILE ptr) as long
declare function putw (byval as long, byval as FILE ptr) as long
end extern

#endif
''
'' end of crt/solaris/stdio.bi
