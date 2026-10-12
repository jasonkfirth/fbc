'' FreeBASIC wx-c native values qualification
'' File: wxc-values.bas
'' Purpose: Compare native scalar widths and Unicode values with C.
'' Responsibilities: Checked string conversion, colours and object cleanup.
'' This file contains no GUI loop or replacement widgets.
#include once "wx-c/wx.bi"

Print "WXC_ABI="; LTrim(Str(SizeOf(Any Ptr))); ","; LTrim(Str(SizeOf(wxInt))); ","; _
    LTrim(Str(SizeOf(wxLong))); ","; LTrim(Str(SizeOf(size_t))); ","; LTrim(Str(SizeOf(wchar_t)))
Dim As String utf8 = "A" & Chr(&HC3, &HA9, &HCE, &HA9)
Dim As wxString Ptr text_value = wxString_ctorUTF8(utf8)
Dim As wxString Ptr empty_value = wxString_ctorUTF8("")
If text_value = 0 OrElse empty_value = 0 Then End 4
If wxString_GetLength(text_value) <> 3 OrElse wxString_GetLength(empty_value) <> 0 Then End 4
Dim expected(0 To 2) As wchar_t = {65, 233, 937}
For index As Integer = 0 To 2
    If wxString_CharAtUTF16(text_value, index) <> expected(index) Then End 5
Next index
Dim As wxColour Ptr colour_value = wxColour_ctorByParts(17, 129, 250)
If colour_value = 0 Then End 6
If wxColour_Red(colour_value) <> 17 Then End 6
If wxColour_Green(colour_value) <> 129 Then End 6
If wxColour_Blue(colour_value) <> 250 Then End 6
wxColour_dtor(colour_value)
wxString_dtor(empty_value)
wxString_dtor(text_value)
Print "WXC_VALUES=PASS unicode=3 empty=0 rgb=17,129,250"
End 0
'' end of wxc-values.bas
