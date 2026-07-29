''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: put-depth-conversion-smoke.bas
''
'' Purpose:
''
''     Verify PUT accepts a CPU FB.IMAGE whose storage depth differs from the
''     normalized screen-page depth.
''
'' Responsibilities:
''
''     - create a 32-bit source image
''     - PUT it onto a 16-bit screen page with transparent mode
''     - confirm the converted visible colour is non-zero
''     - repeat the BLOAD image path at sprite-frame scale
''
'' This file intentionally does NOT contain:
''
''     - custom PUT callbacks
''     - framebuffer locking
''     - file-codec coverage
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#elseif defined( GFX3_OPENGL_TEST )
	const backend_flags = fb.GFX_OPENGL
#else
	const backend_flags = 0
#endif

'' gfxlib2-era games may request 15 bits; gfxlib3 normalizes that storage to
'' RGB565 while preserving the requested API path.
if screenres( 640, 480, 15, 2, backend_flags ) <> 0 then end 1

'' Legacy double-buffered games commonly render to page 1 while page 0 is
'' visible.  Exercise the same non-default work-page selection as Duel.
screenset 1, 0

'' Match the page and VIEW SCREEN sequence used by legacy double-buffered
'' games before their first sprite frame.
view screen ( 0, 0 )-( 639, 479 )
line ( 0, 0 )-( 639, 479 ), rgb( 221, 221, 221 ), bf
view screen ( 2, 30 )-( 491, 429 )
line ( 2, 30 )-( 491, 429 ), rgb( 0, 0, 0 ), bf

dim as any ptr source_image = imagecreate( 8, 8, rgb( 40, 170, 230 ), 32 )
if source_image = 0 then end 2

put ( 4, 34 ), source_image, trans
if point( 7, 37 ) = 0 then end 3

'' A no-depth IMAGECREATE follows the current 16-bit screen storage.  This
'' mirrors BLOAD destinations used by older RGB565 games.
dim as any ptr native_image = imagecreate( 8, 8, rgb( 230, 110, 40 ) )
if native_image = 0 then end 4
put ( 16, 34 ), native_image, trans
if point( 19, 37 ) = 0 then end 5

'' A 24-bit BMP BLOADed into a current-depth image is the legacy asset path
'' used by existing RGB565 games.
dim as any ptr bloaded_image = imagecreate( 13, 16, rgb( 255, 0, 255 ) )
if bloaded_image = 0 then end 6
if bload( "OMA/duel999/data/star0.bmp", bloaded_image ) <> 0 then end 7
put ( 0, 46 ), bloaded_image, trans
if point( 6, 53 ) = 0 then end 8

'' Older sprite engines use integer-limit sentinels for inactive objects.
'' Their offscreen PUTs must clip away instead of causing a coordinate error.
put ( 0, csng( -2147483648 ) ), bloaded_image, trans

'' Repeated image PUT must reuse the renderer-owned GPU upload surface without
'' losing transparent sprite ordering during a sprite-heavy frame.
for index as integer = 1 to 1024
	put ( 0, 46 ), bloaded_image, trans
next

imagedestroy bloaded_image
imagedestroy native_image
imagedestroy source_image
screen 0
end 0

'' end of put-depth-conversion-smoke.bas
