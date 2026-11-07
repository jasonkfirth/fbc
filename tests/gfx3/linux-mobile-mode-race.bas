/'
    Project: FreeBASIC gfxlib3 Linux tests
    -------------------------------------

    File: linux-mobile-mode-race.bas

    Purpose:

        Exercise mobile controls while real graphics modes close and reopen.

    Responsibilities:

        - run keyboard and text-region controls from a worker thread
        - verify each newly opened window reports its drawable dimensions
        - stop and join the worker before releasing its synchronization object

    This file intentionally does NOT contain:

        - physical input injection or a real keyboard service
        - application widgets or user data

    Threading and ownership:

        This file owns the Shared state. state_mutex protects stop_worker and
        control_count; the main thread creates it before starting the worker
        and destroys it only after joining that worker.

    Run on an isolated Linux X11 display with the GDBus session unset:

        fbc -gfx3 -mt linux-mobile-mode-race.bas
        FBGFX=gles FBGFX_GFX3_PLATFORM=x11 ./linux-mobile-mode-race
'/

#include once "fbgfx3.bi"

const test_width as integer = 320
const test_height as integer = 240
const test_depth as integer = 32
const mode_cycles as integer = 12
' Match the supported layout-DPI bounds of the Linux graphics backend.
const min_layout_dpi as integer = 72
const max_layout_dpi as integer = 768
' Rotation is reported in degrees at each quarter turn.
const rotation_upright as integer = 0
const rotation_right as integer = 90
const rotation_inverted as integer = 180
const rotation_left as integer = 270

dim shared as any ptr state_mutex
dim shared as integer stop_worker
dim shared as integer control_count

private sub control_worker(byval unused as any ptr)
    dim as integer visible, available, hide_request = 0
    dim as integer x = 4
    dim as integer y = 5
    dim as integer rect_width = 40
    dim as integer rect_height = 20
    dim as integer window_width, window_height, dpi_value, rotation_value
    dim as integer stop_now

    do
        mutexlock(state_mutex)
        stop_now = stop_worker
        mutexunlock(state_mutex)
        if stop_now <> 0 then exit do

        screencontrol FB.GET_SOFT_KEYBOARD, visible, available
        screencontrol FB.SET_SOFT_KEYBOARD, hide_request
        screencontrol FB.SET_SOFT_KEYBOARD_REGION, x, y, rect_width, rect_height
        screencontrol FB.GET_WINDOW_METRICS, window_width, window_height, dpi_value, rotation_value
        mutexlock(state_mutex)
        control_count += 1
        mutexunlock(state_mutex)
        sleep 1, 1
    loop
end sub

dim as any ptr worker
dim as integer failure = 0, completed = 0
dim as integer window_width, window_height, dpi_value, rotation_value

state_mutex = mutexcreate()
if state_mutex = 0 then end 2
worker = threadcreate(@control_worker)
if worker = 0 then
    mutexdestroy(state_mutex)
    end 3
end if

for cycle as integer = 1 to mode_cycles
    if screenres(test_width, test_height, test_depth, 1, FB.GFX_RESIZABLE) <> 0 then
        failure = 4
        exit for
    end if
    screencontrol FB.GET_WINDOW_METRICS, window_width, window_height, dpi_value, rotation_value
    if window_width <> test_width orelse window_height <> test_height then failure = 5
    if dpi_value < min_layout_dpi orelse dpi_value > max_layout_dpi then failure = 6
    if rotation_value <> rotation_upright andalso rotation_value <> rotation_right andalso _
       rotation_value <> rotation_inverted andalso rotation_value <> rotation_left then failure = 7
    sleep 20, 1
    screen 0
    if failure <> 0 then exit for
    completed += 1
next

mutexlock(state_mutex)
stop_worker = 1
mutexunlock(state_mutex)
threadwait(worker)
mutexdestroy(state_mutex)
' The failure value is restricted to the small exit codes assigned above.
' fblint: disable-next-line FBL-NUM-003
if failure <> 0 then end failure
if completed <> mode_cycles orelse control_count = 0 then end 8
print "GFX3_LINUX_MOBILE_MODE_RACE_PASS cycles="; completed; _
    " controls="; control_count; " size="; window_width; "x"; window_height; _
    " dpi="; dpi_value; " rotation="; rotation_value
end 0

' end of linux-mobile-mode-race.bas
