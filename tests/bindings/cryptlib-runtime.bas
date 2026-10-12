'' Project: FreeBASIC native library regression checks
'' File: cryptlib-runtime.bas
'' Purpose: Check native hash calls and their caller-owned output storage.
'' Responsibilities: Incremental digests, 32-bit outputs and canary checks.
'' This file contains no replacement hash algorithm or security configuration.

#lang "fb"
#include once "cryptlib.bi"

Const GUARD_VALUE As ULong = &h13579BDF
Type ContextOutput
	before_value As ULong
	value As CRYPT_CONTEXT
	after_value As ULong
End Type
Type LengthOutput
	before_value As ULong
	value As Long
	after_value As ULong
End Type
Type DigestOutput
	before_value As ULong
	value(0 To CRYPT_MAX_HASHSIZE - 1) As UByte
	after_value As ULong
End Type

Function check_hash(ByVal algorithm As CRYPT_ALGO_TYPE, ByRef expected As Const String) As Boolean
	Dim As ContextOutput context
	Dim As LengthOutput length
	Dim As DigestOutput digest
	context.before_value = GUARD_VALUE
	context.after_value = GUARD_VALUE
	length.before_value = GUARD_VALUE
	length.after_value = GUARD_VALUE
	digest.before_value = GUARD_VALUE
	digest.after_value = GUARD_VALUE
	Dim As Long status = cryptCreateContext(@context.value, CRYPT_UNUSED, algorithm)
	If cryptStatusError(status) Then Return False
	Dim As String input_value = "abc"
	'' Separate updates exercise retained native state. A valid address also
	'' accompanies the final zero-length update, as required by the C API.
	status = cryptEncrypt(context.value, StrPtr(input_value), 1)
	If cryptStatusOK(status) Then status = cryptEncrypt(context.value, StrPtr(input_value) + 1, 2)
	If cryptStatusOK(status) Then status = cryptEncrypt(context.value, StrPtr(input_value), 0)
	'' valueLength is a 32-bit output, not an input buffer capacity. The digest
	'' storage must accommodate CRYPT_MAX_HASHSIZE bytes before making this call.
	If cryptStatusOK(status) Then status = cryptGetAttributeString(context.value, _
		CRYPT_CTXINFO_HASHVALUE, @digest.value(0), @length.value)
	Dim As Long release_status = cryptDestroyContext(context.value)
	If cryptStatusError(status) OrElse cryptStatusError(release_status) Then Return False
	If context.before_value <> GUARD_VALUE OrElse context.after_value <> GUARD_VALUE Then Return False
	If length.before_value <> GUARD_VALUE OrElse length.after_value <> GUARD_VALUE Then Return False
	If digest.before_value <> GUARD_VALUE OrElse digest.after_value <> GUARD_VALUE Then Return False
	If length.value <= 0 OrElse length.value > CRYPT_MAX_HASHSIZE Then Return False
	If length.value * 2 <> Len(expected) Then Return False
	Dim As String observed
	For index As Long = 0 To length.value - 1
		observed += Right("0" & Hex(digest.value(index)), 2)
	Next
	Return observed = expected
End Function

If cryptStatusError(cryptInit()) Then End 1
Scope
	Dim As CRYPT_CONTEXT context
	Dim As Long status = cryptCreateContext(@context, CRYPT_UNUSED, CRYPT_ALGO_MD5)
	If status <> CRYPT_ERROR_NOTAVAIL Then
		If cryptStatusOK(status) Then cryptDestroyContext(context)
		cryptEnd()
		End 1
	End If
End Scope
If check_hash(CRYPT_ALGO_SHA2, "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD") = False Then
	cryptEnd()
	End 1
End If
If check_hash(CRYPT_ALGO_SHA1, "A9993E364706816ABA3E25717850C26C9CD0D89D") = False Then
	cryptEnd()
	End 1
End If
If cryptStatusError(cryptEnd()) Then End 1
Print "CRYPTLIB_RUNTIME=PASS"

'' end of cryptlib-runtime.bas
