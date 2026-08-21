''
'' FreeBASIC CRT fundamental types for AROS
'' -----------------------------------------
''
'' File: crt/stddef.bi
''
'' Purpose:
''
''     Define the fundamental C types used by every AROS CRT declaration.
''
'' Responsibilities:
''
''     - model the AROS ILP32 and LP64 pointer-sized integer types
''     - expose the unsigned UCS-4 wchar_t and wint_t ABI
''     - provide the standard NULL constant
''
'' This file intentionally does NOT contain:
''
''     - declarations for other operating systems
''     - AROS structures owned by individual C headers
''     - architecture baseline or instruction-selection policy
''

#ifndef __crt_stddef_bi__
#define __crt_stddef_bi__

'' ---------------------------------------------------------------------------
'' Pointer-sized C types
'' ---------------------------------------------------------------------------

#ifdef __FB_64BIT__
	type ptrdiff_t as integer alias "long"
	type size_t as uinteger alias "long"
	#ifndef ssize_t
		type ssize_t as integer alias "long"
	#endif
#else
	type ptrdiff_t as integer
	type size_t as uinteger
	#ifndef ssize_t
		type ssize_t as integer
	#endif
#endif

'' ---------------------------------------------------------------------------
'' Wide-character types
'' ---------------------------------------------------------------------------

#ifndef wchar_t
	type wchar_t as ulong
#endif

#ifndef wint_t
	type wint_t as ulong
#endif

#ifndef NULL
#define NULL 0
#endif

#endif

'' end of crt/stddef.bi
