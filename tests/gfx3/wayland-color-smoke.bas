/'
    Project: FreeBASIC gfxlib3 tests
    File: wayland-color-smoke.bas
    Purpose: Display a changing color pattern through the native graphics path.
    Responsibilities: Exercise frame drawing, page swaps and bounded shutdown.
    This file intentionally does NOT inject input or change display settings.
'/
#lang "fb"
#include once "fbgfx3.bi"

Const TEST_WIDTH As Integer = 360
Const TEST_HEIGHT As Integer = 480
Const TEST_FRAMES As Integer = 300

If ScreenRes(TEST_WIDTH, TEST_HEIGHT, 32, 2, FB.GFX_RESIZABLE) <> 0 Then End 1
Dim As Integer work_page = 1
ScreenSet work_page, 0
For frame_index As Integer = 1 To TEST_FRAMES
    ScreenLock
    Line (0, 0)-(TEST_WIDTH - 1, TEST_HEIGHT - 1), RGB(32, 64, 128), BF
    Line (20, 20)-(340, 120), RGB(0, 220, 80), BF
    Line (20, 150)-(20 + (frame_index Mod 300), 170), RGB(240, 30, 20), BF
    Draw String (32, 50), "FreeBASIC gfxlib3 display test", RGB(0, 0, 0)
    ScreenUnlock
    Dim As Integer visible_page = work_page
    work_page = 1 - work_page
    ScreenSet work_page, visible_page
    Sleep 50, 1
Next frame_index
Screen 0
Print "GFX3_WAYLAND_COLOR_DONE"
End 0
' end of wayland-color-smoke.bas
