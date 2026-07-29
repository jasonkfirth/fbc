''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: heterogeneous-blit-smoke.bas
''
'' Purpose:
''
''     Verify that one packed PUT stream may reference alternating GPU-cached
''     CPU images without changing FIFO rendering semantics.
''
'' Responsibilities:
''
''     - create two non-uniform CPU images which cannot use the rectangle shortcut
''     - alternate their transparent PUTs inside one backend-sized packet
''     - verify the last overlapping sprite remains the visible result
''
'' This file intentionally does NOT contain:
''
''     - transformed or custom-blender PUT coverage
''     - timing thresholds
''     - direct GPU-surface API calls
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"

#ifdef GFX3_VULKAN_TEST
    const backend_flags = fb.GFX_VULKAN
#else
    const backend_flags = fb.GFX_OPENGL
#endif

const transparent_color = rgb( 255, 0, 255 )
const first_color = rgb( 24, 80, 160 )
const second_color = rgb( 200, 104, 32 )

if screenres( 32, 24, 32, 1, backend_flags ) <> 0 then end 1

dim as any ptr first_image = imagecreate( 2, 2, transparent_color, 32 )
dim as any ptr second_image = imagecreate( 2, 2, transparent_color, 32 )
if first_image = 0 orelse second_image = 0 then end 2

pset first_image, ( 0, 0 ), first_color
pset second_image, ( 0, 0 ), second_color

for index as integer = 0 to 699
    if ( index and 1 ) = 0 then
        put ( 8, 6 ), first_image, trans
    else
        put ( 8, 6 ), second_image, trans
    end if
next index

dim as ulong result_color = cuint( point( 8, 6 ) )
if result_color <> second_color then
    screen 0
    print "GFX3_HETEROGENEOUS_BLIT_FAIL expected &h"; hex( second_color ); _
        " got &h"; hex( result_color )
    if result_color = first_color then end 5
    if result_color = rgb( 0, 0, 0 ) then end 6
    end 3
end if
if cuint( point( 9, 6 ) ) <> rgb( 0, 0, 0 ) then end 4

imagedestroy second_image
imagedestroy first_image
screen 0
end 0

'' end of heterogeneous-blit-smoke.bas
