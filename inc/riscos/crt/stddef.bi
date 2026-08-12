''
'' FreeBASIC CRT primitive definitions for GCCSDK UnixLib
'' ------------------------------------------------------
''
'' File: crt/stddef.bi
''
'' Purpose:
''
''     Define the complete UnixLib standard primitive type interface.
''
'' Responsibilities:
''
''     - map size and pointer differences to the 32-bit ILP32 model
''     - define UnixLib's unsigned 32-bit wide-character type
''     - provide the standard NULL constant
''
'' This file intentionally does NOT contain:
''
''     - structures owned by other CRT headers
''     - compiler ABI layout rules
''     - declarations for other operating systems
''

#ifndef __crt_stddef_bi__
#define __crt_stddef_bi__

'' GCCSDK uses the 32-bit ILP32 data model.
type ptrdiff_t as integer
type size_t as uinteger

#ifndef ssize_t
type ssize_t as integer
#endif

'' UnixLib uses an unsigned 32-bit wchar_t.
#ifndef wchar_t
type wchar_t as ulong
#endif

#ifndef wint_t
type wint_t as wchar_t
#endif

#ifndef NULL
#define NULL 0
#endif

#endif

'' end of crt/stddef.bi
