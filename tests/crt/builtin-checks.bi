'' FreeBASIC Compiler Test Suite
'' File: builtin-checks.bi
'' Purpose: Check the core declarations in builtin.bi at run time.
'' Responsibilities: Exercise libc calls, bit operations, overflow results,
'' object sizes, and code-generation hints with the selected backend.
'' This file does not select a backend or test C declaration diagnostics.

#include once "../../inc/builtin.bi"

dim as zstring * 16 dst
dim as zstring * 16 tail
dim as const zstring ptr src = @"abcdef"
dim as ulongint bits = 8ull
dim as long signed_result = any
dim as ulong unsigned_result = any
dim as __fb_builtin_clong signed_long_result = any
dim as __fb_builtin_culong unsigned_long_result = any
dim as longint signed_longint_result = any
dim as ulongint unsigned_longint_result = any

__builtin_memset( @dst, 0, sizeof( dst ) )
__builtin_memcpy( @dst, src, 7 )

if( __builtin_memcmp( @dst, src, 7 ) <> 0 ) then
	end 1
end if

if( __builtin_memchr( src, asc( "c" ), 7 ) = 0 ) then
	end 1
end if

if( __builtin_strlen( src ) <> 6 ) then
	end 1
end if

if( __builtin_strcmp( src, @"abcdef" ) <> 0 ) then
	end 1
end if

if( __builtin_strncmp( src, @"abcxyz", 3 ) <> 0 ) then
	end 1
end if

__builtin_strcpy( @tail, @"abc" )
__builtin_strcat( @tail, @"def" )

if( __builtin_strcmp( @tail, src ) <> 0 ) then
	end 1
end if

__builtin_strncpy( @tail, @"xy", 3 )
__builtin_strncat( @tail, @"z123", 1 )

if( __builtin_strcmp( @tail, @"xyz" ) <> 0 ) then
	end 1
end if

if( __builtin_strchr( src, asc( "d" ) ) = 0 ) then
	end 1
end if

if( __builtin_strrchr( @"abca", asc( "a" ) ) = 0 ) then
	end 1
end if

if( __builtin_strstr( src, @"cde" ) = 0 ) then
	end 1
end if

if( __builtin_strpbrk( src, @"dx" ) = 0 ) then
	end 1
end if

if( __builtin_strspn( src, @"abc" ) <> 3 ) then
	end 1
end if

if( __builtin_strcspn( src, @"de" ) <> 3 ) then
	end 1
end if

if( __builtin_expect( 1, 1 ) = 0 ) then
	end 1
end if

if( __builtin_expect_with_probability( 1, 1, 0.9 ) = 0 ) then
	end 1
end if

if( __builtin_ffs( 8 ) <> 4 ) then
	end 1
end if

if( __builtin_ffsl( 8 ) <> 4 ) then
	end 1
end if

if( __builtin_ffsll( 8 ) <> 4 ) then
	end 1
end if

if( __builtin_clz( 1 ) <> 31 ) then
	end 1
end if

if( __builtin_clzl( 1 ) <> (sizeof( __fb_builtin_culong ) * 8) - 1 ) then
	end 1
end if

if( __builtin_clzll( 1 ) <> 63 ) then
	end 1
end if

if( __builtin_ctz( 8 ) <> 3 ) then
	end 1
end if

if( __builtin_ctzl( 8 ) <> 3 ) then
	end 1
end if

if( __builtin_ctzll( bits ) <> 3 ) then
	end 1
end if

if( __builtin_clrsb( 0 ) <= 0 ) then
	end 1
end if

if( __builtin_clrsbl( 0 ) <= 0 ) then
	end 1
end if

if( __builtin_clrsbll( 0 ) <= 0 ) then
	end 1
end if

if( __builtin_popcount( &hfull ) <> 4 ) then
	end 1
end if

if( __builtin_popcountl( &hfull ) <> 4 ) then
	end 1
end if

if( __builtin_popcountll( &hfull ) <> 4 ) then
	end 1
end if

if( __builtin_parity( &b1011 ) <> 1 ) then
	end 1
end if

if( __builtin_parityl( &b1011 ) <> 1 ) then
	end 1
end if

if( __builtin_parityll( &b1011 ) <> 1 ) then
	end 1
end if

if( __builtin_bswap16( &h1234 ) <> &h3412 ) then
	end 1
end if

if( __builtin_bswap32( &h12345678 ) <> &h78563412 ) then
	end 1
end if

if( __builtin_bswap64( &h1122334455667788ull ) <> &h8877665544332211ull ) then
	end 1
end if

if( __builtin_object_size( @dst, 0 ) < sizeof( dst ) ) then
	end 1
end if

'' NetBSD pkgsrc GCC 12, OpenBSD egcc, and DragonFly GCC accept the prototype
'' but do not lower this builtin.
#if (not defined( __FB_NETBSD__ )) and (not defined( __FB_OPENBSD__ )) and (not defined( __FB_DRAGONFLY__ ))
	if( __builtin_dynamic_object_size( @dst, 0 ) < sizeof( dst ) ) then
		end 1
	end if
#endif

if( __builtin_sadd_overflow( 1, 2, @signed_result ) ) then
	end 1
end if

if( signed_result <> 3 ) then
	end 1
end if

if( __builtin_sadd_overflow( &h7fffffff, 1, @signed_result ) = FALSE ) then
	end 1
end if

if( __builtin_saddl_overflow( 1, 2, @signed_long_result ) ) then
	end 1
end if

if( __builtin_saddll_overflow( 1, 2, @signed_longint_result ) ) then
	end 1
end if

