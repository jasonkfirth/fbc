'' Project: FreeBASIC native library regression checks
'' File: big-int-abi.bas
'' Purpose: Compare C layouts and exercise the actual big_int library.
'' Responsibilities: Signed outputs, limb stride, bit counts and owned buffers.
'' This file contains no replacement arithmetic or unbounded benchmark.

#lang "fb"
#include once "big_int/big_int_full.bi"

Print "ABI "; SizeOf(big_int_word); " "; SizeOf(big_int_dword); " "; _
	SizeOf(sign_type); " "; SizeOf(bin_op_type); " "; SizeOf(size_t); " "; _
	SizeOf(big_int); " "; OffsetOf(big_int, sign); " "; OffsetOf(big_int, len); " "; _
	OffsetOf(big_int, len_allocated); " "; SizeOf(big_int_str); " "; _
	OffsetOf(big_int_str, len); " "; OffsetOf(big_int_str, len_allocated)

Dim As big_int Ptr a = big_int_create(1), b = big_int_create(1)
Dim As big_int_str Ptr text = big_int_str_create(1)
Dim As Integer exit_status = 1
Do
	If a = 0 OrElse b = 0 OrElse text = 0 Then Exit Do
	If big_int_from_int(-17, a) <> 0 Then Exit Do
	If big_int_from_int(-5, b) <> 0 Then Exit Do
	If big_int_add(a, b, a) <> 0 Then Exit Do
	'' Output parameters belong to C, which writes four bytes even on a
	'' 64-bit host. Neighbouring sentinels also detect an oversized write.
	Dim As Long signed_output(0 To 2) = { 123456789, 0, 123456789 }
	If big_int_to_int(a, @signed_output(1)) <> 0 Then Exit Do
	If signed_output(1) <> -22 Then Exit Do
	big_int_cmp(a, b, @signed_output(1))
	If signed_output(1) <> -1 Then Exit Do
	big_int_is_zero(a, @signed_output(1))
	If signed_output(1) <> 0 Then Exit Do
	If signed_output(0) <> 123456789 OrElse signed_output(2) <> 123456789 Then Exit Do
	If a->sign <> MINUS OrElse a->len <> 1 Then Exit Do
	If a->num[0] <> 22 Then Exit Do
	If big_int_to_str(a, 10, text) <> 0 Then Exit Do
	If *text->str <> "-22" Then Exit Do
	'' 2^64 + 1 spans three 32-bit limbs. Looking at each limb checks the
	'' element stride independently of the structure offsets above.
	If big_int_from_int(1, b) <> 0 Then Exit Do
	If big_int_set_bit(b, 64, b) <> 0 Then Exit Do
	If b->len <> 3 OrElse b->len_allocated < b->len Then Exit Do
	If b->num[0] <> 1 OrElse b->num[1] <> 0 OrElse b->num[2] <> 1 Then Exit Do
	Dim As ULong unsigned_output(0 To 2) = { 123456789, 0, 123456789 }
	big_int_bit_length(b, @unsigned_output(1))
	If unsigned_output(1) <> 65 Then Exit Do
	big_int_bit1_cnt(b, @unsigned_output(1))
	If unsigned_output(1) <> 2 Then Exit Do
	If unsigned_output(0) <> 123456789 OrElse unsigned_output(2) <> 123456789 Then Exit Do
	Dim As size_t position
	If big_int_scan1_bit(b, 1, @position) <> 0 OrElse position <> 64 Then Exit Do
	If big_int_to_str(b, 10, text) <> 0 Then Exit Do
	If text->len <> 20 OrElse *text->str <> "18446744073709551617" Then Exit Do
	If big_int_from_str(text, 10, a) <> 0 Then Exit Do
	big_int_cmp(a, b, @signed_output(1))
	If signed_output(1) <> 0 Then Exit Do
	exit_status = 0
	Exit Do
Loop

'' Creation can fail independently, so release only acquired objects.
If text <> 0 Then big_int_str_destroy(text)
If b <> 0 Then big_int_destroy(b)
If a <> 0 Then big_int_destroy(a)
If exit_status = 0 Then Print "BIG_INT_NATIVE_PASS"
End exit_status

'' end of big-int-abi.bas
