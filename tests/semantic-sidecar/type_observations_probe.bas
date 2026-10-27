' Project: FreeBASIC semantic sidecar tests
' File: type_observations_probe.bas
' Purpose: Preserve written callback structure and selected native byte inputs.
' Responsibilities: Alias boundaries, callback arrays and folded decoder sites.
' This file intentionally does NOT score types or execute function pointers.

Type ScoreCallback As Function(ByVal As Integer) As Integer
Dim CallbackSlot As Function(ByVal As Integer) As Integer
Dim CallbackSlots(0 To 1) As Function(ByVal As Integer) As Integer
Dim AliasSlot As ScoreCallback
Dim PackedValue As String = MKI<32>(17)
Dim DecodedValue As Long = CVI<32>("ABCD")
' end of type_observations_probe.bas
