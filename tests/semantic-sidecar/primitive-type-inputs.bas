' Project: fb-linter configured type policy
' File: primitive-type-inputs.bas
' Purpose: Distinguish written primitive types from aliases and inferred values.
' Responsibilities: Native modifiers, callback components, queries and macros.
' This file intentionally does NOT execute the subject or infer type tokens.
#lang "fb"
Type NumberAlias As Long ' TYPE: Long,1
Dim typed_value As Double ' TYPE: Double,1
Dim pointer_value As Const Long Ptr ' TYPE: Long,1
Dim fixed_value As String * 8 ' TYPE: String,1
Type CallbackType As Function(ByVal As Long) As Double ' TYPE: Long,1;Double,1
Declare Function read_value(ByVal As Long) As Double ' TYPE: Long,1;Double,1
Dim alias_value As NumberAlias
Var inferred_value = 1.5
Dim sized_value As Integer<32> ' TYPE: Integer,1
Dim unsigned_value As Unsigned Long ' TYPE: Long,1
Dim queried_value As UInteger = SizeOf(Double) ' TYPE: UInteger,1;Double,1
#Define DECLARE_NUMBER(item) Dim item As Long
DECLARE_NUMBER(macro_value) ' TYPE: DECLARE_NUMBER,1
#If SizeOf(Long) < 1
    #Error Invalid Long width
#EndIf
' end of primitive-type-inputs.bas
