'' Project: FreeBASIC variant helper library regression tests
'' File: variant-native-abi.bas
'' Purpose: Exchange native OLE values through the public wrapper contract.
'' Responsibilities: Check record extents, conversions, copies and owned results.
'' This file intentionally does NOT exercise GUI code or conversion failures.

#lang "fb"
#include once "variant.bi"

Type GuardedVariant
	first_value As ULongInt
	value As VARIANT
	last_value As ULongInt
End Type

If SizeOf(VARIANT) <> SizeOf(VARIANT_) Then End 1
If SizeOf(GuardedVariant) <> SizeOf(VARIANT_) + 16 Then End 2

#ifdef VARIANT_ABI_BASELINE
	Const expected_narrow_chars = 6
#else
	Const expected_narrow_chars = 5
#endif

For cycle As Integer = 1 To 32
	Dim narrow_text As ZString * 6 = "alpha"
	Dim wide_text As WString * 6 = "alpha"
	Dim narrow_value As VARIANT = VARIANT(@narrow_text)
	Dim wide_value As VARIANT = VARIANT(@wide_text)
	If V_VT(@narrow_value.var_) <> VT_BSTR OrElse V_VT(@wide_value.var_) <> VT_BSTR Then End 3
	If SysStringLen(V_BSTR(@narrow_value.var_)) <> expected_narrow_chars Then End 4
	If SysStringLen(V_BSTR(@wide_value.var_)) <> 5 Then End 5
	Dim narrow_copy As String = narrow_value
	Dim wide_copy As String = wide_value
	If narrow_copy <> "alpha" OrElse wide_copy <> "alpha" Then End 6

	Dim raw_value As VARIANT_
	VariantInit(@raw_value)
	V_VT(@raw_value) = VT_BSTR
	V_BSTR(@raw_value) = SysAllocStringLen(@wide_text, 5)
	If V_BSTR(@raw_value) = 0 Then End 7
	Dim wrapped_copy As VARIANT = VARIANT(raw_value)
	VariantClear(@raw_value)
	Dim copied_text As String = wrapped_copy
	If copied_text <> "alpha" Then End 8

	Dim surrounded As GuardedVariant
	surrounded.first_value = &h1122334455667788ull
	surrounded.last_value = &h8877665544332211ull
	surrounded.value = wrapped_copy
	wrapped_copy = NOTHING
	copied_text = surrounded.value
	If copied_text <> "alpha" Then End 9
	If surrounded.first_value <> &h1122334455667788ull Then End 10
	If surrounded.last_value <> &h8877665544332211ull Then End 11

	Dim first_number As VARIANT = VARIANT(CInt(17))
	Dim second_number As VARIANT = VARIANT(CInt(19))
	Dim total As VARIANT = first_number + second_number
	Dim integer_result As Integer = total
	If integer_result <> 36 Then End 12
	Dim large_input As LongInt = &h100000011ll
	Dim large_number As VARIANT = VARIANT(large_input)
	Dim large_result As LongInt = large_number
	If large_result <> large_input Then End 13
	Dim real_number As VARIANT = VARIANT(CDbl(3.5))
	Dim real_result As Double = real_number
	If real_result <> 3.5 Then End 14

	'' Native records and BSTR results own independent copies. Releasing the
	'' wrapper first must leave each returned value usable by its caller.
#if not defined(VARIANT_ABI_BASELINE) or defined(VARIANT_REQUIRE_RAW_CAST)
	Dim returned_native As VARIANT_ = surrounded.value
	surrounded.value = NOTHING
	If V_VT(@returned_native) <> VT_BSTR Then End 15
	If SysStringLen(V_BSTR(@returned_native)) <> 5 Then End 16
	If *CPtr(WString Ptr, V_BSTR(@returned_native)) <> "alpha" Then End 17
	VariantClear(@returned_native)
#endif
	Dim returned_wide As WString Ptr = wide_value
	wide_value = NOTHING
	If returned_wide = 0 Then End 18
	If *returned_wide <> "alpha" Then End 19
	SysFreeString(returned_wide)
Next

Print "VARIANT_NATIVE_ABI_PASS"

'' end of variant-native-abi.bas
