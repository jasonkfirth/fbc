'' Project: FreeBASIC graphics text tests
'' File: text-types.bas
'' Purpose: Compare byte, wide, and UTF-8 bitmap text routing.
'' Responsibilities: Check scalar advances, fallback glyphs, and pixel output.
'' This file intentionally does NOT require a real display or scalable fonts.

' TEST_MODE : COMPILE_AND_RUN_OK
#include once "fbgfx.bi"

screenres 80, 48, 32, 1, fb.GFX_NULL
dim as ustring unicodeText = uchr(65, &hE9, &h1F600)
dim as wstring * 16 wideText = wstr(unicodeText)
dim as string glyphs = chr(65, &hE9, 63)
dim as long byteWidth, byteHeight, wideWidth, wideHeight, unicodeWidth, unicodeHeight
assert(fb.DrawStringSize(glyphs, byteWidth, byteHeight) = 0)
assert(fb.DrawStringSize(wideText, wideWidth, wideHeight) = 0)
assert(fb.DrawStringSize(unicodeText, unicodeWidth, unicodeHeight) = 0)
assert(byteWidth = 24 and wideWidth = byteWidth and unicodeWidth = byteWidth)
assert(byteHeight = wideHeight and byteHeight = unicodeHeight)
dim as any ptr firstImage = imagecreate(32, 32)
dim as any ptr secondImage = imagecreate(32, 32)
dim as any ptr thirdImage = imagecreate(32, 32)
assert(firstImage <> 0 and secondImage <> 0 and thirdImage <> 0)
draw string firstImage, (0, 0), glyphs
draw string secondImage, (0, 0), wideText
draw string thirdImage, (0, 0), unicodeText
for y as integer = 0 to 31
	for x as integer = 0 to 31
		assert(point(x, y, firstImage) = point(x, y, secondImage))
		assert(point(x, y, firstImage) = point(x, y, thirdImage))
	next
next
imagedestroy firstImage
imagedestroy secondImage
imagedestroy thirdImage
screen 0
print "text-types graphics: scalar measurement and pixels passed"

'' end of text-types.bas
