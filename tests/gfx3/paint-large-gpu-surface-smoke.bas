''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: paint-large-gpu-surface-smoke.bas
''
'' Purpose:
''
''     Prove that a GLES PAINT larger than the former small frontier limit
''     remains renderer-resident.
''
'' Responsibilities:
''
''     - create large opaque surfaces without upload capability
''     - fill a plain target and a four-neighbour serpentine through PAINT
''     - read the completed GPU result through the permitted source capability
''
'' This file intentionally does NOT contain:
''
''     - a CPU upload fallback path
''     - performance timing or vendor-specific renderer assumptions
''

#define __FB_GFXLIB3__
#include once "fbgfx3.bi"

#ifdef GFX3_OPENGL_TEST
	const backend_flags = fb.GFX_OPENGL
#elseif defined( GFX3_VULKAN_TEST )
	const backend_flags = fb.GFX_VULKAN
#else
	'' Let Android choose its available GLES renderer. Desktop matrix runs use
	'' the two explicit test defines above, while 0 retains normal auto-selection.
	const backend_flags = 0
#endif

if screenres( 16, 16, 32, 1, backend_flags ) <> 0 then end 1

''
'' CPU compatibility PAINT needs both download and upload capabilities. This
'' surface deliberately omits TRANSFER_DESTINATION, so the result can succeed
'' only when the renderer executes PAINT without staging target pixels.
''
dim as any ptr surface = fb.Gfx3SurfaceCreate( 80, 80, 32, _
	fb.GFX3_SURFACE_RENDER_TARGET or fb.GFX3_SURFACE_TRANSFER_SOURCE, _
	rgb( 0, 0, 0 ) )
if surface = 0 then end 2
dim as long surface_width, surface_height, surface_depth
dim as ulong surface_usage
if fb.Gfx3SurfaceInfo( surface, @surface_width, @surface_height, _
	@surface_depth, @surface_usage ) <> 0 then end 19
if surface_width <> 80 orelse surface_height <> 80 orelse _
	surface_depth <> 32 then end 20
if surface_usage <> (fb.GFX3_SURFACE_RENDER_TARGET or _
	fb.GFX3_SURFACE_TRANSFER_SOURCE) then end 23
pset surface, ( 40, 40 ), rgb( 7, 8, 9 )
if point( 40, 40, surface ) <> rgb( 7, 8, 9 ) then end 21
if fb.Gfx3SurfaceClear( surface, rgb( 0, 0, 0 ) ) <> 0 then end 22

paint surface, ( 0, 0 ), rgb( 73, 74, 75 ), rgb( 1, 2, 3 )
''
'' The GLES frontier advances in batches of 32 expansion passes. These probes
'' distinguish a premature batch-completion decision from an incorrect final
'' copy: (22,10) is at Manhattan distance 32, while (23,10) is at 33.
''
if point( 8, 0, surface ) <> rgb( 73, 74, 75 ) then end 16
if point( 16, 0, surface ) <> rgb( 73, 74, 75 ) then end 17
if point( 24, 0, surface ) <> rgb( 73, 74, 75 ) then end 18
if point( 22, 10, surface ) <> rgb( 73, 74, 75 ) then end 14
if point( 23, 10, surface ) <> rgb( 73, 74, 75 ) then end 15
if point( 79, 79, surface ) <> rgb( 73, 74, 75 ) then end 3
if point( 40, 40, surface ) <> rgb( 73, 74, 75 ) then end 4

'' A border-coloured seed must remain untouched, just as it does in gfxlib2.
dim as any ptr border_surface = fb.Gfx3SurfaceCreate( 80, 80, 32, _
	fb.GFX3_SURFACE_RENDER_TARGET or fb.GFX3_SURFACE_TRANSFER_SOURCE, _
	rgb( 8, 9, 10 ) )
if border_surface = 0 then end 5
paint border_surface, ( 0, 0 ), rgb( 90, 91, 92 ), rgb( 8, 9, 10 )
if point( 0, 0, border_surface ) <> rgb( 8, 9, 10 ) then end 6
if fb.Gfx3SurfaceDestroy( border_surface ) <> 0 then end 7

''
'' These alternating walls leave one 4,720-pixel serpentine path. It is longer
'' than the original 4,096-pixel GLES frontier gate and rejects diagonal or
'' through-wall shortcuts. As above, the destination capability is absent.
''
dim as any ptr maze_surface = fb.Gfx3SurfaceCreate( 80, 120, 32, _
	fb.GFX3_SURFACE_RENDER_TARGET or fb.GFX3_SURFACE_TRANSFER_SOURCE, _
	rgb( 0, 0, 0 ) )
if maze_surface = 0 then end 8
for wall_y as integer = 1 to 117 step 2
	if ( wall_y and 2 ) = 0 then
		line maze_surface, ( 0, wall_y )-( 78, wall_y ), rgb( 200, 201, 202 )
	else
		line maze_surface, ( 1, wall_y )-( 79, wall_y ), rgb( 200, 201, 202 )
	end if
next
paint maze_surface, ( 0, 0 ), rgb( 109, 110, 111 ), rgb( 200, 201, 202 )
if point( 0, 118, maze_surface ) <> rgb( 109, 110, 111 ) then end 9
if point( 0, 1, maze_surface ) <> rgb( 200, 201, 202 ) then end 10
if fb.Gfx3SurfaceDestroy( maze_surface ) <> 0 then end 11

if fb.Gfx3SurfaceDestroy( surface ) <> 0 then end 12
screen 0
end 0

'' end of paint-large-gpu-surface-smoke.bas
