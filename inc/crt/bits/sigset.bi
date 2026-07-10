''
''
'' bits\sigset -- platform signal-set storage used by CRT declarations
''
'' NOTICE: This file is part of the FreeBASIC Compiler package and can't
''         be included in other distributions without authorization.
''
''
#pragma once

#include once "crt/long.bi"

type __sig_atomic_t as long

#ifndef __sigset_t
#ifdef __FB_DARWIN__
'' Darwin represents sigset_t as a single unsigned 32-bit mask.
type __sigset_t as ulong
#else
#define _SIGSET_NWORDS (1024 \ (8 * sizeof( culong )))
type __sigset_t
	__val(0 to _SIGSET_NWORDS-1) as culong
end type
#endif
#endif

'' end of crt/bits/sigset.bi
