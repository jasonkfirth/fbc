/'
    Project: FreeBASIC runtime library tests
    ----------------------------------------

    File: tests/crt/longdouble-ops.bas

    Purpose:

        Verify arithmetic and conversions for the byte-backed CLongDouble
        binding type.

    Responsibilities:

        - compare native long double values without narrowing to Double
        - exercise arithmetic, ordering, and safe integer conversions

    This file intentionally does NOT contain:

        - C runtime helper implementations
        - compiler backend tests for the C long double calling convention
'/

#include once "fbcunit.bi"
#include once "crt/longdouble.bi"

SUITE( fbc_tests.crt.longdouble )

	TEST( longdouble_size )
		#if defined( __FB_64BIT__ ) and not (defined( __FB_ARM__ ) and (defined( __FB_DARWIN__ ) or defined( __FB_WIN32__ )))
			CU_ASSERT( sizeof( clongdouble ) = 16 )
		#elseif defined( __FB_ARM__ ) or defined( __FB_MIPS32__ )
			CU_ASSERT( sizeof( clongdouble ) = 8 )
		#else
			CU_ASSERT( sizeof( clongdouble ) = 12 )
		#endif
	END_TEST

	TEST( longdouble_arithmetic )
		dim as clongdouble left = CLongDoubleFromDoubleValue( 7.25# )
		dim as clongdouble right = CLongDoubleFromDoubleValue( 2.5# )
		dim as clongdouble sum = left + right
		dim as clongdouble difference = left - right
		dim as clongdouble product = left * 2.0#
		dim as clongdouble quotient = product / 2.0#

		CU_ASSERT_EQUAL( cast( double, sum ), 9.75# )
		CU_ASSERT_EQUAL( cast( double, difference ), 4.75# )
		CU_ASSERT_EQUAL( cast( double, product ), 14.5# )
		CU_ASSERT_EQUAL( cast( double, quotient ), 7.25# )
		CU_ASSERT_EQUAL( cast( longint, -left ), -7 )
		CU_ASSERT( left > right )
		CU_ASSERT( left >= 7.25# )
		CU_ASSERT( right < left )
		CU_ASSERT( right <= 2.5# )
		CU_ASSERT( left <> right )
		CU_ASSERT( left = 7.25# )
	END_TEST

	TEST( longdouble_precision )
		#if sizeof( clongdouble ) > sizeof( double )
			dim as clongdouble precise = CLongDoubleFromUnsignedValue( 9007199254740993ull )
			dim as clongdouble rounded_as_double = CLongDoubleFromUnsignedValue( 9007199254740992ull )

			CU_ASSERT( precise > rounded_as_double )
			CU_ASSERT( precise <> rounded_as_double )
			CU_ASSERT_EQUAL( cast( ulongint, precise ), 9007199254740993ull )
		#endif
	END_TEST

	TEST( longdouble_integer_bounds )
		dim as clongdouble signed_limit = CLongDoubleFromDoubleValue( 9223372036854775808.0# )
		dim as clongdouble negative_fraction = CLongDoubleFromDoubleValue( -5.75# )
		dim as clongdouble negative = CLongDoubleFromDoubleValue( -1.0# )

		CU_ASSERT_EQUAL( cast( longint, signed_limit ), &h7fffffffffffffffll )
		CU_ASSERT_EQUAL( cast( longint, negative_fraction ), -5 )
		CU_ASSERT_EQUAL( cast( ulongint, negative ), 0ull )
	END_TEST

END_SUITE

/' end of tests/crt/longdouble-ops.bas '/
