'' Project: FreeBASIC runtime tests
'' File: print_using_end.bas
'' Purpose: Check final-value USING cleanup when a field remains unconsumed.
'' Responsibilities: String/numeric mismatches, unused fields and later output.
'' This file intentionally does NOT inspect allocator state or printer devices.

#include "fbcunit.bi"

SUITE( fbc_tests.string_.print_using_end )

	TEST( unconsumed_final_field )

		const TESTFILE = "./string/print_using_end.txt"
		dim as integer file_number = freefile()
		dim as integer open_result = open( TESTFILE for output as #file_number )
		CU_ASSERT_EQUAL( open_result, 0 )
		if( open_result <> 0 ) then exit sub

		'' Each statement emits a USING-end call after its final value. A
		'' mismatched or unused field must not leave that call a released mask.
		print #file_number, using "###"; "hello"
		print #file_number, using "!"; 7
		print #file_number, using "## !"; 7
		print #file_number, "after"
		close #file_number

		open_result = open( TESTFILE for input as #file_number )
		CU_ASSERT_EQUAL( open_result, 0 )
		if( open_result <> 0 ) then exit sub

		dim as string expected(0 to 3) = { "", "%7", " 7 ", "after" }
		for i as integer = 0 to ubound( expected )
			dim as string actual
			CU_ASSERT( eof( file_number ) = 0 )
			if( eof( file_number ) ) then exit for
			line input #file_number, actual
			CU_ASSERT_EQUAL( actual, expected(i) )
		next
		CU_ASSERT( eof( file_number ) )
		close #file_number
		kill TESTFILE

	END_TEST

END_SUITE

'' end of print_using_end.bas
