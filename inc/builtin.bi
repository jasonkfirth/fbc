''
'' FreeBASIC compiler builtins
''
'' This include exposes a core set of GCC/clang __builtin_* entry points to
'' FreeBASIC code using normal DECLARE statements.
''
'' Responsibilities:
''   - spell GCC/clang builtin prototypes with target C ABI types
''   - keep byte-count parameters tied to the target C size_t model
''   - keep C long parameters tied to the target C data model
''
'' This file intentionally does not contain:
''   - portable fallback implementations for non-C backends
''   - target-specific or language-specific builtins
''   - declarations for every C library builtin recognized by GCC/clang
''   - higher-level algorithms built from the builtin calls
''

#pragma once

#if (__FB_BACKEND__ <> "gcc") and (__FB_BACKEND__ <> "clang")
	#error "builtin.bi requires -gen gcc or -gen clang"
#endif

'' GCC and clang check builtin declarations against the target C ABI.  On LP64
'' targets both C long and size_t are based on long, while Win64 keeps C long at
'' 32bit and uses unsigned long long for size_t.
type __fb_builtin_bool as boolean alias "_Bool"

#if defined( __FB_64BIT__ ) and defined( __FB_UNIX__ )
	type __fb_builtin_clong as integer alias "long"
	type __fb_builtin_culong as uinteger alias "long"
	type __fb_builtin_size_t as uinteger alias "long"
	type __fb_builtin_uint64_t as uinteger alias "long"
#else
	type __fb_builtin_clong as long alias "long"
	type __fb_builtin_culong as ulong alias "long"
	type __fb_builtin_size_t as uinteger
	type __fb_builtin_uint64_t as ulongint
#endif

extern "C"

'' Memory and string builtins mirror the C library prototypes but use the
'' compiler's builtin names.  GCC and clang can still lower these to inline
'' operations.
#ifndef __builtin_memchr
	declare function __builtin_memchr cdecl alias "__builtin_memchr" ( byval as const any ptr, byval as long, byval as __fb_builtin_size_t ) as any ptr
#endif
#ifndef __builtin_memcmp
	declare function __builtin_memcmp cdecl alias "__builtin_memcmp" ( byval as const any ptr, byval as const any ptr, byval as __fb_builtin_size_t ) as long
#endif
#ifndef __builtin_memcpy
	declare function __builtin_memcpy cdecl alias "__builtin_memcpy" ( byval as any ptr, byval as const any ptr, byval as __fb_builtin_size_t ) as any ptr
#endif
#ifndef __builtin_memmove
	declare function __builtin_memmove cdecl alias "__builtin_memmove" ( byval as any ptr, byval as const any ptr, byval as __fb_builtin_size_t ) as any ptr
#endif
#ifndef __builtin_memset
	declare function __builtin_memset cdecl alias "__builtin_memset" ( byval as any ptr, byval as long, byval as __fb_builtin_size_t ) as any ptr
#endif
#ifndef __builtin_strlen
	declare function __builtin_strlen cdecl alias "__builtin_strlen" ( byval as const zstring ptr ) as __fb_builtin_size_t
#endif
#ifndef __builtin_strcmp
	declare function __builtin_strcmp cdecl alias "__builtin_strcmp" ( byval as const zstring ptr, byval as const zstring ptr ) as long
#endif
#ifndef __builtin_strncmp
	declare function __builtin_strncmp cdecl alias "__builtin_strncmp" ( byval as const zstring ptr, byval as const zstring ptr, byval as __fb_builtin_size_t ) as long
#endif
#ifndef __builtin_strcpy
	declare function __builtin_strcpy cdecl alias "__builtin_strcpy" ( byval as zstring ptr, byval as const zstring ptr ) as zstring ptr
#endif
#ifndef __builtin_strncpy
	declare function __builtin_strncpy cdecl alias "__builtin_strncpy" ( byval as zstring ptr, byval as const zstring ptr, byval as __fb_builtin_size_t ) as zstring ptr
