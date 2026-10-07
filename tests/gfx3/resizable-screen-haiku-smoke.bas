''
'' Project: FreeBASIC graphics runtime tests
'' File: resizable-screen-haiku-smoke.bas
'' Purpose: Verify native client resizing through the shared SCREEN contract.
'' Responsibilities: Check resize events, pitch, both pages, black expansion,
'' SCREENLOCK deferral, shrinking, and safe console mouse fallback.
'' This file contains no interactive input or window-decoration assertions.
''

#lang "fb"
#ifdef GFX3_TEST
    #define __FB_GFXLIB3__
#endif
#include once "fbgfx.bi"

extern "C"
    declare function bitmap_boundaries alias "fb_test_haiku_bitmap_boundaries" () as long
    declare function resize_client alias "fb_test_haiku_resize_client" _
        (byval handle as longint, byval width as long, byval height as long) as long
end extern

if bitmap_boundaries() <> 0 then end 34

function wait_for_resize(byref event as fb.EVENT) as integer
    for attempt as integer = 1 to 400 ' FB-LINTER: DISABLE-LINE FBL311 REASON: The counter bounds this resize polling loop.
        while ScreenEvent(@event)
            if event.type = fb.EVENT_WINDOW_RESIZE then return -1
        wend
        Sleep 5, 1
    next
    return 0
end function

dim as integer requested_depth = ValInt(Command(1))
if requested_depth <> 8 andalso requested_depth <> 16 andalso _
    requested_depth <> 32 then end 1
if ScreenRes(160, 120, requested_depth, 2, fb.GFX_RESIZABLE) <> 0 then end 2
WindowTitle "FreeBASIC resizable Haiku smoke"

dim as longint native_window, native_display ' FB-LINTER: DISABLE-LINE FBL423 REASON: ScreenControl returns the native handle in LongInt fields.
ScreenControl fb.GET_WINDOW_HANDLE, native_window, native_display
if native_window = 0 then end 3

dim as uinteger first_pixel, second_pixel
ScreenSet 0, 0
PSet (10, 10), 12
first_pixel = Point(10, 10)
ScreenSet 1, 0
PSet (11, 11), 10
second_pixel = Point(11, 11)
ScreenSet 0, 0
ScreenSync

dim as fb.EVENT event
if resize_client(native_window, 321, 241) <> 0 then end 4
if wait_for_resize(event) = 0 then end 5
if event.width <> 321 orelse event.height <> 241 then end 6

dim as integer logical_width, logical_height, depth, bytes_per_pixel
dim as integer pitch, refresh_rate
dim as string driver
ScreenInfo logical_width, logical_height, depth, bytes_per_pixel, pitch, _
    refresh_rate, driver
if logical_width <> 321 orelse logical_height <> 241 then end 7
if depth <> requested_depth orelse pitch <> logical_width * bytes_per_pixel then end 8
ScreenSet 0, 0
if Point(10, 10) <> first_pixel then end 9
if (Point(320, 240) and &hFFFFFF) <> 0 then end 10
ScreenSet 1, 0
if Point(11, 11) <> second_pixel then end 11
if (Point(320, 240) and &hFFFFFF) <> 0 then end 12

while ScreenEvent(@event) ' FB-LINTER: DISABLE-LINE FBL-LOOP-012 REASON: ScreenEvent consumes each queued event.
    Sleep 1, 1
wend
ScreenLock
dim as any ptr locked_pointer = ScreenPtr
if locked_pointer = 0 then end 13
if resize_client(native_window, 352, 264) <> 0 then end 14
Sleep 100, 1
if ScreenPtr <> locked_pointer then end 15
ScreenUnlock
if wait_for_resize(event) = 0 then end 16
if event.width <> 352 orelse event.height <> 264 then end 17
ScreenSet 0, 0
if Point(10, 10) <> first_pixel then end 18
ScreenSet 1, 0
if Point(11, 11) <> second_pixel then end 19

while ScreenEvent(@event) ' FB-LINTER: DISABLE-LINE FBL-LOOP-012 REASON: ScreenEvent consumes each queued event.
    Sleep 1, 1
wend
if resize_client(native_window, 200, 150) <> 0 then end 20
if wait_for_resize(event) = 0 then end 21
if event.width <> 200 orelse event.height <> 150 then end 22
ScreenControl fb.GET_SCREEN_SIZE, logical_width, logical_height
if logical_width <> 200 orelse logical_height <> 150 then end 23
ScreenSet 0, 0
if Point(10, 10) <> first_pixel then end 24
ScreenSet 1, 0
if Point(11, 11) <> second_pixel then end 25

'' Exercise repeated odd-width replacements and native row padding.
for resize_cycle as integer = 1 to 8
    if resize_client(native_window, 161 + resize_cycle, 121 + resize_cycle) <> 0 then end 29
    if wait_for_resize(event) = 0 then end 30
    if event.width <> 161 + resize_cycle orelse _
        event.height <> 121 + resize_cycle then end 31
    ScreenSet 0, 0
    if Point(10, 10) <> first_pixel then end 32
    ScreenSet 1, 0
    if Point(11, 11) <> second_pixel then end 33
next

Screen 0
dim as integer mouse_x = 0, mouse_y = 0, mouse_z = 0
dim as integer mouse_buttons = 0, mouse_clip = 0
if GetMouse(mouse_x, mouse_y, mouse_z, mouse_buttons, mouse_clip) <> 1 then end 26
if mouse_x <> -1 orelse mouse_y <> -1 orelse mouse_z <> -1 orelse _
    mouse_buttons <> -1 orelse mouse_clip <> -1 then end 28
if ScreenRes(160, 120, requested_depth, 1, _
    fb.GFX_RESIZABLE or fb.GFX_FULLSCREEN) = 0 then end 27
print "PASS Haiku resize depth="; requested_depth; " driver="; driver
end 0

'' end of resizable-screen-haiku-smoke.bas