if( __builtin_uadd_overflow( 1, 2, @unsigned_result ) ) then
	end 1
end if

if( __builtin_uaddl_overflow( 1, 2, @unsigned_long_result ) ) then
	end 1
end if

if( __builtin_uaddll_overflow( 1, 2, @unsigned_longint_result ) ) then
	end 1
end if

if( __builtin_ssub_overflow( 3, 2, @signed_result ) ) then
	end 1
end if

if( __builtin_ssubl_overflow( 3, 2, @signed_long_result ) ) then
	end 1
end if

if( __builtin_ssubll_overflow( 3, 2, @signed_longint_result ) ) then
	end 1
end if

if( __builtin_usub_overflow( 3, 2, @unsigned_result ) ) then
	end 1
end if

if( __builtin_usubl_overflow( 3, 2, @unsigned_long_result ) ) then
	end 1
end if

if( __builtin_usubll_overflow( 3, 2, @unsigned_longint_result ) ) then
	end 1
end if

if( __builtin_smul_overflow( 3, 2, @signed_result ) ) then
	end 1
end if

if( __builtin_smull_overflow( 3, 2, @signed_long_result ) ) then
	end 1
end if

if( __builtin_smulll_overflow( 3, 2, @signed_longint_result ) ) then
	end 1
end if

if( __builtin_umul_overflow( 3, 2, @unsigned_result ) ) then
	end 1
end if

if( __builtin_umull_overflow( 3, 2, @unsigned_long_result ) ) then
	end 1
end if

if( __builtin_umulll_overflow( 3, 2, @unsigned_longint_result ) ) then
	end 1
end if

__builtin_prefetch( @dst, 0, 0 )
__builtin_prefetch( @dst )
__builtin_prefetch( @dst, 1 )

'' Zero has no set bit. Redundant sign bits are defined for zero and -1,
'' unlike clz/ctz on zero, whose results are deliberately not tested.
if( __builtin_ffs( 0 ) <> 0 ) then end 2
if( __builtin_ffsl( 0 ) <> 0 ) then end 2
if( __builtin_ffsll( 0 ) <> 0 ) then end 2
if( __builtin_ffsll( &h8000000000000000ll ) <> 64 ) then end 2
if( __builtin_clrsb( -1 ) <> 31 ) then end 3
if( __builtin_clrsbl( -1 ) <> sizeof(__fb_builtin_clong) * 8 - 1 ) then end 3
if( __builtin_clrsbll( -1 ) <> 63 ) then end 3
if( __builtin_clrsb( &h80000000l ) <> 0 ) then end 3
if( __builtin_clrsb( -2 ) <> 30 ) then end 3
if( __builtin_popcountll( &hffffffffffffffffull ) <> 64 ) then end 4
if( __builtin_parityll( &hffffffffffffffffull ) <> 0 ) then end 4
if( __builtin_parityll( 0 ) <> 0 ) then end 4

'' An overflow builtin must store the wrapped value even if its boolean
'' result is unused. Signed overflow here is defined by the builtin API.
if( __builtin_sadd_overflow( &h7fffffffl, 1, @signed_result ) = FALSE ) then end 5
if( signed_result <> &h80000000l ) then end 5
if( __builtin_uaddll_overflow( &hffffffffffffffffull, 1, @unsigned_longint_result ) = FALSE ) then end 5
if( unsigned_longint_result <> 0 ) then end 5
if( __builtin_ssubll_overflow( &h8000000000000000ll, 1, @signed_longint_result ) = FALSE ) then end 6
if( signed_longint_result <> &h7fffffffffffffffll ) then end 6
if( __builtin_usub_overflow( 0, 1, @unsigned_result ) = FALSE ) then end 6
if( unsigned_result <> &hfffffffful ) then end 6
if( __builtin_smul_overflow( &h7fffffffl, 2, @signed_result ) = FALSE ) then end 7
if( signed_result <> -2 ) then end 7
__builtin_umulll_overflow( &hffffffffffffffffull, 2, @unsigned_longint_result )
if( unsigned_longint_result <> &hfffffffffffffffeull ) then end 7

if( __builtin_expect( -7, 0 ) <> -7 ) then end 8
if( __builtin_expect_with_probability( -7, 0, 0.0 ) <> -7 ) then end 8
if( __builtin_expect_with_probability( -7, 0, 1.0 ) <> -7 ) then end 8

'' A conditional pointer may use a compiler temporary. Its object size must
'' describe the selected buffer, never the pointer temporary's storage.
dim firstBuffer(0 to 63) as ubyte
dim secondBuffer(0 to 63) as ubyte
dim bufferChoice as long = val(command(1))
if( __builtin_object_size( iif(bufferChoice, @firstBuffer(0), @secondBuffer(0)), 0 ) < sizeof(firstBuffer) ) then end 11

'' libc builtins remain usable as function pointers, including in static
'' initializers. Their aliases must resolve to libc rather than __builtin_*.
dim shared copyBytes as function cdecl( byval as any ptr, byval as const any ptr, byval as __fb_builtin_size_t ) as any ptr = @__builtin_memcpy
if( copyBytes( @dst, src, 7 ) <> @dst ) then end 9
if( __builtin_memmove( cptr(ubyte ptr, @dst) + 1, @dst, 6 ) <> cptr(ubyte ptr, @dst) + 1 ) then end 9
if( dst <> "aabcdef" ) then end 9

'' These calls must end an LLVM basic block even when source statements
'' follow them. The branch is never taken during this run-time test.
if( __builtin_strlen( src ) = 0 ) then
	__builtin_trap()
	__builtin_unreachable()
	end 10
end if

'' end of builtin-checks.bi
