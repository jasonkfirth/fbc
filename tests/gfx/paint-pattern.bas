'' FreeBASIC graphics tests: paint-pattern.bas
'' Check packed two-color fills, target phase, clipping and rejected patterns.
'' This is a pixel oracle using public APIs; it does not emulate VBDOS colors.
' TEST_MODE : COMPILE_AND_RUN_OK

#include once "fbgfx.bi"
#ifdef __FB_GFXLIB3__
#include once "fbgfx3.bi"
#endif

dim shared as integer checks

private sub require( byval condition as boolean, byref message as const string )
	checks += 1
	if condition then exit sub
	screen 0
	print "FAIL: " & message
	end 1
end sub

'' One GET readback per fill keeps this exhaustive oracle practical on GPUs.
'' Convert native RGB565 bytes to the same expanded RGB value as POINT.
function snapshot_pixel( byval pixels as ubyte ptr, byval pitch as integer, _
	byval bpp as integer, byval x as integer, byval y as integer ) as ulong
	dim as ubyte ptr pixel = pixels + y * pitch + x * bpp
	if bpp = 1 then return pixel[0]
	if bpp = 4 then return *cptr( ulong ptr, pixel )
	dim as ulong value = *cptr( ushort ptr, pixel )
	return ((value and &h001F) shl 3) or ((value shr 2) and 7) or _
		((value and &h07E0) shl 5) or ((value shr 1) and &h300) or _
		((value and &hF800) shl 8) or ((value shl 3) and &h70000)
end function

dim as integer depths(0 to 3) = { 1, 8, 16, 32 }
require( fb.PaintPattern( 0, 0, 0, chr( 255 ) ) <> 0, "display required" )
for depth_index as integer = 0 to 3
	dim as integer depth = depths(depth_index)
	require( screenres( 48, 80, depth, 1, fb.GFX_NULL ) = 0, "screen mode" )
	dim as any ptr snapshot = imagecreate( 48, 80, 0, depth )
	require( snapshot <> 0, "snapshot allocation" )
	dim as ubyte ptr snapshot_pixels
	dim as long snapshot_pitch, snapshot_bpp
	imageinfo snapshot, , , snapshot_bpp, snapshot_pitch, snapshot_pixels
	dim as ulong foreground = 1, background = 0, border = 1
	if depth <> 1 then foreground = 5 : background = 3 : border = 7
	if depth > 8 then
		foreground = rgb( 240, 80, 40 )
		background = rgb( 24, 112, 200 )
		border = rgb( 208, 216, 48 )
	end if
	pset ( 0, 0 ), foreground
	dim as ulong foreground_pixel = point( 0, 0 )
	pset ( 0, 0 ), background
	dim as ulong background_pixel = point( 0, 0 )
	pset ( 0, 0 ), border
	dim as ulong border_pixel = point( 0, 0 )
	for rows as integer = 1 to 64
		dim as string pattern
		for row as integer = 0 to rows - 1
			pattern &= chr( (row * 37 + 128) and 255 )
		next
		line ( 0, 0 )-( 47, 79 ), background, bf
		line ( 3, 4 )-( 35, 73 ), border, b
		require( fb.PaintPattern( 0, 5, 6, pattern, foreground, background, border ) = 0, "packed fill" )
		get ( 0, 0 )-( 47, 79 ), snapshot
		for y as integer = 0 to 79
			for x as integer = 0 to 47
				dim as ulong expected = background_pixel
				if x >= 3 and x <= 35 and y >= 4 and y <= 73 then
					if x = 3 or x = 35 or y = 4 or y = 73 then
						expected = border_pixel
					elseif (asc( pattern, (y mod rows) + 1 ) shr (7 - (x and 7))) and 1 then
						expected = foreground_pixel
					end if
				end if
				require( snapshot_pixel( snapshot_pixels, snapshot_pitch, snapshot_bpp, x, y ) = expected, "packed pixel and border" )
			next
		next
	next
	'' Invalid rows must leave the existing framebuffer untouched.
	dim as ulong saved_pixel = point( 6, 6 )
	require( fb.PaintPattern( 0, 6, 6, "", foreground, background, border ) <> 0, "empty pattern" )
	require( fb.PaintPattern( 0, 6, 6, string( 65, 255 ), foreground, background, border ) <> 0, "oversized pattern" )
	require( culng( point( 6, 6 ) ) = saved_pixel, "invalid pattern leaves pixels" )

	'' Relative VIEW plus WINDOW and STEP still use physical phase.
	line ( 0, 0 )-( 47, 79 ), background, bf
	view ( 11, 19 )-( 26, 34 )
	window screen ( 0, 0 )-( 15, 15 )
	pset ( 1, 1 ), background
	require( fb.PaintPattern( 0, 1, 1, chr( 128, 64, 32 ), foreground, background, border, -1 ) = 0, "VIEW WINDOW STEP" )
	require( point( 2 ) = 2 and point( 3 ) = 2, "STEP pen" )
	window
	view
	get ( 0, 0 )-( 47, 79 ), snapshot
	for y as integer = 0 to 79
		for x as integer = 0 to 47
			dim as ulong expected = background_pixel
			if x >= 11 and x <= 26 and y >= 19 and y <= 34 then
				if ((128 shr (y mod 3)) shr (7 - (x and 7))) and 1 then expected = foreground_pixel
			end if
			require( snapshot_pixel( snapshot_pixels, snapshot_pitch, snapshot_bpp, x, y ) = expected, "viewport phase" )
		next
	next

	dim as any ptr target = imagecreate( 13, 17, background, depth )
	require( target <> 0, "image target" )
	require( fb.PaintPattern( target, 4, 5, chr( 128, 0, 255 ), foreground, background, border ) = 0, "image fill" )
	for y as integer = 0 to 16
		for x as integer = 0 to 12
			dim as ulong expected = background_pixel
			if (asc( chr( 128, 0, 255 ), y mod 3 + 1 ) shr (7 - (x and 7))) and 1 then expected = foreground_pixel
			require( culng( point( x, y, target ) ) = expected, "image phase" )
		next
	next
	imagedestroy target
	imagedestroy snapshot
