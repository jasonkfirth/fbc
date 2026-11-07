''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: mobile-controls-smoke.bas
''
'' Purpose:
''
''     Exercise display, device and software-keyboard controls through the
''     gfxlib3 ScreenControl path on a live graphics mode.
''
'' Responsibilities:
''
''     - validate handheld and touch capability values
''     - report live window size, DPI, rotation and keyboard state
''     - issue bounded keyboard show and hide requests
''
'' This file intentionally does NOT contain:
''
''     - text-input focus or synthetic touch input
''     - platform-specific display configuration
''     - assertions about a keyboard's visual layout
''
'' Ownership:
''
''     Replaces mobile-controls-gfx3.log for each run and closes it before
''     releasing the graphics mode.
''

#include once "fbgfx3.bi"

const MOBILE_CONTROLS_MODE_WIDTH as integer = 360
const MOBILE_CONTROLS_MODE_HEIGHT as integer = 480
const MOBILE_CONTROLS_MODE_DEPTH as integer = 32
const MOBILE_CONTROLS_MODE_PAGES as integer = 1
const MOBILE_CONTROLS_MODE_FLAGS as integer = 0
const MOBILE_KEYBOARD_FIRST_SHOW_FRAME as integer = 60
const MOBILE_KEYBOARD_FIRST_HIDE_FRAME as integer = 240
const MOBILE_KEYBOARD_SECOND_SHOW_FRAME as integer = 360
const MOBILE_KEYBOARD_SECOND_HIDE_FRAME as integer = 540
const MOBILE_CONTROLS_SAMPLE_INTERVAL as integer = 30
const MOBILE_CONTROLS_LAST_FRAME as integer = 599

dim as integer window_width, window_height, dpi_value, rotation_value
dim as integer keyboard_visible, keyboard_available, request_value
dim as integer device_class, touch_capability
dim as integer report_file = freefile

if screenres( _
    MOBILE_CONTROLS_MODE_WIDTH, MOBILE_CONTROLS_MODE_HEIGHT, _
    MOBILE_CONTROLS_MODE_DEPTH, MOBILE_CONTROLS_MODE_PAGES, _
    MOBILE_CONTROLS_MODE_FLAGS _
) <> 0 then end 1

' This fixture owns a fixed per-run report; replacing the previous run is intentional.
' fblint: disable-next-line FBL-IO-005
open "mobile-controls-gfx3.log" for output as #report_file
if err <> 0 then
    screen 0
    end 4
end if

device_class = -1
touch_capability = -3
screencontrol FB.GET_DEVICE_CLASS, device_class
screencontrol FB.GET_TOUCH_CAPABILITY, touch_capability
if device_class < FB.GFX_DEVICE_CLASS_UNKNOWN orelse _
   device_class > FB.GFX_DEVICE_CLASS_HANDHELD then
    close #report_file
    screen 0
    end 2
end if
if touch_capability <> -2 andalso touch_capability <> 0 andalso _
   touch_capability <> -1 then
    close #report_file
    screen 0
    end 3
end if

print #report_file, "DEVICE "; device_class; " TOUCH "; touch_capability
screencontrol FB.GET_WINDOW_METRICS, _
    window_width, window_height, dpi_value, rotation_value
print #report_file, "BEFORE "; window_width; " "; window_height; " "; _
    dpi_value; " "; rotation_value

for frame_index as integer = 0 to MOBILE_CONTROLS_LAST_FRAME
    if frame_index = MOBILE_KEYBOARD_FIRST_SHOW_FRAME orelse _
       frame_index = MOBILE_KEYBOARD_SECOND_SHOW_FRAME then
        request_value = -1
        screencontrol FB.SET_SOFT_KEYBOARD, request_value
    end if
    if frame_index = MOBILE_KEYBOARD_FIRST_HIDE_FRAME orelse _
       frame_index = MOBILE_KEYBOARD_SECOND_HIDE_FRAME then
        request_value = 0
        screencontrol FB.SET_SOFT_KEYBOARD, request_value
    end if
    if frame_index mod MOBILE_CONTROLS_SAMPLE_INTERVAL = 0 then
        screencontrol FB.GET_WINDOW_METRICS, _
            window_width, window_height, dpi_value, rotation_value
        screencontrol FB.GET_SOFT_KEYBOARD, _
            keyboard_visible, keyboard_available
        print #report_file, "FRAME "; frame_index; " "; window_width; " "; _
            window_height; " DPI "; dpi_value; " ROT "; rotation_value; _
            " IME "; keyboard_visible; " "; keyboard_available
    end if
    sleep 16, 1
next

request_value = 0
screencontrol FB.SET_SOFT_KEYBOARD, request_value
print #report_file, "GFX3_MOBILE_CONTROLS_DONE"
close #report_file
screen 0
print "GFX3_MOBILE_CONTROLS_DONE"
end 0

'' end of mobile-controls-smoke.bas
