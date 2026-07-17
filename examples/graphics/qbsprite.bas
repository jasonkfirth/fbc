'' QB-ish graphics test

sub createsprite( sprite() as byte, byval w as integer, byval h as integer, byval bpp as integer = 1 )
	redim sprite(0 to w*h*bpp-1 + 2*len(short)) as byte

	cls

	for y as integer = 0 to h-1
		for x as integer = 0 to w-1
			pset (x, y), (x xor y) * 4
		next
	next

	line (0,0)-(w-1, h-1), 0, B

	get (0, 0)-(w-1,h-1), sprite(0)

	cls
end sub

const SCREEN_MODE = 13
const xres = 320
const yres = 200
const PALETTE_LAST = 255
const VGA_PALETTE_COMPONENT_MAX = 63
const SPRITE_DRAW_COUNT = 100
const TEXT_ROW_RANGE = 23
const TEXT_COLUMN_RANGE = 40
const FRAME_DELAY_MS = 100

'' Mode 13 is intentional because this demonstrates QB-compatible graphics.
'' FB-LINTER: DISABLE-NEXT-LINE FBL734
screen SCREEN_MODE
if screenptr = 0 then
	print "Unable to create graphics mode 13."
	end 1
end if

'' Random placement is the visual effect, not deterministic test data.
'' FB-LINTER: DISABLE-NEXT-LINE FBL-NUM-012
randomize timer

window (0, 0)-(xres-1, yres-1)
view (4,4)-(xres-4,yres-4)

dim as integer mypal(0 to PALETTE_LAST)
for i as integer = 0 to PALETTE_LAST
	mypal(i) = (i \ 4) shl 16
next
mypal(PALETTE_LAST) = VGA_PALETTE_COMPONENT_MAX shl 16 or _
                      VGA_PALETTE_COMPONENT_MAX shl 8 or _
                      VGA_PALETTE_COMPONENT_MAX

palette using mypal

const spritesize = 64
dim mysprite() as byte
createsprite( mysprite(), spritesize, spritesize )

color PALETTE_LAST

do
	'' Draw the sprite in 100 different random places :)
	for i as integer = 1 to SPRITE_DRAW_COUNT
		put (-spritesize+rnd*(xRes-1+spritesize), -spritesize+rnd*(yRes-1+spritesize)), mysprite, PSET
		'color rnd*254
		'line -( rnd*(xRes-1), rnd*(yRes-1) )
		'line ( rnd*(xRes-1), rnd*(yRes-1) )-( rnd*(xRes-1), rnd*(yRes-1) )
		'circle ( rnd*(xRes-1), rnd*(yRes-1) ), rnd*100, , , , 1.0
	next

	'' Draw some text at a random position
	locate 1 + rnd*TEXT_ROW_RANGE, 1 + rnd*TEXT_COLUMN_RANGE
	color PALETTE_LAST, 0
	print "QB gfx!";

	'' Slow down the loop a little, so the pretty graphics can be seen...
	sleep FRAME_DELAY_MS, 1
loop while( len( inkey() ) = 0 )
