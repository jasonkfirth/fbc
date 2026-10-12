'' FreeBASIC TRE examples
'' File: match.bas
'' Purpose: Print successive identifier matches.
'' Responsibilities: Check compile/match status, bound offsets and release the pattern.
'' This file contains no replacement parser or TRE implementation.

#include once "regex.bi"

function printmatches(ByVal pattern As String, ByVal text As String) As Integer
	Dim re As regex_t
	Dim match As regmatch_t
	Dim status As Long = regcomp(@re, pattern, 0)
	If status <> 0 Then
		Print "regex compile failed: "; status
		Return 1
	End If
	Dim input_data As Const ZString Ptr = StrPtr(text)
	If Len(text) = 0 Then input_data = @""
	Dim offset As Integer = 0
	Do
		status = regnexec(@re, input_data+offset, Len(text)-offset, 1, @match, IIf(offset = 0, 0, REG_NOTBOL))
		If status <> 0 Then Exit Do
		Dim first As Integer = offset+match.rm_so
		Dim last As Integer = offset+match.rm_eo
		If first < offset Or last < first Or last > Len(text) Then
			status = REG_BADPAT
			Exit Do
		End If
		Print "<"; Mid(text, first+1, last-first); ">"
		offset = last
		If first = last Then
			If offset = Len(text) Then Exit Do
			offset += 1
		End If
	Loop
	regfree(@re)
	If status <> 0 And status <> REG_NOMATCH Then Return 1
	Return 0
end function

#ifndef REGEX_EXAMPLE_NO_MAIN
Dim status As Integer = printmatches("[a-zA-Z_][a-zA-Z_0-9]*", "foo _bar 123 foo123 BAR 456 !!! Foo__ ???")
If status <> 0 Then End status
Sleep
#endif

'' end of match.bas
