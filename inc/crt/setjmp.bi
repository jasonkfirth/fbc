''
''
'' setjmp -- header translated with help of SWIG FB wrapper
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#ifndef __crt_setjmp_bi__
#define __crt_setjmp_bi__

#ifdef __FB_WIN32__
	#if defined( __FB_ARM__ ) and defined( __FB_64BIT__ )
		#define _JBLEN 24
		#define _JBTYPE ulongint
	#elseif defined( __FB_ARM__ )
		#define _JBLEN 28
		#define _JBTYPE long
	#else
		#define _JBLEN 16
	#endif

	#if defined( __FB_64BIT__ ) and not defined( __FB_ARM__ )
		type SETJMP_FLOAT128
			Part(0 to 1) as ulongint
		end type
		#define _JBTYPE SETJMP_FLOAT128
	#elseif not defined( _JBTYPE )
		#define _JBTYPE long
	#endif

	type jmp_buf
		__opaque(0 to _JBLEN-1) as _JBTYPE
	end type

#elseif defined( __FB_DOS__ )
	type jmp_buf
		as uinteger __eax, __ebx, __ecx, __edx, __esi
		as uinteger __edi, __ebp, __esp, __eip, __eflags
		as ushort __cs, __ds, __es, __fs, __gs, __ss
		as uinteger __sigmask
		as uinteger __signum
		as uinteger __exception_ptr
		as ubyte __fpu_state(0 to 108-1)
	end type

#elseif defined( __FB_DARWIN__ )
	''
	'' Darwin defines jmp_buf as an opaque array of C int values.  Its storage
	'' is deliberately larger than the glibc register structure used below and
	'' differs between Intel and ARM targets.
	''
	#if defined( __FB_64BIT__ ) and defined( __FB_ARM__ )
		#define _JBLEN 48
	#elseif defined( __FB_ARM__ )
		#define _JBLEN 28
	#elseif defined( __FB_64BIT__ )
		#define _JBLEN 37
	#else
		#define _JBLEN 18
	#endif

	type jmp_buf
		__opaque(0 to _JBLEN-1) as long
	end type

	type sigjmp_buf
		__opaque(0 to _JBLEN) as long
	end type

#else
	#if defined( __FB_RISCV64__ )
		'' riscv64 glibc
		type __jmp_buf
			__pc		as longint
			__regs(0 to 12-1)	as longint
			__sp		as longint
			__fpregs(0 to 12-1)	as double
		end type
	#elseif defined( __FB_LOONGARCH64__ )
		'' loongarch64 glibc
		type __jmp_buf
			__pc		as longint
			__sp		as longint
			__x		as longint
			__fp		as longint
			__regs(0 to 9-1)	as longint
			__fpregs(0 to 8-1)	as double
		end type
	#elseif defined( __FB_S390X__ )
		'' s390x glibc
		type __jmp_buf
			__gregs(0 to 10-1)	as longint
			__fpregs(0 to 8-1)	as longint
		end type
	#elseif defined( __FB_PPC__ ) and defined( __FB_64BIT__ )
		'' powerpc64 glibc
		type __jmp_buf
			__opaque(0 to 64-1) as longint
		end type
	#elseif defined( __FB_PPC__ )
		'' powerpc32 glibc
		type __jmp_buf
			__opaque(0 to 64+(12*4)-1) as long
		end type
	#elseif defined( __FB_64BIT__ ) and defined( __FB_ARM__ )
		'' aarch64 glibc
		type __jmp_buf
			__opaque(0 to 22-1) as ulongint
		end type
	#elseif defined( __FB_ARM__ )
		'' arm glibc
		type __jmp_buf
			__opaque(0 to 64-1) as long
		end type
	#elseif defined( __FB_64BIT__ )
		'' x86_64 glibc
		type __jmp_buf
			__opaque(0 to 8-1) as longint
		end type
	#else
		'' x86 glibc
		type __jmp_buf
			__opaque(0 to 6-1) as long
		end type
	#endif

	#include once "crt/bits/sigset.bi"

	type jmp_buf
		__jmpbuf		as __jmp_buf
		__mask_was_saved	as long
		__saved_mask		as __sigset_t
	end type
#endif

extern "C"

#if defined( __FB_WIN32__ ) and defined( __FB_ARM__ )
declare function setjmp alias "__mingw_setjmp" (byval as jmp_buf ptr) as long
declare sub longjmp alias "__mingw_longjmp" (byval as jmp_buf ptr, byval as long)
#elseif defined( __FB_WIN32__ )
declare function setjmp alias "_setjmp" (byval as jmp_buf ptr) as long
declare sub longjmp (byval as jmp_buf ptr, byval as long)
#elseif defined( __FB_DARWIN__ )
declare function setjmp (byval as jmp_buf ptr) as long
declare sub longjmp (byval as jmp_buf ptr, byval as long)
declare function _setjmp (byval as jmp_buf ptr) as long
declare sub _longjmp (byval as jmp_buf ptr, byval as long)
declare function sigsetjmp (byval as sigjmp_buf ptr, byval as long) as long
declare sub siglongjmp (byval as sigjmp_buf ptr, byval as long)
declare sub longjmperror ()
#else
declare function setjmp (byval as jmp_buf ptr) as long
declare sub longjmp (byval as jmp_buf ptr, byval as long)
#endif

end extern

#endif

'' end of crt/setjmp.bi
