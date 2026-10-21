'' Project: FreeBASIC test suite
'' File: runtime-assertions.bas
'' Purpose: Verify ASSERT and ASSERTWARN macro use in a running program.
'' Responsibilities: Keep ASSERT success and ASSERTWARN failure non-fatal.
'' This file intentionally does NOT contain: a failing ASSERT termination.
' TEST_MODE : COMPILE_AND_RUN_OK

Dim As Integer reached = 0

Assert( reached = 0 )
AssertWarn( reached = 1 )
reached = 1
If reached <> 1 Then End 1
End 0

'' end of runtime-assertions.bas
