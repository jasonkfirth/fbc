#include "fbcunit.bi"

SUITE( fbc_tests.boolean_.nan_compare )

	TEST( comparisons )
		dim as double divisor = 0.0
		dim as double nanvalue = 0.0 / divisor

		CU_ASSERT_EQUAL( nanvalue = nanvalue, FALSE )
		CU_ASSERT_EQUAL( nanvalue <> nanvalue, TRUE )
		CU_ASSERT_EQUAL( nanvalue < nanvalue, FALSE )
		CU_ASSERT_EQUAL( nanvalue <= nanvalue, FALSE )
		CU_ASSERT_EQUAL( nanvalue > nanvalue, FALSE )
		CU_ASSERT_EQUAL( nanvalue >= nanvalue, FALSE )
	END_TEST

END_SUITE
