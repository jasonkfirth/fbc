''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: paint-coalescing-smoke.bas
''
'' Purpose:
''
''     Verify exact PAINT results when a renderer coalesces compatible opaque
''     recolours and when a border-coloured fill must preserve its topology.
''
'' Responsibilities:
''
''     - queue adjacent compatible PAINT commands with one final observation
''     - prove the last compatible colour replaces the complete enclosed region
''     - prove a fill matching the border is never removed from the command run
''     - repeat both cases on a renderer-only GPU surface
''
'' This file intentionally does NOT contain:
''
''     - a patterned or alpha PAINT workload
''     - renderer-specific expected pixels
''     - performance thresholds
''

#define __FB_GFXLIB3__
#include once "fbgfx3.bi"

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#else
	const backend_flags = fb.GFX_OPENGL
#endif

const border_color = rgb( 0, 0, 255 )
const final_color = rgb( 17, 193, 71 )

if screenres( 64, 48, 32, 1, backend_flags ) <> 0 then end 1

line ( 2, 2 )-( 29, 29 ), border_color, b
paint ( 8, 8 ), rgb( 201, 31, 11 ), border_color
paint ( 8, 8 ), rgb( 29, 41, 211 ), border_color
paint ( 8, 8 ), final_color, border_color

if point( 8, 8 ) <> final_color then end 2
if point( 28, 28 ) <> final_color then end 3
if point( 2, 12 ) <> border_color then end 4
if point( 1, 12 ) <> rgb( 0, 0, 0 ) then end 5

line ( 34, 2 )-( 61, 29 ), border_color, b
paint ( 40, 8 ), border_color, border_color
paint ( 40, 8 ), rgb( 255, 255, 0 ), border_color

'' The first fill turns the interior into border pixels. The second seed is
'' therefore already on the border and must leave that interior unchanged.
if point( 40, 8 ) <> border_color then end 6
if point( 60, 28 ) <> border_color then end 7

'' Omitting transfer-destination capability prevents the compatibility layer
'' from substituting a CPU flood and therefore exercises backend coalescing.
dim as any ptr surface = fb.Gfx3SurfaceCreate( 32, 24, 32, _
	fb.GFX3_SURFACE_RENDER_TARGET or fb.GFX3_SURFACE_TRANSFER_SOURCE, _
	rgb( 0, 0, 0 ) )
if surface = 0 then end 8

line surface, ( 1, 1 )-( 14, 20 ), border_color, b
paint surface, ( 4, 4 ), rgb( 201, 31, 11 ), border_color
paint surface, ( 4, 4 ), rgb( 29, 41, 211 ), border_color
paint surface, ( 4, 4 ), final_color, border_color
if point( 4, 4, surface ) <> final_color then end 9
if point( 13, 19, surface ) <> final_color then end 10

line surface, ( 17, 1 )-( 30, 20 ), border_color, b
paint surface, ( 20, 4 ), border_color, border_color
paint surface, ( 20, 4 ), rgb( 255, 255, 0 ), border_color
if point( 20, 4, surface ) <> border_color then end 11
if point( 29, 19, surface ) <> border_color then end 12

if fb.Gfx3SurfaceDestroy( surface ) <> 0 then end 13

screen 0
end 0

'' end of paint-coalescing-smoke.bas
