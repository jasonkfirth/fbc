'' Project: FreeBASIC tests
'' File: ustring-index-temporary-invalid.bas
'' Purpose: Reject writes to a temporary UTF-8 function result.
'' Responsibilities: Keep scalar assignment tied to a writable descriptor.
'' This file intentionally does NOT contain runtime tests.

' TEST_MODE : COMPILE_ONLY_FAIL
function textResult() as ustring
	return "abc"
end function
textResult()[0] = 65

'' end of ustring-index-temporary-invalid.bas
