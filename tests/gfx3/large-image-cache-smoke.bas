''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: large-image-cache-smoke.bas
''
'' Purpose:
''
''     Verify and measure exact cache validation for an FB.IMAGE larger than
''     the original one-megabyte snapshot cutoff.
''
'' Responsibilities:
''
''     - expose a writable pointer through IMAGEINFO
''     - repeat a stable large-image PUT workload
''     - prove that a later direct pixel write refreshes the GPU copy
''
'' This file intentionally does NOT contain:
''
''     - an immutable-image shortcut
''     - a vendor-specific performance threshold
''     - scaled, rotated, or destination-reading PUT modes
''

#include once "fbgfx.bi"

#ifdef GFX3_OPENGL_TEST
	const backend_flags = fb.GFX_OPENGL
#elseif defined( GFX3_VULKAN_TEST )
	const backend_flags = fb.GFX_VULKAN
#else
	const backend_flags = 0
#endif

#ifdef __FB_ANDROID__
	const transfer_count = 32
#else
	const transfer_count = 256
#endif

const image_width = 1024
const image_height = 768
const expected_initial = rgba( 16, 42, 84, 255 )
const expected_changed = rgba( 210, 120, 35, 255 )

if screenres( 128, 96, 32, 1, backend_flags ) <> 0 then end 1

dim as any ptr source_image = imagecreate( image_width, image_height, _
	expected_initial, 32 )
if source_image = 0 then end 2

dim as long reported_width, reported_height, reported_bpp
dim as long reported_pitch, reported_size
dim as any ptr pixels

if imageinfo( source_image, reported_width, reported_height, reported_bpp, _
	reported_pitch, pixels, reported_size ) <> 0 then end 3
if reported_width <> image_width orelse reported_height <> image_height _
	orelse reported_bpp <> 4 orelse pixels = 0 then end 4

'' Complete the initial allocation and upload before measuring stable reuse.
put ( 0, 0 ), source_image, pset
if cuint( point( 0, 0 ) ) <> expected_initial then end 5

dim as double started = timer
for transfer_index as integer = 1 to transfer_count
	put ( 0, 0 ), source_image, pset
next
dim as ulong stable_pixel = point( 0, 0 )
dim as double stable_seconds = timer - started
if stable_pixel <> expected_initial then end 6

'' IMAGEINFO made this pointer writable. The next PUT must detect the direct
'' edit and upload it even though all preceding comparisons were stable.
cast( ulong ptr, pixels )[0] = expected_changed
put ( 0, 0 ), source_image, pset
if cuint( point( 0, 0 ) ) <> expected_changed then end 7

imagedestroy source_image
screen 0

print "large_image_cache_seconds="; stable_seconds
print "large_image_cache_transfers="; transfer_count
print "large_image_cache_pixel="; stable_pixel
end 0

'' end of large-image-cache-smoke.bas
