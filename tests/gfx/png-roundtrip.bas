' TEST_MODE : COMPILE_AND_RUN_OK

''
'' FreeBASIC Graphics Library Tests
'' --------------------------------
''
'' File: png-roundtrip.bas
''
'' Purpose:
''
''     Verify dependency-free PNG support in BSAVE and BLOAD.
''
'' Responsibilities:
''
''     - round-trip RGBA image data through a PNG file
''     - verify case-insensitive .png format selection
''     - verify indexed pixels and palette values
''     - reject a PNG whose chunk checksum was damaged
''
'' Ownership:
''
''     - temporary PNG files belong to this test and are removed before and
''       after use
''
'' This file intentionally does NOT contain:
''
''     - visual or interactive checks
''     - benchmarks
''     - tests for formats other than PNG
''

#include once "fbgfx.bi"

function test_file_path( byref leaf_name as const string ) as string
	dim directory as string = environ( "TMPDIR" )

	if len( directory ) = 0 then directory = environ( "TEMP" )
	if len( directory ) = 0 then directory = environ( "TMP" )
	if len( directory ) = 0 then return leaf_name

	dim final_character as string = right( directory, 1 )
	if ( final_character <> "/" ) and _
	   ( final_character <> chr( 92 ) ) then
		directory += chr( 47 )
	end if
	return directory + leaf_name
end function

'' Keep transient images out of source trees watched by synchronization tools.
dim as string TRUECOLOR_FILE
dim as string INDEXED_FILE
dim as string RGB565_FILE
dim as string INDEXED_TRUECOLOR_FILE
dim as string DAMAGED_FILE

TRUECOLOR_FILE = test_file_path( "freebasic-png-roundtrip-rgba.PnG" )
INDEXED_FILE = test_file_path( "freebasic-png-roundtrip-indexed.png" )
RGB565_FILE = test_file_path( "freebasic-png-roundtrip-rgb565.png" )
INDEXED_TRUECOLOR_FILE = _
	test_file_path( "freebasic-png-roundtrip-indexed-truecolor.png" )
DAMAGED_FILE = test_file_path( "freebasic-png-roundtrip-damaged.png" )

sub remove_test_file( byref filename as const string )
	if len( dir( filename ) ) <> 0 then kill filename
end sub

function image_pixels( byval image as any ptr, byref image_width as long, _
	byref image_height as long, byref image_bpp as long, _
	byref image_pitch as long ) as ubyte ptr
	dim as any ptr pixels
	dim as long size

	if imageinfo( image, image_width, image_height, image_bpp, image_pitch, _
	    pixels, size ) <> 0 then
		return 0
	end if
	return cptr( ubyte ptr, pixels )
end function

remove_test_file TRUECOLOR_FILE
remove_test_file INDEXED_FILE
remove_test_file RGB565_FILE
remove_test_file INDEXED_TRUECOLOR_FILE
remove_test_file DAMAGED_FILE

if screenres( 32, 24, 32, , fb.GFX_NULL ) <> 0 then end 1

dim as any ptr source = imagecreate( 19, 13 )
dim as any ptr loaded = imagecreate( 19, 13 )
if ( source = 0 ) or ( loaded = 0 ) then end 2

dim as long source_width, source_height, source_bpp, source_pitch
dim as long loaded_width, loaded_height, loaded_bpp, loaded_pitch
dim as ubyte ptr source_pixels = image_pixels( source, source_width, _
	source_height, source_bpp, source_pitch )
dim as ubyte ptr loaded_pixels = image_pixels( loaded, loaded_width, _
	loaded_height, loaded_bpp, loaded_pitch )
if ( source_pixels = 0 ) or ( loaded_pixels = 0 ) then end 3
if ( source_bpp <> 4 ) or ( loaded_bpp <> 4 ) then end 4

for y as long = 0 to source_height - 1
	dim as ulong ptr row = cptr( ulong ptr, source_pixels + y * source_pitch )
	for x as long = 0 to source_width - 1
		row[x] = rgba( x * 11 + y * 3, x * 5 + y * 17, _
			x * 19 + y * 7, x * 13 + y * 23 )
	next
next

if bsave( TRUECOLOR_FILE, source ) <> 0 then end 5

