'' Project: FreeBASIC compiler tests
'' File: string/mki-invalid-width-128.bas
'' Purpose: Reject a byte encoder width before selecting a missing runtime call.
'' Responsibilities: Normal invalid-size diagnostics instead of a null-call crash.
'' This file intentionally does NOT reject ordinary Integer<128> declarations.
' TEST_MODE : COMPILE_ONLY_FAIL

dim encoded as string = mki<128>(1)

'' end of string/mki-invalid-width-128.bas
