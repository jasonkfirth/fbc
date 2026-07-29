''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: opengl-gpu-surface-state-smoke.bas
''
'' Purpose:
''
''     Isolate command-state interactions between ordinary screen drawing and
''     a following GPU-only surface rectangle on the OpenGL backend.
''
'' Responsibilities:
''
''     - recreate the alpha-primitive mode transition
''     - add screen operations in ordered stages selected by COMMAND(1)
''     - prove a following GPU-only opaque BF retains its independent target
''
'' This file intentionally does NOT contain:
''
''     - renderer-selection fallback coverage
''     - GLES or Vulkan conformance claims
''     - a broad graphics API sweep
''

#define __FB_GFXLIB3__
#include once "fbgfx3.bi"

dim stage as integer = valint( command( 1 ) )
if stage < 0 orelse stage > 6 then end 1

const as ulong destination_color = &hFF204060u
const as ulong source_color = &h8030C080u
const as ulong opaque_color = &hFF102030u

'' First open without alpha so the transition matches the public alpha smoke.
if screenres( 64, 64, 32, 2, fb.GFX_OPENGL ) <> 0 then end 2
dim alpha_enabled as integer
screencontrol fb.GET_ALPHA_PRIMITIVES, alpha_enabled
if alpha_enabled <> false then end 3
pset ( 1, 1 ), source_color
if culng( point( 1, 1 ) ) <> source_color then end 4
screen 0

if screenres( 64, 64, 32, 2, fb.GFX_OPENGL or _
	fb.GFX_ALPHA_PRIMITIVES ) <> 0 then end 5
screencontrol fb.GET_ALPHA_PRIMITIVES, alpha_enabled
if alpha_enabled = false then end 6

if stage >= 1 then
	pset ( 2, 2 ), destination_color
	pset ( 2, 2 ), source_color
end if

if stage >= 2 then
	if culng( point( 2, 2 ) ) <> &h80288070u then end 7
end if

if stage >= 3 then
	line ( 4, 4 )-( 8, 4 ), destination_color
	line ( 4, 4 )-( 8, 4 ), source_color
	if culng( point( 6, 4 ) ) <> &h80288070u then end 8
end if

if stage >= 4 then
	line ( 10, 6 )-( 15, 11 ), destination_color, bf
	line ( 10, 6 )-( 15, 11 ), source_color, bf
	if culng( point( 12, 8 ) ) <> &h80288070u then end 9
end if

if stage >= 5 then
	line ( 18, 6 )-( 25, 13 ), &hFFFFFFFFu, b
	line ( 19, 7 )-( 24, 12 ), destination_color, bf
	paint ( 21, 9 ), source_color, &hFFFFFFFFu
	if culng( point( 21, 9 ) ) <> &h80288070u then end 10
end if

if stage >= 6 then
	dim cpu_image as any ptr = imagecreate( 8, 8, destination_color, 32 )
	if cpu_image = 0 then end 11
	pset cpu_image, ( 3, 3 ), source_color
	line cpu_image, ( 1, 5 )-( 6, 5 ), source_color
	imagedestroy cpu_image
end if

dim gpu_surface as any ptr = fb.Gfx3SurfaceCreate( 64, 64, 32, , _
	destination_color )
if gpu_surface = 0 then end 12
pset gpu_surface, ( 3, 3 ), source_color
if culng( point( 3, 3, gpu_surface ) ) <> &h80288070u then end 13
line gpu_surface, ( 5, 5 )-( 10, 10 ), opaque_color, bf
if culng( point( 7, 7, gpu_surface ) ) <> opaque_color then end 14
if fb.Gfx3SurfaceDestroy( gpu_surface ) <> 0 then end 15

screen 0
print "gfxlib3 OpenGL GPU surface state stage " & stage
end 0

'' end of opengl-gpu-surface-state-smoke.bas
