' Project: FreeBASIC written type root observations
' File: type_macro_roots_probe.bas
' Purpose: Retain physical parents containing generated argument and result types.
' Responsibilities: Inline, mixed, generated and final-token origin boundaries.
' This file intentionally does NOT score complexity or execute callbacks.
#lang "fb"
#define ChildCallback function (byval as function (byval as long) as long) as long
#define WholeCallback function (byval as ChildCallback) as long
dim InlineValue as function (byval as function (byval as function (byval as long) as long) as long) as long
dim MixedValue as function (byval as ChildCallback) as long
dim GeneratedValue as WholeCallback
dim TailValue as function (byval as function (byval as long) as long) as ChildCallback
' end of type_macro_roots_probe.bas
