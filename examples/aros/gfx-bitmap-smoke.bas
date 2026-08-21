''
'' FreeBASIC AROS bitmap graphics smoke test
'' ------------------------------------------
''
'' File: gfx-bitmap-smoke.bas
''
'' Purpose:
''
''     Exercise BLOAD and PUT with a true-colour BMP on native AROS gfxlib2.
''
'' Responsibilities:
''
''     - load the staged title bitmap into an ImageCreate buffer
''     - copy the image through gfxlib2's PSET path
''     - record representative screen pixel values for the host test runner
''     - keep the completed image visible for a screenshot
''
'' This file intentionally does NOT contain:
''
''     - game logic or input handling
''     - image file conversion
''     - platform-specific presentation code
''

#include once "fbgfx.bi"

const IMAGE_WIDTH = 640
const IMAGE_HEIGHT = 480
const HOLD_SECONDS = 12
dim elapsed_seconds as double
dim image as any ptr
dim marker_file as integer
dim current_time as double
dim sample0 as uinteger
dim sample1 as uinteger
dim sample2 as uinteger
dim start_time as double

screenres IMAGE_WIDTH, IMAGE_HEIGHT, 32, 2
if screenptr = 0 then
    end 1
end if

screenset 1, 0

image = imagecreate(IMAGE_WIDTH, IMAGE_HEIGHT, rgb(255, 0, 255))
if image = 0 then
    screen 0
    end 1
end if

if bload("GfxSmoke:title.bmp", image) <> 0 then
    imagedestroy image
    screen 0
    end 1
end if

''
'' BLOAD constructs a 32-bit ImageCreate buffer.  The two-page transparent
'' blit and ScreenCopy sequence matches Star Phalanx's normal frame boundary.
'' POINT reads the completed work page, allowing host evidence to distinguish
'' loader errors from presentation bugs.
''
put (0, 0), image, trans
screencopy 1, 0
cls
sleep 0
sample0 = point(50, 50)
sample1 = point(320, 200)
sample2 = point(500, 350)

marker_file = freefile
open "GfxSmoke:gfx-colour-smoke.drawn" for output as #marker_file
print #marker_file, "AROS_GFX_COLOUR_SMOKE: DRAWN"
print #marker_file, "AROS_GFX_BMP_SMOKE: P50_50="; hex(sample0, 8)
print #marker_file, "AROS_GFX_BMP_SMOKE: P320_200="; hex(sample1, 8)
print #marker_file, "AROS_GFX_BMP_SMOKE: P500_350="; hex(sample2, 8)
close #marker_file

''
'' Window activation can interrupt the regular graphics SLEEP path on AROS.
'' Keep polling presentation until the fixed hold interval expires instead.
''
start_time = timer
do
    sleep 0
    current_time = timer
    elapsed_seconds = current_time - start_time
    if elapsed_seconds < 0 then
        elapsed_seconds += 86400
    end if
loop until elapsed_seconds >= HOLD_SECONDS

imagedestroy image
screen 0

'' end of gfx-bitmap-smoke.bas
