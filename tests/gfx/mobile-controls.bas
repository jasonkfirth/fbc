/'
    Project: FreeBASIC graphics qualification
    File: mobile-controls.bas
    Purpose: Exercise window metrics and software keyboard commands on a live mode.
    Responsibilities: Bounded requests, observed state and desktop no-op checks.
    This file does not implement a GUI toolkit or change device settings.
    The fixture owns its report and graphics mode, closing both on completion.
'/
#lang "fb"
#include once "fbgfx.bi"
Dim As Integer width_value, height_value, dpi_value, rotation_value
Dim As Integer visible, available, request_value
Dim report_file As Integer = FreeFile
Open "mobile-controls.log" For Output As #report_file
ScreenControl FB.GET_WINDOW_METRICS, width_value, height_value, dpi_value, rotation_value
Print #report_file, "BEFORE "; width_value; " "; height_value; " "; dpi_value; " "; rotation_value
If ScreenRes(360, 480, 32) <> 0 Then Close #report_file: End 1
For frame_index As Integer = 0 To 599
    If frame_index = 60 OrElse frame_index = 360 Then
        request_value = -1: ScreenControl FB.SET_SOFT_KEYBOARD, request_value
    End If
    If frame_index = 240 OrElse frame_index = 540 Then
        request_value = 0: ScreenControl FB.SET_SOFT_KEYBOARD, request_value
    End If
    If frame_index Mod 30 = 0 Then
        ScreenControl FB.GET_WINDOW_METRICS, width_value, height_value, dpi_value, rotation_value
        ScreenControl FB.GET_SOFT_KEYBOARD, visible, available
        Print #report_file, "FRAME "; frame_index; " "; width_value; " "; height_value; " "; dpi_value; " "; rotation_value; " IME "; visible; " "; available
    End If
    Sleep 16, 1
Next frame_index
request_value = 0: ScreenControl FB.SET_SOFT_KEYBOARD, request_value
Screen 0
Print #report_file, "MOBILE_CONTROLS_DONE"
Close #report_file
' end of mobile-controls.bas
