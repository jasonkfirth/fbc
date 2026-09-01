''
'' FreeBASIC test suite
'' --------------------
''
'' File: file/pipe.bas
''
'' Purpose:
''
''     Verify that OPEN PIPE can read output from the target command shell.
''
'' Responsibilities:
''
''     - use syntax shared by the supported Windows command interpreters
''     - open a command-backed input stream
''     - verify that the expected directory entry is returned
''
'' This file intentionally does NOT contain:
''
''     - shell implementation tests
''     - output-pipe coverage
''     - platform pipe runtime implementation details
''

# include "fbcunit.bi"

#if (not defined( __FB_JS__ )) and (not defined( __FB_WII__ ))

SUITE( fbc_tests.file_.pipe_ )

	#ifdef __FB_WIN32__
		const filename = ".\file\pipe.bas"
	#else
		const filename = "./file/pipe.bas"
	#endif

	TEST( pipeInput )
		dim as integer pipe_file = freefile( )

		#ifdef __FB_WIN32__
			'' OPEN PIPE already asks the C runtime to launch the platform's
			'' command interpreter.  DIR /B is accepted by both COMMAND.COM on
			'' Windows 95/98/ME and CMD.EXE on the NT family.
			dim as string pipe_command = "dir /b " + filename
		#else
			dim as string pipe_command = "ls " + filename
		#endif

		if open pipe ( pipe_command for input as #pipe_file ) = 0 then

			dim as string text
			dim as integer files = 0

			do while not eof( pipe_file )
				line input #pipe_file, text
				if len( trim( text ) ) > 0 then files += 1
			loop

			CU_ASSERT_EQUAL( files, 1 )

			close #pipe_file

		else
			CU_FAIL( "file not found" )
		end if

	END_TEST

END_SUITE

#endif

'' end of file/pipe.bas
