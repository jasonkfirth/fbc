''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: console-font-smoke.bas
''
'' Purpose:
''
''     Prove that standard graphics SCREEN modes and WIDTH use the same
''     8 by 8, 8 by 14, and 8 by 16 text grids as gfxlib2.
''
'' Responsibilities:
''
''     - check the default EGA and VGA console dimensions
''     - switch between valid WIDTH font grids
''     - retain LOCATE and SCREEN character-cell behavior after a switch
''     - leave an invalid WIDTH request non-destructive
''
'' This file intentionally does NOT contain:
''
''     - visible glyph-image comparisons
''     - console scrolling or view-print coverage
''     - Unicode console behavior
''
#ifndef GFX2_REFERENCE
    #ifndef __FB_GFXLIB3__
        #define __FB_GFXLIB3__
    #endif
#endif

#ifdef GFX3_OPENGL_TEST
    setenviron "FBGFX=opengl"
#elseif defined( GFX3_VULKAN_TEST )
    setenviron "FBGFX=vulkan"
#elseif not defined( GFX3_AUTOMATIC_TEST )
    setenviron "FBGFX=null"
#endif

dim packed_size as integer

screen 9
packed_size = width( )
if ( packed_size and &hFFFF ) <> 80 then end 1
if ( packed_size shr 16 ) <> 25 then end 2
locate 25, 80
print "E";
if screen( 25, 80 ) <> asc( "E" ) then end 3

'' A 20-row request produces a non-canonical 17-pixel cell. gfxlib2 leaves
'' WIDTH unchanged in that case, so the request must not alter the active grid.
width 80, 20
packed_size = width( )
if ( packed_size and &hFFFF ) <> 80 then end 4
if ( packed_size shr 16 ) <> 25 then end 5

screen 11
packed_size = width( )
if ( packed_size and &hFFFF ) <> 80 then end 6
if ( packed_size shr 16 ) <> 30 then end 7
width 80, 60
packed_size = width( )
if ( packed_size and &hFFFF ) <> 80 then end 8
if ( packed_size shr 16 ) <> 60 then end 9
locate 60, 80
print "V";
if screen( 60, 80 ) <> asc( "V" ) then end 10

width , 30
packed_size = width( )
if ( packed_size and &hFFFF ) <> 80 then end 11
if ( packed_size shr 16 ) <> 30 then end 12

screen 0
print "gfxlib console font grids PASS"
end 0

'' end of console-font-smoke.bas
