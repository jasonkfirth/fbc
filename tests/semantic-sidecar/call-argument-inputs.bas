'' Project: FreeBASIC semantic sidecar tests
'' File: call-argument-inputs.bas
'' Purpose: Preserve selected caller expressions before argument lowering.
'' Responsibilities: Arithmetic, conversions, defaults and selected formals.
'' This file intentionally does NOT execute unresolved external procedures.
#lang "fb"

Declare Sub Receive Cdecl (ByVal amount As UInteger, ByVal spare As Integer = 0)
Declare Sub ReceivePascal Pascal (ByVal amount As Integer)
Declare Sub ReceiveVariadic Cdecl (ByVal amount As Integer, ...)
Declare Sub ReceiveOverloaded Overload (ByVal amount As Integer)
Declare Sub ReceiveOverloaded Overload (ByVal amount As String)

Type ArgumentOwner
    value As Integer
    Declare Sub Receive(ByVal amount As Integer)
End Type

Sub CheckInputs(ByVal count As Integer)
    Dim As WString * 20 wideValue = "wide"
    Dim As ArgumentOwner owner
    Receive(Len(wideValue) + 1)
    Receive(Len(wideValue) * SizeOf(wideValue[0]), 9)
    ReceivePascal(1.5)
    ReceiveVariadic(count, count + 1, 2.5)
    ReceiveOverloaded(count And 31)
    ReceiveOverloaded("text")
    owner.Receive(count + 1)
#if 0
    Receive(999)
#endif
End Sub

'' end of call-argument-inputs.bas
