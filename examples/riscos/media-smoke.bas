'
' Project: FreeBASIC RISC OS examples
' -----------------------------------
'
' File: media-smoke.bas
'
' Purpose:
'
'     Prove native gfxlib2 presentation and sfxlib playback on RISC OS.
'
' Responsibilities:
'
'     - verify both native Wimp-window and fullscreen graphics modes
'     - verify red, green, blue, and magenta reach native presentation
'       surfaces without channel reversal
'     - exercise native pointer positioning, visibility, and polling
'     - verify case-insensitive DIR matching through UnixLib suffix directories
'     - play a generated tone through DigitalRenderer and SoundDMA
'     - retain the windowed frame for package-verifier screenshot capture
'     - emit deterministic console markers for emulator automation
'
' This file intentionally does NOT contain:
'
'     - interactive keyboard testing
'     - media-file decoding
'     - timing or audio-quality measurements
'     - RPCEmu launch automation
'

#include once "fbgfx.bi"

declare function fb_riscosGfxReadPresentedPixel cdecl _
    alias "fb_riscosGfxReadPresentedPixel" _
    ( byval x as long, byval y as long ) as ulong
declare function fb_riscosGfxReadSourcePixel cdecl _
    alias "fb_riscosGfxReadSourcePixel" _
    ( byval x as long, byval y as long ) as ulong
declare function fb_riscosGfxPointerIsVisible cdecl _
    alias "fb_riscosGfxPointerIsVisible" ( ) as long

declare function fb_sfxDeviceCurrent cdecl _
    alias "fb_sfxDeviceCurrent" ( ) as long
declare function fb_sfxDeviceName cdecl _
    alias "fb_sfxDeviceName" ( byval id as long ) as zstring ptr

' Magenta is represented exactly in both RISC OS 16bpp and 32bpp modes.
const expected_pixel = rgb( 255, 0, 255 )
const expected_red = rgb( 255, 0, 0 )
const expected_green = rgb( 0, 255, 0 )
const expected_blue = rgb( 0, 0, 255 )

''
'' RISC OS desktops may use an 8bpp VIDC palette for a Wimp window.  That
'' palette is a nearest-colour presentation surface, so an exact 255-level
'' comparison is not valid there.  A valid primary must still be bright and
'' must dominate the two channels that do not describe that primary.
''
function windowed_primary_visible( byval pixel as ulong, byval primary as long ) as long
    dim as long blue_component
    dim as long green_component
    dim as long red_component

    red_component = ( pixel shr 16 ) and 255
    green_component = ( pixel shr 8 ) and 255
    blue_component = pixel and 255

    select case primary
    case 0
        return red_component >= 128 andalso _
            red_component >= green_component + 64 andalso _
            red_component >= blue_component + 64
    case 1
        return green_component >= 128 andalso _
            green_component >= red_component + 64 andalso _
            green_component >= blue_component + 64
    case 2
        return blue_component >= 128 andalso _
            blue_component >= red_component + 64 andalso _
            blue_component >= green_component + 64
    end select

    return 0
end function

function windowed_magenta_visible( byval pixel as ulong ) as long
    dim as long blue_component
    dim as long green_component
    dim as long red_component

    red_component = ( pixel shr 16 ) and 255
    green_component = ( pixel shr 8 ) and 255
    blue_component = pixel and 255

    return red_component >= 128 andalso blue_component >= 128 andalso _
        red_component >= green_component + 64 andalso _
        blue_component >= green_component + 64
end function

dim as string graphics_driver
dim as string directory_entry
dim as zstring ptr sound_driver
dim as long directory_attributes
dim as long mouse_x
dim as long mouse_y
dim as long mouse_wheel
dim as long mouse_buttons
dim as long mouse_clip
dim as long windowed_hold_milliseconds
dim as ulong presented_pixel

setenviron "SFXLIB_DEBUG=1"
setenviron "GFXLIB_DEBUG=1"
setenviron "SFXLIB_DEBUG_LOG=media-smoke/sfx-debug"
setenviron "GFXLIB_DEBUG_LOG=media-smoke/gfx-debug"

directory_entry = dir( "dir-smoke/bmp", &h10, directory_attributes )
if len( directory_entry ) = 0 then
    print "FB_RISCOS_MEDIA_ERROR=dir-suffix-parent"
    end 19
end if

if ( directory_attributes and &h10 ) = 0 then
    print "FB_RISCOS_MEDIA_ERROR=dir-suffix-parent-attrib"
    end 20
end if

directory_entry = dir( "dir-smoke/bmp/*.BMP" )
print "FB_RISCOS_DIR_ENTRY=" & directory_entry
if lcase( directory_entry ) <> "upper.bmp" then
    print "FB_RISCOS_MEDIA_ERROR=dir-case-wildcard"
    end 21
end if

