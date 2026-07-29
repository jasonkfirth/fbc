''
'' Project: FreeBASIC gfxlib3 benchmarks
'' -------------------------------------
''
'' File: control-query-benchmark.bas
''
'' Purpose:
''
''     Measure public display, control, event, and input query commands which
''     do not perform GPU raster work.
''
'' Responsibilities:
''
''     - time SCREENINFO, SCREENLIST, SCREENCONTROL, and WINDOWTITLE
''     - time SCREENEVENT and SCREENGLPROC control-plane queries
''     - time keyboard, mouse, joystick, XPad, and touch snapshot reads
''     - retain separate machine-readable timings for every public command
''
'' This file intentionally does NOT contain:
''
''     - synthetic operating-system input injection
''     - blocking GETKEY or LINE INPUT calls
''     - a claim that control-plane work belongs in a GPU shader
''

#if defined( __FB_ANDROID__ ) and not defined( GFX2_REFERENCE )
    #ifndef __FB_GFXLIB3__
        #define __FB_GFXLIB3__
    #endif
#endif

#include once "fbgfx.bi"

#ifdef GFX3_OPENGL_TEST
    const backend_flags = fb.GFX_OPENGL
#elseif defined( GFX3_VULKAN_TEST )
    const backend_flags = fb.GFX_VULKAN
#else
    const backend_flags = 0
#endif

#ifdef __FB_ANDROID__
    const query_iterations = 256
    const platform_iterations = 16
#else
    const query_iterations = 1024
    const platform_iterations = 32
#endif

dim as long mode_width
dim as long mode_height
dim as long mode_depth
dim as long mode_bpp
dim as long mode_pitch
dim as long mode_refresh
dim as string driver_name
dim as long control_value
dim as fb.EVENT event_info
dim as any ptr gl_procedure
dim as long mouse_x
dim as long mouse_y
dim as long mouse_z
dim as long mouse_buttons
dim as long mouse_clip
dim as integer joystick_buttons
dim as integer xpad_buttons
dim as integer xpad_dpad
dim as single axis1, axis2, axis3, axis4, axis5, axis6, axis7, axis8
dim as integer touch_x
dim as integer touch_y
dim as integer touch_id
dim as ulongint checksum
dim as double started
dim as double screeninfo_seconds
dim as double screenlist_seconds
dim as double screencontrol_seconds
dim as double windowtitle_seconds
dim as double screenevent_seconds
dim as double screenglproc_seconds
dim as double multikey_seconds
dim as double getmouse_seconds
dim as double setmouse_seconds
dim as double getjoystick_seconds
dim as double getxpad_seconds
dim as double gettouchcount_seconds
dim as double gettouch_seconds
dim as double gettouchhit_seconds
dim as double screensync_seconds

if screenres( 320, 240, 32, 2, backend_flags ) <> 0 then end 1

started = timer
for index as integer = 0 to query_iterations - 1
    screeninfo mode_width, mode_height, mode_depth, mode_bpp, mode_pitch, _
        mode_refresh, driver_name
    checksum += culng( mode_width + mode_height + mode_depth + mode_bpp )
next
screeninfo_seconds = timer - started

started = timer
for index as integer = 0 to platform_iterations - 1
    checksum += culng( screenlist( 32 ) )
next
screenlist_seconds = timer - started

started = timer
for index as integer = 0 to query_iterations - 1
    screencontrol fb.GET_SCREEN_DEPTH, control_value
    checksum += culng( control_value )
next
screencontrol_seconds = timer - started

started = timer
for index as integer = 0 to platform_iterations - 1
    if ( index and 1 ) = 0 then
        windowtitle "gfxlib control benchmark A"
    else
        windowtitle "gfxlib control benchmark B"
    end if
next
screensync
windowtitle_seconds = timer - started

started = timer
for index as integer = 0 to query_iterations - 1
    checksum += culng( screenevent( @event_info ) )
next
screenevent_seconds = timer - started

started = timer
for index as integer = 0 to query_iterations - 1
    gl_procedure = screenglproc( "glGetString" )
    if gl_procedure <> 0 then checksum += 1
next
screenglproc_seconds = timer - started

started = timer
for index as integer = 0 to query_iterations - 1
    checksum += culng( multikey( fb.SC_ESCAPE ) )
next
multikey_seconds = timer - started

started = timer
for index as integer = 0 to query_iterations - 1
    checksum += culng( getmouse( mouse_x, mouse_y, mouse_z, mouse_buttons, _
        mouse_clip ) )
next
getmouse_seconds = timer - started

started = timer
for index as integer = 0 to platform_iterations - 1
    checksum += culng( setmouse( index mod 320, ( index * 3 ) mod 240, 1, 0 ) )
next
setmouse_seconds = timer - started

started = timer
for index as integer = 0 to query_iterations - 1
    checksum += culng( getjoystick( 0, joystick_buttons, axis1, axis2, axis3, _
        axis4, axis5, axis6, axis7, axis8 ) )
next
getjoystick_seconds = timer - started

started = timer
for index as integer = 0 to query_iterations - 1
    checksum += culng( getxpad( 0, xpad_buttons, axis1, axis2, axis3, axis4, _
        axis5, axis6, xpad_dpad ) )
next
getxpad_seconds = timer - started

started = timer
for index as integer = 0 to query_iterations - 1
    checksum += culng( gettouchcount() )
next
gettouchcount_seconds = timer - started

started = timer
for index as integer = 0 to query_iterations - 1
    checksum += culng( gettouch( 0, touch_x, touch_y, touch_id ) )
next
gettouch_seconds = timer - started

started = timer
for index as integer = 0 to query_iterations - 1
    checksum += culng( gettouchhit( 0, 0, 16, 16 ) )
next
gettouchhit_seconds = timer - started

started = timer
for index as integer = 0 to platform_iterations - 1
    screensync
next
screensync_seconds = timer - started

screen 0
print "control_query_screeninfo_seconds="; screeninfo_seconds
print "control_query_screenlist_seconds="; screenlist_seconds
print "control_query_screencontrol_seconds="; screencontrol_seconds
print "control_query_windowtitle_seconds="; windowtitle_seconds
print "control_query_screenevent_seconds="; screenevent_seconds
print "control_query_screenglproc_seconds="; screenglproc_seconds
print "control_query_multikey_seconds="; multikey_seconds
print "control_query_getmouse_seconds="; getmouse_seconds
print "control_query_setmouse_seconds="; setmouse_seconds
print "control_query_getjoystick_seconds="; getjoystick_seconds
print "control_query_getxpad_seconds="; getxpad_seconds
print "control_query_gettouchcount_seconds="; gettouchcount_seconds
print "control_query_gettouch_seconds="; gettouch_seconds
print "control_query_gettouchhit_seconds="; gettouchhit_seconds
print "control_query_screensync_seconds="; screensync_seconds
print "control_query_checksum="; checksum
end 0

'' end of control-query-benchmark.bas
