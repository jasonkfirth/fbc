'' FreeBASIC date/time runtime regressions
'' File: c-format.bas
'' Purpose: Check the C formatting ABI used by DATE$ and TIME$.
'' Responsibilities: Verify promoted arguments and valid clock strings.
'' This file intentionally does NOT set the system clock or timezone.

#include "fbcunit.bi"
#include once "crt/stdio.bi"
#include once "vbcompat.bi"

SUITE( fbc_tests.datetime.c_format )

	TEST( promoted_arguments )
		dim buffer as zstring * 128
		dim count as long
		'' LONG is the C int width on both 32-bit and 64-bit FB targets.
		count = sprintf(@buffer, "%02d-%02d-%04d", clng(10), clng(16), clng(2026))
		CU_ASSERT_EQUAL( buffer, "10-16-2026" )
		CU_ASSERT_EQUAL( count, 10 )
		count = sprintf(@buffer, "%02d:%02d:%02d", clng(13), clng(46), clng(27))
		CU_ASSERT_EQUAL( buffer, "13:46:27" )
		CU_ASSERT_EQUAL( count, 8 )
		count = sprintf(@buffer, "%d %u %.2f %s", clng(-123), culng(4000000000), _
			cdbl(1.25), strptr("text"))
		CU_ASSERT_EQUAL( buffer, "-123 4000000000 1.25 text" )
		CU_ASSERT_EQUAL( count, len("-123 4000000000 1.25 text") )
	END_TEST

	TEST( clock_strings )
		dim current_date as string = date$
		dim current_time as string = time$
		CU_ASSERT_EQUAL( len(current_date), 10 )
		CU_ASSERT_EQUAL( len(current_time), 8 )
		CU_ASSERT( isdate(current_date) )
		CU_ASSERT( valint(left(current_time, 2)) >= 0 )
		CU_ASSERT( valint(left(current_time, 2)) <= 23 )
		CU_ASSERT( valint(mid(current_time, 4, 2)) <= 59 )
		CU_ASSERT( valint(right(current_time, 2)) <= 60 )
	END_TEST

END_SUITE

'' end of c-format.bas
