' Project: FreeBASIC semantic call provenance
' File: runtime-macro-calls.bas
' Purpose: Preserve selected RTL calls from enabled assertion macros.
' Responsibilities: ASSERT, ASSERTWARN and a nested user macro.
' This file intentionally does NOT execute the assertion subjects.
#lang "fb"
#Define VerifyValue(condition) Assert(condition)

Function CheckValue(ByVal item As Long) As Long
    Assert(item > 0)
    AssertWarn(item < 100)
    VerifyValue(item <> 7)
    Return item
End Function
' end of runtime-macro-calls.bas
