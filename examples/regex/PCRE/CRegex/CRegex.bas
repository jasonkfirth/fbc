'' FreeBASIC PCRE examples
'' File: CRegex.bas
'' Purpose: Own a compiled PCRE pattern and its current captures.
'' Responsibilities: Preserve subject lifetime, guard capture bounds and advance empty matches.
'' This file contains no PCRE implementation or asynchronous matching.
'' Build: fbc -lib CRegex.bas

#include once "CRegex.bi"
#include once "../match-progress.bi"
#include once "crt/string.bi"

constructor CRegex(ByVal pattern As ZString Ptr, ByVal opt As options)
	reg = NULL
	extra = NULL
	vectb = NULL
	subject = NULL
	sublen = 0
	substrcnt = 0
	substrlist = NULL
	error_code = 0
	matched_count = 0
	has_match = False
	If pattern = NULL Then
		error_code = PCRE_ERROR_NULL
		Return
	End If
	Dim err_msg As Const ZString Ptr
	Dim err_ofs As Long
	reg = pcre_compile2(pattern, opt, @error_code, @err_msg, @err_ofs, NULL)
	If reg = NULL Then Return
	extra = pcre_study(reg, 0, @err_msg)
	If err_msg <> NULL Then
		error_code = PCRE_ERROR_NOMEMORY
		Return
	End If
	'' CAPTURECOUNT writes a C int, including on 64-bit targets.
	error_code = pcre_fullinfo(reg, extra, PCRE_INFO_CAPTURECOUNT, @substrcnt)
	If error_code <> 0 Then Return
	If substrcnt < 0 Or substrcnt >= &h7fffffff \ 3 Then
		error_code = PCRE_ERROR_NOMEMORY
		Return
	End If
	substrcnt += 1
	vectb = Callocate(3 * substrcnt, SizeOf(Long))
	If vectb = NULL Then error_code = PCRE_ERROR_NOMEMORY
end constructor

sub CRegex.clearSubstrlist()
	If substrlist <> NULL Then pcre_free_substring_list(substrlist)
	substrlist = NULL
end sub

destructor CRegex()
	clearSubstrlist()
	If vectb <> NULL Then Deallocate(vectb)
	If extra <> NULL Then pcre_free_study(extra)
	If reg <> NULL Then pcre_free(reg)
end destructor

function CRegex.getMaxMatches() As Integer
	If reg = NULL Or vectb = NULL Then Return -1
	Return substrcnt-1
end function

function CRegex.getErrorCode() As Long
	Return error_code
end function

function CRegex.search(ByVal subject As ZString Ptr, ByVal lgt As Integer, ByVal opt As options) As Integer
	has_match = False
	matched_count = 0
	If reg = NULL Or vectb = NULL Then Return False
	If subject = NULL Then
		error_code = PCRE_ERROR_NULL
		Return False
	End If
	If lgt < -1 Then
		error_code = PCRE_ERROR_BADLENGTH
		Return False
	End If
	If lgt = -1 Then lgt = Len(*subject)
	If lgt > &h7fffffff Then
		error_code = PCRE_ERROR_BADLENGTH
		Return False
	End If
	'' Own the exact supplied bytes, including zeros for an explicit length.
	'' A caller's temporary string must not dangle during searchNext().
	Dim copied_subject As String = Space(lgt)
	If Len(copied_subject) <> lgt Then
		error_code = PCRE_ERROR_NOMEMORY
		Return False
	End If
	If lgt > 0 Then memcpy(StrPtr(copied_subject), subject, lgt)
	'' Copy first: subject may point into this object's previous capture list.
	clearSubstrlist()
	owned_subject = copied_subject
	this.subject = StrPtr(owned_subject)
	If lgt = 0 Then this.subject = @""
	sublen = lgt
	Dim result As Long = pcre_exec(reg, extra, this.subject, sublen, 0, opt, vectb, 3*substrcnt)
	If result < 0 Then
		error_code = result
		Return False
	End If
	error_code = 0
	matched_count = IIf(result = 0, substrcnt, result)
	has_match = True
	Return True
end function

function CRegex.searchNext(ByVal opt As options) As Integer
	clearSubstrlist()
	If Not has_match Then Return False
	has_match = False
	Dim result As Long = pcre_next_match(reg, extra, subject, sublen, vectb[0], vectb[1], opt, vectb, 3*substrcnt)
	If result < 0 Then
		error_code = result
		matched_count = 0
		Return False
	End If
	error_code = 0
	matched_count = IIf(result = 0, substrcnt, result)
	has_match = True
	Return True
end function

function CRegex.getStr(ByVal i As Integer) As ZString Ptr
	If i < 0 Then Return subject
	If Not has_match Or i >= matched_count Then Return NULL
	If vectb[2*i] < 0 Then Return NULL
	If substrlist = NULL Then
		error_code = pcre_get_substring_list(subject, vectb, matched_count, @substrlist)
		If error_code <> 0 Then Return NULL
	End If
	Return Cast(ZString Ptr, substrlist[i])
end function

function CRegex.getOfs(ByVal i As Integer) As Integer
	If i < 0 Then Return 0
	If Not has_match Or i >= matched_count Then Return -1
	Return vectb[2*i]
end function

function CRegex.getLen(ByVal i As Integer) As Integer
	If i < 0 Then Return 0
	If Not has_match Or i >= matched_count Then Return -1
	If vectb[2*i] < 0 Then Return -1
	Return vectb[2*i+1]-vectb[2*i]
end function

'' end of CRegex.bas
