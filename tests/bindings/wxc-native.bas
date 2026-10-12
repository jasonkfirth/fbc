'' FreeBASIC external-library qualification
'' File: wxc-native.bas
'' Purpose: Check the wx-c binding against the native publisher probe.
'' Responsibilities: Native widths, UTF-8 decoding, callbacks and GDI pixels.
'' This file contains no replacement widgets or custom window event loop.
#include once "wx-c/wx.bi"
#include once "windows.bi"

'' These handles belong to wxWidgets while wxApp_Run runs on this thread.
Dim Shared As wxApp Ptr test_app
Dim Shared As wxFrame Ptr test_frame
Dim Shared As HWND frame_hwnd
Dim Shared As Integer init_calls, exit_calls, checked_pixels
Declare Function OnInit WXCALL () As wxBool
Declare Function OnExit WXCALL () As wxInt

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
If Environ("WXC_GUI_TEST") = "1" Then
    test_app = wxApp_ctor()
    If test_app = 0 Then End 8
    wxApp_RegisterVirtual(test_app, @OnInit, @OnExit)
    wxApp_Run(0, 0)
    If init_calls <> 1 OrElse exit_calls <> 1 OrElse checked_pixels <> 49 Then End 9
    If IsWindow(frame_hwnd) Then End 9
    Print "WXC_GUI=PASS init=1 exit=1 pixels=49 frame_destroyed=1"
End If
Print "WXC_NATIVE=PASS unicode=3 empty=0 rgb=17,129,250 owned_objects_released=3"

Function FindFrame StdCall (ByVal hwnd_value As HWND, ByVal unused_value As LPARAM) As WINBOOL
    Dim As DWORD owner
    Dim As ZString * 128 title
    GetWindowThreadProcessId(hwnd_value, @owner)
    GetWindowText(hwnd_value, @title, SizeOf(title))
    If owner = GetCurrentProcessId() AndAlso title = "wx-c native qualification" Then frame_hwnd = hwnd_value
    Return TRUE
End Function

Function OnInit WXCALL () As wxBool
    Dim As wxString Ptr title = wxString_ctorUTF8("wx-c native qualification")
    Dim As wxColour Ptr colour = wxColour_ctorByParts(17, 129, 250)
    test_frame = wxFrame_ctor()
    If test_frame = 0 OrElse title = 0 OrElse colour = 0 Then End 10
    If wxFrame_Create(test_frame, 0, -1, title, -1, -1, 320, 240, _
                      wxCAPTION Or wxSYSTEM_MENU Or wxCLOSE_BOX, 0) = 0 Then End 11
    wxString_dtor(title)
    wxWindow_SetBackgroundColour(test_frame, colour)
    wxColour_dtor(colour)
    wxWindow_Show(test_frame, 1)
    wxWindow_Refresh(test_frame, 1, 0)
    wxWindow_Update(test_frame)
    EnumThreadWindows(GetCurrentThreadId(), @FindFrame, 0)
    If frame_hwnd = 0 Then End 12
    Dim As HDC dc = GetDC(frame_hwnd)
    If dc = 0 Then End 13
    For y As Integer = 20 To 80 Step 10
        For x As Integer = 20 To 80 Step 10
            '' Windows COLORREF stores red in its low byte, unlike FB RGB().
            If GetPixel(dc, x, y) <> (17 Or (129 Shl 8) Or (250 Shl 16)) Then End 14
            checked_pixels += 1
        Next x
    Next y
    ReleaseDC(frame_hwnd, dc)
    init_calls += 1
    Print "WXC_GUI_READY pixels=49"
    Return wxApp_OnInit(test_app)
End Function

Function OnExit WXCALL () As wxInt
    exit_calls += 1
    Return wxApp_OnExit(test_app)
End Function
'' end of wxc-native.bas
