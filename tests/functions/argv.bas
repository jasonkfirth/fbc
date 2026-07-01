' TEST_MODE : COMPILE_AND_RUN_OK

' Test arguments to main().
' A better test of COMMAND would be to actually call with some arguments...

#if defined(__FB_WIN32__) or defined(__FB_CYGWIN__)
	#include "windows.bi"
#endif

ASSERT( __FB_ARGC__ = 1 )

'' Test argv[0] against __FILE__
dim as string expected_exe_1 = __FILE__
expected_exe_1 = left( expected_exe_1, len( expected_exe_1 ) - 4 )  ' Remove .bas
#if defined(__FB_WIN32__) or defined(__FB_CYGWIN__)
	dim as string expected_exe_with_ext = expected_exe_1 + ".exe"
	dim as string expected_exe_invoked = expected_exe_1

#ifndef __FB_CYGWIN__
	expected_exe_1 += ".exe"
#else
	expected_exe_invoked = "./" + expected_exe_1
#endif

	'' Also test argv[0] against full exe path + name
	'' Windows (or MSYS2, or make?) passes the full absolute path in argv[0],
	'' even though the makefile invokes the test .exe with relative path.
	dim expected_exe_2 as zstring * (MAX_PATH + 1)
	GetModuleFileName( GetModuleHandle( NULL ), @expected_exe_2, sizeof( expected_exe_2 ) - 1 )
#if defined(__FB_CYGWIN__)
	for i as integer = 0 to len( expected_exe_2 ) - 1
		if( expected_exe_2[i] = asc( "\" ) ) then
			expected_exe_2[i] = asc( "/" )
		end if
	next
#endif
	ASSERT( len( expected_exe_2 ) >= len( expected_exe_1 ) )
	ASSERT( right( expected_exe_2, len( expected_exe_with_ext ) ) = expected_exe_with_ext )

	ASSERT( (*__FB_ARGV__[0] = expected_exe_1) or _
	        (*__FB_ARGV__[0] = expected_exe_with_ext) or _
	        (*__FB_ARGV__[0] = expected_exe_invoked) or _
	        (*__FB_ARGV__[0] = expected_exe_2) )
	ASSERT( (command( 0 ) = expected_exe_1) or _
	        (command( 0 ) = expected_exe_with_ext) or _
	        (command( 0 ) = expected_exe_invoked) or _
	        (command( 0 ) = expected_exe_2) )
#elseif defined( __FB_DOS__ )
#if ENABLE_CHECK_BUGS
	'' This depends on what kind of DOS is being run, in a bash like shell, etc.
	'' On DOS and Win98, should work
	'' On WinXP, there's confusion between short and long filenames.
	'' Just skip the test on DOS
	expected_exe_1 += ".exe"

	ASSERT( right( *__FB_ARGV__[0], len(expected_exe_1) ) = expected_exe_1 )
	ASSERT( right( command( 0 ), len(expected_exe_1) )    = expected_exe_1 )
#endif
#elseif defined( __FB_NUTTX__ )
	'' NuttX smoke tests are built as named builtin applications. The program
	'' name comes from NSH's builtin command table rather than from the host
	'' source path used in __FILE__.
	ASSERT( __FB_ARGV__[0] <> 0 )
	ASSERT( len( *__FB_ARGV__[0] ) > 0 )
	ASSERT( command( 0 ) = *__FB_ARGV__[0] )
#else
	ASSERT( *__FB_ARGV__[0] = expected_exe_1 )
	ASSERT( command( 0 )    = expected_exe_1 )
#endif


ASSERT( __FB_ARGV__[1] = 0 )
ASSERT( command( 1 ) = "" )
ASSERT( command( -1 ) = "" )  ' whole commandline
