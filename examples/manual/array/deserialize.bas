'' examples/manual/array/deserialize.bas
''
'' FreeBASIC Manual: DESERIALIZE
'' Purpose: Restore a whole array and handle incomplete input.
'' Responsibilities: Bounds, embedded nulls and unchanged data on failure.
'' This example intentionally contains no transport framing.
'' --------

#include once "fbserialize.bi"

Dim source(Any) As String
Dim destination(Any) As String
ReDim source(-1 To 1)
source(-1) = "first"
source(0) = "A" & Chr(0) & "B"
source(1) = "last"

'' Empty parentheses select the whole array rather than one element.
Dim As String payload = Serialize(source())
If Err <> 0 Then
    Print "Serialize failed: "; Err
    End 1
End If

'' Start with one existing element at index 7 to make retained bounds visible.
ReDim destination(7 To 7)
destination(7) = "keep this value"

'' Missing the last byte must fail before replacing any existing data.
Dim As String incomplete = Left(payload, Len(payload) - 1)
If Deserialize(incomplete, destination()) Then End 1
If Err <> FB_SERIALIZE_INVALID Then End 1
If LBound(destination) <> 7 OrElse UBound(destination) <> 7 Then End 1
If destination(7) <> "keep this value" Then End 1
Print "Incomplete input left the destination unchanged."

If Deserialize(payload, destination()) = 0 Then
    Print "Deserialize failed: "; Err
    End 1
End If

Print "Restored bounds: "; LBound(destination); " to "; UBound(destination)
Print destination(-1); " / "; destination(1)
Print "Middle string length: "; Len(destination(0))
Print "Middle byte: "; Asc(destination(0), 2)

'' end of deserialize.bas
