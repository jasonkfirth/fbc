''
'' stdio -- Wii CRT stdio declarations
''
'' The Wii target uses devkitPPC/libogc, which provides the standard C
'' stdio API through newlib.  FreeBASIC only needs the opaque FILE type,
'' the standard stream symbols, and the platform constants here; the
'' common crt/stdio.bi wrapper declares the portable stdio functions.
''
#ifndef __crt_wii_stdio_bi__
#define __crt_wii_stdio_bi__

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2
#define BUFSIZ 1024
#define FILENAME_MAX 1024
#define FOPEN_MAX 20
#define P_tmpdir "/tmp"
#define L_tmpnam FILENAME_MAX
#define TMP_MAX 26
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

type FILE
	__opaque(0 to 111) as ubyte
end type

type fpos_t as long

extern "c"
extern __sf alias "__sf" as FILE

declare function snprintf (byval s as zstring ptr, byval n as size_t, byval format as zstring ptr, ...) as long
declare function vsnprintf (byval s as zstring ptr, byval n as size_t, byval format as zstring ptr, byval arg as va_list) as long
declare function popen (byval as zstring ptr, byval as zstring ptr) as FILE ptr
declare function pclose (byval as FILE ptr) as long
declare function getw (byval as FILE ptr) as long
declare function putw (byval as long, byval as FILE ptr) as long
end extern

#define stdin cptr(FILE ptr, @__sf)
#define stdout (cptr(FILE ptr, @__sf) + 1)
#define stderr (cptr(FILE ptr, @__sf) + 2)

#endif

'' end of crt/wii/stdio.bi
