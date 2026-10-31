/'
    Project: FreeBASIC graphics qualification
    File: window-metrics.bas
    Purpose: Verify desktop metrics and keyboard no-ops through the public ABI.
    Responsibilities: Native, 32-bit and 64-bit result overloads on a null mode.
    This file does not create a visible window or claim mobile device coverage.
    The fixture owns its diagnostic stream and null graphics mode. Normal
    completion closes both; a failed assertion ends the process.
'/
#lang "fb"
#include once "fbgfx.bi"
Const As Integer probe_width = 320, probe_height = 240
Dim As Integer width_value, height_value, dpi_value, rotation_value
Dim As Integer visible, available, request_value
Dim As Long width32, height32, dpi32, rotation32
Dim As LongInt width64, height64, dpi64, rotation64
Dim report_file As Integer = FreeFile
Open Err For Output As #report_file
If ScreenRes(probe_width,probe_height,32,1,FB.GFX_NULL) <> 0 Then End 1
ScreenControl FB.GET_WINDOW_METRICS, width_value, height_value, dpi_value, rotation_value
If width_value <> probe_width OrElse height_value <> probe_height OrElse dpi_value <= 0 OrElse rotation_value <> 0 Then End 2
ScreenControl FB.GET_WINDOW_METRICS, width32, height32, dpi32, rotation32
ScreenControl FB.GET_WINDOW_METRICS, width64, height64, dpi64, rotation64
If width32 <> width_value OrElse height32 <> height_value OrElse dpi32 <> dpi_value Then End 3
If width64 <> width_value OrElse height64 <> height_value OrElse dpi64 <> dpi_value Then End 4
request_value = -1
ScreenControl FB.SET_SOFT_KEYBOARD, request_value
If request_value <> -1 Then End 5
ScreenControl FB.GET_SOFT_KEYBOARD, visible, available
If visible <> 0 OrElse available <> 0 Then End 6
request_value = 0
ScreenControl FB.SET_SOFT_KEYBOARD, request_value
ScreenControl FB.CLEAR_SOFT_KEYBOARD_REGIONS
Screen 0
Print #report_file, "WINDOW_METRICS_ABI=PASS dpi="; dpi_value
Close #report_file
' end of window-metrics.bas
