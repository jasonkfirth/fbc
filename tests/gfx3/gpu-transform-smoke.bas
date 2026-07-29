''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: gpu-transform-smoke.bas
''
'' Purpose:
''
''     Verify GPU-resident scaling, rotation, and Mode 7 projective mapping.
''
'' Responsibilities:
''
''     - verify nearest-neighbour scaling retains exact source texels
''     - verify a 90-degree rotation around the source centre
''     - verify Mode 7 horizon projection and source-rectangle wrapping
''     - verify adjacent overlapping transforms retain submission order
''     - verify invalid dimensions and projection values are rejected
''
'' This file intentionally does NOT contain:
''
''     - visual-only assertions
''     - CPU FB.IMAGE transformed drawing
''     - throughput thresholds
''

#include once "fbgfx3.bi"

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#elseif defined( GFX3_OPENGL_TEST )
	const backend_flags = fb.GFX_OPENGL
#else
	const backend_flags = fb.GFX_NULL
#endif

dim pixels( 0 to 3 ) as ulong = { _
	rgba( 0, 0, 0, 255 ), rgba( 200, 100, 40, 255 ), _
	rgba( 20, 40, 220, 255 ), rgba( 240, 240, 240, 255 ) }
dim source as any ptr
dim destination as any ptr

if screenres( 32, 24, 32, 1, backend_flags ) <> 0 then end 1
source = fb.Gfx3SurfaceCreate( 2, 2, 32, fb.GFX3_SURFACE_ASSET )
if source = 0 then end 2
if fb.Gfx3SurfaceUpload( source, 0, 0, 2, 2, 8, @pixels( 0 ) ) <> 0 then end 3

if fb.Gfx3SurfaceBlitScaled( 0, source, 0, 0, 2, 2, 2, 2, 4, 4 ) <> 0 then end 4
if point( 2, 2 ) <> pixels( 0 ) then end 5
if point( 3, 3 ) <> pixels( 0 ) then end 6
if point( 4, 2 ) <> pixels( 1 ) then end 7
if point( 2, 4 ) <> pixels( 2 ) then end 8
if point( 5, 5 ) <> pixels( 3 ) then end 9

'' The second destination pixel is one quarter of the way between the top
'' source texels after accounting for pixel-centre sampling.
if fb.Gfx3SurfaceBlitScaled( 0, source, 0, 0, 2, 2, 10, 2, 4, 4, _
	fb.GFX3_PUT_PSET, 255, fb.GFX3_FILTER_LINEAR ) <> 0 then end 10
if point( 11, 2 ) <> rgba( 50, 25, 10, 255 ) then end 11

if fb.Gfx3SurfaceBlitRotated( 0, source, 0, 0, 2, 2, _
	20.0, 10.0, 90.0 ) <> 0 then end 12
if point( 20, 9 ) <> pixels( 0 ) then end 13
if point( 20, 10 ) <> pixels( 1 ) then end 14
if point( 19, 9 ) <> pixels( 2 ) then end 15
if point( 19, 10 ) <> pixels( 3 ) then end 16

if fb.Gfx3SurfaceMode7( 0, source, 0, 0, 2, 2, 0, 8, 16, 8, _
	0.0, 0.0, 1.0, 0.0, 7.5, 4.0 ) <> 0 then end 17
if point( 8, 8 ) <> pixels( 0 ) then end 18
if point( 9, 8 ) <> pixels( 2 ) then end 19
if point( 8, 9 ) <> pixels( 0 ) then end 20

'' A transform target remains opaque and GPU-resident. Ordinary PUT accepts it
'' directly, so neither operation requires a staging download.
destination = fb.Gfx3SurfaceCreate( 6, 6, 32 )
if destination = 0 then end 21
if fb.Gfx3SurfaceBlitScaled( destination, source, 0, 0, 2, 2, _
	1, 1, 4, 4 ) <> 0 then end 22
put ( 24, 16 ), destination, pset
if point( 25, 17 ) <> pixels( 0 ) then end 23
if point( 28, 20 ) <> pixels( 3 ) then end 24
if fb.Gfx3SurfaceDestroy( destination ) <> 0 then end 25

'' These commands remain adjacent until POINT introduces a completion boundary.
'' GLES must execute them through its instanced transform batch. The second
'' source rectangle deliberately overwrites part of the first destination.
if fb.Gfx3SurfaceBlitScaled( 0, source, 0, 0, 2, 2, _
	26, 0, 4, 4 ) <> 0 then end 26
if fb.Gfx3SurfaceBlitScaled( 0, source, 1, 0, 1, 1, _
	27, 1, 2, 2 ) <> 0 then end 27
if point( 26, 0 ) <> pixels( 0 ) then end 28
if point( 27, 1 ) <> pixels( 1 ) then end 29

if fb.Gfx3SurfaceBlitScaled( 0, source, 0, 0, 2, 2, _
	0, 0, 0, 4 ) = 0 then end 30
if fb.Gfx3SurfaceBlitRotated( 0, source, 0, 0, 2, 2, _
	8.0, 8.0, 0.0, 0.0, 1.0 ) = 0 then end 31
if fb.Gfx3SurfaceMode7( 0, source, 0, 0, 2, 2, 0, 0, 8, 8, _
	0.0, 0.0, 0.0, 0.0, 4.0, 4.0 ) = 0 then end 32
if fb.Gfx3SurfaceBlitScaled( 0, source, 0, 0, 3, 2, _
	0, 0, 3, 2 ) = 0 then end 33

if fb.Gfx3SurfaceDestroy( source ) <> 0 then end 34
screen 0
end 0

'' end of gpu-transform-smoke.bas
