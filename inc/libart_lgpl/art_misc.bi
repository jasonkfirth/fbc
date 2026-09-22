''
''
'' art_misc -- header translated with help of SWIG FB wrapper
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __art_misc_bi__
#define __art_misc_bi__

#include once "art_config.bi"

type art_boolean as integer

#define ART_FALSE 0
#define ART_TRUE 1
#define M_PI 3.14159265358979323846
#define M_SQRT2 1.41421356237309504880

' These C APIs are variadic, so they must use FreeBASIC's C calling convention.
declare sub art_die cdecl (byval fmt as zstring ptr, ...)
declare sub art_warn cdecl (byval fmt as zstring ptr, ...)
declare sub art_dprint cdecl (byval fmt as zstring ptr, ...)

#endif
