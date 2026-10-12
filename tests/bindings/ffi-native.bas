'' FreeBASIC libffi native qualification
'' File: ffi-native.bas
'' Purpose: Compare C closure layouts and call actual narrow integer interfaces.
'' Responsibilities: Guarded result storage, native callbacks and owned cleanup.
'' This file contains no trampoline implementation or simulated libffi calls.

#lang "fb"
#include once "ffi.bi"

Type closure_alignment
	tag As UByte
	item As ffi_closure
End Type
Type result_guard
	before As ULongInt
	value As ffi_arg
	after As ULongInt
End Type

Function scalar Cdecl(ByVal text As ZString Ptr) As ULong
	If text <> 0 AndAlso *text = "ffi" Then Return &hffffffffUL
	Return 0
End Function
Sub callback Cdecl(ByVal cif As ffi_cif Ptr, ByVal result As Any Ptr, ByVal args As Any Ptr Ptr, ByVal userdata As Any Ptr)
	*CPtr(ffi_arg Ptr, result) = scalar(*CPtr(ZString Ptr Ptr, args[0]))
	*CPtr(ULong Ptr, userdata) += 1
End Sub

Print "ABI "; SizeOf(ffi_arg); " "; SizeOf(ffi_type); " "; SizeOf(ffi_cif); " "; SizeOf(ffi_closure); " "; _
	OffsetOf(ffi_cif, arg_types); " "; OffsetOf(ffi_cif, flags); " "; OffsetOf(ffi_closure, cif); " "; _
	OffsetOf(ffi_closure, fun); " "; OffsetOf(ffi_closure, user_data); " "; OffsetOf(closure_alignment, item); " "; _
	FFI_TRAMPOLINE_SIZE; " "; FFI_DEFAULT_ABI
If Command(1) = "abi" Then End 0

Dim As ffi_type Ptr types(0 To 0) = {@ffi_type_pointer}
Dim As ZString Ptr text = @"ffi"
Dim As Any Ptr values(0 To 0) = {@text}
Dim As ffi_cif cif
If ffi_prep_cif(@cif, FFI_DEFAULT_ABI, 1, @ffi_type_uint32, @types(0)) <> FFI_OK Then End 1
Dim As result_guard result = (&h123456789abcdef0ULL, 0, &hfedcba9876543210ULL)
ffi_call(@cif, FFI_FN(@scalar), @result.value, @values(0))
If result.value <> &hffffffffUL OrElse result.before <> &h123456789abcdef0ULL OrElse result.after <> &hfedcba9876543210ULL Then End 2

Dim As Function Cdecl(ByVal text As ZString Ptr) As ULong invoke
Dim As ffi_closure Ptr closure = ffi_closure_alloc(SizeOf(ffi_closure), @invoke)
If closure = 0 Then End 3
Dim As ULong calls = 0
Dim As Integer status = 0
If ffi_prep_closure_loc(closure, @cif, @callback, @calls, invoke) <> FFI_OK Then
	status = 4
ElseIf invoke("ffi") <> &hffffffffUL OrElse calls <> 1 Then
	status = 5
End If
ffi_closure_free(closure)
If status <> 0 Then End status
Print "LIBFFI_NATIVE=PASS uint32=4294967295 guards=2 closure=1 cleanup=1"
'' end of ffi-native.bas
