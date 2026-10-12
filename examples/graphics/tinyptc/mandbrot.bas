'' Project: FreeBASIC TinyPTC examples
'' File: mandbrot.bas
'' Purpose: Render a Mandelbrot zoom through the TinyPTC display.
'' Responsibilities: Framebuffer rendering and Windows throughput reporting.
'' This file contains no reusable window or timing abstraction.
'' The one display loop owns the framebuffer and closes the display on exit.

#ifdef __FB_WIN32__
#include once "windows.bi"
#endif

#include once "tinyptc.bi"

const SCR_WIDTH 	= 320*1
const SCR_HEIGHT 	= 240*1
const SCR_SIZE	 	= SCR_WIDTH*SCR_HEIGHT

const _MAX 			= 128\2
const SX 			= -2.025 	' start value real
const SY 			= -1.125 	' start value imaginary
const EX 			= 0.6    	' end value real
const EY 			= 1.125  	' end value imaginary

declare sub mandelbrot
declare function DotsColor( byval xval as double, byval yval as double ) as double
declare function HSBtoRGB( byval hue as double, byval saturation as double, byval brightness as double ) as long

'' globals
dim shared as double xstart, ystart
dim shared as double xzoom, yzoom
dim shared as ulong buffer( 0 to SCR_SIZE-1 )

if( ptc_open( "mandelbrot test", SCR_WIDTH, SCR_HEIGHT ) = 0 ) then
    end -1
end if

xstart = SX
ystart = SY
dim as double xend = EX
dim as double yend = EY

#ifdef __FB_WIN32__
'' GetTickCount and the throughput caption belong to the Windows display.
dim as ulong starttime, tottime, sectimer
dim as integer frames
dim as HWND hwnd
hwnd = GetActiveWindow( )
frames = 0
tottime = 0
sectimer = GetTickCount()
#endif

dim as double fac = 0.99
dim as double finc = .01

do
#ifdef __FB_WIN32__
    starttime = GetTickCount()
#endif

    xend = xend * fac
    yend = yend * fac
    xstart = xstart * fac
    ystart = ystart * fac
    xzoom = (xend - xstart) / SCR_WIDTH
    yzoom = (yend - ystart) / SCR_HEIGHT

    mandelbrot( )

#ifdef __FB_WIN32__
    tottime = tottime + (GetTickCount() - starttime)
    frames = frames + 1
#endif

    fac = fac - finc
    if( fac <= 0.91 ) then
        fac = 0.93
        finc = -finc
    elseif( fac >= 1.1 ) then
        fac = .99
        finc = -finc
        xstart = SX
        ystart = SY
        xend = EX
        yend = EY
    end if

    ptc_update( @buffer(0) )

#ifdef __FB_WIN32__
    if( GetTickCount( ) - sectimer >= 1000 ) then

        if tottime > 0 then
            SetWindowText( hwnd, (SCR_WIDTH * SCR_HEIGHT * frames) \ tottime & " kpixels p/ sec " )
        end if

        sectimer = GetTickCount( )
        frames = 0
        tottime = 0
    end if
#endif


loop until( inkey = chr( 27 ) )

ptc_close()

'END

' -------------------------------------------------------------------------
' Mandelbrot
' -------------------------------------------------------------------------
' calculate all points
private sub mandelbrot static
    dim as integer x, y, col
    dim as ulong ptr p
    dim as double old, h

    p = @buffer(0)
    old = 0
    col = 0

    FOR y = 0 TO SCR_HEIGHT-1
        FOR x = 0 TO SCR_WIDTH-1

            h = DotsColor( xstart + xzoom * x, ystart + yzoom * y )

            IF h <> old then
                col = HSBtoRGB( h, 0.8, 1.0 - h * h )
                old = h
            END IF

            'buffer( y * SCR_WIDTH + x ) = col
            *p = col
            p += 1

        NEXT x
    NEXT y
end sub


' -------------------------------------------------------------------------
' DotsColor
' -------------------------------------------------------------------------
' color value from 0.0 to 1.0 by iterations
private function DotsColor( byval xval as double, byval yval as double ) as double static

    dim as integer j
    dim as double m, r, i

    j = 0 : m = 0 : r = 0 : i = 0
    do WHILE (j < _MAX) AND (m < 4.0)
        j = j + 1
        m = r * r - i * i
        i = 2.0 * r * i + yval
        r = m + xval
    loop

    function = j / _MAX

end function




' -------------------------------------------------------------------------
' HSB2RGB
' -------------------------------------------------------------------------
private function HSBtoRGB( byval hue as double, byval saturation as double, byval brightness as double ) as long static
    dim as double red, green, blue, domainOffset

    IF brightness <= 0.0 THEN
        function = 0
        exit function
    end if
    'IF saturation = 0.0 THEN
    '    function = CINT(brightness*255) shl 16 + CINT(brightness*255) shl 8 + CINT(brightness*255)
    '    exit function
    'end if

    select case hue
    case is < 1.0/6.0
        ' red domain; green ascends
        domainOffset = hue
        red   = brightness
        blue  = brightness * (1.0 - saturation)
        green = blue + (brightness - blue) * domainOffset * 6.0

    case is < 2.0/6.0
        ' yellow domain; red descends
        domainOffset = hue - 1.0/6.0
        green = brightness
        blue  = brightness * (1.0 - saturation)
        red   = green - (brightness - blue) * domainOffset * 6.0

    case is < 3.0/6.0
        ' green domain; blue ascends
        domainOffset = hue - 2.0/6.0
        green = brightness
        red   = brightness * (1.0 - saturation)
        blue  = red + (brightness - red) * domainOffset * 6.0

    case is < 4.0/6.0
        ' cyan domain; green descends
        domainOffset = hue - 3.0/6.0
        blue  = brightness
        red   = brightness * (1.0 - saturation)
        green = blue - (brightness - red) * domainOffset * 6.0

    case is < 5.0/6.0
        ' blue domain; red ascends
        domainOffset = hue - 4.0/6.0
        blue  = brightness
        green = brightness * (1.0 - saturation)
        red   = green + (brightness - green) * domainOffset * 6.0

    case else
        ' magenta domain; blue descends
        domainOffset = hue - 5.0/6.0
        red   = brightness
        green = brightness * (1.0 - saturation)
        blue  = red - (brightness - green) * domainOffset * 6.0
    ENd select

    function = rgb( red*255.0, green*255.0, blue*255.0 )

end function

'' end of mandbrot.bas
