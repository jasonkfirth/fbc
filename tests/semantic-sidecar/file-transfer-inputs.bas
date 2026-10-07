' Project: FreeBASIC semantic observations
' File: file-transfer-inputs.bas
' Purpose: Retain typed GET/PUT inputs before runtime lowering.
' Responsibilities: Scalar objects, arrays, overloads and function grammar.
' This file intentionally does NOT run any file operation.
#lang "fb"

Type NativeAlias As Integer
Type RecordData
    nativeCount As NativeAlias
    textValue As String
    addressValue As Any Ptr
    flags As Short
End Type
Type FixedData
    codeValue As Long
    bufferValue As ZString * 8
End Type
Type NestedData
    child As RecordData
End Type
Dim As RecordData value, values(0 To 1)
Dim As FixedData fixedValue
Dim As NestedData nestedValue
Dim As String textValue
Dim As Long numbers(0 To 2)
Dim As Any Ptr addressValue
Put #1, , value
Get #1, , value
Put #1, , values()
Get #1, , values()
Put #1, , fixedValue
Put #1, , nestedValue
Put #1, , textValue
Put #1, , numbers()
Put #1, , addressValue
Dim As Integer statusValue = Put(1, , value)
statusValue = Get(1, , value)
Dim As Integer recordBytes = Len(RecordData)
Dim As Integer fixedBytes = Len(FixedData)
#If 0
    Put #1, , missingValue
#EndIf
' end of file-transfer-inputs.bas
