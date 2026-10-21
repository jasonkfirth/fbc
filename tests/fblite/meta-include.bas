'' Project: FreeBASIC test suite
'' File: meta-include.bas
'' Purpose: Verify the FBLite $INCLUDE metacommand inserts source text.
'' Responsibilities: Read a declaration provided by an included file.
'' This file intentionally does NOT contain: a #INCLUDE directive.
' TEST_MODE : COMPILE_AND_RUN_OK

#lang "fblite"
'$INCLUDE: "meta-include.bi"

If fbcTestsMetaIncludeValue <> 42 Then End 1
End 0

'' end of meta-include.bas
