'
' Project: FreeBASIC NuttX examples
' ---------------------------------
'
' File: fbgfx_smoke.bas
'
' Purpose:
'
'     Exercise the first real gfxlib2 path under the NuttX QEMU harness.
'
' Responsibilities:
'
'     - enter the low-memory paletted graphics modes used during bring-up
'     - prove graphics PRINT changes the framebuffer
'     - prove PSET/POINT still work through the generic gfxlib2 core
'     - touch the keyboard and software mouse hooks
'
' This file intentionally does NOT contain:
'
'     - board-specific HDMI setup
'     - USB HID setup
'     - audio tests
'     - broad fbctests coverage
'

screen 1

if screenptr = 0 then
    end 20
end if

pset (2, 2), 3

if point(2, 2) <> 3 then
    end 21
end if

screen 7

if screenptr = 0 then
    end 22
end if

pset (3, 3), 12

if point(3, 3) <> 12 then
    end 23
end if

screen 13

print "gfx print ok"

dim framebuffer as ubyte ptr
dim nonzero_pixels as integer
dim i as integer

framebuffer = screenptr

if framebuffer = 0 then
    end 10
end if

for i = 0 to (320 * 16) - 1
    if framebuffer[i] <> 0 then
        nonzero_pixels += 1
    end if
next

if nonzero_pixels = 0 then
    end 11
end if

pset (10, 10), 4

if point(10, 10) <> 4 then
    end 12
end if

dim mx as integer
dim my as integer
dim mz as integer
dim mb as integer
dim mc as integer

if getmouse(mx, my, mz, mb, mc) <> 0 then
    end 13
end if

if (mx < 0) or (my < 0) then
    end 14
end if

dim key_text as string

for i = 1 to 1000
    key_text = inkey$

    if len(key_text) > 0 then
        exit for
    end if

    sleep 10
next

if key_text <> "z" then
    end 15
end if

screen 0

print "NUTTX_GFX_SMOKE_OK"

end 0

' end of fbgfx_smoke.bas
