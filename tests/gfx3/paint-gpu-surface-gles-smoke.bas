''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: paint-gpu-surface-gles-smoke.bas
''
'' Purpose:
''
''     Isolate renderer-resident PAINT on a GPU-only surface.
''
'' Responsibilities:
''
''     - create a renderable surface without transfer-destination capability
''     - fill a small non-border region through the backend PAINT command
''     - read the result through the permitted transfer-source capability
''
'' This file intentionally does NOT contain:
''
''     - normal screen-page PAINT, which may use a compatibility shadow
''     - a CPU upload fallback
''     - a large or serpentine performance workload
''
#define __FB_GFXLIB3__
#include once "fbgfx3.bi"

if screenres( 16, 16, 32, 1, 0 ) <> 0 then end 1

dim as any ptr surface = fb.Gfx3SurfaceCreate( 4, 4, 32, _
	fb.GFX3_SURFACE_RENDER_TARGET or fb.GFX3_SURFACE_TRANSFER_SOURCE, _
	rgb( 0, 0, 0 ) )
if surface = 0 then end 2

paint surface, ( 0, 0 ), rgb( 73, 74, 75 ), rgb( 1, 2, 3 )
if point( 3, 3, surface ) <> rgb( 73, 74, 75 ) then end 3
if point( 1, 2, surface ) <> rgb( 73, 74, 75 ) then end 4

if fb.Gfx3SurfaceDestroy( surface ) <> 0 then end 5
screen 0
end 0

'' end of paint-gpu-surface-gles-smoke.bas
