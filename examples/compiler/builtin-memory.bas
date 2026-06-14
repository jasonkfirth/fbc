''
'' FreeBASIC compiler builtin example
''
'' Demonstrates GCC/clang memory and string builtins through inc/builtin.bi.
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

dim as zstring * 32 text_buffer
dim as const zstring ptr source_text = @"compiler builtins"

__builtin_memset( @text_buffer, 0, sizeof( text_buffer ) )
__builtin_memcpy( @text_buffer, source_text, __builtin_strlen( source_text ) + 1 )

if( __builtin_strcmp( @text_buffer, source_text ) <> 0 ) then
	end 1
end if

if( __builtin_strstr( @text_buffer, @"builtins" ) = 0 ) then
	end 1
end if

if( __builtin_memchr( @text_buffer, asc( " " ), __builtin_strlen( @text_buffer ) ) = 0 ) then
	end 1
end if

' end of builtin-memory.bas
