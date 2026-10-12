'' Project: FreeBASIC runtime declarations
'' File: crt/iconv.bi
'' Purpose: Declare the platform iconv API used by portable bindings.
'' Responsibilities: Select supported targets and map their exported symbols.
'' This file intentionally does not provide text-conversion helpers.

#pragma once

#if (not defined(__FB_LINUX__)) and (not defined(__FB_NUTTX__)) and (not defined(__FB_DARWIN__)) and (not defined(__FB_FREEBSD__)) and (not defined(__FB_HAIKU__))
	#error "Target platform not supported; this is the header for POSIX iconv implementations."
#endif

#ifdef __FB_DARWIN__
	'' Darwin keeps iconv in a separate system library instead of libc.
	#inclib "iconv"
#elseif defined(__FB_HAIKU__)
	'' Haiku's GNU libiconv library exports the libiconv-prefixed API.
	#inclib "iconv"
#endif

#include once "crt/stddef.bi"

extern "C"

const _ICONV_H = 1
type iconv_t as any ptr
#ifdef __FB_HAIKU__
declare function iconv_open alias "libiconv_open"(byval __tocode as const zstring ptr, byval __fromcode as const zstring ptr) as iconv_t
declare function iconv alias "libiconv"(byval __cd as iconv_t, byval __inbuf as zstring ptr ptr, byval __inbytesleft as size_t ptr, byval __outbuf as zstring ptr ptr, byval __outbytesleft as size_t ptr) as size_t
declare function iconv_close alias "libiconv_close"(byval __cd as iconv_t) as long
#else
declare function iconv_open(byval __tocode as const zstring ptr, byval __fromcode as const zstring ptr) as iconv_t
declare function iconv(byval __cd as iconv_t, byval __inbuf as zstring ptr ptr, byval __inbytesleft as size_t ptr, byval __outbuf as zstring ptr ptr, byval __outbytesleft as size_t ptr) as size_t
declare function iconv_close(byval __cd as iconv_t) as long
#endif

end extern

'' end of crt/iconv.bi
