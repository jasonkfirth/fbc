'' Project: FreeBASIC runtime ABI tests
'' File: optional-headers-aliases.bas
'' Purpose: Verify runtime aliases after adding public header overloads.
'' Responsibilities: Link and call byte-string and pathname C entry points.
'' This file intentionally does NOT contain: filesystem changes or Unicode policy.

#include "fbcunit.bi"
#include once "string.bi"
#include once "file.bi"

SUITE( fbc_tests.string_.optional_headers_aliases )

	TEST( runtime_c_aliases )
		'' OVERLOAD must not change these established runtime symbol names.
		CU_ASSERT_EQUAL( format(12.5, "0.0"), "12.5" )
		CU_ASSERT_EQUAL( StrComp("abc", "abc"), 0 )
		CU_ASSERT_EQUAL( StrReverse("abc"), "cba" )
		CU_ASSERT_EQUAL( FileExists(""), 0 )
		CU_ASSERT( FileLen("") <= 0 )
	END_TEST

END_SUITE

'' end of optional-headers-aliases.bas
