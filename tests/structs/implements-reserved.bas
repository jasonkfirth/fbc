'' Project: FreeBASIC test suite
'' File: implements-reserved.bas
'' Purpose: Verify the reserved IMPLEMENTS keyword remains unimplemented.
'' Responsibilities: Reject an attempted interface implementation.
'' This file intentionally does NOT contain: a working interface model.
' TEST_MODE : COMPILE_ONLY_FAIL

Type UnsupportedInterface
	Value As Integer
End Type

Type UnsupportedImplementation Implements UnsupportedInterface
	Value As Integer
End Type

'' end of implements-reserved.bas
