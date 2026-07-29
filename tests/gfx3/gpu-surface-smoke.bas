''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: gpu-surface-smoke.bas
''
'' Purpose:
''
''     Verify opaque GPU-only surfaces through their extension API and as
''     ordinary FreeBASIC graphics primitive targets.
''
'' Responsibilities:
''
''     - create, clear, draw, read, upload, download, map, and blit GPU surfaces
''     - verify PSET, POINT, LINE/BF, and CIRCLE target routing
''     - present a non-page surface without constructing a CPU FB.IMAGE
''     - verify usage flags reject unavailable render, sample, and transfer paths
''     - verify PAINT and PUT CUSTOM cannot write a transfer-only surface
''     - verify explicit destruction and mode-owned cleanup
''
'' This file intentionally does NOT contain:
''
''     - persistent GPU pointers or direct backend handles
''     - arc, PAINT, text, GET, or PUT target coverage
''     - performance timing
''

#define __FB_GFXLIB3__
#include once "fbgfx3.bi"

function capability_blender _
	( _
		byval source_pixel as ulong, _
		byval destination_pixel as ulong, _
		byval parameter as any ptr _
	) as ulong

	return source_pixel xor destination_pixel
end function

#ifdef GFX3_VULKAN_TEST
	const backend_flags = fb.GFX_VULKAN
#elseif defined(GFX3_OPENGL_TEST)
	const backend_flags = 0
#else
	const backend_flags = fb.GFX_NULL
#endif

if screenres( 64, 64, 32, 1, backend_flags ) <> 0 then end 1

dim as any ptr source_surface = fb.Gfx3SurfaceCreate( 32, 32, 32, , _
	rgb( 0, 0, 0 ) )
dim as any ptr destination_surface = fb.Gfx3SurfaceCreate( 32, 32, 32 )
if source_surface = 0 orelse destination_surface = 0 then end 2
dim as long surface_width, surface_height, surface_depth
dim as ulong surface_usage
if fb.Gfx3SurfaceInfo( source_surface, @surface_width, @surface_height, _
	@surface_depth, @surface_usage ) <> 0 then end 16
if surface_width <> 32 orelse surface_height <> 32 orelse _
	surface_depth <> 32 orelse surface_usage <> fb.GFX3_SURFACE_ALL then end 17

pset source_surface, (2, 2), rgb( 255, 0, 0 )
line source_surface, (4, 4)-(12, 12), rgb( 0, 255, 0 ), bf
circle source_surface, (20, 20), 4, rgb( 0, 0, 255 )
circle source_surface, (26, 8), 3, rgb( 255, 255, 0 ), 0, 1.5707963
if point( 2, 2, source_surface ) <> rgb( 255, 0, 0 ) then end 3
if point( 8, 8, source_surface ) <> rgb( 0, 255, 0 ) then end 4
if point( 24, 20, source_surface ) <> rgb( 0, 0, 255 ) then end 5
if point( 29, 8, source_surface ) <> rgb( 255, 255, 0 ) then end 18

line source_surface, (1, 16)-(10, 25), rgb( 0, 0, 255 ), b
paint source_surface, (4, 20), rgb( 255, 0, 255 ), rgb( 0, 0, 255 )
if point( 4, 20, source_surface ) <> rgb( 255, 0, 255 ) then end 19
draw source_surface, "BM 14,14 C" & rgb( 255, 255, 255 ) & " R8"
if point( 18, 14, source_surface ) <> rgb( 255, 255, 255 ) then end 20
draw string source_surface, (1, 26), "A", rgb( 100, 110, 120 )
dim as integer glyph_pixels
for glyph_y as integer = 26 to 31
	for glyph_x as integer = 1 to 8
		if point( glyph_x, glyph_y, source_surface ) = rgb( 100, 110, 120 ) _
			then glyph_pixels += 1
	next
next
if glyph_pixels = 0 then end 21

