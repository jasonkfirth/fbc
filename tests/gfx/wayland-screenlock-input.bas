/'
    Project: FreeBASIC graphics tests
    File: wayland-screenlock-input.bas
    Purpose: Exercise input and page changes while a frame is locked.
    Responsibilities: Verify that nested driver calls return before unlock.
    This file intentionally does NOT inject input or change display settings.
'/
#lang "fb"
#include once "fbgfx.bi"

Dim As Integer mouse_x, mouse_y, mouse_wheel, mouse_buttons
Dim As Integer mouse_result, pressed
Dim As String key_value

If ScreenRes(160, 120, 32, 2, FB.GFX_RESIZABLE) <> 0 Then
    Print "WAYLAND_SCREENLOCK_INPUT_OPEN_FAILED"
    End 1
End If

ScreenLock
mouse_result = GetMouse(mouse_x, mouse_y, mouse_wheel, mouse_buttons)
key_value = Inkey
pressed = MultiKey(FB.SC_ESCAPE)
ScreenSet 1, 1
ScreenControl FB.SET_SOFT_KEYBOARD_REGION, 0, 0, 40, 20
ScreenControl FB.CLEAR_SOFT_KEYBOARD_REGIONS
ScreenUnlock

Screen 0
Print "WAYLAND_SCREENLOCK_INPUT_PASS"
End 0
' end of wayland-screenlock-input.bas
