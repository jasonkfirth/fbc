' Project: FreeBASIC semantic sidecar tests
' File: procedure-linkage.bas
' Purpose: Separate accepted declaration linkage from CDECL call convention.
' Responsibilities: Foreign prototypes, later bodies and ordinary BASIC aliases.
' This file intentionally does NOT call untyped arguments or foreign libraries.
#lang "fb"

Extern "C"
    Declare Function BoundC(ByVal fixedValue As Long, ...) As Long
End Extern
Function BoundC CDecl(ByVal fixedValue As Long, ...) As Long
    Return fixedValue
End Function

Extern "C++"
    Declare Function BoundCpp(ByVal fixedValue As Long, ...) As Long
End Extern
Function BoundCpp CDecl(ByVal fixedValue As Long, ...) As Long
    Return fixedValue
End Function

Function BasicVariadic CDecl(ByVal fixedValue As Long, ...) As Long
    Return fixedValue
End Function
Function AliasedBasic CDecl Alias "c_style_name"(ByVal fixedValue As Long, ...) As Long
    Return fixedValue
End Function
Declare Function PrototypeOnly CDecl(ByVal fixedValue As Long, ...) As Long
Declare Function FixedCdecl CDecl(ByVal fixedValue As Long) As Long
Function FixedCdecl CDecl(ByVal fixedValue As Long) As Long
    Return fixedValue
End Function

' end of procedure-linkage.bas