#endif
#ifndef __builtin_strcat
	declare function __builtin_strcat cdecl alias "__builtin_strcat" ( byval as zstring ptr, byval as const zstring ptr ) as zstring ptr
#endif
#ifndef __builtin_strncat
	declare function __builtin_strncat cdecl alias "__builtin_strncat" ( byval as zstring ptr, byval as const zstring ptr, byval as __fb_builtin_size_t ) as zstring ptr
#endif
#ifndef __builtin_strchr
	declare function __builtin_strchr cdecl alias "__builtin_strchr" ( byval as const zstring ptr, byval as long ) as zstring ptr
#endif
#ifndef __builtin_strrchr
	declare function __builtin_strrchr cdecl alias "__builtin_strrchr" ( byval as const zstring ptr, byval as long ) as zstring ptr
#endif
#ifndef __builtin_strstr
	declare function __builtin_strstr cdecl alias "__builtin_strstr" ( byval as const zstring ptr, byval as const zstring ptr ) as zstring ptr
#endif
#ifndef __builtin_strpbrk
	declare function __builtin_strpbrk cdecl alias "__builtin_strpbrk" ( byval as const zstring ptr, byval as const zstring ptr ) as zstring ptr
#endif
#ifndef __builtin_strspn
	declare function __builtin_strspn cdecl alias "__builtin_strspn" ( byval as const zstring ptr, byval as const zstring ptr ) as __fb_builtin_size_t
#endif
#ifndef __builtin_strcspn
	declare function __builtin_strcspn cdecl alias "__builtin_strcspn" ( byval as const zstring ptr, byval as const zstring ptr ) as __fb_builtin_size_t
#endif

'' Bit-operation builtins follow GCC's family naming: no suffix is int or
'' unsigned int, l is long, and ll is long long.
#ifndef __builtin_ffs
	declare function __builtin_ffs cdecl alias "__builtin_ffs" ( byval as long ) as long
#endif
#ifndef __builtin_ffsl
	declare function __builtin_ffsl cdecl alias "__builtin_ffsl" ( byval as __fb_builtin_clong ) as long
#endif
#ifndef __builtin_ffsll
	declare function __builtin_ffsll cdecl alias "__builtin_ffsll" ( byval as longint ) as long
#endif
#ifndef __builtin_clz
	declare function __builtin_clz cdecl alias "__builtin_clz" ( byval as ulong ) as long
#endif
#ifndef __builtin_clzl
	declare function __builtin_clzl cdecl alias "__builtin_clzl" ( byval as __fb_builtin_culong ) as long
#endif
#ifndef __builtin_clzll
	declare function __builtin_clzll cdecl alias "__builtin_clzll" ( byval as ulongint ) as long
#endif
#ifndef __builtin_ctz
	declare function __builtin_ctz cdecl alias "__builtin_ctz" ( byval as ulong ) as long
#endif
#ifndef __builtin_ctzl
	declare function __builtin_ctzl cdecl alias "__builtin_ctzl" ( byval as __fb_builtin_culong ) as long
#endif
#ifndef __builtin_ctzll
	declare function __builtin_ctzll cdecl alias "__builtin_ctzll" ( byval as ulongint ) as long
#endif
#ifndef __builtin_clrsb
	declare function __builtin_clrsb cdecl alias "__builtin_clrsb" ( byval as long ) as long
#endif
#ifndef __builtin_clrsbl
	declare function __builtin_clrsbl cdecl alias "__builtin_clrsbl" ( byval as __fb_builtin_clong ) as long
#endif
#ifndef __builtin_clrsbll
	declare function __builtin_clrsbll cdecl alias "__builtin_clrsbll" ( byval as longint ) as long
#endif
#ifndef __builtin_popcount
	declare function __builtin_popcount cdecl alias "__builtin_popcount" ( byval as ulong ) as long
#endif
#ifndef __builtin_popcountl
	declare function __builtin_popcountl cdecl alias "__builtin_popcountl" ( byval as __fb_builtin_culong ) as long
#endif
#ifndef __builtin_popcountll
	declare function __builtin_popcountll cdecl alias "__builtin_popcountll" ( byval as ulongint ) as long
