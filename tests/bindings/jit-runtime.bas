'' Project: FreeBASIC native library regression checks
'' File: jit-runtime.bas
'' Purpose: Execute signed integer and native-float code from the real libjit.
'' Responsibilities: Result canaries, C callback widths and constant layouts.
'' Ownership: The context owns all generated functions and instruction values.
'' This file contains no replacement JIT implementation or large allocations.

#lang "fb"
#include once "jit.bi"

#if not defined(__FB_WIN32__) and not defined(JIT_NFLOAT_IS_DOUBLE)
	#error This test requires the libjit --disable-long-double configuration
#endif

Extern "C"
	Declare Function fbc_jit_check_callback(ByVal manager As jit_memory_manager_t, _
		ByVal marker As Any Ptr) As Long
End Extern

'' Keep failure reporting in one place. Build routines return to their caller,
'' which releases the build lock and context before ending the test.
Function check_result(ByVal condition As Integer, ByRef phase As Const String) As Integer
	If condition = 0 Then Print "LIBJIT_RUNTIME_FAIL="; phase
	Return condition
End Function

'' The C helper sends wide size/alignment values without allocating them.
Function check_allocation Cdecl(ByVal memctx As jit_memory_context_t, _
	ByVal byte_count As jit_size_t, ByVal alignment As jit_size_t) As Any Ptr
	Dim As jit_size_t expected_size = 33, expected_alignment = 16
	#ifdef __FB_64BIT__
		expected_size += Cast(jit_size_t, &h100000000ull)
		expected_alignment += Cast(jit_size_t, &h100000000ull)
	#endif
	If byte_count <> expected_size OrElse alignment <> expected_alignment Then Return 0
	Return memctx
End Function

Function build_mul_add(ByVal context As jit_context_t) As jit_function_t
	Dim params(0 To 2) As jit_type_t = {jit_type_int, jit_type_int, jit_type_int}
	Dim As jit_type_t signature = jit_type_create_signature(jit_abi_cdecl, jit_type_int, @params(0), 3, 1)
	If signature = 0 Then Return 0
	Dim As jit_function_t func = jit_function_create(context, signature)
	jit_type_free(signature)
	If func = 0 Then Return 0
	Dim As jit_value_t x = jit_value_get_param(func, 0)
	Dim As jit_value_t y = jit_value_get_param(func, 1)
	Dim As jit_value_t z = jit_value_get_param(func, 2)
	If x = 0 OrElse y = 0 OrElse z = 0 Then Return 0
	Dim As jit_value_t product = jit_insn_mul(func, x, y)
	If product = 0 Then Return 0
	Dim As jit_value_t total = jit_insn_add(func, product, z)
	If total = 0 Then Return 0
	If jit_insn_return(func, total) = 0 Then Return 0
	If jit_function_compile(func) = 0 Then Return 0
	Return func
End Function

Function build_constant(ByVal context As jit_context_t) As jit_function_t
	Dim As jit_type_t signature = jit_type_create_signature(jit_abi_cdecl, jit_type_nfloat, 0, 0, 1)
	If signature = 0 Then Return 0
	Dim As jit_function_t func = jit_function_create(context, signature)
	jit_type_free(signature)
	If func = 0 Then Return 0
	Dim As jit_constant_t source
	source.type = jit_type_nfloat
	source.un.nfloat_value = 6.25
	Dim As jit_value_t value = jit_value_create_constant(func, @source)
	If value = 0 Then Return 0
	'' A struct returned by value must also obey the target C ABI.
	Dim As jit_constant_t returned = jit_value_get_constant(value)
	If check_result(returned.type = source.type AndAlso returned.un.nfloat_value = 6.25, "constant-return") = 0 Then Return 0
	Dim As jit_constant_t converted
	If check_result(jit_constant_convert(@converted, @source, jit_type_int, 1), "constant-convert") = 0 Then Return 0
	If check_result(converted.type = jit_type_int AndAlso converted.un.int_value = 6, "constant-converted-value") = 0 Then Return 0
	If jit_insn_return(func, value) = 0 Then Return 0
	If jit_function_compile(func) = 0 Then Return 0
	Return func
End Function

Type checked_result
	before_value As ULong
	value As jit_int
	after_value As ULong
End Type

Dim As jit_memory_manager manager
manager.alloc_data = @check_allocation
Dim As Long marker
If check_result(jit_type_get_size(jit_type_int) = SizeOf(jit_int), "library-int-size") = 0 Then End 1
If check_result(jit_type_get_size(jit_type_nint) = SizeOf(jit_nint), "library-native-int-size") = 0 Then End 1
If check_result(jit_type_get_size(jit_type_long) = SizeOf(jit_long), "library-long-size") = 0 Then End 1
If check_result(jit_type_get_size(jit_type_nfloat) = SizeOf(jit_nfloat), "library-native-float-size") = 0 Then End 1
If check_result(fbc_jit_check_callback(@manager, @marker), "callback") = 0 Then End 1
If check_result(jit_nfloat_add(1.25, -3.5) = -2.25, "native-float-add") = 0 Then End 1
If check_result(jit_nfloat_to_int(-17.75) = -17, "native-float-to-int") = 0 Then End 1

Dim As jit_context_t context = jit_context_create()
If context = 0 Then End 1
jit_context_build_start(context)
Dim As jit_function_t arithmetic = build_mul_add(context)
Dim As jit_function_t constant_func = build_constant(context)
jit_context_build_end(context)
If arithmetic = 0 OrElse constant_func = 0 Then
	check_result(0, "function-build")
	jit_context_destroy(context)
	End 1
End If

'' These products stay within the C signed-int range. Negative returns and
'' boundary results expose storage errors hidden by the original positive 17.
Dim inputs(0 To 4, 0 To 3) As jit_int = { _
	{3, 5, 2, 17}, {-3, 5, 2, -13}, {3, -5, -2, -17}, _
	{2147483647, 1, 0, 2147483647}, {-2147483647, 1, -1, -2147483648} }
Dim As Integer passed = -1
For index As Integer = 0 To 4
	Dim As checked_result result
	result.before_value = &h1234abcd
	result.value = 123
	result.after_value = &habcd4321
	Dim args(0 To 2) As jit_int Ptr = {@inputs(index, 0), @inputs(index, 1), @inputs(index, 2)}
	If check_result(jit_function_apply(arithmetic, @args(0), @result.value), "integer-apply") = 0 Then passed = 0
	If check_result(result.value = inputs(index, 3), "integer-result") = 0 Then passed = 0
	If check_result(result.before_value = &h1234abcd AndAlso result.after_value = &habcd4321, "integer-canaries") = 0 Then passed = 0
Next
Dim As jit_nfloat floating_result
If check_result(jit_function_apply(constant_func, 0, @floating_result), "constant-apply") = 0 Then passed = 0
If check_result(floating_result = 6.25, "constant-result") = 0 Then passed = 0
jit_context_destroy(context)
If passed = 0 Then End 1
Print "LIBJIT_RUNTIME=PASS signed=5 callback=1 native_float=1 constant=1"

'' end of jit-runtime.bas
