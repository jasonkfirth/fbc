'
' Project: FreeBASIC NuttX examples
' ---------------------------------
'
' File: fbgfx_demo.bas
'
' Purpose:
'
'     Keep a visible SCREEN 13 graphics pattern on the RP2350-PiZero
'     HDMI/DVI output during board bring-up.
'
' Responsibilities:
'
'     - enter the low-memory paletted SCREEN 13 mode
'     - draw a high-contrast palette pattern that should be obvious on HDMI
'     - keep the screen active until a key is pressed
'
' This file intentionally does NOT contain:
'
'     - SD card tests
'     - sound tests
'     - USB keyboard setup
'     - broad gfxlib2 conformance coverage
'

screen 13

if screenptr = 0 then
    print "FB_NUTTX_GFX_DEMO_NO_SCREEN"
    end 20
end if

palette 0, 0
palette 1, &h0033ff
palette 2, &h00cc33
palette 3, &h00ffff
palette 4, &hff3333
palette 5, &hff33ff
palette 6, &hffff33
palette 7, &hffffff
palette 8, &h000088
palette 9, &h008800
palette 10, &h008888
palette 11, &h880000
palette 12, &h880088
palette 13, &h888800
palette 14, &h888888
palette 15, &h444444

dim as integer x
dim as integer y
dim as integer band
dim as string key_text

for y = 0 to 199
    band = (y \ 25) mod 8

    for x = 0 to 319
        pset (x, y), ((x \ 40) + band) mod 16
    next
next

line (0, 0)-(319, 199), 7, b
line (1, 1)-(318, 198), 0, b
line (8, 8)-(311, 191), 15, b

locate 5, 6
color 7, 0
print "FreeBASIC NuttX"
locate 7, 6
print "RP2350 DVI demo"
locate 10, 6
print "Press Q to exit"

do
    key_text = inkey$
loop while len(key_text) > 0

do
    sleep 25
    key_text = inkey$
loop while (key_text <> "q") and (key_text <> "Q")

screen 0
print "FB_NUTTX_GFX_DEMO_OK"
end 0

' end of fbgfx_demo.bas
