''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: vulkan-api-smoke.bas
''
'' Purpose:
''
''     Verify that ordinary FreeBASIC SCREENRES, PSET, POINT, and LINE calls
''     can use the explicit gfxlib3 Vulkan backend through the real render
''     thread, including the LINE rectangle forms.
''
'' Responsibilities:
''
''     - select Vulkan with the gfxlib3-only public flag
''     - verify 32-bit point storage and readback
''     - verify solid line rasterization through the compute backend
''     - verify filled rectangles through the device-local clear path
''     - verify full ellipses through the Float64 midpoint compute path
''     - verify CPU image upload and built-in PUT through Vulkan compute
''     - verify indexed-depth logical color masking
''     - exercise automatic visible-page presentation and mode teardown
''
'' This file intentionally does NOT contain:
''
''     - displayed-pixel capture, which belongs to vulkan-presentation.c
''     - arc filtering checks
''     - OpenGL fallback behavior
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"

if screenres(32, 24, 32, 1, FB.GFX_VULKAN) <> 0 then end 1

pset (3, 4), &hDEADBEEF
pset (31, 23), &h12345678
line (0, 0)-(7, 7), &hA1B2C3D4
line (10, 2)-(14, 5), &h50607080, bf
circle (24, 4), 2, &h0A0B0C0D
if point(3, 4) <> &hDEADBEEF then end 2
if point(31, 23) <> &h12345678 then end 3
if point(0, 0) <> &hA1B2C3D4 then end 4
if point(4, 4) <> &hA1B2C3D4 then end 5
if point(7, 7) <> &hA1B2C3D4 then end 6
if point(10, 2) <> &h50607080 then end 7
if point(12, 3) <> &h50607080 then end 8
if point(14, 5) <> &h50607080 then end 9
if point(15, 5) = &h50607080 then end 10
if point(26, 4) <> &h0A0B0C0D then end 11

dim as any ptr source_image = imagecreate(4, 4, &h10203040, 32)
if source_image = 0 then end 12
pset source_image, (1, 2), &hCAFEBABE
put (20, 10), source_image, pset
if point(20, 10) <> &h10203040 then end 13
if point(21, 12) <> &hCAFEBABE then end 14
imagedestroy source_image

screen 0

if screenres(32, 24, 8, 1, FB.GFX_VULKAN) <> 0 then end 15

pset (5, 6), &h1234
line (1, 8)-(6, 8), &h1234
if point(5, 6) <> &h34 then end 16
if point(3, 8) <> &h34 then end 17

screen 0
end 0

'' end of vulkan-api-smoke.bas
