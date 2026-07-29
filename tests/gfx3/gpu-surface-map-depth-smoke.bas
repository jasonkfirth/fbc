''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: gpu-surface-map-depth-smoke.bas
''
'' Purpose:
''
''     Verify the scoped CPU staging map layout for indexed and RGB565 opaque
''     GPU surfaces.
''
'' Responsibilities:
''
''     - verify the exact byte pitch of 8-bit staging storage
''     - verify the exact word pitch of 16-bit RGB565 staging storage
''     - commit partial writes through Gfx3SurfaceUnmap
''     - compare raw GPU downloads after the staged upload
''
'' This file intentionally does NOT contain:
''
''     - CPU FB.IMAGE headers
''     - persistent pixel pointers
''     - presentation or primitive compatibility coverage
''

#define __FB_GFXLIB3__
#include once "fbgfx3.bi"

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#elseif defined(GFX3_OPENGL_TEST)
	const backend_flags = 0
#else
	const backend_flags = fb.GFX_NULL
#endif

if screenres( 16, 16, 32, 1, backend_flags ) <> 0 then end 1

''
'' gfxlib3 stores every native 1/2/4/8-bit pixel in one byte. A map must expose
'' that stable, byte-addressable form rather than a packed bitstream.
''
dim as any ptr surface8 = fb.Gfx3SurfaceCreate( 3, 2, 8 )
dim as any ptr pixels
dim as long pitch
dim as ubyte downloaded8( 0 to 5 )
if surface8 = 0 then end 2
if fb.Gfx3SurfaceMap( surface8, fb.GFX3_MAP_WRITE, pixels, pitch ) <> 0 then _
	end 3
if pixels = 0 orelse pitch <> 3 then end 4
cptr( ubyte ptr, pixels )[0] = &h3a
cptr( ubyte ptr, pixels )[pitch + 2] = &h4b
if fb.Gfx3SurfaceUnmap( surface8 ) <> 0 then end 5
if fb.Gfx3SurfaceDownload( surface8, 0, 0, 3, 2, 3, _
	@downloaded8( 0 ) ) <> 0 then end 6
if downloaded8( 0 ) <> &h3a orelse downloaded8( 5 ) <> &h4b then end 7
if fb.Gfx3SurfaceDestroy( surface8 ) <> 0 then end 8

''
'' A 16-bit surface is RGB565 and uses two bytes per staging pixel. Raw values
'' are intentional here: this verifies memory layout independently of POINT's
'' public RGB expansion.
''
dim as any ptr surface16 = fb.Gfx3SurfaceCreate( 3, 2, 16 )
dim as ushort downloaded16( 0 to 5 )
if surface16 = 0 then end 9
if fb.Gfx3SurfaceMap( surface16, fb.GFX3_MAP_WRITE, pixels, pitch ) <> 0 then _
	end 10
if pixels = 0 orelse pitch <> 3 * sizeof( ushort ) then end 11
cptr( ushort ptr, pixels )[0] = &h07e0
cptr( ushort ptr, pixels )[pitch \ sizeof( ushort ) + 2] = &hf81f
if fb.Gfx3SurfaceUnmap( surface16 ) <> 0 then end 12
if fb.Gfx3SurfaceDownload( surface16, 0, 0, 3, 2, _
	3 * sizeof( ushort ), @downloaded16( 0 ) ) <> 0 then end 13
if downloaded16( 0 ) <> &h07e0 orelse downloaded16( 5 ) <> &hf81f then end 14
if fb.Gfx3SurfaceDestroy( surface16 ) <> 0 then end 15

screen 0
end 0

'' end of gpu-surface-map-depth-smoke.bas