dim as ubyte signature( 0 to 7 )
dim as long file_number = freefile()
if open( TRUECOLOR_FILE for binary access read as #file_number ) <> 0 then end 6
get #file_number, , signature()
close #file_number
dim as ubyte expected_signature( 0 to 7 ) = { 137, 80, 78, 71, 13, 10, 26, 10 }
for i as long = 0 to 7
	if signature(i) <> expected_signature(i) then end 7
next

if bload( TRUECOLOR_FILE, loaded ) <> 0 then end 8
for y as long = 0 to source_height - 1
	dim as ulong ptr source_row = _
		cptr( ulong ptr, source_pixels + y * source_pitch )
	dim as ulong ptr loaded_row = _
		cptr( ulong ptr, loaded_pixels + y * loaded_pitch )
	for x as long = 0 to source_width - 1
		if loaded_row[x] <> source_row[x] then end 9
	next
next

''
'' A changed IDAT payload must fail its CRC before the decoder consumes it.
''
file_number = freefile()
if open( TRUECOLOR_FILE for binary access read as #file_number ) <> 0 then end 10
dim as long file_size = lof( file_number )
dim as ubyte file_bytes( 0 to file_size - 1 )
get #file_number, , file_bytes()
close #file_number

dim as long idat_offset = -1
for i as long = 8 to file_size - 5
	if ( file_bytes(i) = asc( "I" ) ) and _
	   ( file_bytes(i + 1) = asc( "D" ) ) and _
	   ( file_bytes(i + 2) = asc( "A" ) ) and _
	   ( file_bytes(i + 3) = asc( "T" ) ) then
		idat_offset = i + 4
		exit for
	end if
next
if ( idat_offset < 0 ) or ( idat_offset >= file_size ) then end 11
file_bytes(idat_offset) xor= 1

file_number = freefile()
if open( DAMAGED_FILE for binary access write as #file_number ) <> 0 then end 12
put #file_number, , file_bytes()
close #file_number
if bload( DAMAGED_FILE, loaded ) = 0 then end 13

imagedestroy loaded
imagedestroy source

if screenres( 32, 16, 16, , fb.GFX_NULL ) <> 0 then end 14
source = imagecreate( 23, 9 )
loaded = imagecreate( 23, 9 )
if ( source = 0 ) or ( loaded = 0 ) then end 15
source_pixels = image_pixels( source, source_width, source_height, _
	source_bpp, source_pitch )
loaded_pixels = image_pixels( loaded, loaded_width, loaded_height, _
	loaded_bpp, loaded_pitch )
if ( source_bpp <> 2 ) or ( loaded_bpp <> 2 ) then end 16
for y as long = 0 to source_height - 1
	dim as ushort ptr row = _
		cptr( ushort ptr, source_pixels + y * source_pitch )
	for x as long = 0 to source_width - 1
		row[x] = ( ( x * 3 + y ) and 31 ) shl 11 _
			or ( ( x * 5 + y * 7 ) and 63 ) shl 5 _
			or ( ( x * 11 + y * 13 ) and 31 )
	next
next
if bsave( RGB565_FILE, source ) <> 0 then end 17
if bload( RGB565_FILE, loaded ) <> 0 then end 18
for y as long = 0 to source_height - 1
	dim as ushort ptr source_row = _
		cptr( ushort ptr, source_pixels + y * source_pitch )
	dim as ushort ptr loaded_row = _
		cptr( ushort ptr, loaded_pixels + y * loaded_pitch )
	for x as long = 0 to source_width - 1
		if loaded_row[x] <> source_row[x] then end 19
	next
next
imagedestroy loaded
imagedestroy source

if screenres( 16, 16, 8, , fb.GFX_NULL ) <> 0 then end 20
palette 7, 40, 80, 120

source = imagecreate( 9, 7, 7 )
loaded = imagecreate( 9, 7, 0 )
if ( source = 0 ) or ( loaded = 0 ) then end 21
dim as ulong loaded_palette( 0 to 255 )
if bsave( INDEXED_FILE, source ) <> 0 then end 22
if bload( INDEXED_FILE, loaded, @loaded_palette(0) ) <> 0 then end 23

source_pixels = image_pixels( source, source_width, source_height, _
	source_bpp, source_pitch )
loaded_pixels = image_pixels( loaded, loaded_width, loaded_height, _
	loaded_bpp, loaded_pitch )
if ( source_pixels = 0 ) or ( loaded_pixels = 0 ) then end 24
for y as long = 0 to source_height - 1
	for x as long = 0 to source_width - 1
		if loaded_pixels[y * loaded_pitch + x] <> 7 then end 25
	next
next

dim as ulong expected_palette_value = _
	( 40 shr 2 ) or ( ( 80 shr 2 ) shl 8 ) or ( ( 120 shr 2 ) shl 16 )
if loaded_palette(7) <> expected_palette_value then end 26

if bsave( INDEXED_TRUECOLOR_FILE, source, 0, @loaded_palette(0), 24 ) <> 0 then
	end 27
end if

imagedestroy loaded
imagedestroy source

if screenres( 16, 16, 32, , fb.GFX_NULL ) <> 0 then end 28
loaded = imagecreate( 9, 7 )
if loaded = 0 then end 29
if bload( INDEXED_TRUECOLOR_FILE, loaded ) <> 0 then end 30
loaded_pixels = image_pixels( loaded, loaded_width, loaded_height, _
	loaded_bpp, loaded_pitch )
if loaded_pixels = 0 then end 31
for y as long = 0 to loaded_height - 1
	dim as ulong ptr loaded_row = _
		cptr( ulong ptr, loaded_pixels + y * loaded_pitch )
	for x as long = 0 to loaded_width - 1
		if loaded_row[x] <> rgb( 40, 80, 120 ) then end 32
	next
next
imagedestroy loaded

remove_test_file TRUECOLOR_FILE
remove_test_file INDEXED_FILE
remove_test_file RGB565_FILE
remove_test_file INDEXED_TRUECOLOR_FILE
remove_test_file DAMAGED_FILE

end 0

'' end of png-roundtrip.bas
