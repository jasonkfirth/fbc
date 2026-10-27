' Project: FreeBASIC semantic array mutation inputs
' File: array-mutation-inputs.bas
' Purpose: Retain direct array operations independently of runtime lowering.
' Responsibilities: ERASE, REDIM, preservation, macros and excluded field places.
' This file intentionally does NOT execute the subjects or infer array usefulness.
#Lang "fb"

Type ArrayHolder
    Items(0 To 1) As Long
End Type

#Define CLEAR_ITEMS(Value) Erase Value

Private Sub DirectArrays()
    Dim FixedItems(0 To 1) As Long
    Dim DynamicItems() As Long
    Erase FixedItems
    ReDim DynamicItems(0 To 2)
    ReDim Preserve DynamicItems(0 To 3)
    Erase DynamicItems
    Dim Holder As ArrayHolder
    Erase Holder.Items
End Sub

Private Sub MacroArray()
    Dim MacroItems(0 To 1) As Long
    CLEAR_ITEMS(MacroItems)
End Sub

' end of array-mutation-inputs.bas
