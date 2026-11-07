/'
    Project: FreeBASIC graphics qualification
    File: mobile-controls.bas
    Purpose: Exercise window metrics and software keyboard commands on a live mode.
    Responsibilities: Bounded requests, observed state and desktop no-op checks.
    This file does not implement a GUI toolkit or change device settings.
    Ownership:
        This fixture intentionally replaces mobile-controls.log on each run,
        then closes the report and graphics mode on normal completion.
'/
#lang "fb"
#include once "fbgfx.bi"
/'
    The 16 ms loop checks keyboard requests at roughly one, four, six and
    nine seconds so each state has time to reach the platform session.
'/
Const MOBILE_KEYBOARD_FIRST_SHOW_FRAME As Integer = 60
Const MOBILE_KEYBOARD_FIRST_HIDE_FRAME As Integer = 240
Const MOBILE_KEYBOARD_SECOND_SHOW_FRAME As Integer = 360
Const MOBILE_KEYBOARD_SECOND_HIDE_FRAME As Integer = 540
Dim As Integer width_value, height_value, dpi_value, rotation_value
Dim As Integer visible, available, request_value
Dim As Integer device_class, touch_capability
Dim report_file As Integer = FreeFile
device_class = -1
touch_capability = -3
' This fixture owns its fixed per-run report; replacing the previous run is intentional.
' fblint: disable-next-line FBL-IO-005
Open "mobile-controls.log" For Output As #report_file
If Err <> 0 Then
	End 1
End If
ScreenControl FB.GET_DEVICE_CLASS, device_class
ScreenControl FB.GET_TOUCH_CAPABILITY, touch_capability
If device_class < FB.GFX_DEVICE_CLASS_UNKNOWN OrElse _
	device_class > FB.GFX_DEVICE_CLASS_HANDHELD Then
	Close #report_file
	End 2
End If
If touch_capability <> -2 AndAlso touch_capability <> 0 AndAlso _
	touch_capability <> -1 Then
	Close #report_file
	End 3
End If
Print #report_file, "DEVICE "; device_class; " TOUCH "; touch_capability
ScreenControl FB.GET_WINDOW_METRICS, width_value, height_value, dpi_value, rotation_value
Print #report_file, "BEFORE "; width_value; " "; height_value; " "; dpi_value; " "; rotation_value
If ScreenRes(360, 480, 32) <> 0 Then
	Close #report_file
	End 1
End If
For frame_index As Integer = 0 To 599
    If frame_index = MOBILE_KEYBOARD_FIRST_SHOW_FRAME OrElse _
       frame_index = MOBILE_KEYBOARD_SECOND_SHOW_FRAME Then
        request_value = -1
        ScreenControl FB.SET_SOFT_KEYBOARD, request_value
    End If
    If frame_index = MOBILE_KEYBOARD_FIRST_HIDE_FRAME OrElse _
       frame_index = MOBILE_KEYBOARD_SECOND_HIDE_FRAME Then
        request_value = 0
        ScreenControl FB.SET_SOFT_KEYBOARD, request_value
    End If
    If frame_index Mod 30 = 0 Then
        ScreenControl FB.GET_WINDOW_METRICS, width_value, height_value, dpi_value, rotation_value
        ScreenControl FB.GET_SOFT_KEYBOARD, visible, available
        Print #report_file, "FRAME "; frame_index; " "; width_value; " "; height_value; " "; dpi_value; " "; rotation_value; " IME "; visible; " "; available
    End If
    Sleep 16, 1
Next frame_index
request_value = 0
ScreenControl FB.SET_SOFT_KEYBOARD, request_value
Screen 0
Print #report_file, "MOBILE_CONTROLS_DONE"
Close #report_file
' end of mobile-controls.bas
