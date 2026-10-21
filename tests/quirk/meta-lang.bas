'' Project: FreeBASIC test suite
'' File: meta-lang.bas
'' Purpose: Verify the legacy $LANG metacommand selects a dialect.
'' Responsibilities: Use an FBLite-only option after selecting that dialect.
'' This file intentionally does NOT contain: a #LANG directive.
' TEST_MODE : COMPILE_AND_RUN_OK

'$LANG: "fblite"

Option Base 1
Dim values(3) As Integer
If LBound(values) <> 1 Then End 1
End 0

'' end of meta-lang.bas
