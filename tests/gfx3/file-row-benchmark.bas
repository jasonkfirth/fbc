''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: file-row-benchmark.bas
''
'' Purpose:
''
''     Compare the compatibility-only image file and row-conversion commands
''     provided by gfxlib2 and gfxlib3.
''
'' Responsibilities:
''
''     - time raw-memory BSAVE and BLOAD independently
''     - time IMAGECONVERTROW without file system work in the timed section
''     - verify that the restored data and converted row are usable
''     - remove the single temporary file created by this fixture
''
'' This file intentionally does NOT contain:
''
''     - GPU-screen BMP encoding or presentation timing
''     - external image decoding coverage
''     - vendor-specific performance thresholds
''

#include once "fbgfx.bi"

#ifdef GFX3_OPENGL_TEST
	const backend_flags = fb.GFX_OPENGL
#elseif defined( GFX3_VULKAN_TEST )
	const backend_flags = fb.GFX_VULKAN
#else
	const backend_flags = 0
#endif

const file_name = "gfx3-file-row-benchmark.bin"
const byte_count = 262144
const row_pixels = 4096
const file_iterations = 32
const row_iterations = 4096

dim source_bytes( 0 to byte_count - 1 ) as ubyte
dim restored_bytes( 0 to byte_count - 1 ) as ubyte
dim source_row( 0 to row_pixels - 1 ) as ulong
dim destination_row( 0 to row_pixels - 1 ) as ulong
dim as double bsave_started, bload_started, row_started
dim as double bsave_seconds, bload_seconds, row_seconds
dim as uinteger expected_checksum, restored_checksum, row_checksum

'' gfxlib2's raw-memory BSAVE prepares its active graphics target before it
'' writes the raw block. Open a minimal mode for both libraries so this is a
'' valid comparison of the documented command rather than a no-screen quirk.
if screenres( 16, 16, 32, 1, backend_flags ) <> 0 then end 1

for index as integer = 0 to byte_count - 1
	source_bytes( index ) = ( index * 37 + 19 ) and &hff
next
for index as integer = 0 to row_pixels - 1
	source_row( index ) = rgba( ( index * 3 ) and &hff, _
		( index * 5 ) and &hff, ( index * 7 ) and &hff, _
		( index * 11 ) and &hff )
next

'' Prepare the BLOAD input outside each timed loop. This also proves the
'' benchmark starts from a valid raw FreeBASIC graphics block.
if bsave( file_name, @source_bytes( 0 ), byte_count ) <> 0 then end 2

bsave_started = timer
for iteration as integer = 1 to file_iterations
	if bsave( file_name, @source_bytes( 0 ), byte_count ) <> 0 then end 3
next
bsave_seconds = timer - bsave_started

bload_started = timer
for iteration as integer = 1 to file_iterations
	if bload( file_name, @restored_bytes( 0 ) ) <> 0 then end 4
next
bload_seconds = timer - bload_started

for index as integer = 0 to byte_count - 1
	expected_checksum += source_bytes( index )
	restored_checksum += restored_bytes( index )
next
if restored_checksum <> expected_checksum then end 5

row_started = timer
for iteration as integer = 1 to row_iterations
	imageconvertrow @source_row( 0 ), 32, @destination_row( 0 ), 32, _
		row_pixels, 0
next
row_seconds = timer - row_started

for index as integer = 0 to row_pixels - 1
	if destination_row( index ) <> source_row( index ) then end 6
	row_checksum += destination_row( index )
next

kill file_name
screen 0
print "file_row_bsave_seconds="; bsave_seconds
print "file_row_bload_seconds="; bload_seconds
print "file_row_convert_seconds="; row_seconds
print "file_row_checksum="; row_checksum
end 0

'' end of file-row-benchmark.bas
