''
'' FreeBASIC graphics runtime tests
'' File: input-events-haiku-smoke.bas
'' Purpose: Verify native Haiku input reaches ScreenEvent and legacy polling.
'' Responsibilities: Check repeat/release, modifiers, buttons and wheel signs.
'' This test contains no VNC authentication or remote desktop interaction.
''
#lang "fb"
#include once "fbgfx.bi"

extern "C"
    declare function inject alias "fb_test_haiku_input" _
        (byval handle as longint, byval stage as long) as long
end extern

function expect_event(byval kind as integer, byval first as integer, _
    byval second as integer = 0) as integer
    dim as fb.EVENT event
    for attempt as integer = 1 to 400 ' FB-LINTER: DISABLE-LINE FBL311 REASON: The counter bounds this event polling loop.
        while ScreenEvent(@event)
            if event.type = fb.EVENT_WINDOW_GOT_FOCUS orelse _
                event.type = fb.EVENT_WINDOW_LOST_FOCUS then continue while
            if event.type <> kind then
                print "Unexpected event type "; event.type; " expected "; kind
                return 0
            end if
            select case kind
            case fb.EVENT_KEY_PRESS, fb.EVENT_KEY_RELEASE, fb.EVENT_KEY_REPEAT
                return IIf(event.scancode = first andalso event.ascii = second, -1, 0)
            case fb.EVENT_MOUSE_MOVE
                return IIf(event.x = first andalso event.y = second, -1, 0)
            case fb.EVENT_MOUSE_BUTTON_PRESS, fb.EVENT_MOUSE_BUTTON_RELEASE
                return IIf(event.button = first, -1, 0)
            case fb.EVENT_MOUSE_WHEEL, fb.EVENT_MOUSE_HWHEEL
                return IIf(event.z = first, -1, 0)
            end select
        wend
        Sleep 5, 1
    next
    return 0
end function

if ScreenRes(160, 120, 32, 1, fb.GFX_RESIZABLE) <> 0 then end 1
WindowTitle "FreeBASIC Haiku input smoke"
dim as longint handle, display ' FB-LINTER: DISABLE-LINE FBL423 REASON: ScreenControl returns the native handle in LongInt fields.
ScreenControl fb.GET_WINDOW_HANDLE, handle, display
if handle = 0 then end 2
Sleep 100, 1
dim as fb.EVENT pending
while ScreenEvent(@pending) ' FB-LINTER: DISABLE-LINE FBL-LOOP-012 REASON: ScreenEvent consumes each queued event.
    Sleep 1, 1
wend
while Inkey <> "" ' FB-LINTER: DISABLE-LINE FBL-LOOP-012 REASON: Inkey consumes each queued character.
    Sleep 1, 1
wend

if inject(handle, 1) <> 0 then end 3
if expect_event(fb.EVENT_KEY_PRESS, fb.SC_A, Asc("a")) = 0 then end 4
if expect_event(fb.EVENT_KEY_REPEAT, fb.SC_A, Asc("a")) = 0 then end 5
if expect_event(fb.EVENT_KEY_RELEASE, fb.SC_A, Asc("a")) = 0 then end 6
if Inkey <> "a" orelse Inkey <> "a" then end 7
if MultiKey(fb.SC_A) <> 0 then end 8

if inject(handle, 2) <> 0 then end 9
if expect_event(fb.EVENT_KEY_PRESS, fb.SC_ENTER, 13) = 0 then end 10
if expect_event(fb.EVENT_KEY_RELEASE, fb.SC_ENTER, 13) = 0 then end 11
if expect_event(fb.EVENT_KEY_PRESS, fb.SC_RIGHTBRACKET, Asc("]")) = 0 then end 12
if expect_event(fb.EVENT_KEY_RELEASE, fb.SC_RIGHTBRACKET, Asc("]")) = 0 then end 13
if Inkey <> Chr(13) orelse Inkey <> "]" then end 14

if inject(handle, 3) <> 0 then end 15
if expect_event(fb.EVENT_KEY_PRESS, fb.SC_CONTROL, 0) = 0 then end 16
if expect_event(fb.EVENT_KEY_PRESS, fb.SC_C, 3) = 0 then end 17
if expect_event(fb.EVENT_KEY_RELEASE, fb.SC_C, 3) = 0 then end 18
if expect_event(fb.EVENT_KEY_RELEASE, fb.SC_CONTROL, 0) = 0 then end 19
if expect_event(fb.EVENT_KEY_PRESS, fb.SC_ALT, 0) = 0 then end 20
if expect_event(fb.EVENT_KEY_PRESS, fb.SC_A, Asc("a")) = 0 then end 21
if expect_event(fb.EVENT_KEY_RELEASE, fb.SC_A, Asc("a")) = 0 then end 22
if expect_event(fb.EVENT_KEY_RELEASE, fb.SC_ALT, 0) = 0 then end 23
while Inkey <> "" ' FB-LINTER: DISABLE-LINE FBL-LOOP-012 REASON: Inkey consumes each queued character.
    Sleep 1, 1
wend

if inject(handle, 4) <> 0 then end 24
if expect_event(fb.EVENT_MOUSE_MOVE, 40, 50) = 0 then end 25
if expect_event(fb.EVENT_MOUSE_BUTTON_PRESS, fb.BUTTON_LEFT) = 0 then end 26
if expect_event(fb.EVENT_MOUSE_BUTTON_PRESS, fb.BUTTON_RIGHT) = 0 then end 27
if expect_event(fb.EVENT_MOUSE_BUTTON_RELEASE, fb.BUTTON_LEFT) = 0 then end 28
if expect_event(fb.EVENT_MOUSE_BUTTON_RELEASE, fb.BUTTON_RIGHT) = 0 then end 29
dim as integer x, y, z, buttons
if GetMouse(x, y, z, buttons) <> 0 then end 30
if x <> 40 orelse y <> 50 orelse buttons <> 3 then end 31
if GetMouse(x, y, z, buttons) <> 0 orelse buttons <> 0 then end 32

if inject(handle, 5) <> 0 then end 33
if expect_event(fb.EVENT_MOUSE_WHEEL, 1) = 0 then end 34
if expect_event(fb.EVENT_MOUSE_HWHEEL, 1) = 0 then end 35
if GetMouse(x, y, z, buttons) <> 0 orelse z <> 1 then end 36
if ScreenEvent(@pending) <> 0 then end 37

if inject(handle, 6) <> 0 then end 38
if expect_event(fb.EVENT_KEY_PRESS, fb.SC_A, Asc("A")) = 0 then end 39
if expect_event(fb.EVENT_KEY_RELEASE, fb.SC_A, Asc("A")) = 0 then end 40
if expect_event(fb.EVENT_KEY_PRESS, fb.SC_F1, 0) = 0 then end 41
if expect_event(fb.EVENT_KEY_RELEASE, fb.SC_F1, 0) = 0 then end 42
if Inkey <> "A" orelse Inkey <> Chr(255, Asc(";")) then end 43
Screen 0
print "PASS Haiku native keyboard, modifiers, mouse and wheel events"
end 0
'' end of input-events-haiku-smoke.bas
