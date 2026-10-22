' Project: FreeBASIC source observation tests
' File: hint-binding-ownership.bas
' Purpose: Keep synthetic function-pointer parameters outside source hint closure.
' Responsibilities: Named declarations, anonymous type formals and selected references.
' This file intentionally does NOT execute or link the imported function.
#lang "fb"

Extern "C"
    Declare Function ImportedProbe Alias "hint_binding_probe" (ByVal handle As Long, _
        ByVal first_offset As LongInt, ByVal second_offset As LongInt, ByVal mode As Long) As Long
End Extern

Sub ObserveCallback()
    Dim As Function(ByVal As Long, ByVal As LongInt, ByVal As LongInt, ByVal As Long) As Long callback = @ImportedProbe
    Dim As Long result = callback(0, 0, 0, 0)
End Sub

' end of hint-binding-ownership.bas
