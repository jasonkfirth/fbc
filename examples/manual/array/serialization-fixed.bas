'' examples/manual/array/serialization-fixed.bas
''
'' FreeBASIC Manual: Compiler-assisted serialization
'' Purpose: Round-trip scalar values, fixed strings and fixed arrays.
'' Responsibilities: Preserve stored bytes, array bounds and element order.
'' This example intentionally contains no dynamic records or transport code.
'' --------

#include once "fbserialize.bi"

Dim As Long original_number = -42, restored_number
Dim As String payload = Serialize(original_number)
If Err <> 0 Then End 1
If Deserialize(payload, restored_number) = 0 Then End 1
If restored_number <> original_number Then End 1
Print "Number: "; restored_number

'' A fixed string includes its stored padding, as well as embedded nulls.
Dim As String * 6 original_text, restored_text
original_text = "A" & Chr(0) & "B"
payload = Serialize(original_text)
If Err <> 0 Then End 1
If Deserialize(payload, restored_text) = 0 Then End 1
For position As Integer = 1 To Len(original_text)
    If Asc(restored_text, position) <> Asc(original_text, position) Then End 1
Next position
Print "Fixed string bytes: "; Len(restored_text)
Print "Embedded null byte: "; Asc(restored_text, 2)

'' Fixed arrays require the same element type, dimensions and bounds.
Dim original_values(-1 To 0, 2 To 4) As Long
Dim restored_values(-1 To 0, 2 To 4) As Long
Dim As Integer row, column
For row = -1 To 0
    For column = 2 To 4
        original_values(row, column) = row * 10 + column
    Next column
Next row

payload = Serialize(original_values())
If Err <> 0 Then End 1
If Deserialize(payload, restored_values()) = 0 Then End 1
For row = -1 To 0
    For column = 2 To 4
        If restored_values(row, column) <> original_values(row, column) Then End 1
    Next column
Next row
Print "Array corners: "; restored_values(-1, 2); " / "; restored_values(0, 4)

'' end of serialization-fixed.bas
