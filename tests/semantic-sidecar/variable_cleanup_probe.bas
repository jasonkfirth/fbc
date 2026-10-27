' Project: FreeBASIC semantic variable cleanup
' File: variable_cleanup_probe.bas
' Purpose: Measure the exact automatic scope-cleanup decision for source variables.
' Responsibilities: Managed, fixed, aggregate, array, reference and static controls.
' This file intentionally does NOT execute its destructor or allocate large arrays.
#lang "fb"

Type PlainRecord
    ItemValue As Long
End Type

Type OwnedRecord
    ItemValue As Long
    Declare Destructor()
End Type

Destructor OwnedRecord()
    Print ItemValue
End Destructor

Sub CleanupProbe(ByVal InputValue As Long, ByRef ReferenceInput As Long)
    ' CLEANUP: InputValue=0 ReferenceInput=0 ScalarValue=0 ManagedText=1
    Dim As Long ScalarValue = InputValue
    Dim As String ManagedText = "managed"
    ' CLEANUP: FixedText=0 ZeroText=0 WideText=0 FixedNumbers=0 DynamicNumbers=1
    Dim As String * 16 FixedText = "fixed"
    Dim As ZString * 16 ZeroText
    Dim As WString * 16 WideText
    Dim As Long FixedNumbers(0 To 1)
    Dim As Long DynamicNumbers()
    ' CLEANUP: PlainValue=0 OwnedValue=1 FixedOwned=1 OwnedPointer=0 StaticText=0 AliasValue=0
    Dim As PlainRecord PlainValue
    Dim As OwnedRecord OwnedValue
    Dim As OwnedRecord FixedOwned(0 To 1)
    Dim As OwnedRecord Ptr OwnedPointer
    Static As String StaticText
    Dim ByRef As Long AliasValue = ScalarValue
    Print ScalarValue, ReferenceInput, ManagedText, FixedText, ZeroText, WideText
    Print FixedNumbers(0), PlainValue.ItemValue, OwnedValue.ItemValue, FixedOwned(0).ItemValue, OwnedPointer, StaticText, AliasValue
End Sub

' end of variable_cleanup_probe.bas
