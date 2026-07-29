''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: bload-raw-smoke.bas
''
'' Purpose:
''
''     Verify FreeBASIC BSAVE/BLOAD raw blocks for ordinary memory and the
''     active GPU-backed screen page.
''
'' Responsibilities:
''
''     - save and restore an explicit byte block
''     - save and restore the current 32-bit screen page
''     - exercise the synchronized GPU download/upload route
''     - remove only the files created by this fixture
''
'' This file intentionally does NOT contain:
''
''     - BMP palette, RLE, or bitfield coverage
''     - file-system permission policy tests
''     - screen page size negotiation
''
#ifndef GFX2_REFERENCE
    #ifndef __FB_GFXLIB3__
        #define __FB_GFXLIB3__
    #endif
#endif

const raw_filename = "gfx3-bload-raw-memory.bin"
const screen_filename = "gfx3-bload-raw-screen.bin"
const screen_width = 16
const screen_height = 16
const screen_bytes = screen_width * screen_height * 4

#ifdef GFX3_OPENGL_TEST
    setenviron "FBGFX=opengl"
#elseif defined( GFX3_VULKAN_TEST )
    setenviron "FBGFX=vulkan"
#elseif not defined( GFX3_AUTOMATIC_TEST )
    setenviron "FBGFX=null"
#endif

dim original( 0 to 6 ) as ubyte = { &h00, &h11, &h22, &h33, &hA5, &hCC, &hFF }
dim restored( 0 to 6 ) as ubyte

if bsave( raw_filename, @original( 0 ), sizeof( original( 0 ) ) * 7 ) <> 0 then end 1
if bload( raw_filename, @restored( 0 ) ) <> 0 then end 2
for index as integer = 0 to 6
    if restored( index ) <> original( index ) then end 3
next

#ifndef GFX2_REFERENCE
'' gfxlib2's Linux Null driver faults while restoring its own raw screen dump.
'' Keep the shared oracle to its stable explicit-memory block and verify the
'' GPU-backed screen route directly through gfxlib3's supported backends.
if screenres( screen_width, screen_height, 32 ) <> 0 then end 4
pset ( 2, 3 ), rgba( 10, 20, 30, 40 )
pset ( 14, 15 ), rgba( 200, 180, 160, 140 )
if bsave( screen_filename, 0, screen_bytes ) <> 0 then end 5
cls
if point( 2, 3 ) <> rgb( 0, 0, 0 ) then end 6
if bload( screen_filename, 0 ) <> 0 then end 7
if culng( point( 2, 3 ) ) <> rgba( 10, 20, 30, 40 ) then end 8
if culng( point( 14, 15 ) ) <> rgba( 200, 180, 160, 140 ) then end 9

screen 0
#endif
kill raw_filename
#ifndef GFX2_REFERENCE
kill screen_filename
#endif
print "gfxlib raw bload PASS"
end 0

'' end of bload-raw-smoke.bas
