''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: gpu-surface-benchmark.bas
''
'' Purpose:
''
''     Measure each public gfxlib3 GPU-surface transfer operation which has no
''     direct gfxlib2 API equivalent.
''
'' Responsibilities:
''
''     - time create/destroy, clear, upload, download, map, blit, and present
''     - force completed transfer results before every reported measurement
''     - run unchanged on forced OpenGL, forced Vulkan, and Android GLES
''
'' This file intentionally does NOT contain:
''
''     - a gfxlib2 comparison, because GPU surfaces are a gfxlib3 extension
''     - primitive raster timing, which belongs in primitive-benchmark.bas
''     - a vendor-specific timing threshold
''

#define __FB_GFXLIB3__
#include once "fbgfx3.bi"

#ifdef GFX3_OPENGL_TEST
	const backend_flags = fb.GFX_OPENGL
#elseif defined( GFX3_VULKAN_TEST )
	const backend_flags = fb.GFX_VULKAN
#else
	const backend_flags = 0
#endif

#ifdef __FB_ANDROID__
	const create_count = 16
	const operation_count = 32
#else
	const create_count = 64
	const operation_count = 256
#endif

const surface_width = 64
const surface_height = 64
const surface_pitch = surface_width * sizeof( ulong )

dim as any ptr source_surface
dim as any ptr destination_surface
dim as any ptr temporary_surface
dim as any ptr mapped_pixels
dim as long mapped_pitch
dim as ulong upload_pixels( 0 to surface_width * surface_height - 1 )
dim as ulong download_pixels( 0 to surface_width * surface_height - 1 )
dim as integer ordered_pixel
dim as double started
dim as double create_seconds, clear_seconds, upload_seconds
dim as double download_seconds, map_seconds, blit_seconds, present_seconds

if screenres( 128, 96, 32, 1, backend_flags ) <> 0 then end 1

for index as integer = 0 to ubound( upload_pixels )
	upload_pixels( index ) = rgba( index and 255, ( index * 3 ) and 255, _
		( index * 7 ) and 255, 255 )
next

started = timer
for index as integer = 1 to create_count
	temporary_surface = fb.Gfx3SurfaceCreate( surface_width, surface_height, _
		32, fb.GFX3_SURFACE_ALL, rgb( index and 255, 0, 0 ) )
	if temporary_surface = 0 then end 2
	if fb.Gfx3SurfaceDestroy( temporary_surface ) <> 0 then end 3
next
create_seconds = timer - started

source_surface = fb.Gfx3SurfaceCreate( surface_width, surface_height, 32, _
	fb.GFX3_SURFACE_ALL )
destination_surface = fb.Gfx3SurfaceCreate( surface_width, surface_height, _
	32, fb.GFX3_SURFACE_ALL )
if source_surface = 0 orelse destination_surface = 0 then end 4

started = timer
for index as integer = 1 to operation_count
	if fb.Gfx3SurfaceClear( source_surface, rgba( index and 255, _
		( index * 5 ) and 255, ( index * 11 ) and 255, 255 ) ) <> 0 then end 5
next
ordered_pixel = point( 0, 0, source_surface )
clear_seconds = timer - started

started = timer
for index as integer = 1 to operation_count
	if fb.Gfx3SurfaceUpload( source_surface, 0, 0, surface_width, _
		surface_height, surface_pitch, @upload_pixels( 0 ) ) <> 0 then end 6
next
ordered_pixel = point( 0, 0, source_surface )
upload_seconds = timer - started

started = timer
for index as integer = 1 to operation_count
	if fb.Gfx3SurfaceDownload( source_surface, 0, 0, surface_width, _
		surface_height, surface_pitch, @download_pixels( 0 ) ) <> 0 then end 7
next
download_seconds = timer - started
if download_pixels( 7 ) <> upload_pixels( 7 ) then end 8

started = timer
for index as integer = 1 to operation_count
	if fb.Gfx3SurfaceMapRect( source_surface, 0, 0, surface_width, _
		surface_height, fb.GFX3_MAP_READ, mapped_pixels, mapped_pitch ) <> 0 _
		then end 9
	if mapped_pixels = 0 orelse mapped_pitch <> surface_pitch then end 10
	if cptr( ulong ptr, mapped_pixels )[7] <> upload_pixels( 7 ) then end 11
	if fb.Gfx3SurfaceUnmap( source_surface ) <> 0 then end 12
next
map_seconds = timer - started

started = timer
for index as integer = 1 to operation_count
	if fb.Gfx3SurfaceBlit( destination_surface, source_surface, 0, 0, _
		surface_width, surface_height, 0, 0, fb.GFX3_PUT_PSET ) <> 0 then end 13
next
ordered_pixel = point( 7, 0, destination_surface )
blit_seconds = timer - started
if ordered_pixel <> upload_pixels( 7 ) then end 14

started = timer
for index as integer = 1 to operation_count
	if fb.Gfx3SurfacePresent( destination_surface, true ) <> 0 then end 15
next
present_seconds = timer - started

if fb.Gfx3SurfaceDestroy( destination_surface ) <> 0 then end 16
if fb.Gfx3SurfaceDestroy( source_surface ) <> 0 then end 17
screen 0

print "gpu_surface_benchmark_create_seconds="; create_seconds
print "gpu_surface_benchmark_clear_seconds="; clear_seconds
print "gpu_surface_benchmark_upload_seconds="; upload_seconds
print "gpu_surface_benchmark_download_seconds="; download_seconds
print "gpu_surface_benchmark_map_seconds="; map_seconds
print "gpu_surface_benchmark_blit_seconds="; blit_seconds
print "gpu_surface_benchmark_present_seconds="; present_seconds
print "gpu_surface_benchmark_pixel="; ordered_pixel
end 0

'' end of gpu-surface-benchmark.bas