if screenres( 320, 240, 32 ) <> 0 then
    print "FB_RISCOS_MEDIA_ERROR=screenres-windowed"
    end 22
end if

screencontrol fb.GET_DRIVER_NAME, graphics_driver
if graphics_driver <> "RISC OS" then
    screen 0
    print "FB_RISCOS_MEDIA_ERROR=gfx-driver"
    end 23
end if

''
'' ScreenLock is intentionally held while normal gfxlib2 primitives run.
'' Interactive programs use this to present a completed frame atomically, so
'' the RISC OS multithreaded runtime must provide the same recursive graphics
'' lock contract as the other native targets.
''
screenlock
pset ( 4, 4 ), expected_red
screenunlock
screensync

if point( 4, 4 ) <> expected_red then
    screen 0
    print "FB_RISCOS_MEDIA_ERROR=windowed-screenlock"
    end 24
end if

line ( 0, 0 )-( 319, 239 ), rgb( 8, 16, 32 ), bf
line ( 12, 12 )-( 307, 227 ), rgb( 255, 255, 255 ), b
circle ( 160, 120 ), 64, rgb( 255, 208, 32 )
paint ( 160, 120 ), rgb( 24, 96, 168 ), rgb( 255, 208, 32 )
line ( 20, 28 )-( 35, 43 ), expected_red, bf
line ( 40, 28 )-( 55, 43 ), expected_green, bf
line ( 60, 28 )-( 75, 43 ), expected_blue, bf
line ( 80, 28 )-( 95, 43 ), expected_pixel, bf
pset ( 23, 29 ), expected_red
pset ( 43, 29 ), expected_green
pset ( 63, 29 ), expected_blue
pset ( 83, 29 ), expected_pixel
screensync

if point( 83, 29 ) <> expected_pixel then
    screen 0
    print "FB_RISCOS_MEDIA_ERROR=windowed-logical-pixel"
    end 24
end if

presented_pixel = fb_riscosGfxReadSourcePixel( 23, 29 )
if presented_pixel <> expected_red then
    screen 0
    print "FB_RISCOS_MEDIA_ERROR=windowed-source-red"
    print "FB_RISCOS_WINDOWED_SOURCE_RED=" & hex( presented_pixel )
    end 25
end if

presented_pixel = fb_riscosGfxReadSourcePixel( 43, 29 )
if presented_pixel <> expected_green then
    screen 0
    print "FB_RISCOS_MEDIA_ERROR=windowed-source-green"
    print "FB_RISCOS_WINDOWED_SOURCE_GREEN=" & hex( presented_pixel )
    end 25
end if

presented_pixel = fb_riscosGfxReadSourcePixel( 63, 29 )
if presented_pixel <> expected_blue then
    screen 0
    print "FB_RISCOS_MEDIA_ERROR=windowed-source-blue"
    print "FB_RISCOS_WINDOWED_SOURCE_BLUE=" & hex( presented_pixel )
    end 25
end if

presented_pixel = fb_riscosGfxReadSourcePixel( 83, 29 )
if presented_pixel <> expected_pixel then
    screen 0
    print "FB_RISCOS_MEDIA_ERROR=windowed-source-magenta"
    print "FB_RISCOS_WINDOWED_SOURCE_MAGENTA=" & hex( presented_pixel )
    end 25
end if

presented_pixel = fb_riscosGfxReadPresentedPixel( 23, 29 )
if windowed_primary_visible( presented_pixel, 0 ) = 0 then
    screen 0
    print "FB_RISCOS_MEDIA_ERROR=windowed-presented-red"
    print "FB_RISCOS_WINDOWED_RED_PRESENTED=" & hex( presented_pixel )
    end 25
end if

presented_pixel = fb_riscosGfxReadPresentedPixel( 43, 29 )
if windowed_primary_visible( presented_pixel, 1 ) = 0 then
    screen 0
    print "FB_RISCOS_MEDIA_ERROR=windowed-presented-green"
    print "FB_RISCOS_WINDOWED_GREEN_PRESENTED=" & hex( presented_pixel )
    end 25
end if

presented_pixel = fb_riscosGfxReadPresentedPixel( 63, 29 )
if windowed_primary_visible( presented_pixel, 2 ) = 0 then
    screen 0
    print "FB_RISCOS_MEDIA_ERROR=windowed-presented-blue"
    print "FB_RISCOS_WINDOWED_BLUE_PRESENTED=" & hex( presented_pixel )
    end 25
end if

presented_pixel = fb_riscosGfxReadPresentedPixel( 83, 29 )
if windowed_magenta_visible( presented_pixel ) = 0 then
    screen 0
    print "FB_RISCOS_MEDIA_ERROR=windowed-presented-magenta"
    print "FB_RISCOS_WINDOWED_MAGENTA_PRESENTED=" & hex( presented_pixel )
    end 25
end if