dim as any ptr cpu_image = imagecreate( 2, 2, rgb( 33, 44, 55 ), 32 )
dim as any ptr captured_image = imagecreate( 2, 2, 0, 32 )
if cpu_image = 0 orelse captured_image = 0 then end 22
put source_surface, (28, 0), cpu_image, pset
if point( 28, 0, source_surface ) <> rgb( 33, 44, 55 ) then end 23
get source_surface, (28, 0)-(29, 1), captured_image
if point( 0, 0, captured_image ) <> rgb( 33, 44, 55 ) then end 24
imagedestroy cpu_image
imagedestroy captured_image

dim as ulong upload_pixels( 0 to 3 ) = { _
	rgb( 1, 2, 3 ), rgb( 4, 5, 6 ), _
	rgb( 7, 8, 9 ), rgb( 10, 11, 12 ) _
}
dim as ulong download_pixels( 0 to 3 )
if fb.Gfx3SurfaceUpload( source_surface, 14, 2, 2, 2, 8, _
	@upload_pixels( 0 ) ) <> 0 then end 6
if fb.Gfx3SurfaceDownload( source_surface, 14, 2, 2, 2, 8, _
	@download_pixels( 0 ) ) <> 0 then end 7
for index as integer = 0 to 3
	if download_pixels( index ) <> upload_pixels( index ) then end 8
next

''
'' Rectangle maps transfer only their stated rectangle. Their pitch is based on
'' the rectangle width, and writes commit at that rectangle's target origin.
''
dim as any ptr rectangle_pixels
dim as long rectangle_pitch
if fb.Gfx3SurfaceMapRect( source_surface, 14, 2, 2, 2, fb.GFX3_MAP_READ, _
	rectangle_pixels, rectangle_pitch ) <> 0 then end 58
if rectangle_pixels = 0 orelse rectangle_pitch <> 2 * sizeof( ulong ) then _
	end 59
if cptr( ulong ptr, rectangle_pixels )[0] <> upload_pixels( 0 ) orelse _
	cptr( ulong ptr, rectangle_pixels )[3] <> upload_pixels( 3 ) then end 60
if fb.Gfx3SurfaceUnmap( source_surface ) <> 0 then end 61
if fb.Gfx3SurfaceMapRect( source_surface, 13, 1, 1, 1, fb.GFX3_MAP_WRITE, _
	rectangle_pixels, rectangle_pitch ) <> 0 then end 62
cptr( ulong ptr, rectangle_pixels )[0] = rgb( 111, 112, 113 )
if fb.Gfx3SurfaceUnmap( source_surface ) <> 0 then end 63
if point( 13, 1, source_surface ) <> rgb( 111, 112, 113 ) then end 64
if fb.Gfx3SurfaceMapRect( source_surface, -1, 0, 1, 1, fb.GFX3_MAP_READ, _
	rectangle_pixels, rectangle_pitch ) = 0 then end 65

''
'' A scoped map is a CPU staging copy, not a GPU pointer.  It must preserve the
'' complete surface during a partial write, reject ordinary surface work while
'' it owns the staging image, and commit the replacement pixels at unmap.
''
dim as any ptr mapped_pixels
dim as long mapped_pitch
if fb.Gfx3SurfaceMap( source_surface, fb.GFX3_MAP_READ, mapped_pixels, _
	mapped_pitch ) <> 0 then end 44
if mapped_pixels = 0 orelse mapped_pitch <> 32 * sizeof( ulong ) then end 45
if cptr( ulong ptr, mapped_pixels )[14 + 2 * 32] <> upload_pixels( 0 ) then _
	end 46
if fb.Gfx3SurfaceUnmap( source_surface ) <> 0 then end 47
if fb.Gfx3SurfaceMap( source_surface, fb.GFX3_MAP_WRITE, mapped_pixels, _
	mapped_pitch ) <> 0 then end 48
if fb.Gfx3SurfaceDownload( source_surface, 0, 0, 1, 1, 4, _
	@download_pixels( 0 ) ) = 0 then end 49
