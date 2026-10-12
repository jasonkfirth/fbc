'' FreeBASIC TRE examples
'' File: replace.bas
'' Purpose: Replace matches using numbered backreferences.
'' Responsibilities: Bound captures/tokens, advance empty matches, release the compiled pattern.
'' This file contains no TRE implementation or asynchronous matching.
'' Original PHP-like replacement example by MisterD.

#include once "regex.bi"

function regex_replace(ByRef pattern As String, ByRef replacement As String, ByRef subject As String) As String
	Dim re As regex_t
	If regcomp(@re, pattern, REG_EXTENDED Or REG_ICASE) <> 0 Then Return ""
	Dim matches(0 To re.re_nsub) As regmatch_t
	Dim result_text As String
	Dim scan_offset As Integer = 0, copy_offset As Integer = 0
	Dim input_data As Const ZString Ptr = StrPtr(subject)
	If Len(subject) = 0 Then input_data = @""
	Dim status As Long = REG_NOMATCH
	Do
		Dim flags As Long = IIf(scan_offset = 0, 0, REG_NOTBOL)
		status = regnexec(@re, input_data+scan_offset, Len(subject)-scan_offset, re.re_nsub+1, @matches(0), flags)
		If status <> 0 Then Exit Do
		Dim first As Integer = scan_offset+matches(0).rm_so
		Dim last As Integer = scan_offset+matches(0).rm_eo
		If first < copy_offset Or last < first Or last > Len(subject) Then
			status = REG_BADPAT
			Exit Do
		End If
		result_text &= Mid(subject, copy_offset+1, first-copy_offset)
		'' Backreferences are \0, \1, etc. A double backslash is literal.
		'' Bound the numeric accumulator before indexing the match array.
		Dim token As Integer = 0
		Do While token < Len(replacement)
			If replacement[token] <> Asc("\") Then
				result_text &= Chr(replacement[token])
				token += 1
			ElseIf token+1 < Len(replacement) AndAlso replacement[token+1] = Asc("\") Then
				result_text &= "\"
				token += 2
			Else
				Dim position As Integer = token+1
				Dim capture_index As UInteger = 0
				Dim overflowed As Boolean = False
				Do While position < Len(replacement)
					Dim digit As Long = replacement[position]-Asc("0")
					If digit < 0 Or digit > 9 Then Exit Do
					If capture_index > (CUInt(-1)-digit) \ 10 Then
						overflowed = True
					ElseIf Not overflowed Then
						capture_index = capture_index*10+digit
					End If
					position += 1
				Loop
				If position = token+1 Then
					result_text &= "\"
					token += 1
				Else
					If Not overflowed AndAlso capture_index <= re.re_nsub Then
						If matches(capture_index).rm_so >= 0 Then
							result_text &= Mid(subject, scan_offset+matches(capture_index).rm_so+1, _
							                   matches(capture_index).rm_eo-matches(capture_index).rm_so)
						End If
					End If
					token = position
				End If
			End If
		Loop
		copy_offset = last
		scan_offset = last
		If first = last Then
			'' Byte offsets keep the original subject intact while the search progresses.
			If scan_offset = Len(subject) Then Exit Do
			scan_offset += 1
		End If
	Loop
	regfree(@re)
	If status <> 0 And status <> REG_NOMATCH Then Return ""
	Return result_text & Mid(subject, copy_offset+1)
end function

#ifndef REGEX_EXAMPLE_NO_MAIN
Print regex_replace("-(.+?)-", "*\1*", "Hi -you- strange -user- :D")
#endif

'' end of replace.bas
