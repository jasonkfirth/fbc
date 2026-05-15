#ifndef __crt_openbsd_stdio_bi__
#define __crt_openbsd_stdio_bi__

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2
#define BUFSIZ 1024
#define FILENAME_MAX 1024
#define FOPEN_MAX 20
#define P_tmpdir "/tmp/"
#define L_tmpnam 1024
#define TMP_MAX &h7fffffff

type FILE as _sFILE
type fpos_t as off_t

type __sFstub
	_stub as clong
end type

extern __stdin alias "__stdin" as __sFstub
extern __stdout alias "__stdout" as __sFstub
extern __stderr alias "__stderr" as __sFstub

#define stdin cast(FILE ptr, @__stdin)
#define stdout cast(FILE ptr, @__stdout)
#define stderr cast(FILE ptr, @__stderr)

extern "c"
declare function snprintf (byval s as zstring ptr, byval n as size_t, byval format as zstring ptr, ...) as long
declare function vsnprintf (byval s as zstring ptr, byval n as size_t, byval format as zstring ptr, byval arg as va_list) as long
declare function popen (byval as zstring ptr, byval as zstring ptr) as FILE ptr
declare function pclose (byval as FILE ptr) as long
declare function getw (byval as FILE ptr) as long
declare function putw (byval as long, byval as FILE ptr) as long

end extern

#endif
