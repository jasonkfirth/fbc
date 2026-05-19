''
''
'' stdio -- NetBSD CRT declarations
''
''
#ifndef __crt_netbsd_stdio_bi__
#define __crt_netbsd_stdio_bi__

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

type fpos_t
	_pos as __off_t
	_mbstate_in as __mbstate_t
	_mbstate_out as __mbstate_t
end type

type __sbuf
	_base as ubyte ptr
	_size as long
end type

type _sFILE
	_p as ubyte ptr
	_r as long
	_w as long
	_flags as ushort
	_file as short
	_bf as __sbuf
	_lbfsize as long
	_cookie as any ptr
	_close as function cdecl(byval as any ptr) as long
	_read as function cdecl(byval as any ptr, byval as any ptr, byval as size_t) as ssize_t
	_seek as function cdecl(byval as any ptr, byval as __off_t, byval as long) as __off_t
	_write as function cdecl(byval as any ptr, byval as const any ptr, byval as size_t) as ssize_t
	_ext as __sbuf
	_up as ubyte ptr
	_ur as long
	_ubuf(0 to 2) as ubyte
	_nbuf(0 to 0) as ubyte
	_flush as function cdecl(byval as any ptr) as long
	_lb_unused(0 to (len(__sbuf) - len(any ptr)) - 1) as byte
	_blksize as long
	_offset as __off_t
end type

type FILE as _sFILE

extern __sF(0 to 2) alias "__sF" as FILE

#define stdin @__sF(0)
#define stdout @__sF(1)
#define stderr @__sF(2)

extern "c"
declare function snprintf (byval s as zstring ptr, byval n as size_t, byval format as zstring ptr, ...) as long
declare function vsnprintf (byval s as zstring ptr, byval n as size_t, byval format as zstring ptr, byval arg as va_list) as long
declare function popen (byval as zstring ptr, byval as zstring ptr) as FILE ptr
declare function pclose (byval as FILE ptr) as long
declare function getw (byval as FILE ptr) as long
declare function putw (byval as long, byval as FILE ptr) as long

end extern

#endif
