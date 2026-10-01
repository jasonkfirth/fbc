'' Project: FreeBASIC tests
'' File: ustring-index-address-invalid.bas
'' Purpose: Reject taking the address of a variable-width UTF-8 scalar.
'' Responsibilities: Keep indexed text access out of raw memory lvalues.
'' This file intentionally does NOT contain runtime tests.

' TEST_MODE : COMPILE_ONLY_FAIL
dim as ustring text = "abc"
dim as ulong ptr address = @text[0]

'' end of ustring-index-address-invalid.bas
