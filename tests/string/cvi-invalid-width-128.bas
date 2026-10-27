'' Project: FreeBASIC compiler tests
'' File: string/cvi-invalid-width-128.bas
'' Purpose: Reject a byte decoder width that has no runtime implementation.
'' Responsibilities: Normal invalid-size diagnostics before byte conversion.
'' This file intentionally does NOT reject ordinary Integer<128> declarations.
' TEST_MODE : COMPILE_ONLY_FAIL

dim decoded as integer = cvi<128>("abcdefghijklmnop")

'' end of string/cvi-invalid-width-128.bas
