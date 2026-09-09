/'
    FreeBASIC Runtime Tests
    File: string/vb-helpers.bas
    Purpose: Exercise the optional VB-style byte-string API.
    Responsibilities: Semantics, embedded bytes, aliasing and temporary lifetime.
    This file does not test Unicode collation or the Optical parser.
'/

#include "fbcunit.bi"
#include once "vbcompat.bi"

SUITE( fbc_tests.string_.vb_helpers )

	TEST( comparison )
		CU_ASSERT_EQUAL( StrComp( "", "" ), 0 )
		CU_ASSERT_EQUAL( StrComp( "", "a" ), -1 )
		CU_ASSERT_EQUAL( StrComp( "a", "" ), 1 )
		CU_ASSERT_EQUAL( StrComp( "abc", "abcd" ), -1 )
		CU_ASSERT_EQUAL( StrComp( "z", "a" ), 1 )
		CU_ASSERT_EQUAL( StrComp( "ABC", "abc", vbBinaryCompare ), -1 )
		CU_ASSERT_EQUAL( StrComp( "ABC", "abc", vbTextCompare ), 0 )
		CU_ASSERT_EQUAL( StrComp( "[", "a", fbTextCompare ), -1 )
		CU_ASSERT_EQUAL( StrComp( Chr(255), Chr(128) ), 1 )
		CU_ASSERT_EQUAL( StrComp( "a" & Chr(0) & "b", "a" & Chr(0) & "c" ), -1 )
		CU_ASSERT_EQUAL( StrComp( Chr(192), Chr(224), fbTextCompare ), -1 )
	END_TEST

	TEST( replacement )
		CU_ASSERT_EQUAL( Replace( "abcabc", "ab", "x" ), "xcxc" )
		CU_ASSERT_EQUAL( Replace( "aaaaa", "aa", "b" ), "bba" )
		CU_ASSERT_EQUAL( Replace( "abc", "b", "bbb" ), "abbbc" )
		CU_ASSERT_EQUAL( Replace( "abcabc", "ab", "", 1, 1 ), "cabc" )
		CU_ASSERT_EQUAL( Replace( "aaa", "a", "" ), "" )
		CU_ASSERT_EQUAL( Replace( "abc", "longer", "x" ), "abc" )
		CU_ASSERT_EQUAL( Replace( "abc", "", "x" ), "abc" )
		CU_ASSERT_EQUAL( Replace( "abc", "b", "x", 1, 0 ), "abc" )
		CU_ASSERT_EQUAL( Replace( "abcabc", "a", "x", 4 ), "xbc" )
		CU_ASSERT_EQUAL( Replace( "abcabc", "a", "x", 4, 0 ), "abc" )
		CU_ASSERT_EQUAL( Replace( "abcabc", "", "x", 4 ), "abc" )
		CU_ASSERT_EQUAL( Replace( "abc", "c", "X", 3 ), "X" )
		CU_ASSERT_EQUAL( Replace( "abc", "a", "x", 4 ), "" )
		CU_ASSERT_EQUAL( Replace( "", "a", "x" ), "" )
		CU_ASSERT_EQUAL( Replace( "AbCaBc", "abc", "x", 1, -1, vbTextCompare ), "xx" )
		CU_ASSERT_EQUAL( Replace( "AbCaBc", "abc", "x" ), "AbCaBc" )
		CU_ASSERT_EQUAL( Replace( "a" & Chr(0) & "b", Chr(0), Chr(0, 255) ), "a" & Chr(0, 255) & "b" )
	END_TEST

	TEST( reverse_bytes )
		CU_ASSERT_EQUAL( StrReverse( "" ), "" )
		CU_ASSERT_EQUAL( StrReverse( "a" ), "a" )
		CU_ASSERT_EQUAL( StrReverse( "abcd" ), "dcba" )
		CU_ASSERT_EQUAL( StrReverse( Chr(0, 128, 255) ), Chr(255, 128, 0) )
	END_TEST

	TEST( errors )
		dim result as long = StrComp( "a", "b", -1 )
		dim saved_error as long = Err
		CU_ASSERT_EQUAL( result, 0 )
		CU_ASSERT_EQUAL( saved_error, 1 )
		result = StrComp( "a", "b", 2 )
		saved_error = Err
		CU_ASSERT_EQUAL( result, 0 )
		CU_ASSERT_EQUAL( saved_error, 1 )
		dim text as string = Replace( "abc", "a", "b", 0 )
		saved_error = Err
		CU_ASSERT_EQUAL( text, "" )
		CU_ASSERT_EQUAL( saved_error, 1 )
		text = Replace( "abc", "a", "b", 1, -2 )
		saved_error = Err
		CU_ASSERT_EQUAL( text, "" )
		CU_ASSERT_EQUAL( saved_error, 1 )
		text = Replace( "abc", "a", "b", 1, -1, 2 )
		saved_error = Err
		CU_ASSERT_EQUAL( text, "" )
		CU_ASSERT_EQUAL( saved_error, 1 )
		text = Replace( "abc", "a", "b" )
		saved_error = Err
		CU_ASSERT_EQUAL( text, "bbc" )
		CU_ASSERT_EQUAL( saved_error, 0 )
	END_TEST

	TEST( ownership )
		dim text as string = "abc"
		CU_ASSERT_EQUAL( Replace( text, text, text ), "abc" )
		CU_ASSERT_EQUAL( StrComp( text, text ), 0 )
		CU_ASSERT_EQUAL( text, "abc" )
		text = StrReverse( text )
		CU_ASSERT_EQUAL( text, "cba" )
		text = Replace( text, "b", text )
		CU_ASSERT_EQUAL( text, "ccbaa" )

		'' More calls than the runtime's temporary descriptor pool can hold.
		'' Nested results, BYREF conversions and discarded results must clean up.
		for i as integer = 1 to 1024
			text = Replace( StrReverse( "cba" ), Chr(98), StrReverse( "yx" ) )
			CU_ASSERT_EQUAL( text, "axyc" )
			CU_ASSERT_EQUAL( StrComp( StrReverse( "ABC" ), "cba", fbTextCompare ), 0 )
			Replace( "discard", "a", "aa" )
			StrReverse( "discard" )
		next
		dim fixed_text as string * 4 = "Ab"
		dim zero_text as zstring * 5 = "ab"
		CU_ASSERT_EQUAL( StrComp( fixed_text, "ab  ", fbTextCompare ), 0 )
		CU_ASSERT_EQUAL( Replace( zero_text, "b", "c" ), "ac" )
	END_TEST

END_SUITE

' end of string/vb-helpers.bas
