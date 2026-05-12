''
'' stdio -- Xbox CRT stdio declarations
''
'' The Xbox target uses nxdk's PDCLib.  Its standard streams are exported
'' as FILE pointers named stdin/stdout/stderr, unlike the Win32 CRT _iob
'' array used by the old MinGW 32-bit headers.
''
#ifndef __crt_xbox_stdio_bi__
#define __crt_xbox_stdio_bi__

#define _IOFBF 1
#define _IOLBF 2
#define _IONBF 4
#define BUFSIZ 1024
#define FILENAME_MAX 1024
#define FOPEN_MAX 20
#define P_tmpdir "/tmp"
#define L_tmpnam FILENAME_MAX
#define TMP_MAX 26
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

type FILE as any

type fpos_t as long

extern "c"
extern stdin alias "stdin" as FILE ptr
extern stdout alias "stdout" as FILE ptr
extern stderr alias "stderr" as FILE ptr

declare function snprintf (byval s as zstring ptr, byval n as size_t, byval format as zstring ptr, ...) as long
declare function vsnprintf (byval s as zstring ptr, byval n as size_t, byval format as zstring ptr, byval arg as va_list) as long
declare function popen (byval as zstring ptr, byval as zstring ptr) as FILE ptr
declare function pclose (byval as FILE ptr) as long
declare function getw (byval as FILE ptr) as long
declare function putw (byval as long, byval as FILE ptr) as long
end extern

#endif

'' end of crt/xbox/stdio.bi