if fb.Gfx3SurfaceDestroy( source_surface ) = 0 then end 50
cptr( ulong ptr, mapped_pixels )[15 + 2 * 32] = rgb( 101, 102, 103 )
if fb.Gfx3SurfaceUnmap( source_surface ) <> 0 then end 51
if point( 15, 2, source_surface ) <> rgb( 101, 102, 103 ) then end 52

''
'' A blit that reads and writes one GPU surface must snapshot the source
'' rectangle. This specifically protects GLES from texture-feedback undefined
'' behaviour while keeping the direct-source fast path for separate surfaces.
''
dim as ulong self_blit_pixels( 0 to 3 ) = { _
	rgb( 1, 0, 0 ), rgb( 2, 0, 0 ), _
	rgb( 3, 0, 0 ), rgb( 4, 0, 0 ) _
}
if fb.Gfx3SurfaceUpload( source_surface, 0, 30, 4, 1, 16, _
	@self_blit_pixels( 0 ) ) <> 0 then end 66
if fb.Gfx3SurfaceBlit( source_surface, source_surface, 0, 30, 3, 1, _
	1, 30, fb.GFX3_PUT_PSET ) <> 0 then end 67
if fb.Gfx3SurfaceDownload( source_surface, 0, 30, 4, 1, 16, _
	@download_pixels( 0 ) ) <> 0 then end 68
if download_pixels( 0 ) <> self_blit_pixels( 0 ) orelse _
	download_pixels( 1 ) <> self_blit_pixels( 0 ) orelse _
	download_pixels( 2 ) <> self_blit_pixels( 1 ) orelse _
	download_pixels( 3 ) <> self_blit_pixels( 2 ) then end 69

if fb.Gfx3SurfaceClear( destination_surface, rgb( 20, 30, 40 ) ) <> 0 _
	then end 9
if fb.Gfx3SurfaceBlit( destination_surface, source_surface, 0, 0, 32, 32, _
	0, 0, fb.GFX3_PUT_PSET ) <> 0 then end 10
if point( 8, 8, destination_surface ) <> rgb( 0, 255, 0 ) then end 11

cls
if fb.Gfx3SurfaceBlit( 0, destination_surface, 0, 0, 32, 32, _
	5, 7, fb.GFX3_PUT_PSET ) <> 0 then end 70
if point( 13, 15 ) <> rgb( 0, 255, 0 ) then end 71

if fb.Gfx3SurfacePresent( destination_surface, true ) <> 0 then end 12

''
'' Usage flags are capabilities, not allocation hints.  The all-capabilities
'' surfaces above exercise the normal fast path.  These narrow surfaces prove
'' each explicit extension call accepts only the resource role it needs.
''
dim as any ptr readable_surface = fb.Gfx3SurfaceCreate( 4, 4, 32, _
	fb.GFX3_SURFACE_TRANSFER_SOURCE, rgb( 21, 22, 23 ) )
dim as any ptr writable_surface = fb.Gfx3SurfaceCreate( 4, 4, 32, _
	fb.GFX3_SURFACE_TRANSFER_DESTINATION )
dim as any ptr sampled_surface = fb.Gfx3SurfaceCreate( 4, 4, 32, _
	fb.GFX3_SURFACE_SAMPLED, rgb( 31, 32, 33 ) )
dim as any ptr render_surface = fb.Gfx3SurfaceCreate( 4, 4, 32, _
	fb.GFX3_SURFACE_RENDER_TARGET )
dim as any ptr transfer_surface = fb.Gfx3SurfaceCreate( 4, 4, 32, _
	fb.GFX3_SURFACE_TRANSFER_SOURCE or fb.GFX3_SURFACE_TRANSFER_DESTINATION )
dim as any ptr custom_source = imagecreate( 1, 1, rgb( 91, 92, 93 ), 32 )
if readable_surface = 0 orelse writable_surface = 0 orelse _
	sampled_surface = 0 orelse render_surface = 0 orelse _
	transfer_surface = 0 orelse custom_source = 0 then end 25
