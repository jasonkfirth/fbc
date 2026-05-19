''
''
'' stdio -- DragonFly CRT declarations
''
''
#ifndef __crt_dragonfly_stdio_bi__
#define __crt_dragonfly_stdio_bi__

#include once "crt/sys/types.bi"

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2
#define BUFSIZ 1024
#define FILENAME_MAX 1024
#define FOPEN_MAX 20
#define P_tmpdir "/tmp/"
#define L_tmpnam 1024
#define TMP_MAX 308915776

type FILE as any
type fpos_t as __off_t

extern __stdinp alias "__stdinp" as FILE ptr
extern __stdoutp alias "__stdoutp" as FILE ptr
extern __stderrp alias "__stderrp" as FILE ptr

#define stdin __stdinp
#define stdout __stdoutp
#define stderr __stderrp

extern "c"
declare function snprintf (byval s as zstring ptr, byval n as size_t, byval format as zstring ptr, ...) as long
declare function vsnprintf (byval s as zstring ptr, byval n as size_t, byval format as zstring ptr, byval arg as va_list) as long
declare function popen (byval as zstring ptr, byval as zstring ptr) as FILE ptr
declare function pclose (byval as FILE ptr) as long
declare function getw (byval as FILE ptr) as long
declare function putw (byval as long, byval as FILE ptr) as long

end extern

#endif
