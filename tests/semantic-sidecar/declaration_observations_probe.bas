' Project: FreeBASIC semantic declaration observations
' File: declaration_observations_probe.bas
' Purpose: Preserve selected aliases, string indices and actual member receivers.
' Responsibilities: Pointer qualification, inherited fields and optional defaults.
' This file intentionally does NOT execute the negative string index.
#lang "fb"

Type NumericByte As Byte
Type PointerAlias As Long Ptr

Dim Shared As Long AnchorValue
Dim As NumericByte IndexValue = -1
Dim As Const PointerAlias FixedPointer = @AnchorValue
Dim As String TextValue = "abc"
Dim As Long CharacterValue = TextValue[IndexValue]
Dim As Long GenericValue = Cast(Long, IndexValue) ' CAST: ordinary
Dim As Integer<128> WideFromByte = Cast(Integer<128>, IndexValue) ' CAST: signed-wide
Dim As UInteger<128> UnsignedFromByte = Cast(UInteger<128>, IndexValue) ' CAST: unsigned-wide

Type BaseRecord
    OtherValue As Long
End Type

Type DerivedRecord Extends BaseRecord
    ExpectedValue As Long
    Declare Sub TouchOther()
End Type

Sub DerivedRecord.TouchOther()
    OtherValue += 1
End Sub

Dim Shared As DerivedRecord SharedRecord

' Implicit member writes and member-valued defaults must retain the ordinary
' expression exporter's treatment of parser work without an active range.
Sub DefaultMember(ByRef Value As Long = SharedRecord.OtherValue)
    Value += 1
End Sub

Function ReadMember(ByRef Value As DerivedRecord) As Long
    Return Value.OtherValue
End Function

' end of declaration_observations_probe.bas
