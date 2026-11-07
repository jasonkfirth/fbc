/'
    Project: FreeBASIC Linux graphics tests
    File: wayland-focus-probe.bas
    Purpose: Open a second window briefly during a focus restoration test.
    Responsibilities: Draw one frame, process events and close normally.
    This file does not request a keyboard or synthesize input.
'/
#lang "fb"
#include once "fbgfx.bi"

Const WINDOW_WIDTH As Integer = 240
Const WINDOW_HEIGHT As Integer = 160
Const DURATION_SECONDS As Double = 6.0
Const POLL_INTERVAL_MS As Integer = 40

If ScreenRes(WINDOW_WIDTH, WINDOW_HEIGHT, 32, 1, FB.GFX_RESIZABLE) <> 0 Then
    Print "WAYLAND_FOCUS_PROBE_OPEN_FAILED"
    End 1
End If

ScreenLock
Line (0, 0)-(WINDOW_WIDTH - 1, WINDOW_HEIGHT - 1), RGB(32, 64, 128), BF
Draw String (12, 12), "Focus test", RGB(255, 255, 255)
ScreenUnlock
Print "WAYLAND_FOCUS_PROBE_READY"

Dim As Double started = Timer
Dim As Integer close_requested
Do
    Dim As FB.Event event_value
    While ScreenEvent(@event_value) <> 0
        If event_value.type = FB.EVENT_WINDOW_CLOSE Then close_requested = -1
    Wend
    If close_requested <> 0 Then Exit Do
    Dim As Double elapsed = Timer - started
    If elapsed < 0 Then elapsed += 86400
    If elapsed >= DURATION_SECONDS Then Exit Do
    Sleep POLL_INTERVAL_MS, 1
Loop

Screen 0
Print "WAYLAND_FOCUS_PROBE_CLOSED"
End 0
' end of wayland-focus-probe.bas
