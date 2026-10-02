'' Project: FreeBASIC gfxlib3 Unicode tests
'' File: text-types.bas
'' Purpose: Load Unicode asset paths through all three surface overloads.
'' Responsibilities: Verify decoding, ownership, pixel output, and file cleanup.
'' This file intentionally does NOT require a hardware GPU or native window.

'' The Unicode runner selects gfxlib3 and enables assertions for this test.
#include once "fbgfx3.bi"

dim as ustring filename = "text-asset-" + uchr(&hE9, &h1F600) + ".bmp"
dim as wstring * 128 wideFilename = wstr(filename)
dim as string byteFilename = cast(string, filename)
assert(screenres(32, 24, 32, 1, fb.GFX_NULL) = 0)
dim as any ptr image = imagecreate(4, 4, rgba(12, 34, 56, 255), 32)
assert(image <> 0)
assert(bsave(filename, image) = 0)
imagedestroy image

#macro check_asset(path)
	scope
		dim as any ptr asset = fb.Gfx3SurfaceLoad(path)
		assert(asset <> 0)
		put (7, 5), asset, pset
		assert(point(7, 5) = rgba(12, 34, 56, 255))
		assert(fb.Gfx3SurfaceDestroy(asset) = 0)
	end scope
#endmacro
check_asset(byteFilename)
check_asset(wideFilename)
check_asset(filename)
screen 0
kill filename
print "text-types gfxlib3: Unicode asset filenames passed"

'' end of text-types.bas