#endif
#ifndef __builtin_parity
	declare function __builtin_parity cdecl alias "__builtin_parity" ( byval as ulong ) as long
#endif
#ifndef __builtin_parityl
	declare function __builtin_parityl cdecl alias "__builtin_parityl" ( byval as __fb_builtin_culong ) as long
#endif
#ifndef __builtin_parityll
	declare function __builtin_parityll cdecl alias "__builtin_parityll" ( byval as ulongint ) as long
#endif

'' Byte-swap builtins use fixed-width C integer typedefs.  On LP64 targets,
'' uint64_t is usually unsigned long rather than unsigned long long.
#ifndef __builtin_bswap16
	declare function __builtin_bswap16 cdecl alias "__builtin_bswap16" ( byval as ushort ) as ushort
#endif
#ifndef __builtin_bswap32
	declare function __builtin_bswap32 cdecl alias "__builtin_bswap32" ( byval as ulong ) as ulong
#endif
#ifndef __builtin_bswap64
	declare function __builtin_bswap64 cdecl alias "__builtin_bswap64" ( byval as __fb_builtin_uint64_t ) as __fb_builtin_uint64_t
#endif

'' Object-size builtins are useful for fortify-style wrappers and low-level
'' buffer validation.  The type argument must be a compile-time C int value.
#ifndef __builtin_object_size
	declare function __builtin_object_size cdecl alias "__builtin_object_size" ( byval as const any ptr, byval as long ) as __fb_builtin_size_t
#endif
'' NetBSD pkgsrc GCC 12, OpenBSD egcc, and DragonFly GCC accept this declaration
'' but still emit an external call instead of lowering it as a builtin.
#if (not defined( __FB_NETBSD__ )) and (not defined( __FB_OPENBSD__ )) and (not defined( __FB_DRAGONFLY__ ))
	#ifndef __builtin_dynamic_object_size
		declare function __builtin_dynamic_object_size cdecl alias "__builtin_dynamic_object_size" ( byval as const any ptr, byval as long ) as __fb_builtin_size_t
	#endif
#endif

'' Fixed-type overflow helpers.  The C type-generic overflow builtins are not
'' declared here because FreeBASIC cannot express that signature as one
'' ABI-stable DECLARE.
#ifndef __builtin_sadd_overflow
	declare function __builtin_sadd_overflow cdecl alias "__builtin_sadd_overflow" ( byval as const long, byval as const long, byval as long ptr ) as __fb_builtin_bool
#endif
#ifndef __builtin_saddl_overflow
	declare function __builtin_saddl_overflow cdecl alias "__builtin_saddl_overflow" ( byval as const __fb_builtin_clong, byval as const __fb_builtin_clong, byval as __fb_builtin_clong ptr ) as __fb_builtin_bool
#endif
#ifndef __builtin_saddll_overflow
	declare function __builtin_saddll_overflow cdecl alias "__builtin_saddll_overflow" ( byval as const longint, byval as const longint, byval as longint ptr ) as __fb_builtin_bool
#endif
#ifndef __builtin_uadd_overflow
	declare function __builtin_uadd_overflow cdecl alias "__builtin_uadd_overflow" ( byval as const ulong, byval as const ulong, byval as ulong ptr ) as __fb_builtin_bool
#endif
#ifndef __builtin_uaddl_overflow
	declare function __builtin_uaddl_overflow cdecl alias "__builtin_uaddl_overflow" ( byval as const __fb_builtin_culong, byval as const __fb_builtin_culong, byval as __fb_builtin_culong ptr ) as __fb_builtin_bool
#endif
#ifndef __builtin_uaddll_overflow
	declare function __builtin_uaddll_overflow cdecl alias "__builtin_uaddll_overflow" ( byval as const ulongint, byval as const ulongint, byval as ulongint ptr ) as __fb_builtin_bool
#endif
#ifndef __builtin_ssub_overflow
	declare function __builtin_ssub_overflow cdecl alias "__builtin_ssub_overflow" ( byval as const long, byval as const long, byval as long ptr ) as __fb_builtin_bool
