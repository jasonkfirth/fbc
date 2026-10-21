'' Project: FreeBASIC test suite
'' File: class-reserved.bas
'' Purpose: Verify CLASS remains reserved but unimplemented.
'' Responsibilities: Reject a CLASS declaration instead of accepting a stub.
'' This file intentionally does NOT contain: class semantics.
' TEST_MODE : COMPILE_ONLY_FAIL

Class UnsupportedClass
End Class

'' end of class-reserved.bas
