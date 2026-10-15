' Project: FreeBASIC semantic call provenance
' File: macro-calls.bas
' Purpose: Retain selected call references through macro expansion and lowering.
' Responsibilities: Self calls, overloads, nested expansions and virtual targets.
' This file intentionally does NOT execute the recursive or virtual calls.
#lang "fb"
#Macro DefineSelf()
    Sub MacroSelf()
        MacroSelf()
    End Sub
#EndMacro
DefineSelf()

Type CallReceiver Extends Object
    Declare Function Choose Overload(ByVal item As Long) As Long
    Declare Function Choose Overload(ByVal item As Double) As Long
    Declare Virtual Function Probe() As Long
End Type
Function CallReceiver.Choose(ByVal item As Long) As Long
    Return item
End Function
Function CallReceiver.Choose(ByVal item As Double) As Long
    Return CLng(item)
End Function
Function CallReceiver.Probe() As Long
    Return 0
End Function
' FB-LINTER: DISABLE-NEXT-LINE FBL-PP-001
#Define CallLong(v) ((v).Choose(CLng(1)))
' FB-LINTER: DISABLE-NEXT-LINE FBL-PP-001
#Define CallDouble(v) ((v).Choose(CDbl(1)))
#Define CallBoth(v) (CallLong(v) + CallDouble(v))
' FB-LINTER: DISABLE-NEXT-LINE FBL-PP-001
#Define CallVirtual(v) ((v).Probe())
Function MacroCalls(ByRef item As CallReceiver) As Long
    Return CallBoth(item) + CallVirtual(item)
End Function
Function PhysicalCall(ByRef item As CallReceiver) As Long
    Return item.Choose(CLng(2))
End Function
' end of macro-calls.bas
