/*
 * FreeBASIC Windows CE MIPS runtime
 * ---------------------------------
 *
 * File: wince/mips32el/setjmp.h
 *
 * Purpose:
 *
 *     Define the jump-buffer ABI implemented by the target-owned MIPS
 *     setjmp and longjmp assembly routines.
 *
 * Responsibilities:
 *
 *     - reserve one word for every software-float O32 callee-saved register
 *     - declare setjmp as a function that can return more than once
 *     - declare longjmp as a function that does not return to its caller
 *
 * This file intentionally does NOT contain:
 *
 *     - a dependency on an undocumented COREDLL jump-buffer layout
 *     - hard-float register storage
 *     - signal-mask or structured-exception state
 *     - implementations for ARM or desktop Windows
 */

#ifndef FB_WINCE_MIPS32EL_SETJMP_H
#define FB_WINCE_MIPS32EL_SETJMP_H

#include <_mingw.h>

#define FB_WINCE_MIPS_JBLEN 11

typedef int jmp_buf[FB_WINCE_MIPS_JBLEN];

#ifdef __cplusplus
extern "C" {
#endif

int __cdecl setjmp( jmp_buf environment ) __attribute__((returns_twice));
void __cdecl longjmp( jmp_buf environment, int value )
	__attribute__((noreturn));

#ifdef __cplusplus
}
#endif

#endif

/* end of wince/mips32el/setjmp.h */
