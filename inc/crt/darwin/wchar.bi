''
'' FreeBASIC Darwin CRT bindings
'' -----------------------------
''
'' File: crt/darwin/wchar.bi
''
'' Purpose:
''
''     Provide the Darwin-specific public wide-character types used by the
''     common CRT declarations.
''
'' Responsibilities:
''
''     - map wchar_t and wint_t to their Darwin ABI types
''     - expose Darwin's opaque 128-byte mbstate_t object
''     - define the Darwin WEOF value
''
'' This file intentionally does NOT contain:
''
''     - common wide-character function declarations
''     - character conversion implementations
''

#ifndef __crt_darwin_wchar_bi__
#define __crt_darwin_wchar_bi__

#include once "crt/sys/types.bi"

extern "C"

type mbstate_t as __mbstate_t

#ifndef wchar_t
type wchar_t as __wchar_t
#endif

#ifndef wint_t
type wint_t as __wint_t
#endif

#ifndef WEOF
const WEOF = cast(wint_t, -1)
#endif

end extern

#endif

'' end of crt/darwin/wchar.bi
