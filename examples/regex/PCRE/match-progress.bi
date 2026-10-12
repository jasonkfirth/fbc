'' FreeBASIC PCRE examples
'' File: match-progress.bi
'' Purpose: Advance a global PCRE search after a previous match.
'' Responsibilities: Retry empty matches, preserve UTF-8 and CRLF boundaries.
'' This file contains no pattern or result ownership.
#pragma once
#include once "pcre.bi"

Private Function pcre_next_match(ByVal pattern As pcre Ptr, ByVal extra As pcre_extra Ptr, _
                                 ByVal subject As Const ZString Ptr, ByVal length As Long, _
                                 ByVal first As Long, ByVal last As Long, ByVal options As Long, _
                                 ByVal vector As Long Ptr, ByVal vector_size As Long) As Long
	If first < 0 Or last < first Or last > length Then Return PCRE_ERROR_BADOFFSET
	If first = last Then
		'' An empty match can share its position with a nonempty alternative.
		'' Retry there before moving to the next complete character.
		Dim result As Long = pcre_exec(pattern, extra, subject, length, last, _
		                              options Or PCRE_ANCHORED Or PCRE_NOTEMPTY_ATSTART, vector, vector_size)
		If result <> PCRE_ERROR_NOMATCH Then Return result
		If last = length Then Return PCRE_ERROR_NOMATCH
		Dim pattern_options As CULong
		Dim status As Long = pcre_fullinfo(pattern, extra, PCRE_INFO_OPTIONS, @pattern_options)
		If status <> 0 Then Return status
		Dim newline_options As Long = options And (PCRE_NEWLINE_ANYCRLF Or PCRE_NEWLINE_CRLF)
		If newline_options = 0 Then newline_options = pattern_options And (PCRE_NEWLINE_ANYCRLF Or PCRE_NEWLINE_CRLF)
		Dim crlf As Boolean = newline_options = PCRE_NEWLINE_CRLF Or _
		                      newline_options = PCRE_NEWLINE_ANY Or newline_options = PCRE_NEWLINE_ANYCRLF
		If newline_options = 0 Then
			Dim configured As Long
			If pcre_config(PCRE_CONFIG_NEWLINE, @configured) = 0 Then
				crlf = configured = -1 Or configured = -2 Or configured = ((13 Shl 8) Or 10)
			End If
		End If
		If crlf AndAlso subject[last] = 13 AndAlso last+1 < length AndAlso subject[last+1] = 10 Then last += 1
		last += 1
		If (pattern_options And PCRE_UTF8) <> 0 Then
			While last < length AndAlso (subject[last] And &hc0) = &h80
				last += 1
			Wend
		End If
	End If
	Return pcre_exec(pattern, extra, subject, length, last, options, vector, vector_size)
End Function

'' end of match-progress.bi
