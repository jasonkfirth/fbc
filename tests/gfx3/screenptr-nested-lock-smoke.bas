''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: screenptr-nested-lock-smoke.bas
''
'' Purpose:
''
''     Verify gfxlib2-compatible nested SCREENLOCK/SCREENUNLOCK authority for
''     direct writes through SCREENPTR.
''
'' Responsibilities:
''
''     - preserve direct SCREENPTR writes made without SCREENLOCK
''     - obtain one writable screen pointer under an outer lock
''     - upload an inner lock's stated dirty line
''     - modify a different line before releasing the outer lock
''     - require the outer unlock to make that later write visible to POINT
''     - keep locked POINT/PSET read-modify-write work ordered with LINE
''     - keep full-page CLS synchronized with locked CPU shadow users
''
'' This file intentionally does NOT contain:
''
''     - platform window or presentation checks
''     - a device-specific renderer request
''
#include once "fbgfx.bi"

const width_pixels = 32
const height_pixels = 16
const inner_color = rgb( 32, 96, 192 )
const outer_color = rgb( 196, 72, 28 )

dim as integer renderer_flags = fb.GFX_NULL
#ifdef GFX3_OPENGL
    renderer_flags = fb.GFX_OPENGL
#elseif defined( GFX3_VULKAN )
    renderer_flags = fb.GFX_VULKAN
#elseif defined( GFX3_AUTO )
    renderer_flags = 0
#endif

if screenres( width_pixels, height_pixels, 32, 1, renderer_flags ) <> 0 then
    print "GFX3_SCREENPTR_NESTED_FAIL screenres"
    end 1
end if

dim as integer width_value
dim as integer height_value
dim as integer depth_value
dim as integer bytes_per_pixel
dim as integer pitch
screeninfo width_value, height_value, depth_value, bytes_per_pixel, pitch
if width_value <> width_pixels _
    or height_value <> height_pixels _
    or depth_value <> 32 _
    or bytes_per_pixel <> 4 _
    or pitch < width_pixels * bytes_per_pixel then
    print "GFX3_SCREENPTR_NESTED_FAIL screeninfo"
    screen 0
    end 2
end if

dim as ulong ptr pixels
dim as integer words_per_row = pitch \ sizeof( ulong )
if words_per_row < width_pixels then
    print "GFX3_SCREENPTR_NESTED_FAIL pitch"
    screen 0
    end 3
end if

'' SCREENPTR is an ordering boundary for a caller-local PSET packet. Its CPU
'' view must include work which had not yet reached the render queue.
const pending_pset_color = rgb( 80, 176, 40 )
pset ( 6, 6 ), pending_pset_color
pixels = screenptr
if pixels = 0 _
    or pixels[ words_per_row * 6 + 6 ] <> pending_pset_color then
    print "GFX3_SCREENPTR_NESTED_FAIL pending_pset"
    screen 0
    end 4
end if

'' SCREENPTR is writable without SCREENLOCK in gfxlib2. The next operation
'' which observes GPU state must upload such an untracked CPU write.
const unlocked_color = rgb( 184, 48, 136 )
pixels[ words_per_row * 8 + 8 ] = unlocked_color
if cuint( point( 8, 8 ) ) <> unlocked_color then
    print "GFX3_SCREENPTR_NESTED_FAIL unlocked_write"
    screen 0
    end 15
end if

screenlock
pixels = screenptr
if pixels = 0 then
    print "GFX3_SCREENPTR_NESTED_FAIL screenptr"
    screenunlock
    screen 0
    end 5
end if

'' The inner lock commits row one and clears gfxlib3's conservative marker.
screenlock
pixels[ words_per_row + 1 ] = inner_color
screenunlock 1, 1

'' This later write belongs to the still-active outer lock and must be uploaded.
pixels[ words_per_row * 2 + 2 ] = outer_color
screenunlock 2, 2

if cuint( point( 1, 1 ) ) <> inner_color then
    print "GFX3_SCREENPTR_NESTED_FAIL inner"
    screen 0
    end 6
end if
if cuint( point( 2, 2 ) ) <> outer_color then
    print "GFX3_SCREENPTR_NESTED_FAIL outer"
    screen 0
    end 7
end if

'' SCREENLOCK alone must not turn its downloaded CPU shadow into the source of
'' truth.  Games such as Arkanoid lock around normal GPU drawing commands.
screenlock
line ( 3, 3 )-( 3, 3 ), rgb( 24, 144, 224 )
screenunlock
if cuint( point( 3, 3 ) ) <> rgb( 24, 144, 224 ) then
    print "GFX3_SCREENPTR_NESTED_FAIL locked_gpu_draw"
    screen 0
    end 8
end if

'' PSET batches are caller-local until an ordering boundary.  SCREENUNLOCK is
'' one such boundary for software-rendered games that use it once per frame.
screenlock
pset ( 4, 4 ), rgb( 224, 96, 32 )
screenunlock
if cuint( point( 4, 4 ) ) <> rgb( 224, 96, 32 ) then
    print "GFX3_SCREENPTR_NESTED_FAIL locked_pset"
    screen 0
    end 9
end if

'' Alpha-font renderers read a background pixel before each PSET.  Under a
'' lock, gfxlib3 must keep that software-style loop in the synchronized
'' true-colour shadow instead of performing one GPU readback per pixel.
screenlock
line ( 0, 5 )-( width_pixels - 1, 5 ), rgb( 20, 36, 52 )
for x as integer = 0 to width_pixels - 1
    pset ( x, 5 ), rgb( ( point( x, 5 ) shr 16 ) and 255, 120, 40 )
next x
screenunlock
if cuint( point( width_pixels - 1, 5 ) ) <> rgb( 20, 120, 40 ) then
    print "GFX3_SCREENPTR_NESTED_FAIL locked_read_modify_write"
    screen 0
    end 10
end if

'' CLS replaces the entire old page. A following software-style loop must see
'' the known clear colour without requiring a GPU download first.
const clear_color = rgb( 12, 28, 44 )
screenlock
color , clear_color
cls
for x as integer = 0 to width_pixels - 1
    if cuint( point( x, 6 ) ) <> clear_color then
        print "GFX3_SCREENPTR_NESTED_FAIL locked_clear_read"
        screenunlock
        screen 0
        end 11
    end if
    pset ( x, 6 ), rgb( 12, 96, 44 )
next x
screenunlock
if cuint( point( width_pixels - 1, 6 ) ) <> rgb( 12, 96, 44 ) then
    print "GFX3_SCREENPTR_NESTED_FAIL locked_clear_write"
    screen 0
    end 12
end if

'' A previously returned SCREENPTR remains the same writable storage until the
'' lock ends. CLS must update that storage as well as recording the GPU clear.
screenlock
pixels = screenptr
pixels[ words_per_row * 7 + 1 ] = outer_color
color , clear_color
cls
if pixels[ words_per_row * 7 + 1 ] <> clear_color then
    print "GFX3_SCREENPTR_NESTED_FAIL screenptr_clear"
    screenunlock
    screen 0
    end 13
end if
pixels[ words_per_row * 7 + 1 ] = inner_color
screenunlock
if cuint( point( 1, 7 ) ) <> inner_color then
    print "GFX3_SCREENPTR_NESTED_FAIL screenptr_after_clear"
    screen 0
    end 14
end if

print "GFX3_SCREENPTR_NESTED_PASS"
screen 0
end 0

'' end of screenptr-nested-lock-smoke.bas
