' Project: FreeBASIC Xbox backend validation
' ------------------------------------------
'
' File: screen13_transition_smoke.bas
'
' Purpose:
'
'     Exercise Behold's startup graphics sequence on the Xbox backend.
'
' Responsibilities:
'
'     - enter legacy Screen 13 mode
'     - read the Screen 13 palette
'     - switch to 32-bit two-page ScreenRes mode
'     - present animation only through ScreenCopy
'
' This file intentionally does NOT contain:
'
'     - Behold game logic
'     - sound tests
'     - menu logic

Const ScreenW = 320
Const ScreenH = 200

Dim As Integer red, green, blue
Dim As Integer colorIndex
Dim As Integer shade
Dim As Long paletteMap(0 To 255, 1 To 255)
Dim As Integer frame
Dim As Integer markerX
Dim As Integer status
Dim As Long buttons, dpad
Dim As Single lx, ly, rx, ry, lt, rt

Screen 13

For colorIndex = 0 To 255
    Palette Get colorIndex, red, green, blue
    For shade = 1 To 255
        paletteMap(colorIndex, shade) = Rgb(red / (shade / 4), green / (shade / 4), blue / (shade / 4))
    Next shade
Next colorIndex

ScreenRes ScreenW, ScreenH, 32, 2
ScreenSet 1, 0

Do
    status = GetXPad(0, buttons, lx, ly, rx, ry, lt, rt, dpad)
    markerX = 16 + (frame Mod (ScreenW - 48))

    Cls
    Line (0, 0)-(ScreenW - 1, ScreenH - 1), Rgb(0, 24, 32), BF
    Line (0, 28)-(ScreenW - 1, 146), Rgb(0, 0, 0), BF
    Line (markerX, 164)-(markerX + 16, 180), Rgb(255, 220, 32), BF

    Color Rgb(255, 255, 255), Rgb(0, 0, 0)
    Locate 4, 4
    Print "SCREEN 13 -> SCREENRES 32, 2"
    Locate 6, 4
    Print "Frame:"; frame
    Locate 8, 4
    Print "XPAD status:"; status
    Locate 10, 4
    Print "Buttons:"; buttons
    Locate 12, 4
    Print "DPad:"; dpad

    ScreenCopy
    Sleep 33, 1

    frame += 1
Loop

' end of screen13_transition_smoke.bas
