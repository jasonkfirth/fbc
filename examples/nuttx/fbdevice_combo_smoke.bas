'
' Project: FreeBASIC NuttX examples
' ---------------------------------
'
' File: fbdevice_combo_smoke.bas
'
' Purpose:
'
'     Exercise the controller-style QEMU hub setup as one FreeBASIC program.
'
' Responsibilities:
'
'     - enter the low-memory paletted graphics mode used by the RP2350 DVI path
'     - read injected USB keyboard and mouse events through gfxlib
'     - write and read a file through the mounted USB mass-storage path
'     - report a stable marker for the device-lab audit
'
' Ownership:
'
'     Screen 13 remains active only while the device checks run.  The storage
'     file handle owns one successful Open at a time, and the fixed fixture is
'     removed only after DIR confirms that it exists.
'
' This file intentionally does NOT contain:
'
'     - board-specific HDMI setup
'     - USB host controller initialization
'     - broad filesystem stress tests
'     - network traffic generation
'

const USB_ROOT = "/mnt/sd0"
const USB_FILE = USB_ROOT + "/fb_combo_smoke.txt"
const USB_TEXT = "FreeBASIC combo storage"

' Screen 13 is the NuttX DVI device-lab contract. FB-LINTER: DISABLE-NEXT-LINE FBL734
screen 13

if screenptr = 0 then
    end 20
end if

print "device combo smoke"

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
    screen 0
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
    screen 0
    end 22
end if

if saw_mouse = 0 then
    screen 0
    end 23
end if

sub CleanupUsbFile()

    if dir(USB_FILE) <> "" then
        kill USB_FILE
    end if

end sub

dim as string line_text
dim as integer storage_file_handle = freefile

if storage_file_handle <= 0 then
    screen 0
    print "fbdevice_combo: FreeFile failed"
    end 25
end if

'' Fixed NuttX mass-storage fixture; the Open result is checked immediately below. FB-LINTER: DISABLE-NEXT-LINE FBL-IO-005
open USB_FILE for output as #storage_file_handle
if err <> 0 then
    screen 0
    print "fbdevice_combo: output open failed with ERR ="; err
    end 25
end if

print #storage_file_handle, USB_TEXT
close #storage_file_handle

open USB_FILE for input as #storage_file_handle
if err <> 0 then
    CleanupUsbFile()
    screen 0
    print "fbdevice_combo: input open failed with ERR ="; err
    end 25
end if

line input #storage_file_handle, line_text
close #storage_file_handle

if line_text <> USB_TEXT then
    CleanupUsbFile()
    screen 0
    print "fbdevice_combo: unexpected readback: "; line_text
    end 24
end if

CleanupUsbFile()

screen 0

print "FB_NUTTX_DEVICE_COMBO_SMOKE_OK"

end 0

' end of fbdevice_combo_smoke.bas
