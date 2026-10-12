'' examples/manual/udt/serialize.bas
''
'' FreeBASIC Manual: SERIALIZE
'' Purpose: Make an independent snapshot of a record with owned members.
'' Responsibilities: Nested records, strings, arrays and deep ownership.
'' This example intentionally contains no file or network transport.
'' --------

#include once "fbserialize.bi"

Type Item
    label_text As String
    quantity As Integer
End Type

Type PlayerState
    player_name As String
    health As Integer
    inventory(Any) As Item
End Type

Dim As PlayerState original, restored
original.player_name = "Player One"
original.health = 100
ReDim original.inventory(-2 To 0)
original.inventory(-2).label_text = "Potion"
original.inventory(-2).quantity = 3

Dim As String payload = Serialize(original)
If Err <> 0 Then
    Print "Serialize failed: "; Err
    End 1
End If

If Deserialize(payload, restored) = 0 Then
    Print "Deserialize failed: "; Err
    End 1
End If

'' The restored record owns its strings and arrays. Changing the source
'' and releasing its array and the payload do not change the restored data.
original.player_name = "Changed"
original.inventory(-2).label_text = "Used"
original.inventory(-2).quantity = 0
Erase original.inventory
payload = ""

'' Check restored bounds before accessing an element of the owned array.
If LBound(restored.inventory) <> -2 OrElse UBound(restored.inventory) <> 0 Then End 1
If restored.player_name <> "Player One" OrElse restored.health <> 100 Then End 1
If restored.inventory(-2).label_text <> "Potion" Then End 1
If restored.inventory(-2).quantity <> 3 Then End 1

Print restored.player_name
Print restored.inventory(-2).label_text
Print restored.inventory(-2).quantity

'' end of serialize.bas
