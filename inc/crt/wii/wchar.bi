''
'' wchar -- Wii/newlib CRT wide-character declarations
''
'' newlib stores multibyte conversion state as a small count plus a
'' four-byte value buffer.  Only the platform-specific types and constants
'' live here; the common crt/wchar.bi wrapper declares the shared API.
''
#ifndef __crt_wii_wchar_bi__
#define __crt_wii_wchar_bi__

#include once "crt/stddef.bi"

union mbstate_t__value
	__wch as wint_t
	__wchb(0 to 4-1) as ubyte
end union

type mbstate_t
	__count as long
	__value as mbstate_t__value
end type

#ifndef WEOF
#define WEOF cast(wint_t, -1)
#endif

#endif

'' end of crt/wii/wchar.bi
