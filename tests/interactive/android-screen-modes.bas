''
'   FreeBASIC Android interactive smoke tests
'   -----------------------------------------
'
'   File: android-screen-modes.bas
'
'   Purpose:
'
'       Exercise legacy SCREEN modes, runtime mode changes, and the
'       software page switching path used by games.
'
'   Responsibilities:
'
'       * switch through the common legacy SCREEN modes
'       * draw visible text and color bars after each mode change
'       * return to a 32-bit paged ScreenRes mode
'       * verify ScreenSet-based page flipping visually
'
'   This file intentionally does NOT contain:
'
'       * automated image comparison
'       * Android wrapper build logic
'       * game-specific input tests
''

#define FB_ANDROID_HIDE_KEYBOARD_BUTTON
#define FB_ANDROID_LANDSCAPE

Function WaitForInputOrTimeout(ByVal milliseconds As Integer) As Integer
    Dim As Double stopTime
    Dim As Integer mx, my, mw, mb

    stopTime = Timer + (milliseconds / 1000.0)

    Do
        If Len(Inkey$) > 0 Then
            WaitForInputOrTimeout = -1
            Exit Function
        End If

        If GetMouse(mx, my, mw, mb) = 0 Then
            If (mb And 1) Then
                WaitForInputOrTimeout = -1
                Exit Function
            End If
        End If

        Sleep 10, 1
    Loop While Timer < stopTime

    WaitForInputOrTimeout = 0
End Function

Sub DrawModeBanner(ByVal mode As Integer)
    Dim As Integer w, h, bypp, pitch

    ScreenInfo w, h, bypp, pitch
    Cls

    Print "ANDROID SCREEN MODE SWITCH SMOKE"
    Print
    Print "SCREEN "; mode
    Print "width="; w; " height="; h; " bpp="; bypp * 8
    Print
    Print "This screen should change modes every second."

    If bypp > 0 Then
        Line (10, 80)-(w - 10, h - 10), Rgb(255, 255, 255), B
        Line (20, 95)-(w - 20, 120), Rgb(255, 0, 0), BF
        Line (20, 125)-(w - 20, 150), Rgb(0, 255, 0), BF
        Line (20, 155)-(w - 20, 180), Rgb(0, 0, 255), BF
    End If

    ScreenSync
    WaitForInputOrTimeout 1000
End Sub

Function fb_android_program_main CDecl(ByVal argc As Integer, ByVal argv As ZString Ptr Ptr) As Integer
    Dim modes(0 To 9) As Integer = {0, 1, 2, 7, 8, 9, 10, 11, 12, 13}
    Dim As Integer i

    For i = 0 To 9
        Screen modes(i)
        DrawModeBanner modes(i)
    Next i

    ScreenRes 320, 200, 32, 2
    ScreenSet 1, 0
    Cls Rgb(0, 0, 64)
    Color Rgb(255, 255, 255), Rgb(0, 0, 64)
    Print "Returned to ScreenRes 320x200x32 pages=2"
    Print "Now flipping pages."
    ScreenCopy
    WaitForInputOrTimeout 700

    ScreenSet 0, 1
    Cls Rgb(180, 40, 40)
    Color Rgb(255, 255, 255), Rgb(180, 40, 40)
    Print "Visible page should now be page 1."
    ScreenSet 0, 0
    WaitForInputOrTimeout 2000

    fb_android_program_main = 0
End Function

'' end of android-screen-modes.bas
