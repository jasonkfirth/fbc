'' Project: FreeBASIC test suite
'' File: any-byref.bas
'' Purpose: Verify BYREF ANY prototypes accept mismatched argument types.
'' Responsibilities: Call a typed definition through an untyped declaration.
'' This file intentionally does NOT contain: raw memory inspection.

#include "fbcunit.bi"

Dim Shared fbcTestsAnyParameterCalls As Integer

Declare Sub fbcTestsAnyParameter(ByRef declaredValue As Any)

Private Sub fbcTestsAnyParameter(ByRef definedValue As Integer)
	fbcTestsAnyParameterCalls += 1
End Sub

SUITE( fbc_tests.functions.any_byref )

	TEST( declaration_disables_argument_type_check )
		Dim As Double value = 3.5
		fbcTestsAnyParameter( value )
		CU_ASSERT_EQUAL( fbcTestsAnyParameterCalls, 1 )
		CU_ASSERT_EQUAL( value, 3.5 )
	END_TEST

END_SUITE

'' end of any-byref.bas
