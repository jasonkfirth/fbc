/'
    FreeBASIC Runtime Tests
    File: file/file-exists-empty.bas
    Purpose: Reject absent filenames without dereferencing an empty string.
    Responsibilities: Empty dynamic strings, literals and null pathname pointers.
    This file does not create files or change filesystem state.
'/
#include once "file.bi"
#include "fbcunit.bi"

SUITE( fbc_tests.file_.file_exists_empty )
	TEST( empty_paths )
		dim path as string
		CU_ASSERT( FileExists( path ) = 0 )
		CU_ASSERT( FileExists( "" ) = 0 )
		CU_ASSERT( FileExists( cptr( const zstring ptr, 0 ) ) = 0 )
		CU_ASSERT( FileExists( command(0) ) <> 0 )
	END_TEST
END_SUITE

/' end of file-exists-empty.bas '/
