'' examples/manual/proguide/serialization-version.bas
''
'' FreeBASIC Manual: Compiler-assisted serialization
'' Purpose: Read an older application record through a stable envelope.
'' Responsibilities: Version dispatch and explicit field migration.
'' This example intentionally contains no automatic schema conversion.
'' --------

#include once "fbserialize.bi"

'' Keep this envelope definition stable across application versions.
Type Envelope
    version As ULong
    payload As String
End Type

Type PlayerV1
    player_name As String
    score As Long
End Type

Type PlayerV2
    player_name As String
    score As Long
    level As Long
End Type

Dim As PlayerV1 old_player
old_player.player_name = "Player One"
old_player.score = 42

Dim As Envelope outgoing
outgoing.version = 1
outgoing.payload = Serialize(old_player)
If Err <> 0 Then End 1
Dim As String packet = Serialize(outgoing)
If Err <> 0 Then End 1

Dim As Envelope incoming
If Deserialize(packet, incoming) = 0 Then End 1

'' Decode into the selected wire type first. The application supplies
'' defaults for added fields instead of asking the codec to guess them.
Dim As PlayerV2 current_player
Select Case incoming.version
Case 1
    Dim As PlayerV1 previous_player
    If Deserialize(incoming.payload, previous_player) = 0 Then End 1
    current_player.player_name = previous_player.player_name
    current_player.score = previous_player.score
    current_player.level = 1
Case 2
    If Deserialize(incoming.payload, current_player) = 0 Then End 1
Case Else
    Print "Unsupported application version: "; incoming.version
    End 1
End Select

Print current_player.player_name
Print "Score: "; current_player.score
Print "Level: "; current_player.level

'' end of serialization-version.bas
