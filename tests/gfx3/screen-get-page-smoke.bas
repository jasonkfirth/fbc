''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: screen-get-page-smoke.bas
''
'' Purpose:
''
''     Verify that GET captures the completed GPU work page into an existing
''     CPU FB.IMAGE when a program uses explicit screen pages.
''
'' Responsibilities:
''
''     - draw a transparent CPU image onto work page one
''     - copy work page one to the visible page
''     - capture a page with GET and a region with Gfx3ScreenCapture
''     - preserve authoritative SCREENLOCK shadow writes through both paths
''     - restore the captured image with PUT PSET and verify its pixels
''
'' This file intentionally does NOT contain:
''
''     - timing thresholds
''     - image file decoding
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"
#include once "fbgfx3.bi"

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#else
	const backend_flags = fb.GFX_OPENGL
#endif

#ifdef GFX3_16BIT_TEST
	const screen_depth = 16
#else
	const screen_depth = 32
#endif

const as ulong transparent_color = rgb( 255, 0, 255 )
const as ulong background_color = rgb( 12, 24, 48 )
const as ulong marker_color = rgb( 220, 96, 32 )

function screen_color( byval source_color as ulong ) as ulong
#ifdef GFX3_16BIT_TEST
	dim red as ulong = ( source_color shr 16 ) and &hffu
	dim green as ulong = ( source_color shr 8 ) and &hffu
	dim blue as ulong = source_color and &hffu
	dim packed as ulong = ( blue shr 3 ) or _
		( ( green shl 3 ) and &h07e0u ) or _
		( ( red shl 8 ) and &hf800u )

	return ( ( packed and &h001fu ) shl 3 ) or _
		( ( packed shr 2 ) and &h7u ) or _
		( ( packed and &h07e0u ) shl 5 ) or _
		( ( packed shr 1 ) and &h300u ) or _
		( ( packed and &hf800u ) shl 8 ) or _
		( ( packed shl 3 ) and &h70000u )
#else
	return source_color
#endif
end function

if screenres( 64, 48, screen_depth, 2, backend_flags or _
	fb.GFX_ALPHA_PRIMITIVES ) <> 0 then end 1

screenset 1, 0
line ( 0, 0 )-( 63, 47 ), background_color, bf

dim source_image as any ptr = imagecreate( 64, 40, transparent_color, _
	screen_depth )
dim captured_image as any ptr = imagecreate( 64, 48 )
dim partial_image as any ptr = imagecreate( 12, 12, 0, screen_depth )
if source_image = 0 orelse captured_image = 0 orelse _
	partial_image = 0 then end 2

line source_image, ( 9, 7 )-( 18, 16 ), marker_color, bf
put ( 0, 0 ), source_image, trans
screencopy 1, 0

'' The first small screen capture exercises a GPU-to-CPU regional download.
if fb.Gfx3ScreenCapture( partial_image, 8, 6, 12, 12 ) <> 0 then end 16
if culng( point( 2, 2, partial_image ) ) <> _
	screen_color( marker_color ) then end 9
if culng( point( 0, 0, partial_image ) ) <> _
	screen_color( background_color ) then end 10
if culng( point( 11, 11, partial_image ) ) <> _
	screen_color( background_color ) then end 11

'' A valid CPU shadow remains authoritative while SCREENLOCK is active.
screenlock
pset ( 30, 20 ), marker_color
if fb.Gfx3ScreenCapture( partial_image, 25, 15, 12, 12 ) <> 0 then end 17
screenunlock
if culng( point( 5, 5, partial_image ) ) <> _
	screen_color( marker_color ) then end 12

get ( 0, 0 )-( 63, 47 ), captured_image
if culng( point( 10, 8, captured_image ) ) <> _
	screen_color( marker_color ) then end 3
if culng( point( 30, 20, captured_image ) ) <> _
	screen_color( marker_color ) then end 13
if culng( point( 30, 44, captured_image ) ) <> _
	screen_color( background_color ) then end 5

line ( 0, 0 )-( 63, 47 ), rgb( 0, 0, 0 ), bf
put ( 0, 0 ), captured_image, pset
if culng( point( 10, 8 ) ) <> screen_color( marker_color ) then end 6
if culng( point( 30, 20 ) ) <> screen_color( marker_color ) then end 14
if culng( point( 30, 44 ) ) <> screen_color( background_color ) then end 8

imagedestroy partial_image
imagedestroy captured_image
imagedestroy source_image
screen 0
end 0

'' end of screen-get-page-smoke.bas
