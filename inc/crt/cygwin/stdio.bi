''
''
'' stdio -- Cygwin CRT stdio declarations
''
#ifndef __crt_cygwin_stdio_bi__
#define __crt_cygwin_stdio_bi__

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2
#define BUFSIZ 1024
#define FILENAME_MAX 4096
#define FOPEN_MAX 20
#define P_tmpdir "/tmp"
#define L_tmpnam FILENAME_MAX
#define TMP_MAX 26

type FILE as any

type fpos_t as clong

type __cygwin_reent
	_errno as long
	_stdin as FILE ptr
	_stdout as FILE ptr
	_stderr as FILE ptr
end type

extern "c"
declare function __getreent () as __cygwin_reent ptr
declare function snprintf (byval s as zstring ptr, byval n as size_t, byval format as zstring ptr, ...) as long
declare function vsnprintf (byval s as zstring ptr, byval n as size_t, byval format as zstring ptr, byval arg as va_list) as long
declare function popen (byval as zstring ptr, byval as zstring ptr) as FILE ptr
declare function pclose (byval as FILE ptr) as long
declare function getw (byval as FILE ptr) as long
declare function putw (byval as long, byval as FILE ptr) as long
end extern

#define stdin (__getreent()->_stdin)
#define stdout (__getreent()->_stdout)
#define stderr (__getreent()->_stderr)

#endif
