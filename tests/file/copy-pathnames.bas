'' FreeBASIC runtime tests: copy-pathnames.bas
'' Verify UTF-8 path handling, source-alias protection and overwrite length.
'' An exclusive fixture owns its files; no user files or metadata are changed.

#include once "file.bi"
#include "fbcunit.bi"

SUITE( fbc_tests.file_.copy_pathnames )
#if defined(__FB_UNIX__) or defined(__FB_WIN32__) or defined(__FB_CYGWIN__)
	TEST( copy_files )
		const root_path = "fb-copy-pathnames-test.tmp"
		dim as long status = mkdir( root_path )
		CU_ASSERT_EQUAL( status, 0 )
		if status <> 0 then exit sub
		dim as string source = root_path & "/" & chr( &hC3, &hA9 ) & ".txt"
		dim as string destination = root_path & "/" & chr( &hE6, &h97, &hA5 ) & ".txt"
		dim as string contents = "source" & chr( 0 ) & string( 33000, "x" )
		dim as integer file_number = freefile
		status = open( source for binary as #file_number )
		CU_ASSERT_EQUAL( status, 0 )
		if status = 0 then
			CU_ASSERT_EQUAL( put( #file_number, , contents ), 0 )
			CU_ASSERT_EQUAL( close( #file_number ), 0 )
			CU_ASSERT( FileCopy( source, source ) <> 0 )
			CU_ASSERT_EQUAL( FileLen( source ), len( contents ) )
			CU_ASSERT_EQUAL( FileCopy( source, destination ), 0 )
			CU_ASSERT_EQUAL( FileLen( destination ), len( contents ) )
			status = open( destination for append as #file_number )
			CU_ASSERT_EQUAL( status, 0 )
			if status = 0 then
				print #file_number, "discard this old tail"
				CU_ASSERT_EQUAL( close( #file_number ), 0 )
			end if
			CU_ASSERT_EQUAL( FileCopy( source, destination ), 0 )
			status = open( destination for binary as #file_number )
			CU_ASSERT_EQUAL( status, 0 )
			if status = 0 then
				dim as string actual = input( lof( file_number ), file_number )
				CU_ASSERT_EQUAL( actual, contents )
				CU_ASSERT_EQUAL( close( #file_number ), 0 )
			end if
			CU_ASSERT_EQUAL( kill( destination ), 0 )
			CU_ASSERT_EQUAL( kill( source ), 0 )
		end if
		CU_ASSERT_EQUAL( rmdir( root_path ), 0 )
	END_TEST
#endif
END_SUITE

'' end of copy-pathnames.bas
