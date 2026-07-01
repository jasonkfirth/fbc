'
' Project: FreeBASIC NuttX examples
' ---------------------------------
'
' File: fbgfx_hid_smoke.bas
'
' Purpose:
'
'     Prove that gfxlib2 can read keyboard and mouse events from NuttX
'     USB HID device nodes under the QEMU hub harness.
'
' Responsibilities:
'
'     - enter a low-memory paletted graphics mode
'     - wait for an injected USB keyboard event
'     - wait for an injected USB mouse movement event
'     - report a stable marker for the device-lab audit
'
' This file intentionally does NOT contain:
'
'     - broad graphics drawing coverage
'     - board-specific HDMI setup
'     - USB host controller initialization
'     - serial-console input fallback checks
'

screen 13

if screenptr = 0 then
    end 20
end if

print "gfx hid input smoke"

dim start_x as integer
dim start_y as integer
dim start_z as integer
dim start_buttons as integer
dim start_clip as integer

dim mx as integer
dim my as integer
dim mz as integer
dim mb as integer
dim mc as integer

if getmouse(start_x, start_y, start_z, start_buttons, start_clip) <> 0 then
    end 21
end if

dim key_text as string
dim saw_key as integer
dim saw_mouse as integer
dim i as integer

for i = 1 to 250
    key_text = inkey$

    if key_text = "z" then
        saw_key = -1
    end if

    if getmouse(mx, my, mz, mb, mc) = 0 then
        if (mx <> start_x) or (my <> start_y) or (mz <> start_z) or _
            (mb <> start_buttons) then
            saw_mouse = -1
        end if
    end if

    if (saw_key <> 0) and (saw_mouse <> 0) then
        exit for
    end if

    sleep 20
next

if saw_key = 0 then
    end 22
end if

if saw_mouse = 0 then
    end 23
end if

screen 0

print "FB_NUTTX_GFX_HID_SMOKE_OK"

end 0

' end of fbgfx_hid_smoke.bas
