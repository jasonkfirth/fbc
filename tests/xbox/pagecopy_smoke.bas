' Project: FreeBASIC Xbox backend validation
' ------------------------------------------
'
' File: pagecopy_smoke.bas
'
' Purpose:
'
'     Exercise two-page graphics presentation on the Xbox backend.
'
' Responsibilities:
'
'     - create a normal two-page graphics screen
'     - draw changing content to the off-screen work page
'     - present only through ScreenCopy
'
' This file intentionally does NOT contain:
'
'     - controller input tests
'     - Behold game logic
'     - emulator launch automation

Const ScreenW = 320
Const ScreenH = 240

Dim frame As Integer
Dim markerX As Integer

ScreenRes ScreenW, ScreenH, 8, 2
ScreenSet 1, 0

Do
    markerX = 16 + (frame Mod (ScreenW - 48))

    Cls
    Line (0, 0)-(ScreenW - 1, ScreenH - 1), 1, BF
    Line (0, 72)-(ScreenW - 1, 118), 0, BF
    Line (markerX, 150)-(markerX + 16, 166), 14, BF

    Color 15, 0
    Locate 5, 5
    Print "XBOX PAGECOPY SMOKE"
    Locate 7, 5
    Print "Frame:"; frame
    Locate 9, 5
    Print "Only ScreenCopy presents this image."
    Locate 11, 5
    Print "Stable background, moving marker."

    ScreenCopy
    Sleep 33, 1

    frame += 1
Loop

' end of pagecopy_smoke.bas
