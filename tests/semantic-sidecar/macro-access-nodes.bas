' Project: FreeBASIC semantic macro access tests
' File: macro-access-nodes.bas
' Purpose: Distinguish source-selected macro lvalues from generated cleanup.
' Responsibilities: String writes, field receiver reads and exact native tags.
' This file intentionally does NOT execute the subject or infer alias effects.
#Lang "fb"

#Define CHANGE_TEXT(Value) Value = "changed"
#Define CHANGE_FIELD(Value) Value.Text = "changed"

Type Holder
    Text As String
End Type

Private Sub Subject()
    Dim LocalText As String
    Dim ObjectValue As Holder
    CHANGE_TEXT(LocalText)
    CHANGE_FIELD(ObjectValue)
    Print LocalText, ObjectValue.Text
End Sub

' end of macro-access-nodes.bas
