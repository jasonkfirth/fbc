' Project: FreeBASIC Xbox backend validation
' ------------------------------------------
'
' File: xpad_smoke.bas
'
' Purpose:
'
'     Display GETXPAD state while presenting through ScreenCopy.
'
' Responsibilities:
'
'     - poll controller port 0
'     - display status, buttons, d-pad, and analog values
'     - keep a frame counter moving so page presentation is visible
'
' This file intentionally does NOT contain:
'
'     - game logic
'     - sound tests
'     - emulator launch automation

Const ScreenW = 320
Const ScreenH = 240

Dim frame As Integer
Dim status As Integer
Dim buttons As Long
Dim dpad As Long
Dim lx As Single
Dim ly As Single
Dim rx As Single
Dim ry As Single
Dim lt As Single
Dim rt As Single
Dim markerX As Integer

ScreenRes ScreenW, ScreenH, 8, 2
ScreenSet 1, 0

Do
    status = GetXPad(0, buttons, lx, ly, rx, ry, lt, rt, dpad)
    markerX = 16 + (frame Mod (ScreenW - 48))

    Cls
    Line (0, 0)-(ScreenW - 1, ScreenH - 1), 1, BF
    Line (0, 32)-(ScreenW - 1, 198), 0, BF
    Line (markerX, 210)-(markerX + 16, 226), 14, BF

    Color 15, 0
    Locate 4, 4
    Print "XBOX XPAD SMOKE"
    Locate 6, 4
    Print "Frame:"; frame
    Locate 8, 4
    Print "Status:"; status
    Locate 10, 4
    Print "Buttons:"; buttons
    Locate 12, 4
    Print "DPad:"; dpad
    Locate 14, 4
    Print "LX:"; lx; " LY:"; ly
    Locate 16, 4
    Print "RX:"; rx; " RY:"; ry
    Locate 18, 4
    Print "LT:"; lt; " RT:"; rt

    ScreenCopy
    Sleep 33, 1

    frame += 1
Loop

' end of xpad_smoke.bas
