''
'' This fbgfx example deals with:
'' - palette
'' - multiple pages and double buffering
'' - direct access to the screen memory
'' - drawing to GET/PUT buffers
''

#define MAX_EXPLOSIONS     32
#define MAX_EXPLOSION_SIZE 100
#define MIN_EXPLOSION_SIZE (MAX_EXPLOSION_SIZE \ 4)
#define SCREEN_MODE         14
#define SCREEN_WIDTH        320
#define SCREEN_HEIGHT       240
#define SCREEN_DEPTH        8
#define SCREEN_PAGES        3
#define PALETTE_SIZE        256
#define LOGO_COLOR_COUNT    64
#define LOGO_PALETTE_OFFSET 192
#define FIRE_COLOR          191
#define FIRE_SEED_COUNT     6
#define EXPLOSION_CHANCE    50
#define EXPLOSION_LIFETIME  192
#define VGA_COMPONENT_MAX   &h3F

#include "fbgfx.bi"

type EXPLOSION_TYPE
	sprite as ubyte ptr
	x as integer
	y as integer
	used as integer
	count as integer
end type

sub release_explosion(byref explosion as EXPLOSION_TYPE)
	if explosion.sprite <> 0 then
		imagedestroy explosion.sprite
		explosion.sprite = 0
	end if

	explosion.used = FALSE
	explosion.count = 0
end sub

sub animate_fire(byval buffer as ubyte ptr, byval new_ as integer = 0)
	dim w as integer, h as integer, pitch as integer
	dim c0 as integer, c1 as integer, c2 as integer, c3 as integer
	dim header as FB.PUT_HEADER ptr

	if buffer = 0 then exit sub

	header = cast(FB.PUT_HEADER ptr, buffer)
	w = header->width
	h = header->height
	pitch = header->pitch

	if new_ then
		line buffer, (0, 0)-(w-1, h-1), 0, bf
		for i as integer = 1 to FIRE_SEED_COUNT
			'' The main program seeds the generator before creating explosions.
			'' FB-LINTER: DISABLE-NEXT-LINE FBL-NUM-011
			circle buffer, ((w\4)+(rnd*(w\2)), (h\4)+(rnd*(h\2))), (w\6), FIRE_COLOR,,,,F
		next
	else
		for y as integer = 1 to h-2
			for x as integer = 1 to w-2
				c0 = buffer[sizeof(FB.PUT_HEADER) + (y * pitch) + x - 1]
				c1 = buffer[sizeof(FB.PUT_HEADER) + (y * pitch) + x + 1]
				c2 = buffer[sizeof(FB.PUT_HEADER) + ((y - 1) * pitch) + x]
				c3 = buffer[sizeof(FB.PUT_HEADER) + ((y + 1) * pitch) + x]
				c0 = ((c0 + c1 + c2 + c3) \ 4) - rnd*2
				if (cint(c0) < 0) then c0 = 0
				buffer[sizeof(FB.PUT_HEADER) + (y * pitch) + x] = c0
			next
		next
	end if
end sub


	dim pal(0 to PALETTE_SIZE - 1) as integer, r as integer, g as integer, b as integer
	dim explosion(0 to MAX_EXPLOSIONS - 1) as EXPLOSION_TYPE
	dim work_page as integer

	screen SCREEN_MODE, SCREEN_DEPTH, SCREEN_PAGES
	if screenptr = 0 then
		print "Unable to create the graphics screen."
		end 1
	end if

	'' The moving flames are intended to differ on every run.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL-NUM-012
	randomize timer

	'' load image and get palette
	screenset 2
	if bload(exepath() & "/../fblogo.bmp") <> 0 then
		print "Unable to load fblogo.bmp."
		end 1
	end if
	palette get using pal

	'' The image uses the first 64 colors. Since the fire uses colors 0-191,
	'' move the logo palette and pixels into colors 192-255.
	screenlock
	dim as byte ptr pixel = screenptr()
	for i as integer = 0 to (SCREEN_WIDTH * SCREEN_HEIGHT) - 1
		pixel[i] = LOGO_PALETTE_OFFSET + pixel[i]
	next
	screenunlock
	for i as integer = 0 to LOGO_COLOR_COUNT - 1
		pal(LOGO_PALETTE_OFFSET + i) = pal(i)
	next

	'' create fire palette
	for i as integer = 0 to LOGO_COLOR_COUNT - 1
		pal(i) = i
		pal(LOGO_COLOR_COUNT + i) = VGA_COMPONENT_MAX or (i shl 8)
		pal((LOGO_COLOR_COUNT * 2) + i) = _
		    (VGA_COMPONENT_MAX shl 8) or VGA_COMPONENT_MAX or (i shl 16)
	next
	palette using pal

	'' start demo
	screenset 1, 0
	work_page = 1

	do
		screencopy 2, work_page

		for i as integer = 0 to MAX_EXPLOSIONS-1
			if (explosion(i).used = FALSE) andalso ((rnd * EXPLOSION_CHANCE) < 1) then
				dim as integer size = MIN_EXPLOSION_SIZE + _
				                      cint(rnd * (MAX_EXPLOSION_SIZE - MIN_EXPLOSION_SIZE))

				with explosion(i)
					.sprite = imagecreate( size, size )
					if .sprite <> 0 then
						.x = rnd * SCREEN_WIDTH
						.y = rnd * SCREEN_HEIGHT
						.used = TRUE
						.count = EXPLOSION_LIFETIME
						animate_fire .sprite, TRUE
					end if
				end with
			end if

			if explosion(i).used then
				animate_fire( explosion(i).sprite )

				put (explosion(i).x, explosion(i).y), explosion(i).sprite, trans

				explosion(i).count -= 1
				if explosion(i).count <= 0 then
					release_explosion explosion(i)
				end if
			end if
		next

		screensync
		work_page xor= 1
		screenset work_page, work_page xor 1
	loop while inkey = ""

	for i as integer = 0 to MAX_EXPLOSIONS - 1
		release_explosion explosion(i)
	next
