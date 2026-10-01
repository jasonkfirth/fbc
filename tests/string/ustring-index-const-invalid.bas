'' Project: FreeBASIC tests
'' File: ustring-index-const-invalid.bas
'' Purpose: Reject indexed writes through a const UTF-8 descriptor.
'' Responsibilities: Verify that scalar indexing retains the parent's constness.
'' This file intentionally does NOT contain runtime tests.

' TEST_MODE : COMPILE_ONLY_FAIL
sub modify( byref text as const ustring )
	text[0] = 65
end sub

'' end of ustring-index-const-invalid.bas
