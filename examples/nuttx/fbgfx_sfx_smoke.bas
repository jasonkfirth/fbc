'
' Project: FreeBASIC NuttX examples
' ---------------------------------
'
' File: fbgfx_sfx_smoke.bas
'
' Purpose:
'
'     Exercise gfxlib and generated-tone sfxlib in the same NuttX module.
'
' Responsibilities:
'
'     - initialize a small QB-style graphics mode
'     - draw and verify a software-framebuffer pixel
'     - issue one foreground SOUND command
'     - return to text mode before printing the serial success marker
'
' This file intentionally does NOT contain:
'
'     - media file loading
'     - board-specific audio transport setup
'     - hardware-video assertions
'

print "fbcombo: starting"

screen 13

if screenptr = 0 then
    end 30
end if

line (0, 0)-(319, 199), 4, bf
line (12, 12)-(307, 187), 15, b
circle (160, 100), 42, 14
paint (160, 100), 2, 14

pset (10, 10), 5

if point(10, 10) <> 5 then
    end 31
end if

sound 440, 2

screen 0

print "FB_NUTTX_GFX_SFX_SMOKE_OK"

end 0

' end of fbgfx_sfx_smoke.bas
