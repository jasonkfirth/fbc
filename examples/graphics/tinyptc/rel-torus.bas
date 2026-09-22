#include "tinyptc.bi"

declare sub smooth(target() as integer)
declare sub put_pixel(target() as integer, byval x as integer, byval y as integer, byval col as integer)
declare sub domain

const SCR_WIDTH = 320
const SCR_HEIGHT = 240
const SCR_SIZE = SCR_WIDTH*SCR_HEIGHT


const PI = 3.141593

const LENS = 256\2
const XMID = SCR_WIDTH \2
const YMID = SCR_HEIGHT \2

   ' The single display loop owns the framebuffer and trigonometric lookup tables.
   '' FB-LINTER: DISABLE-NEXT-LINE FBL301
dim shared buffer( 0 to SCR_SIZE-1 ) as integer
dim shared Lcos(0 to 359) as single
dim shared Lsin(0 to 359) as single

domain

sub domain
    dim as long frame
    dim as single sx, sy, sz, cx, cy, cz, xx, xy, xz, yx, yy, yz, zx, zy, zz, rx, ry, rz
    dim as single ax, ay, az, a, p, q, r, x, y, z
    dim as integer i, ang, angx, angy, angz, rad, dist, ox, oy, tx, ty

    if( ptc_open( "Playing with the torus", SCR_WIDTH, SCR_HEIGHT ) = 0 ) then
        end -1
    end if

    randomize timer

    for i = 0 to 359
        Lcos(i) = cos ( i * PI /180)
        Lsin(i) = sin ( i * PI /180)
    next i


    rad = 60

    angx = INT(RND * 360)
    angy = INT(RND * 360)
    angz = INT(RND * 360)


    p = -11
    q = 12

    do
        frame = (frame + 1) and &h7fffffff
        p = p + .01
        q = q - .01
        angx = (angx + 1) MOD 360
        angy = (angy + 1) MOD 360
        angz = (angz + 1) MOD 360


        ax = angx * PI / 180
        ay = angy * PI / 180
        az = angz * PI / 180

        cx = COS(ax)
        sx = SIN(ax)
        cy = COS(ay)
        sy = SIN(ay)
        cz = COS(az)
        sz = SIN(az)

        xx = cy * cz
        xy = sx * sy * cz - cx * sz
        xz = cx * sy * cz + sx * sz

        yx = cy * sz
        yy = cx * cz + sx * sy * sz
        yz = -sx * cz + cx * sy * sz

        zx = -sy
        zy = sx * cy
        zz = cx * cy

        FOR ang = 0 TO 359

            a = ang * PI / 180
            r = .5 * (2 + SIN(q * a)) * rad
            x = COS(p * a) * r
            y = COS(q * a) * r
            z = SIN(p * a) * r

            rx = (x * xx + y * xy + z * xz)
            ry = (x * yx + y * yy + z * yz)
            rz = (x * zx + y * zy + z * zz)
            dist = LENS - rz
            IF dist > 0 THEN
                tx = XMID + (rx * LENS / dist)
                ty = YMID - (ry * LENS / dist)
                ox = tx
                oy = ty
                Put_pixel Buffer(), tx, ty, 1111128
            END IF

        next ang
        smooth buffer()
        ptc_update @buffer(0)

    loop until( inkey = chr( 27 ) )


    ptc_close

end sub


' -------------------------------------------------------------------------
' Torus-renderer helpers
' -------------------------------------------------------------------------

private sub put_pixel(target() as integer, byval x as integer, byval y as integer, byval col as integer)

    if( y > 0 and y < SCR_HEIGHT-1 ) then
			if( x > 0 and x < SCR_WIDTH-1 ) then
				target(y * SCR_WIDTH + x) = col
        end if
    end if

end sub


private sub smooth(target() as integer)

    dim as integer firstpixel, maxpixel, offset, pixel, r, g, b, nr, ng, nb

    if ubound(target) < lbound(target) then exit sub
    if ubound(target) - lbound(target) + 1 <> SCR_SIZE then exit sub

    firstpixel = lbound(target)
    maxpixel = ubound(target)
    for offset = firstpixel + SCR_WIDTH to maxpixel-SCR_WIDTH
        pixel = target(offset-1)
        r = pixel shr 16
        g = pixel shr 8 and 255
        b = pixel and 255
        nr = r shr 2
        ng = g shr 2
        nb = b shr 2
        pixel = target(offset+1)
        r = pixel shr 16
        g = pixel shr 8 and 255
        b = pixel and 255
        nr = nr + ( r shr 2 )
        ng = ng + ( g shr 2 )
        nb = nb + ( b shr 2 )
        pixel = target(offset+SCR_WIDTH)
        r = pixel shr 16
        g = pixel shr 8 and 255
        b = pixel and 255
        nr = nr + ( r shr 2 )
        ng = ng + ( g shr 2 )
        nb = nb + ( b shr 2 )
        pixel = target(offset-SCR_WIDTH)
        r = pixel shr 16
        g = pixel shr 8 and 255
        b = pixel and 255
        nr = nr + ( r shr 2 )
        ng = ng + ( g shr 2 )
        nb = nb + ( b shr 2 )
        target(offset) = nr shl 16 or ng shl 8 or nb
    next offset

end sub
