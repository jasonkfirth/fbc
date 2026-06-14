' TEST_MODE : COMPILE_AND_RUN_OK

'
' builtin.bi C backend regression test
'
' The declarations must match GCC/clang builtin prototypes closely enough that
' the C compiler accepts them as builtins, not as external fallback symbols.
'

#cmdline "-gen gcc"
#cmdline "-Wc -Wno-unknown-warning-option"
#cmdline "-Wc -Werror=builtin-declaration-mismatch"
#cmdline "-restart"

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

' end of builtin.bas
