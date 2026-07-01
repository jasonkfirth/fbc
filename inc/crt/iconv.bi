#pragma once

#if (not defined(__FB_LINUX__)) and (not defined(__FB_NUTTX__))
	#error "Target platform not supported; this is the header for POSIX iconv implementations."
#endif

extern "C"

const _ICONV_H = 1
type iconv_t as any ptr
declare function iconv_open(byval __tocode as const zstring ptr, byval __fromcode as const zstring ptr) as iconv_t
declare function iconv(byval __cd as iconv_t, byval __inbuf as zstring ptr ptr, byval __inbytesleft as uinteger ptr, byval __outbuf as zstring ptr ptr, byval __outbytesleft as uinteger ptr) as uinteger
declare function iconv_close(byval __cd as iconv_t) as long

end extern
