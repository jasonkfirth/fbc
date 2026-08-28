''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: gpu-asset-smoke.bas
''
'' Purpose:
''
''     Verify that BLOAD-compatible BMP and PNG files can become
''     GPU-authoritative assets and can be consumed by ordinary PUT syntax.
''
'' Responsibilities:
''
''     - create deterministic BMP and PNG files through the public graphics API
''     - load both formats through Gfx3SurfaceLoad without retaining CPU pixels
''     - use the opaque surface as the source of an ordinary PUT
''
'' This file intentionally does NOT contain:
''
''     - transformed surface operations
''     - malformed image fixtures
''     - performance thresholds
''

#include once "fbgfx3.bi"

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#elseif defined( GFX3_OPENGL_TEST )
	const backend_flags = fb.GFX_OPENGL
#else
	const backend_flags = fb.GFX_NULL
#endif

const bitmap_name = "gfx3-gpu-asset-smoke.bmp"
const png_name = "gfx3-gpu-asset-smoke.png"
dim image as any ptr
dim asset as any ptr

if screenres( 32, 24, 32, 1, backend_flags ) <> 0 then end 1
image = imagecreate( 4, 4, rgba( 12, 34, 56, 255 ), 32 )
if image = 0 then end 2
pset image, ( 1, 2 ), rgba( 210, 80, 17, 255 )
if bsave( bitmap_name, image ) <> 0 then end 3
if bsave( png_name, image ) <> 0 then end 8
imagedestroy image

asset = fb.Gfx3SurfaceLoad( bitmap_name, 32, _
	fb.GFX3_SURFACE_ASSET or fb.GFX3_SURFACE_TRANSFER_SOURCE )
if asset = 0 then end 4
put ( 7, 5 ), asset, pset
if point( 7, 5 ) <> rgba( 12, 34, 56, 255 ) then end 5
if point( 8, 7 ) <> rgba( 210, 80, 17, 255 ) then end 6
if fb.Gfx3SurfaceDestroy( asset ) <> 0 then end 7

asset = fb.Gfx3SurfaceLoad( png_name, 32, _
	fb.GFX3_SURFACE_ASSET or fb.GFX3_SURFACE_TRANSFER_SOURCE )
if asset = 0 then end 9
cls
put ( 7, 5 ), asset, pset
if point( 7, 5 ) <> rgba( 12, 34, 56, 255 ) then end 10
if point( 8, 7 ) <> rgba( 210, 80, 17, 255 ) then end 11
if fb.Gfx3SurfaceDestroy( asset ) <> 0 then end 12

screen 0
kill bitmap_name
kill png_name
end 0

'' end of gpu-asset-smoke.bas
