''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: qb-screen-smoke.bas
''
'' Purpose:
''
''     Verify QuickBASIC SCREEN mode setup and visible/active page selection.
''
'' Responsibilities:
''
''     - open the historical eight-page SCREEN 7 mode
''     - retain separate pixels on active pages zero and one
''     - select each page for both visible and active use
''     - close the QB mode through SCREEN 0
''
'' This file intentionally does NOT contain:
''
''     - hardware palette register checks
''     - visible-window pixel capture
''     - QB text-mode or controller behavior
''
#lang "qb"

#ifndef GFX2_REFERENCE
    #ifndef __FB_GFXLIB3__
        #define __FB_GFXLIB3__
    #endif
#endif

declare function qb_page_set alias "fb_GfxPageSet" _
    ( byval work_page as long, byval visible_page as long ) as long

'' The compile-time switches keep this QB-language fixture portable. Android
'' deliberately leaves FBGFX unset and follows the normal Vulkan/GLES policy.
#ifdef GFX3_OPENGL_TEST
    setenviron "FBGFX=opengl"
#elseif defined( GFX3_VULKAN_TEST )
    setenviron "FBGFX=vulkan"
#elseif defined( GFX3_AUTOMATIC_TEST )
    '' Use automatic backend selection.
#elseif defined( __FB_ANDROID__ )
    '' Use automatic backend selection on the physical GLES target.
#else
    setenviron "FBGFX=null"
#endif

dim page_zero_pixel as integer

screen 7, 0, 1
pset ( 4, 4 ), 6
qb_page_set( 0, 0 )
if point( 4, 4 ) <> 0 then
    page_zero_pixel = point( 4, 4 )
    screen 0
    print "QB page zero expected 0, got " & page_zero_pixel
    end 1
end if
pset ( 4, 4 ), 3
qb_page_set( 1, 1 )
if point( 4, 4 ) <> 6 then end 2
qb_page_set( 0, 0 )
if point( 4, 4 ) <> 3 then end 3

screen 0
end 0

'' end of qb-screen-smoke.bas
