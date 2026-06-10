''
''
'' stddef -- header translated with help of SWIG FB wrapper
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __crt_stddef_bi__
#define __crt_stddef_bi__

'' On LP64 targets, C's ptrdiff_t and size_t are long/unsigned long rather
'' than long long.  Clang checks built-in CRT declarations against that spelling.
#if defined( __FB_64BIT__ ) and defined( __FB_UNIX__ )
	type ptrdiff_t as integer alias "long"
#else
	type ptrdiff_t as integer
#endif

#ifdef __FB_DOS__
	type size_t as ulong alias "long"
	#ifndef ssize_t
	type ssize_t as long
	#endif
#elseif defined( __FB_64BIT__ ) and defined( __FB_UNIX__ )
	type size_t as uinteger alias "long"
	#ifndef ssize_t
	type ssize_t as integer alias "long"
	#endif
#else
	type size_t as uinteger
	#ifndef ssize_t
	type ssize_t as integer
	#endif
#endif

#ifndef wchar_t
#if defined(__FB_DOS__) or defined(__FB_ANDROID__)
	type wchar_t as ubyte
#elseif defined( __FB_WIN32__ ) or defined( __FB_CYGWIN__ )
	type wchar_t as ushort
#else
	type wchar_t as long
#endif
#endif

#ifndef wint_t
#if defined( __FB_CYGWIN__ )
	type wint_t as ulong
#else
	type wint_t as wchar_t
#endif
#endif

#ifndef NULL
#define NULL 0
#endif

#endif
