''
''
'' wchar -- Cygwin CRT wide-character declarations
''
#ifndef __crt_cygwin_wchar_bi__
#define __crt_cygwin_wchar_bi__

#include once "crt/stdio.bi"
#include once "crt/stdarg.bi"
#include once "crt/stddef.bi"
#include once "crt/long.bi"
#include once "crt/stdint.bi"

union mbstate_t__value
	__wch as wint_t
	__wchb(0 to 4-1) as ubyte
end union

type mbstate_t
	__count as long
	__value as mbstate_t__value
end type

#ifndef WEOF
#define WEOF &hfffffffful
#endif

extern "C"

declare function __mbrlen (byval __s as zstring ptr, byval __n as size_t, byval __ps as mbstate_t ptr) as size_t
declare function __wcstod_internal   (byval __nptr as wchar_t ptr, byval __endptr as wchar_t ptr ptr, byval __group as long) as double
declare function __wcstof_internal   (byval __nptr as wchar_t ptr, byval __endptr as wchar_t ptr ptr, byval __group as long) as single
declare function __wcstold_internal  (byval __nptr as wchar_t ptr, byval __endptr as wchar_t ptr ptr, byval __group as long) as double
declare function __wcstol_internal   (byval __nptr as wchar_t ptr, byval __endptr as wchar_t ptr ptr, byval __base as long, byval __group as long) as clong
declare function __wcstoul_internal  (byval __nptr as wchar_t ptr, byval __endptr as wchar_t ptr ptr, byval __base as long, byval __group as long) as culong
declare function __wcstoll_internal  (byval __nptr as wchar_t ptr, byval __endptr as wchar_t ptr ptr, byval __base as long, byval __group as long) as longint
declare function __wcstoull_internal (byval __nptr as wchar_t ptr, byval __endptr as wchar_t ptr ptr, byval __base as long, byval __group as long) as ulongint

end extern

#endif
