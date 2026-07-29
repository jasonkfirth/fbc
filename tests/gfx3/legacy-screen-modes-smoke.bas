''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: legacy-screen-modes-smoke.bas
''
'' Purpose:
''
''     Verify the historical SCREEN modes newly provided by gfxlib3.
''
'' Responsibilities:
''
''     - check the dimensions, depth, and text grid of SCREEN 3 through 6
''     - exercise the clipped twenty-fifth Hercules character row
''     - verify monochrome modes start with a white foreground palette entry
''     - prove the two Hercules graphics pages retain independent pixels
''
'' This file intentionally does NOT contain:
''
''     - emulation of adapter registers or physical video memory
''     - composite monitor artifact colours
''     - visible-window capture or aspect-ratio checks
''

#ifndef __FB_GFXLIB3__
    #define __FB_GFXLIB3__
#endif

declare function page_set alias "fb_GfxPageSet" _
    ( byval work_page as long, byval visible_page as long ) as long

setenviron "FBGFX=null"

sub fail( byval code as integer )
    screen 0
    end code
end sub

sub check_mode( byval screen_mode as integer, _
    byval expected_width as integer, byval expected_height as integer, _
    byval expected_depth as integer, byval expected_columns as integer, _
    byval expected_rows as integer, byval maximum_attribute as integer, _
    byval error_base as integer )

    dim mode_width as integer
    dim mode_height as integer
    dim mode_depth as integer
    dim bytes_per_pixel as integer
    dim pitch as integer
    dim packed_size as integer

    screen screen_mode
    screeninfo mode_width, mode_height, mode_depth, bytes_per_pixel, pitch
    if ( mode_width <> expected_width ) or _
       ( mode_height <> expected_height ) then
        fail error_base + 1
    end if
    if mode_depth <> expected_depth then fail error_base + 2
    if bytes_per_pixel <> 1 then fail error_base + 3

    packed_size = width( )
    if ( packed_size and &hFFFF ) <> expected_columns then
        fail error_base + 4
    end if
    if ( packed_size shr 16 ) <> expected_rows then fail error_base + 5

    locate expected_rows, expected_columns
    print "X";
    if screen( expected_rows, expected_columns ) <> asc( "X" ) then
        fail error_base + 6
    end if

    pset ( expected_width - 1, expected_height - 1 ), maximum_attribute
    if point( expected_width - 1, expected_height - 1 ) <> _
       maximum_attribute then
        fail error_base + 7
    end if
end sub

check_mode 3, 720, 348, 1, 80, 25, 1, 10

'' Hercules supplies two complete logical pages even though the final two
'' scanlines of its twenty-fifth text row lie below the visible framebuffer.
pset ( 3, 3 ), 1
page_set 1, 1
if point( 3, 3 ) <> 0 then fail 20
pset ( 3, 3 ), 1
page_set 0, 0
if point( 3, 3 ) <> 1 then fail 21

dim red_value as integer
dim green_value as integer
dim blue_value as integer

palette get 1, red_value, green_value, blue_value
if ( red_value <> 255 ) or ( green_value <> 255 ) or _
   ( blue_value <> 255 ) then
    fail 22
end if

'' A full-screen line advance scrolls by one 14-pixel Hercules cell. The
'' clipped source rectangle must still move all physical scanlines above it.
cls
pset ( 10, 20 ), 1
for line_number as integer = 1 to 25
    print
next
if point( 10, 6 ) <> 1 then fail 23

check_mode 4, 640, 400, 1, 80, 25, 1, 30
palette get 1, red_value, green_value, blue_value
if ( red_value <> 255 ) or ( green_value <> 255 ) or _
   ( blue_value <> 255 ) then
    fail 38
end if
color 4
palette get 1, red_value, green_value, blue_value
'' Standard colour 4 is the normal-intensity 170/255 VGA red.
if ( red_value <> 170 ) or ( green_value <> 0 ) or _
   ( blue_value <> 0 ) then
    screen 0
    print "SCREEN 4 COLOR 4 palette:"; red_value; green_value; blue_value
    end 39
end if

check_mode 5, 320, 200, 4, 40, 25, 15, 40
check_mode 6, 640, 200, 2, 80, 25, 3, 50

screen 0
print "GFX3_LEGACY_SCREEN_MODES_PASS"
end 0

'' end of legacy-screen-modes-smoke.bas
