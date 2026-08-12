' TEST_MODE : COMPILE_AND_RUN_OK

''
'' FreeBASIC command sweep tests
'' ---------------------------
''
'' File: gfxlib-command-sweep.bas
''
'' Purpose:
''
''     Compile and run one program that touches every public gfxlib
''     command and function registered by the compiler for -lang fb.
''
'' Responsibilities:
''
''     - exercise parser-only graphics statements from parser-quirk-gfx.bas
''     - exercise gfxlib-linked intrinsics from rtl-gfx.bas
''     - use the null graphics driver so the test can run without a window
''     - verify a few visible side effects through framebuffer reads
''
'' This file intentionally does NOT contain:
''
''     - QB-only stick/strig coverage
''     - direct hardware port I/O registered beside gfxlib in rtl-gfx.bas
''     - platform build orchestration
''

#include once "fbgfx.bi"

const SKIP_NO_GFX = 77

dim shared as integer failures
dim shared as string diagnostics

sub record_failure( byref message as string )
	diagnostics += message + chr( 10 )
	failures += 1
end sub

sub delete_file_if_present( byref filename as string )
	dim as integer f = freefile()

	if( open( filename for input as #f ) = 0 ) then
		close #f
		kill filename
	end if
end sub

sub expect_long _
	( _
		byref label as string, _
		byval actual as long, _
		byval expected as long _
	)

	if( actual <> expected ) then
		record_failure label + ": expected " + str( expected ) + ", got " + str( actual )
	end if
end sub

function custom_blender _
	( _
		byval source_pixel as ulong, _
		byval destination_pixel as ulong, _
		byval parameter as any ptr _
	) as ulong

	function = source_pixel xor destination_pixel
end function

if( screenres( 64, 64, 32, 2, fb.GFX_NULL ) <> 0 ) then
	end SKIP_NO_GFX
end if

windowtitle "FreeBASIC gfxlib command sweep"

screen , 0, 0
screenset 0, 1

line (0, 0)-(63, 63), rgb( 0, 0, 0 ), bf
pset (1, 1), rgb( 255, 0, 0 )
expect_long "pset/point", point( 1, 1 ), rgb( 255, 0, 0 )

preset (1, 1)
expect_long "preset/point", point( 1, 1 ), rgb( 0, 0, 0 )

line (2, 2)-(20, 2), rgb( 0, 255, 0 )
line (4, 4)-(14, 14), rgb( 0, 0, 255 ), b
line (16, 4)-(24, 12), rgb( 12, 34, 56 ), bf
circle (32, 16), 8, rgb( 255, 255, 0 )
circle (48, 16), 6, rgb( 255, 0, 255 ), 0.0, 6.2831853, 1.0, f

paint (17, 5), rgb( 64, 64, 64 ), rgb( 12, 34, 56 )
draw "BM 5,30 C" & rgb( 0, 255, 255 ) & " R10 D10 L10 U10"
draw string (4, 44), "gfx", rgb( 200, 200, 200 )

view (0, 0)-(63, 63), rgb( 0, 0, 0 ), rgb( 255, 255, 255 )
window (0, 0)-(63, 63)
window
view

dim as any ptr image_a = imagecreate( 16, 16, rgb( 16, 32, 48 ), 32 )
dim as any ptr image_b = imagecreate( 16, 16, rgb( 0, 0, 0 ), 32 )

if( image_a = 0 orelse image_b = 0 ) then
	screen 0
	print "imagecreate failed"
	end 2
end if

pset image_a, (1, 1), rgb( 255, 128, 0 )
line image_a, (2, 2)-(8, 8), rgb( 8, 16, 24 ), bf
circle image_a, (10, 10), 3, rgb( 128, 0, 128 )
paint image_a, (3, 3), rgb( 40, 40, 40 ), rgb( 8, 16, 24 )
draw image_a, "BM 1,14 C" & rgb( 20, 220, 20 ) & " R8"
draw string image_a, (1, 4), "i", rgb( 255, 255, 255 )

put (0, 0), image_a, pset
get (0, 0)-(15, 15), image_b
put (18, 0), image_b, trans
put (36, 0), image_b, preset
put (0, 18), image_b, and
put (18, 18), image_b, or
put (36, 18), image_b, xor
put (0, 36), image_b, alpha, 128
put (18, 36), image_b, add
put (36, 36), image_b, custom, @custom_blender, 0

dim as long iw, ih, ibpp, ipitch, isize
dim as any ptr idata
if( imageinfo( image_a, iw, ih, ibpp, ipitch, idata, isize ) <> 0 ) then
	record_failure "imageinfo long failed"
end if

expect_long "imageinfo width", iw, 16
expect_long "imageinfo height", ih, 16
expect_long "imageinfo bytes per pixel", ibpp, 4

dim as longint iwl, ihl, ibppl, ipitchl, isizel
dim as any ptr idatal
if( imageinfo( image_a, iwl, ihl, ibppl, ipitchl, idatal, isizel ) <> 0 ) then
	record_failure "imageinfo longint failed"
end if

dim as ulong src_row(0 to 3) = { rgb( 1, 2, 3 ), rgb( 4, 5, 6 ), rgb( 7, 8, 9 ), rgb( 10, 11, 12 ) }
dim as ulong dst_row(0 to 3)
imageconvertrow @src_row(0), 32, @dst_row(0), 32, 4, 1

dim as string filename = "gfxlib-command-sweep.bmp"
delete_file_if_present filename
if( bsave( filename, image_a, 0 ) <> 0 ) then
	record_failure "bsave image failed"
end if

if( bload( filename, image_b ) <> 0 ) then
	record_failure "bload image failed"
else
	expect_long "bload image pixel", point( 0, 0, image_b ), rgb( 16, 32, 48 )
end if

delete_file_if_present filename

dim as long sw, sh, sd, sbpp, spitch, srefresh
dim as string sdriver
screeninfo sw, sh, sd, sbpp, spitch, srefresh, sdriver
expect_long "screeninfo width", sw, 64
expect_long "screeninfo height", sh, 64

dim as longint screen_wl, screen_hl, screen_dl, screen_bppl, screen_pitchl, screen_refreshl
dim as string sdriverl
screeninfo screen_wl, screen_hl, screen_dl, screen_bppl, screen_pitchl, screen_refreshl, sdriverl

dim as string title
screencontrol fb.GET_WINDOW_TITLE, title
screencontrol fb.GET_DRIVER_NAME, sdriver

dim as long control_value = 0
screencontrol fb.POLL_EVENTS, control_value
screencontrol fb.GET_SCREEN_DEPTH, control_value
screencontrol fb.GET_PEN_POS, sw, sh
screencontrol fb.SET_PEN_POS, sw, sh

dim as longint control_value64 = 0
screencontrol fb.GET_SCREEN_BPP, control_value64

dim as fb.EVENT event_
dim as long event_result = screenevent( @event_ )
event_result = screenevent()

dim as any ptr screen_memory = screenptr()
if( screen_memory = 0 ) then
	record_failure "screenptr returned NULL"
end if

dim as single px = pointcoord( 0 )
dim as single py = pointcoord( 1 )
dim as single mapped = pmap( 10.0, 0 )

dim as long mouse_x, mouse_y, mouse_z, mouse_buttons, mouse_clip
dim as long mouse_result = getmouse( mouse_x, mouse_y, mouse_z, mouse_buttons, mouse_clip )
dim as long mouse_set_result = setmouse( 8, 8, 1, 0 )

dim as longint mouse_x64, mouse_y64, mouse_z64, mouse_buttons64, mouse_clip64
mouse_result = getmouse( mouse_x64, mouse_y64, mouse_z64, mouse_buttons64, mouse_clip64 )

dim as integer joystick_buttons, xpad_buttons, xpad_dpad
dim as single axis1, axis2, axis3, axis4, axis5, axis6, axis7, axis8
dim as long joystick_result = getjoystick( 0, joystick_buttons, axis1, axis2, axis3, axis4, axis5, axis6, axis7, axis8 )
dim as long xpad_result = getxpad( 0, xpad_buttons, axis1, axis2, axis3, axis4, axis5, axis6, xpad_dpad )
dim as long key_result = multikey( 1 )

dim as any ptr gl_proc = screenglproc( "glGetString" )

screensync
flip 0, 1
screencopy 1, 0
screenlock
pset (2, 2), rgb( 90, 91, 92 )
screenunlock 0, 63

if( screenlist( 32 ) < 0 ) then
	record_failure "screenlist returned a negative value"
end if

if( screenres( 64, 64, 8, 1, fb.GFX_NULL ) = 0 ) then
	dim as long pal(0 to 255)
	dim as long pr, pg, pb

	palette 1, rgb( 3, 5, 7 )
	palette 2, 10, 20, 30
	palette get 2, pr, pg, pb
	palette using pal(0)
	palette get using pal(0)
end if

imagedestroy image_b
imagedestroy image_a

screen 0

if( failures <> 0 ) then
	print diagnostics;
	end 1
end if

end 0

'' end of gfxlib-command-sweep.bas
