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
	#define _JBLEN 16

	#ifdef __FB_64BIT__
		type SETJMP_FLOAT128
			Part(0 to 1) as ulongint
		end type
		#define _JBTYPE SETJMP_FLOAT128
	#else
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

#ifdef __FB_WIN32__
declare function setjmp alias "_setjmp" (byval as jmp_buf ptr) as long
#else
declare function setjmp (byval as jmp_buf ptr) as long
#endif

declare sub longjmp (byval as jmp_buf ptr, byval as long)

end extern

#endif
