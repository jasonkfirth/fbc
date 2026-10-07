'' Project: FreeBASIC semantic sidecar tests
'' File: declaration-typing.bas
'' Purpose: Exercise parser-owned declaration types and initializer choices.
'' Responsibilities: Original signatures, accepted groups and expansion origins.
'' This file intentionally does NOT implement linter policy or infer syntax.
#lang "fblite"
DefInt A-Z
Dim implicitValue, suffixValue%, explicitValue As Integer
Dim simpleZero As Integer = 2 - 2, simpleOne As Integer = 4 - 3
Dim complexValue As Integer = 2, disabledInit As Integer = Any
Dim unsignedMaximum As ULongInt = &HFFFFFFFFFFFFFFFFULL
Dim arrayValue(0 To 1) As Integer = {0, 1}
Type SingleFieldRecord
    value As Integer
End Type
Dim recordValue As SingleFieldRecord = Type<SingleFieldRecord>(0)
Const inferredValue = 2 + 3, explicitConstant As Integer = 4
Const As Integer prefixConstant = 7
Const suffixConstant% = 8
#Define DECLARE_PAIR(n1, n2) Dim As Integer n1, n2
DECLARE_PAIR(macroFirst, macroLast)
Declare Sub Prototype(ByVal declarationValue As Integer)
Sub Prototype(ByVal bodyValue)
    Print bodyValue
End Sub
Sub Definition(implicitFormal, suffixFormal%, ByVal explicitFormal As Integer)
    Print implicitFormal, suffixFormal, explicitFormal
End Sub
Declare Sub Variadic Cdecl(ByVal tag As Integer, ...)
Type CallbackType As Sub(ByVal innerFormal As Integer)
Print implicitValue, suffixValue, explicitValue
'' end of declaration-typing.bas