#ifdef __FB_GFXLIB3__
	dim as any ptr surface = fb.Gfx3SurfaceCreate( 13, 17, depth, fb.GFX3_SURFACE_ALL, background )
	require( surface <> 0, "GPU surface" )
	require( fb.PaintPattern( surface, 2, 3, chr( 255 ), foreground, background, border ) = 0, "GPU surface fill" )
	require( culng( point( 4, 5, surface ) ) = foreground_pixel, "GPU surface pixel" )
	require( fb.Gfx3SurfaceDestroy( surface ) = 0, "destroy GPU surface" )
	surface = fb.Gfx3SurfaceCreate( 13, 17, depth, fb.GFX3_SURFACE_SAMPLED or fb.GFX3_SURFACE_TRANSFER_SOURCE, background )
	require( surface <> 0, "read-only surface" )
	dim as ubyte read_only_before(0 to 13 * 17 * 4 - 1)
	dim as ubyte read_only_after(0 to 13 * 17 * 4 - 1)
	require( fb.Gfx3SurfaceDownload( surface, 0, 0, 13, 17, 13 * snapshot_bpp, @read_only_before(0) ) = 0, "read initial surface bytes" )
	require( fb.PaintPattern( surface, 2, 3, chr( 255 ), foreground, background, border ) <> 0, "render capability required" )
	require( fb.Gfx3SurfaceDownload( surface, 0, 0, 13, 17, 13 * snapshot_bpp, @read_only_after(0) ) = 0, "read rejected surface bytes" )
	for byte_index as integer = 0 to 13 * 17 * snapshot_bpp - 1
		require( read_only_before(byte_index) = read_only_after(byte_index), "rejected surface unchanged" )
	next
	require( fb.Gfx3SurfaceDestroy( surface ) = 0, "destroy read-only surface" )
#endif
next
screen 0
print "paint-pattern: "; checks; " checks passed"

'' end of paint-pattern.bas
