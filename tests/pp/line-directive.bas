'' Project: FreeBASIC test suite
'' File: line-directive.bas
'' Purpose: Verify #LINE updates source line and file metadata.
'' Responsibilities: Assert the remapped values used by compiler diagnostics.
'' This file intentionally does NOT contain: runtime line reporting.
' TEST_MODE : COMPILE_ONLY_OK

#line 100 "core-line-remapping.bas"
#assert __LINE__ = 101
#assert __FILE__ = "core-line-remapping.bas"

'' end of line-directive.bas
