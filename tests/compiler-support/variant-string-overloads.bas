'' Project: FreeBASIC variant helper library regression tests
'' File: variant-string-overloads.bas
'' Purpose: Keep all narrow-text overloads consistent with native Unicode BSTRs.
'' Responsibilities: Check assignment, concatenation, comparisons and empty values.
'' This file intentionally does NOT depend on a non-ASCII system code page.

#lang "fb"
#include once "variant.bi"

For cycle As Integer = 1 To 32
	Dim alpha_text As ZString * 6 = "alpha"
	Dim beta_text As ZString * 5 = "beta"
	Dim empty_text As ZString * 1 = ""
	Dim alpha_pointer As ZString Ptr = @alpha_text
	Dim beta_pointer As ZString Ptr = @beta_text
	Dim first_value As VARIANT = VARIANT(alpha_pointer)
	Dim second_value As VARIANT = VARIANT(beta_pointer)
	Dim assigned As VARIANT
	assigned = alpha_pointer
	If SysStringLen(V_BSTR(@assigned.var_)) <> 5 Then End 21
	Dim actual As String = assigned
	If actual <> "alpha" Then End 22
	assigned = assigned
	If V_VT(@assigned.var_) <> VT_BSTR Then End 23
	assigned = assigned.var_
	actual = assigned
	If actual <> "alpha" Then End 24

	Dim combined As VARIANT = first_value + beta_pointer
	actual = combined
	If actual <> "alphabeta" Then End 25
	Dim reversed As VARIANT = alpha_pointer + second_value
	actual = reversed
	If actual <> "alphabeta" Then End 26
	first_value += beta_pointer
	actual = first_value
	If actual <> "alphabeta" Then End 27
	first_value = alpha_pointer

	If (first_value = alpha_pointer) = 0 Then End 28
	If (alpha_pointer = first_value) = 0 Then End 29
	If (first_value <> beta_pointer) = 0 Then End 30
	If (beta_pointer <> first_value) = 0 Then End 31
	If (first_value < beta_pointer) = 0 Then End 32
	If (alpha_pointer < second_value) = 0 Then End 33
	If (first_value <= alpha_pointer) = 0 Then End 34
	If (alpha_pointer <= first_value) = 0 Then End 35
	If (second_value > alpha_pointer) = 0 Then End 36
	If (beta_pointer > first_value) = 0 Then End 37
	If (first_value >= alpha_pointer) = 0 Then End 38
	If (alpha_pointer >= first_value) = 0 Then End 39
	If (first_value = beta_pointer) <> 0 Then End 40
	If (alpha_pointer > second_value) <> 0 Then End 41

	Dim empty_value As VARIANT = VARIANT(@empty_text)
	Dim null_value As VARIANT = VARIANT(CPtr(ZString Ptr, 0))
	If V_VT(@empty_value.var_) <> VT_BSTR OrElse V_VT(@null_value.var_) <> VT_BSTR Then End 42
	If SysStringLen(V_BSTR(@empty_value.var_)) <> 0 Then End 43
	If SysStringLen(V_BSTR(@null_value.var_)) <> 0 Then End 44
	actual = empty_value
	If actual <> "" Then End 45
	actual = null_value
	If actual <> "" Then End 46
	assigned = @empty_text
	actual = assigned
	If actual <> "" Then End 47
Next

Print "VARIANT_STRING_OVERLOADS_PASS"

'' end of variant-string-overloads.bas