setmouse 41, 53, 1, 0
screensync
if fb_riscosGfxPointerIsVisible() = 0 then
    screen 0
    print "FB_RISCOS_MEDIA_ERROR=windowed-mouse-cursor-show"
    end 26
end if

''
'' RPCEmu continues to feed its host pointer at each vertical sync, so an
'' unattended test cannot retain a requested absolute position.  The native
'' backend must still return a valid coordinate in the Wimp work area.
''
if getmouse( mouse_x, mouse_y, mouse_wheel, mouse_buttons, mouse_clip ) <> 0 then
    screen 0
    print "FB_RISCOS_MEDIA_ERROR=windowed-mouse-read"
    end 27
end if

if mouse_x < 0 orelse mouse_x >= 320 orelse _
   mouse_y < 0 orelse mouse_y >= 240 then
    screen 0
    print "FB_RISCOS_MEDIA_ERROR=windowed-mouse-range"
    print "FB_RISCOS_WINDOWED_MOUSE=" & mouse_x & "," & mouse_y
    end 28
end if

''
'' The package verifier compiles this example with its private define so it
'' can capture the completed Wimp frame.  The normal graphics SLEEP hook
'' continues to pump the Wimp event queue during the hold, so title-bar drags
'' and close requests remain responsive.  Normal smoke tests remain prompt,
'' and the fixed delay does not rely on task-specific UnixEnv inheritance.
''
#ifdef FB_RISCOS_PACKAGE_CAPTURE
    windowed_hold_milliseconds = 6000
    sleep windowed_hold_milliseconds
#endif

screen 0
print "FB_RISCOS_GFX_WINDOWED_OK"

if screenres( 320, 240, 32, , fb.GFX_FULLSCREEN ) <> 0 then
    print "FB_RISCOS_MEDIA_ERROR=screenres-fullscreen"
    end 29
end if

line ( 0, 0 )-( 319, 239 ), rgb( 8, 16, 32 ), bf
line ( 12, 12 )-( 307, 227 ), rgb( 255, 255, 255 ), b
circle ( 160, 120 ), 64, rgb( 255, 208, 32 )
paint ( 160, 120 ), rgb( 24, 96, 168 ), rgb( 255, 208, 32 )
pset ( 23, 29 ), expected_pixel
screensync

if point( 23, 29 ) <> expected_pixel then
    screen 0
    print "FB_RISCOS_MEDIA_ERROR=fullscreen-logical-pixel"
    end 30
end if

if fb_riscosGfxReadPresentedPixel( 23, 29 ) <> expected_pixel then
    screen 0
    print "FB_RISCOS_MEDIA_ERROR=fullscreen-presented-pixel"
    end 31
end if

setmouse 41, 53, 1, 0
screensync
if fb_riscosGfxPointerIsVisible() = 0 then
    screen 0
    print "FB_RISCOS_MEDIA_ERROR=fullscreen-mouse-cursor-show"
    end 32
end if

if getmouse( mouse_x, mouse_y, mouse_wheel, mouse_buttons, mouse_clip ) <> 0 then
    screen 0
    print "FB_RISCOS_MEDIA_ERROR=fullscreen-mouse-read"
    end 33
end if

if mouse_x < 0 orelse mouse_x >= 320 orelse _
   mouse_y < 0 orelse mouse_y >= 240 then
    screen 0
    print "FB_RISCOS_MEDIA_ERROR=fullscreen-mouse-range"
    print "FB_RISCOS_FULLSCREEN_MOUSE=" & mouse_x & "," & mouse_y
    end 34
end if

setmouse , , 0
if fb_riscosGfxPointerIsVisible() <> 0 then
    screen 0
    print "FB_RISCOS_MEDIA_ERROR=fullscreen-mouse-cursor-hide"
    end 35
end if

setmouse , , 1
screensync
if fb_riscosGfxPointerIsVisible() = 0 then
    screen 0
    print "FB_RISCOS_MEDIA_ERROR=fullscreen-mouse-cursor-restore"
    end 36
end if

screen 0
print "FB_RISCOS_GFX_FULLSCREEN_OK"

sound 440, 0.20

sound_driver = fb_sfxDeviceName( fb_sfxDeviceCurrent() )
if sound_driver = 0 then
    screen 0
    print "FB_RISCOS_MEDIA_ERROR=sfx-driver-missing"
    end 37
end if

if *sound_driver <> "RISC OS DigitalRenderer" then
    screen 0
    print "FB_RISCOS_MEDIA_ERROR=sfx-driver-fallback"
    print "FB_RISCOS_SFX_DRIVER=" & *sound_driver
    end 38
end if

sleep 250, 1

print "FB_RISCOS_GFX_DRIVER=" & graphics_driver
print "FB_RISCOS_SFX_DRIVER=" & *sound_driver
print "FB_RISCOS_MEDIA_SMOKE_OK"

end 0

' end of media-smoke.bas
