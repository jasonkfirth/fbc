#ifndef __crt_haiku_stdio_bi__
#define __crt_haiku_stdio_bi__

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2
#define BUFSIZ 1024
#define FILENAME_MAX 1024
#define FOPEN_MAX 20
#define P_tmpdir "/tmp"
#define L_tmpnam 1024
#define TMP_MAX 308915776

type FILE as _sFILE

extern "c"
    extern stdin  alias "stdin"  as FILE ptr
    extern stdout alias "stdout" as FILE ptr
    extern stderr alias "stderr" as FILE ptr

    declare function snprintf (byval s as zstring ptr, byval n as size_t, byval format as zstring ptr, ...) as long
    declare function vsnprintf (byval s as zstring ptr, byval n as size_t, byval format as zstring ptr, byval arg as va_list) as long
    declare function popen (byval as zstring ptr, byval as zstring ptr) as FILE ptr
    declare function pclose (byval as FILE ptr) as long

end extern

type fpos_t as longint

#endif
