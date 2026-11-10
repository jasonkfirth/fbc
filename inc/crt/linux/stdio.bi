''
''
'' stdio -- header translated with help of SWIG FB wrapper
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __crt_linux_stdio_bi__
#define __crt_linux_stdio_bi__

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2
#define BUFSIZ 8192
#define FILENAME_MAX 4096
#define FOPEN_MAX 16
#define P_tmpdir "/tmp"
#define L_tmpnam 20
#define TMP_MAX 238328

type FILE as _IO_FILE

extern stdin alias "stdin" as FILE ptr
extern stdout alias "stdout" as FILE ptr
extern stderr alias "stderr" as FILE ptr

#if defined(__FB_LINUX__)
'' glibc fpos_t uses a 64-bit file position and multibyte conversion state.
'' The Linux runtime builds with _FILE_OFFSET_BITS=64, including 32-bit targets.
private union __fb_fpos_state_value
	__wch as ulong
	__wchb(0 to 4-1) as ubyte
end union

private type __fb_fpos_state
	__count as long
	__value as __fb_fpos_state_value
end type

type fpos_t
	__pos as longint
	__state as __fb_fpos_state
end type
#else
type fpos_t as longint
#endif

extern "c"
declare function snprintf (byval s as zstring ptr, byval n as size_t, byval format as zstring ptr, ...) as long
declare function vsnprintf (byval s as zstring ptr, byval n as size_t, byval format as zstring ptr, byval arg as va_list) as long
declare function popen (byval as zstring ptr, byval as zstring ptr) as FILE ptr
declare function pclose (byval as FILE ptr) as long
declare function getw (byval as FILE ptr) as long
declare function putw (byval as long, byval as FILE ptr) as long
end extern

#endif
