/'
    FreeBASIC Runtime Tests
    File: file/path-attributes.bas
    Purpose: Exercise independent pathname attribute queries and changes.
    Responsibilities: Private fixtures, error paths, and Dir cursor preservation.
    This file does not alter existing files or test every host filesystem.
'/

#include "fbcunit.bi"
#include once "vbcompat.bi"

SUITE( fbc_tests.file_.path_attributes )

	TEST( constants )
		'' vbNormal has been a Dir search mask since vbcompat.bi was added.
		CU_ASSERT_EQUAL( vbNormal, 33 )
		CU_ASSERT_EQUAL( fbNormal, 33 )
		CU_ASSERT_EQUAL( fbFileAttrNormal, 0 )
	END_TEST

#if defined(__FB_WIN32__) or defined(__FB_LINUX__) or defined(__FB_DARWIN__) or defined(__FB_FREEBSD__) or defined(__FB_CYGWIN__)
	TEST( filesystem )
		const root_path = "fb-path-attributes-test.tmp"
		const first_path = root_path & "/first.txt"
		const second_path = root_path & "/second.txt"
		'' MKDIR must succeed before creating anything; a previous or concurrent
		'' fixture belongs to its owner and must never be overwritten here.
		dim status as long = MkDir( root_path )
		CU_ASSERT_EQUAL( status, 0 )
		if status <> 0 then exit sub
		dim file_number as long = FreeFile
		status = Open( first_path for output as #file_number )
		CU_ASSERT_EQUAL( status, 0 )
		if status <> 0 then
			RmDir root_path
			exit sub
		end if
		Print #file_number, "keep these bytes"
		Close #file_number
		status = FileCopy( first_path, second_path )
		CU_ASSERT_EQUAL( status, 0 )

		dim attributes as long = GetAttr( root_path )
		dim saved_error as long = Err
		CU_ASSERT_EQUAL( saved_error, 0 )
		CU_ASSERT( (attributes and vbDirectory) <> 0 )
		CU_ASSERT_EQUAL( GetAttr( first_path ) and vbDirectory, 0 )
		dim first_entry as string = Dir( root_path & "/*.txt" )
		CU_ASSERT( first_entry <> "" )
		attributes = GetAttr( first_path )
		status = SetAttr( first_path, vbReadOnly )
		CU_ASSERT_EQUAL( status, 0 )
		CU_ASSERT_EQUAL( GetAttr( first_path ) and vbReadOnly, vbReadOnly )
		dim next_entry as string = Dir()
		CU_ASSERT( next_entry <> "" )
		CU_ASSERT( next_entry <> first_entry )
		CU_ASSERT_EQUAL( Dir(), "" )

		status = SetAttr( first_path, vbDirectory )
		CU_ASSERT_EQUAL( status, 1 )
		CU_ASSERT_EQUAL( GetAttr( first_path ) and vbReadOnly, vbReadOnly )
		status = SetAttr( first_path, fbFileAttrNormal )
		CU_ASSERT_EQUAL( status, 0 )
		CU_ASSERT_EQUAL( GetAttr( first_path ) and vbReadOnly, 0 )

#if defined(__FB_WIN32__) and not defined(__FB_CYGWIN__)
		'' UTF-8 pathname conversion must match OPEN, including the extended
		'' path prefix whose question mark is not a wildcard.
		dim unicode_path as string = root_path & "/" & Chr(&hC3, &hA9) & ".txt"
		status = Open( unicode_path for output as #file_number )
		CU_ASSERT_EQUAL( status, 0 )
		if status = 0 then
			Close #file_number
			CU_ASSERT_EQUAL( GetAttr( unicode_path ) and vbDirectory, 0 )
			dim extended_path as string = "\\?\" & CurDir & "/" & unicode_path
			CU_ASSERT_EQUAL( GetAttr( extended_path ), GetAttr( unicode_path ) )
			CU_ASSERT_EQUAL( SetAttr( extended_path, vbReadOnly ), 0 )
			CU_ASSERT_EQUAL( GetAttr( unicode_path ) and vbReadOnly, vbReadOnly )
			CU_ASSERT_EQUAL( SetAttr( unicode_path, fbFileAttrNormal ), 0 )
			CU_ASSERT_EQUAL( Kill( unicode_path ), 0 )
		end if
		attributes = GetAttr( root_path & "/*.txt" )
		saved_error = Err
		CU_ASSERT_EQUAL( attributes, -1 )
		CU_ASSERT_EQUAL( saved_error, 1 )
		status = SetAttr( first_path, vbHidden or vbSystem or vbArchive )
		CU_ASSERT_EQUAL( status, 0 )
		CU_ASSERT_EQUAL( GetAttr( first_path ), vbHidden or vbSystem or vbArchive )
		status = SetAttr( first_path, fbFileAttrNormal )
		CU_ASSERT_EQUAL( status, 0 )
		CU_ASSERT_EQUAL( GetAttr( first_path ), fbFileAttrNormal )
#else
		status = SetAttr( first_path, vbHidden )
		CU_ASSERT_EQUAL( status, 1 )
		CU_ASSERT_EQUAL( GetAttr( first_path ) and vbHidden, 0 )
		status = Name( second_path, root_path & "/.hidden" )
		CU_ASSERT_EQUAL( status, 0 )
		CU_ASSERT_EQUAL( GetAttr( root_path & "/.hidden" ) and vbHidden, vbHidden )
		status = Name( root_path & "/.hidden", second_path )
		CU_ASSERT_EQUAL( status, 0 )
#endif
		attributes = GetAttr( root_path & "/missing" )
		saved_error = Err
		CU_ASSERT_EQUAL( attributes, -1 )
		CU_ASSERT_EQUAL( saved_error, 2 )
		status = SetAttr( root_path & "/missing", fbFileAttrNormal )
		CU_ASSERT_EQUAL( status, 2 )
		attributes = GetAttr( "" )
		saved_error = Err
		CU_ASSERT_EQUAL( attributes, -1 )
		CU_ASSERT_EQUAL( saved_error, 1 )
		CU_ASSERT_EQUAL( SetAttr( "", fbFileAttrNormal ), 1 )
		CU_ASSERT_EQUAL( SetAttr( first_path, -1 ), 1 )
		CU_ASSERT_EQUAL( FileLen( first_path ), FileLen( second_path ) )
		status = Kill( first_path )
		CU_ASSERT_EQUAL( status, 0 )
		status = Kill( second_path )
		CU_ASSERT_EQUAL( status, 0 )
		status = RmDir( root_path )
		CU_ASSERT_EQUAL( status, 0 )
	END_TEST
#endif

END_SUITE

' end of file/path-attributes.bas
