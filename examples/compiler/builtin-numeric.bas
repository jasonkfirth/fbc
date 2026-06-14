''
'' FreeBASIC compiler builtin example
''
'' Demonstrates GCC/clang numeric builtins through inc/builtin.bi.
''

#if defined( __FB_WIN32__ ) and defined( __FB_ARM__ )
	#cmdline "-gen clang"
#else
	#cmdline "-gen gcc"
#endif

'' Older GCC releases, including the GCC 4.8 shipped with Solaris 11.3, do
'' not support -Werror=builtin-declaration-mismatch.  Keep this example focused
'' on demonstrating the builtin declarations instead of requiring that newer
'' diagnostic option.

#include once "builtin.bi"

'' The checked-overflow builtins are useful, but older GCC releases do not
'' recognize them.  If the C compiler does not lower one of those names as a
'' builtin, it reaches the linker as an unresolved external symbol.  This
'' portable example uses numeric builtins that are available on older GCC too.

dim as long sum = 40 + 2

if( sum <> 42 ) then
	end 1
end if

if( __builtin_popcount( &b101010 ) <> 3 ) then
	end 1
end if

if( __builtin_bswap32( &h01020304 ) <> &h04030201 ) then
	end 1
end if

if( __builtin_expect( iif( sum = 42, 1, 0 ), 1 ) = 0 ) then
	end 1
end if

' end of builtin-numeric.bas
