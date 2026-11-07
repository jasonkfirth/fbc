/'
    Project: FreeBASIC graphics tests
    File: wayland-mode-lifecycle.bas
    Purpose: Exercise repeated window startup, drawing, input polling and exit.
    Responsibilities: Open bounded modes and report the completed cycle count.
    This file intentionally does NOT inject input or change display settings.
'/
#lang "fb"
#include once "fbgfx.bi"

Const TEST_CYCLES As Integer = 6
Const TEST_FRAMES As Integer = 6
Const TEST_WIDTH As Integer = 128
Const TEST_HEIGHT As Integer = 128

For cycle_index As Integer = 1 To TEST_CYCLES
    If ScreenRes(TEST_WIDTH, TEST_HEIGHT, 32, 1, FB.GFX_RESIZABLE) <> 0 Then
        Print "WAYLAND_MODE_LIFECYCLE_OPEN_FAILED "; cycle_index
        End 1
    End If

    For frame_index As Integer = 1 To TEST_FRAMES
        ScreenLock
        Line (0, 0)-(TEST_WIDTH - 1, TEST_HEIGHT - 1), _
            RGB(cycle_index * 20, frame_index * 20, 80), BF
        ScreenUnlock
        Dim As String ignored_key = Inkey
        Dim As Integer ignored_pressed = MultiKey(FB.SC_ESCAPE)
        Sleep 30, 1
    Next frame_index

    Screen 0
Next cycle_index

Print "WAYLAND_MODE_LIFECYCLE_PASS "; TEST_CYCLES
End 0
' end of wayland-mode-lifecycle.bas
