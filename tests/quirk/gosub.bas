'' Project: FreeBASIC test suite
'' -----------------------------------------
''
'' File: gosub.bas
''
'' Purpose:
''
''     Verify GOSUB control flow in the FBLite dialect.
''
'' Responsibilities:
''
''     - check direct and nested GOSUB calls
''     - check RETURN to the caller and to an explicit label
''     - check ON GOSUB dispatch and out-of-range selectors
''
'' This file intentionally does NOT contain:
''
''     - QB line-number syntax
''     - GOSUB implementation backend expectations

#lang "fblite"
#include "fbcunit.bi"

Option GoSub

Private Sub fbcTestsComputedGosub(ByVal selector As Integer, ByRef total As Integer)
	On selector GoSub firstTarget, secondTarget
	GoTo finished

firstTarget:
	total += 1
	Return

secondTarget:
	total += 2
	Return

finished:
End Sub

Private Sub fbcTestsNestedGosub(ByRef total As Integer)
	GoSub outerTarget
	GoTo finished

outerTarget:
	total += 3
	GoSub innerTarget
afterInner:
	total += 5
	Return

innerTarget:
	total += 7
	Return afterInner

finished:
End Sub

SUITE( fbc_tests.quirk.gosub_flow )

	TEST( direct_nested_and_computed_calls )
		Dim As Integer total, selector, passValue

		For passValue = 1 To 50
			For selector = -1 To 3
				fbcTestsComputedGosub( selector, total )
			Next
			fbcTestsNestedGosub( total )
		Next

		CU_ASSERT_EQUAL( total, 50 * (3 + 15) )
	END_TEST

END_SUITE

'' end of gosub.bas
