' Project: FreeBASIC semantic sidecar tests
' File: special-headers.bas
' Purpose: Preserve original special-member ABI header provenance.
' Responsibilities: Constructors, destructors, operators and ordinary name sites.
' This file intentionally does NOT execute the declared procedures.
#lang "fb"
Type HeaderProbe
    storage As Long
    Declare Constructor() ' HEADER: Constructor prototype
    Declare Destructor() ' HEADER: Destructor prototype
    Declare Operator New(ByVal bytes As UInteger) As Any Ptr ' HEADER: Operator prototype
    Declare Operator New[](ByVal bytes As UInteger) As Any Ptr ' HEADER: Operator prototype
    Declare Operator Delete(ByVal storage As Any Ptr) ' HEADER: Operator prototype
    Declare Operator Delete[](ByVal storage As Any Ptr) ' HEADER: Operator prototype
    Declare Operator Cast() As Long ' HEADER: Operator prototype
    Declare Property Value() As Long ' HEADER: Value prototype
End Type
Constructor HeaderProbe() ' HEADER: Constructor definition
End Constructor
Destructor HeaderProbe() ' HEADER: Destructor definition
End Destructor
Operator HeaderProbe.New(ByVal bytes As UInteger) As Any Ptr ' HEADER: Operator definition
    Return 0
End Operator
Operator HeaderProbe.New[](ByVal bytes As UInteger) As Any Ptr ' HEADER: Operator definition
    Return 0
End Operator
Operator HeaderProbe.Delete(ByVal storage As Any Ptr) ' HEADER: Operator definition
End Operator
Operator HeaderProbe.Delete[](ByVal storage As Any Ptr) ' HEADER: Operator definition
End Operator
Operator HeaderProbe.Cast() As Long ' HEADER: Operator definition
    Return 0
End Operator
Property HeaderProbe.Value() As Long ' HEADER: Value definition
    Return 0
End Property
Sub NamedHeader() ' HEADER: NamedHeader definition
End Sub
' end of special-headers.bas
