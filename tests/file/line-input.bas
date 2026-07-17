# include "fbcunit.bi"

SUITE( fbc_tests.file_.line_input )

	type FixedStringLineInputGuard
		text as string * 4
		canary as ubyte
	end type

	type WStringLineInputGuard
		text as wstring * 5
		canary as ubyte
	end type

	TEST( bigList )
		'' FB should be able to read in big-list.txt line by line using Line Input.
		'' All 3349 lines of that file should be read.
		'' Since reading the last line may not trigger EOF yet, we can end up
		'' doing an extra unnecessary Line Input which gives an empty line --
		'' but that should be the only empty line returned.

		var f = freefile( )

		if( open( "file/big-list-eol-lf.txt", for input, as #f ) <> 0 ) then
			CU_FAIL( )
		end if

		dim as integer lines, have_empty_line
		dim ln as string
		while( eof( f ) = FALSE )
			line input #f, ln
			if( len( ln ) > 0 ) then
				CU_ASSERT( have_empty_line = FALSE )
				lines += 1
			else
				CU_ASSERT( have_empty_line = FALSE )
				have_empty_line = TRUE
			end if
		wend
		CU_ASSERT( lines = 3349 )

		close #f
	END_TEST

	TEST( lineInputDestinationCanaries )
		const TESTFILE = "file/line-input-canaries.tmp"

		scope
			var f = freefile( )
			if( open( TESTFILE, for output, as #f ) <> 0 ) then
				CU_FAIL( "could not create file " & TESTFILE )
			end if

			print #f, "ABCD"
			print #f, "WXYZ"
			close #f
		end scope

		scope
			dim as FixedStringLineInputGuard strguard
			dim as WStringLineInputGuard wstrguard

			strguard.text = "    "
			strguard.canary = &h5a
			wstrguard.text = wstr( "" )
			wstrguard.canary = &h5a

			CU_ASSERT( cast( uinteger, @strguard.canary ) = cast( uinteger, @strguard.text ) + len( strguard.text ) )
			CU_ASSERT( cast( uinteger, @wstrguard.canary ) = cast( uinteger, @wstrguard.text ) + sizeof( wstrguard.text ) )

			var f = freefile( )
			if( open( TESTFILE, for input, as #f ) <> 0 ) then
				CU_FAIL( "could not open file " & TESTFILE )
			end if

			line input #f, strguard.text
			line input #f, wstrguard.text
			close #f

			CU_ASSERT( strguard.text = "ABCD" )
			CU_ASSERT( strguard.canary = &h5a )
			CU_ASSERT( wstrguard.text = wstr( "WXYZ" ) )
			CU_ASSERT( wstrguard.canary = &h5a )
		end scope

		CU_ASSERT( kill( TESTFILE ) = 0 )
	END_TEST

END_SUITE