dim as ulong capability_pixel = rgb( 41, 42, 43 )
if fb.Gfx3SurfaceDownload( readable_surface, 0, 0, 1, 1, 4, _
	@download_pixels( 0 ) ) <> 0 then end 26
if fb.Gfx3SurfaceMap( readable_surface, fb.GFX3_MAP_READ, mapped_pixels, _
	mapped_pitch ) <> 0 then end 53
if fb.Gfx3SurfaceUnmap( readable_surface ) <> 0 then end 54
if fb.Gfx3SurfaceMap( readable_surface, fb.GFX3_MAP_WRITE, mapped_pixels, _
	mapped_pitch ) = 0 then end 55
if fb.Gfx3SurfaceMap( writable_surface, fb.GFX3_MAP_READ, mapped_pixels, _
	mapped_pitch ) = 0 then end 56
if fb.Gfx3SurfaceUpload( readable_surface, 0, 0, 1, 1, 4, _
	@capability_pixel ) = 0 then end 27
if fb.Gfx3SurfaceUpload( writable_surface, 0, 0, 1, 1, 4, _
	@capability_pixel ) <> 0 then end 28
if fb.Gfx3SurfaceDownload( writable_surface, 0, 0, 1, 1, 4, _
	@download_pixels( 0 ) ) = 0 then end 29
if fb.Gfx3SurfaceClear( render_surface, rgb( 51, 52, 53 ) ) <> 0 then end 30
if fb.Gfx3SurfaceClear( sampled_surface, rgb( 61, 62, 63 ) ) = 0 then end 31
if fb.Gfx3SurfaceBlit( render_surface, sampled_surface, 0, 0, 1, 1, _
	0, 0, fb.GFX3_PUT_PSET ) <> 0 then end 32
if fb.Gfx3SurfaceBlit( writable_surface, sampled_surface, 0, 0, 1, 1, _
	0, 0, fb.GFX3_PUT_PSET ) = 0 then end 33
if fb.Gfx3SurfaceBlit( render_surface, readable_surface, 0, 0, 1, 1, _
	0, 0, fb.GFX3_PUT_PSET ) = 0 then end 34
if fb.Gfx3SurfacePresent( render_surface, true ) = 0 then end 35

if fb.Gfx3SurfaceDestroy( readable_surface ) <> 0 then end 36
if fb.Gfx3SurfaceDestroy( writable_surface ) <> 0 then end 37
if fb.Gfx3SurfaceDestroy( sampled_surface ) <> 0 then end 38
if fb.Gfx3SurfaceDestroy( render_surface ) <> 0 then end 39
if fb.Gfx3SurfaceUpload( transfer_surface, 0, 0, 1, 1, 4, _
	@capability_pixel ) <> 0 then end 40
paint transfer_surface, ( 0, 0 ), rgb( 71, 72, 73 ), rgb( 81, 82, 83 )
put transfer_surface, ( 0, 0 ), custom_source, custom, @capability_blender, 0
if fb.Gfx3SurfaceDownload( transfer_surface, 0, 0, 1, 1, 4, _
	@download_pixels( 0 ) ) <> 0 then end 41
if download_pixels( 0 ) <> capability_pixel then end 42
imagedestroy custom_source
if fb.Gfx3SurfaceDestroy( transfer_surface ) <> 0 then end 43

if fb.Gfx3SurfaceDestroy( source_surface ) <> 0 then end 13
if fb.Gfx3SurfaceDestroy( destination_surface ) <> 0 then end 14

'' This live surface is intentionally left to SCREEN 0's ordered cleanup.
dim as any ptr mode_owned_surface = fb.Gfx3SurfaceCreate( 8, 8, 32 )
if mode_owned_surface = 0 then end 15
if fb.Gfx3SurfaceMap( mode_owned_surface, fb.GFX3_MAP_READ, mapped_pixels, _
	mapped_pitch ) <> 0 then end 57
screen 0

end 0

'' end of gpu-surface-smoke.bas
