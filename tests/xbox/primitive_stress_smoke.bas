' Project: FreeBASIC Xbox backend validation
' ------------------------------------------
'
' File: primitive_stress_smoke.bas
'
' Purpose:
'
'     Measure software primitive throughput on the Xbox backend.
'
' Responsibilities:
'
'     - draw a repeatable burst of PSet and Circle primitives
'     - present through ScreenCopy
'     - display approximate frame time
'
' This file intentionally does NOT contain:
'
'     - Behold game logic
'     - sound tests
'     - emulator launch automation

Const ScreenW = 320
Const ScreenH = 200
Const PixelCount = 7000
Const CircleCount = 120

Dim As Integer frame
Dim As Integer i
Dim As Integer x, y
Dim As Double startTime
Dim As Double frameTime
Dim As Integer useLock
Dim As Long colorValue

ScreenRes ScreenW, ScreenH, 32, 2
ScreenSet 1, 0

Do
    useLock = (frame \ 180) And 1
    startTime = Timer

    If useLock Then ScreenLock

    Cls

    For i = 0 To PixelCount - 1
        x = ((i * 37) + (frame * 3)) Mod ScreenW
        y = ((i * 19) + frame) Mod ScreenH
        colorValue = Rgb((i * 3) And 255, (i * 5) And 255, (i * 7) And 255)
        PSet (x, y), colorValue
    Next i

    For i = 0 To CircleCount - 1
        x = ((i * 29) + (frame * 2)) Mod ScreenW
        y = 20 + (((i * 13) + frame) Mod (ScreenH - 40))
        Circle (x, y), 3 + (i Mod 6), Rgb(255, 220, 64)
    Next i

    If useLock Then ScreenUnlock

    frameTime = Timer - startTime

    Locate 2, 3
    Color Rgb(255, 255, 255), Rgb(0, 0, 0)
    Print "Primitive stress smoke"
    Locate 4, 3
    Print "PSet:"; PixelCount; " Circle:"; CircleCount
    Locate 6, 3
    Print "Frame:"; frame
    Locate 8, 3
    If useLock Then
        Print "ScreenLock: on "
    Else
        Print "ScreenLock: off"
    End If
    Locate 10, 3
    Print "Draw seconds:"; frameTime

    ScreenCopy
    frame += 1
Loop

' end of primitive_stress_smoke.bas