#endif
#ifndef __builtin_ssubl_overflow
	declare function __builtin_ssubl_overflow cdecl alias "__builtin_ssubl_overflow" ( byval as const __fb_builtin_clong, byval as const __fb_builtin_clong, byval as __fb_builtin_clong ptr ) as __fb_builtin_bool
#endif
#ifndef __builtin_ssubll_overflow
	declare function __builtin_ssubll_overflow cdecl alias "__builtin_ssubll_overflow" ( byval as const longint, byval as const longint, byval as longint ptr ) as __fb_builtin_bool
#endif
#ifndef __builtin_usub_overflow
	declare function __builtin_usub_overflow cdecl alias "__builtin_usub_overflow" ( byval as const ulong, byval as const ulong, byval as ulong ptr ) as __fb_builtin_bool
#endif
#ifndef __builtin_usubl_overflow
	declare function __builtin_usubl_overflow cdecl alias "__builtin_usubl_overflow" ( byval as const __fb_builtin_culong, byval as const __fb_builtin_culong, byval as __fb_builtin_culong ptr ) as __fb_builtin_bool
#endif
#ifndef __builtin_usubll_overflow
	declare function __builtin_usubll_overflow cdecl alias "__builtin_usubll_overflow" ( byval as const ulongint, byval as const ulongint, byval as ulongint ptr ) as __fb_builtin_bool
#endif
#ifndef __builtin_smul_overflow
	declare function __builtin_smul_overflow cdecl alias "__builtin_smul_overflow" ( byval as const long, byval as const long, byval as long ptr ) as __fb_builtin_bool
#endif
#ifndef __builtin_smull_overflow
	declare function __builtin_smull_overflow cdecl alias "__builtin_smull_overflow" ( byval as const __fb_builtin_clong, byval as const __fb_builtin_clong, byval as __fb_builtin_clong ptr ) as __fb_builtin_bool
#endif
#ifndef __builtin_smulll_overflow
	declare function __builtin_smulll_overflow cdecl alias "__builtin_smulll_overflow" ( byval as const longint, byval as const longint, byval as longint ptr ) as __fb_builtin_bool
#endif
#ifndef __builtin_umul_overflow
	declare function __builtin_umul_overflow cdecl alias "__builtin_umul_overflow" ( byval as const ulong, byval as const ulong, byval as ulong ptr ) as __fb_builtin_bool
#endif
#ifndef __builtin_umull_overflow
	declare function __builtin_umull_overflow cdecl alias "__builtin_umull_overflow" ( byval as const __fb_builtin_culong, byval as const __fb_builtin_culong, byval as __fb_builtin_culong ptr ) as __fb_builtin_bool
#endif
#ifndef __builtin_umulll_overflow
	declare function __builtin_umulll_overflow cdecl alias "__builtin_umulll_overflow" ( byval as const ulongint, byval as const ulongint, byval as ulongint ptr ) as __fb_builtin_bool
#endif

'' Branch and code-generation hints.  __builtin_expect() takes C long even on
'' targets where FB Long is not C long.
#ifndef __builtin_expect
	declare function __builtin_expect cdecl alias "__builtin_expect" ( byval as __fb_builtin_clong, byval as __fb_builtin_clong ) as __fb_builtin_clong
#endif
#ifndef __builtin_expect_with_probability
	declare function __builtin_expect_with_probability cdecl alias "__builtin_expect_with_probability" ( byval as __fb_builtin_clong, byval as __fb_builtin_clong, byval as double ) as __fb_builtin_clong
#endif
#ifndef __builtin_prefetch
	declare sub __builtin_prefetch cdecl alias "__builtin_prefetch" ( byval as const any ptr, ... )
#endif
#ifndef __builtin_trap
	declare sub __builtin_trap cdecl alias "__builtin_trap" ( )
#endif
#ifndef __builtin_unreachable
	declare sub __builtin_unreachable cdecl alias "__builtin_unreachable" ( )
#endif

end extern

'' end of builtin.bi
