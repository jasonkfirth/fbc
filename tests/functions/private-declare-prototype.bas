' TEST_MODE : COMPILE_AND_RUN_OK
' Project: FreeBASIC test suite
' File: functions/private-declare-prototype.bas
' Purpose: Keep private external prototypes matched to private definitions.
' Responsibilities: Exercise PRIVATE DECLARE visibility and CDECL procedure matching.
' This file intentionally does NOT contain cross-module visibility tests.

#if __FB_LANG__ <> "qb"
Private Declare Function c2fbPrivateDeclared CDecl (ByVal value As Long) As Long

Private Function c2fbPrivateDeclared CDecl (ByVal value As Long) As Long
    Return value + 12
End Function

If c2fbPrivateDeclared(5) <> 17 Then End 1
#endif

' end of functions/private-declare-prototype.bas
