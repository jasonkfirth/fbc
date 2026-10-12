'' FreeBASIC GMP native qualification
'' File: gmp-native.bas
'' Purpose: Compare C layouts and call actual native limb arithmetic.
'' Responsibilities: Upper limb bits, carry returns and caller-owned output.
'' This file contains no substitute big-number arithmetic implementation.

#lang "fb"
#include once "gmp.bi"

Print "ABI "; SizeOf(mp_limb_t); " "; SizeOf(mp_limb_signed_t); " "; SizeOf(mp_bitcnt_t); " "; _
	SizeOf(mp_size_t); " "; SizeOf(mp_exp_t); " "; SizeOf(__mpz_struct); " "; SizeOf(__mpq_struct); " "; _
	SizeOf(__mpf_struct); " "; OffsetOf(__mpz_struct, _mp_d); " "; OffsetOf(__mpf_struct, _mp_d)
If GMP_NAIL_BITS <> 0 OrElse mp_bits_per_limb <> SizeOf(mp_limb_t) * 8 Then End 1

' The top bit exercises the part of a Win64 limb which C long cannot hold.
Dim As mp_limb_t high = Cast(mp_limb_t, 1) Shl (GMP_LIMB_BITS - 1)
Dim As mp_limb_t source_limbs(0 To 1) = {0, 1}, result_limbs(0 To 1), carry
carry = mpn_add_1(@result_limbs(0), @source_limbs(0), 2, high)
If carry <> 0 OrElse result_limbs(0) <> high OrElse result_limbs(1) <> 1 Then End 2
source_limbs(0) = 1 : source_limbs(1) = 0
carry = mpn_rshift(@result_limbs(0), @source_limbs(0), 2, 1)
If carry <> high OrElse result_limbs(0) <> 0 OrElse result_limbs(1) <> 0 Then End 3

Dim As __mpz_struct number
mpz_init(@number)
Dim As Integer status = 0
Dim As ZString * 64 text
If mpz_set_str(@number, "18446744073709551615", 10) <> 0 Then
	status = 4
ElseIf mpz_getlimbn(@number, 0) <> Cast(mp_limb_t, -1) Then
	status = 5
ElseIf mpz_get_str(@text, 10, @number) = 0 OrElse text <> "18446744073709551615" Then
	status = 5
End If
mpz_clear(@number)
If status <> 0 Then End status
Print "GMP_NATIVE=PASS limb_bits="; GMP_LIMB_BITS; " high="; high; " carry="; carry
'' end of gmp-native.bas
