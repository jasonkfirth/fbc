'' Project: FreeBASIC tests
'' File: ustring-fixed-invalid.bas
'' Purpose: Reject byte-sized fixed declarations of a UTF-8 descriptor.
'' Responsibilities: Check the diagnostic for unsupported fixed USTRING storage.
'' This file intentionally does NOT contain runtime tests.

' TEST_MODE : COMPILE_ONLY_FAIL
dim as ustring * 8 text

'' end of ustring-fixed-invalid.bas
