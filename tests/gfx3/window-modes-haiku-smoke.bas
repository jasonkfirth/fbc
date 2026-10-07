''
'' Project: FreeBASIC graphics runtime tests
'' File: window-modes-haiku-smoke.bas
'' Purpose: Exercise ScreenEvent-only resizing and repeated fullscreen switches.
'' Responsibilities: Check desktop queries, native geometry and looper lifetime.
'' This file contains no remote desktop connection or simulated rendering.
''
#lang "fb"
#include once "fbgfx.bi"
#include once "crt/stdio.bi"

sub mode_trace(byref message as const string)
    dim as string line_text = message & Chr(10)
    fputs(strptr(line_text), stderr)
    fflush(stderr)
end sub

extern "C"
    declare function native_mode alias "fb_test_haiku_mode" _
        (byval handle as longint, byval fullscreen as long, _
        byval width as long, byval height as long) as long
    declare function resize_client alias "fb_test_haiku_resize_client" _
        (byval handle as longint, byval width as long, byval height as long) as long
end extern

function mode_matches(byval fullscreen as long, byval client_width as long, _
    byval client_height as long) as integer
    dim as longint handle, display ' FB-LINTER: DISABLE-LINE FBL423 REASON: ScreenControl returns the native handle in LongInt fields.
    ScreenControl fb.GET_WINDOW_HANDLE, handle, display
    return native_mode(handle, fullscreen, client_width, client_height) = 0
end function

mode_trace "create window"
if ScreenRes(321, 241, 32, 2, fb.GFX_RESIZABLE) <> 0 then end 1
mode_trace "query desktop"
dim as integer desktop_width, desktop_height
ScreenControl fb.GET_DESKTOP_SIZE, desktop_width, desktop_height
if desktop_width < 321 orelse desktop_height < 241 then end 2
dim as fb.EVENT event
dim as longint handle, display ' FB-LINTER: DISABLE-LINE FBL423 REASON: ScreenControl returns the native handle in LongInt fields.

for cycle as integer = 1 to 3
    mode_trace "resize window"
    ScreenControl fb.GET_WINDOW_HANDLE, handle, display
    if resize_client(handle, 355, 257) <> 0 then end 3
    dim as integer resized = 0
    ' The viewer polls ScreenEvent, without a separate ScreenInfo resize pump.
    for attempt as integer = 1 to 400
        while ScreenEvent(@event)
            if event.type = fb.EVENT_WINDOW_RESIZE andalso _
                event.width = 355 andalso event.height = 257 then resized = -1
        wend
        if resized then exit for
        Sleep 5, 1
    next
    if resized = 0 then end 4
    mode_trace "check window"
    if mode_matches(0, 355, 257) = 0 then end 5

    dim as integer current_desktop_width, current_desktop_height
    ScreenControl fb.GET_DESKTOP_SIZE, current_desktop_width, current_desktop_height
    if current_desktop_width <> desktop_width orelse _
        current_desktop_height <> desktop_height then end 6
    mode_trace "enter fullscreen"
    if ScreenRes(desktop_width, desktop_height, 32, 2, fb.GFX_FULLSCREEN) <> 0 then end 7
    mode_trace "check fullscreen"
    if mode_matches(-1, desktop_width, desktop_height) = 0 then end 8
    ScreenSet 1, 0
    Line (0, 0)-(desktop_width - 1, desktop_height - 1), &H123456, BF
    ScreenSet 0, 1
    Sleep 20, 1

    mode_trace "leave fullscreen"
    if ScreenRes(321, 241, 32, 2, fb.GFX_RESIZABLE) <> 0 then end 9
    mode_trace "check restored window"
    if mode_matches(0, 321, 241) = 0 then end 10
next

mode_trace "close graphics"
Screen 0
Print "PASS Haiku ScreenEvent resize, desktop size and repeated fullscreen modes"
' end of window-modes-haiku-smoke.bas
