''
'' Project: FreeBASIC gfxlib3
'' --------------------------
''
'' File: fbgfx3.bi
''
'' Purpose:
''
''     Expose opt-in GPU-resident surface operations beyond the gfxlib2 ABI.
''
'' Responsibilities:
''
''     - create, load, and destroy opaque renderer-owned surfaces
''     - explicitly upload, download, map, clear, blit, transform, and present
''       surfaces
''     - define usage flags without exposing backend GPU handles
''
'' This file intentionally does NOT contain:
''
''     - Vulkan, OpenGL, WGL, GLX, or native window declarations
''     - writable pixel members or a fake FB.IMAGE layout
''     - a stable shader-extension ABI
''

#ifndef __FB_GFX3_BI__
#define __FB_GFX3_BI__

#ifndef __FB_GFXLIB3__
	#define __FB_GFXLIB3__
#endif
#include once "fbgfx.bi"

#if (defined( __FB_WIN32__ ) or defined( __FB_XBOX__ )) and _
	not defined( __FB_64BIT__ )
	#define GFX3CALL stdcall
#else
	#define GFX3CALL cdecl
#endif

namespace fb

	''
	'' A surface's usage bits are an enforced capability contract.  A surface
	'' may be drawn into only with RENDER_TARGET, used as a GPU blit or present
	'' source only with SAMPLED, downloaded only with TRANSFER_SOURCE, and
	'' uploaded only with TRANSFER_DESTINATION.  The default requests all four
	'' capabilities so ordinary GPU-surface programs need no special handling.
	const GFX3_SURFACE_RENDER_TARGET = &h00000001u
	const GFX3_SURFACE_SAMPLED = &h00000002u
	const GFX3_SURFACE_TRANSFER_SOURCE = &h00000004u
	const GFX3_SURFACE_TRANSFER_DESTINATION = &h00000008u
	const GFX3_SURFACE_ALL = GFX3_SURFACE_RENDER_TARGET or _
		GFX3_SURFACE_SAMPLED or GFX3_SURFACE_TRANSFER_SOURCE or _
		GFX3_SURFACE_TRANSFER_DESTINATION
	'' A loaded asset stays GPU-resident and cannot be used as a CPU readback
	'' source unless the caller explicitly adds TRANSFER_SOURCE.
	const GFX3_SURFACE_ASSET = GFX3_SURFACE_SAMPLED or _
		GFX3_SURFACE_TRANSFER_DESTINATION
	''
	'' A map is a temporary, CPU-owned staging image. It is not mapped video
	'' memory: mapping downloads the complete surface and unmapping a writable
	'' map uploads it again. This keeps the opaque surface GPU-resident between
	'' maps while giving code that truly needs CPU pixels a safe, scoped bridge.
	'' The pointer must not be retained after Gfx3SurfaceUnmap or SCREEN 0.
	const GFX3_MAP_READ = &h00000001u
	const GFX3_MAP_WRITE = &h00000002u
	const as long _
		GFX3_PUT_TRANS = 0, _
		GFX3_PUT_PSET = 1, _
		GFX3_PUT_PRESET = 2, _
		GFX3_PUT_AND = 3, _
		GFX3_PUT_OR = 4, _
		GFX3_PUT_XOR = 5, _
		GFX3_PUT_ALPHA = 6, _
		GFX3_PUT_ADD = 7, _
		GFX3_PUT_BLEND = 9
	const as long _
		GFX3_FILTER_NEAREST = 0, _
		GFX3_FILTER_LINEAR = 1
	const as long _
		GFX3_WRAP_CLAMP = 0, _
		GFX3_WRAP_REPEAT = 1

	''
	'' gfxlib3 owns a live GL context on its render thread.  Callbacks submitted
	'' through this type run there in sequence with queued graphics work; only
	'' inside a callback does SCREENGLPROC return an OpenGL/GLES entry point.
	'' A callback must not call the normal SCREEN, drawing, or surface APIs,
	'' because those APIs wait for this same render thread.
	type Gfx3RenderCallback as sub cdecl ( byval user_data as any ptr )

	''
	'' One point record for Gfx3DrawPoints. Alpha is clamped to 0..255; values
	'' below 255 request destination blending in the GPU shader. Grouping a
	'' generated mask or particle field into one call avoids POINT readbacks and
	'' one public PSET call per pixel.
	type Gfx3Point
		x as long
		y as long
		color as ulong
		alpha as ulong
	end type

	extern "C"
	declare function Gfx3SurfaceCreate GFX3CALL alias "fb_Gfx3SurfaceCreate" _
		( byval width as long, byval height as long, byval depth as long = 0, _
		  byval usage as ulong = GFX3_SURFACE_ALL, _
		  byval clear_color as ulong = 0 ) as any ptr
	'' Decode a BLOAD-compatible bitmap into temporary staging memory, upload it
	'' once, and release the CPU copy before returning the opaque surface.
	declare function Gfx3SurfaceLoad GFX3CALL alias "fb_Gfx3SurfaceLoad" _
		( byref filename as const string, byval depth as long = 0, _
		  byval usage as ulong = GFX3_SURFACE_ASSET ) as any ptr
	declare function Gfx3SurfaceDestroy GFX3CALL alias "fb_Gfx3SurfaceDestroy" _
		( byval surface as any ptr ) as long
	declare function Gfx3SurfaceInfo GFX3CALL alias "fb_Gfx3SurfaceInfo" _
		( byval surface as any ptr, byval width as long ptr = 0, _
		  byval height as long ptr = 0, byval depth as long ptr = 0, _
		  byval usage as ulong ptr = 0 ) as long
	declare function Gfx3SurfaceUpload GFX3CALL alias "fb_Gfx3SurfaceUpload" _
		( byval surface as any ptr, byval x as long, byval y as long, _
		  byval width as long, byval height as long, byval pitch as long, _
		  byval pixels as const any ptr ) as long
	declare function Gfx3SurfaceDownload GFX3CALL alias "fb_Gfx3SurfaceDownload" _
		( byval surface as any ptr, byval x as long, byval y as long, _
		  byval width as long, byval height as long, byval pitch as long, _
		  byval pixels as any ptr ) as long
	declare function Gfx3SurfaceMap GFX3CALL alias "fb_Gfx3SurfaceMap" _
		( byval surface as any ptr, byval access as ulong, _
		  byref pixels as any ptr, byref pitch as long ) as long
	declare function Gfx3SurfaceMapRect GFX3CALL alias "fb_Gfx3SurfaceMapRect" _
		( byval surface as any ptr, byval x as long, byval y as long, _
		  byval width as long, byval height as long, byval access as ulong, _
		  byref pixels as any ptr, byref pitch as long ) as long
	declare function Gfx3SurfaceUnmap GFX3CALL alias "fb_Gfx3SurfaceUnmap" _
		( byval surface as any ptr ) as long
	declare function Gfx3SurfaceClear GFX3CALL alias "fb_Gfx3SurfaceClear" _
		( byval surface as any ptr, byval clear_color as ulong ) as long
	'' A null destination selects the current work page and clips the copy to
	'' the active screen VIEW. Opaque destinations use absolute coordinates.
	declare function Gfx3SurfaceBlit GFX3CALL alias "fb_Gfx3SurfaceBlit" _
		( byval destination as any ptr, byval source as any ptr, _
		  byval source_x as long, byval source_y as long, _
		  byval width as long, byval height as long, _
		  byval destination_x as long, byval destination_y as long, _
		  byval mode as long = GFX3_PUT_PSET, _
		  byval alpha as ulong = 255 ) as long
	'' Scale an exact source rectangle into a destination rectangle. Source and
	'' destination dimensions must be positive. A null destination selects the
	'' current work page.
	declare function Gfx3SurfaceBlitScaled GFX3CALL _
		alias "fb_Gfx3SurfaceBlitScaled" _
		( byval destination as any ptr, byval source as any ptr, _
		  byval source_x as long, byval source_y as long, _
		  byval source_width as long, byval source_height as long, _
		  byval destination_x as long, byval destination_y as long, _
		  byval destination_width as long, byval destination_height as long, _
		  byval mode as long = GFX3_PUT_PSET, byval alpha as ulong = 255, _
		  byval filter as long = GFX3_FILTER_NEAREST ) as long
	'' Rotate clockwise in screen coordinates around a source-space pivot and
	'' place that pivot at the supplied destination point. Negative pivot
	'' coordinates select the source centre. Independent negative scale values
	'' mirror an axis; zero scale is invalid.
	declare function Gfx3SurfaceBlitRotated GFX3CALL _
		alias "fb_Gfx3SurfaceBlitRotated" _
		( byval destination as any ptr, byval source as any ptr, _
		  byval source_x as long, byval source_y as long, _
		  byval source_width as long, byval source_height as long, _
		  byval destination_x as single, byval destination_y as single, _
		  byval angle_degrees as single, byval scale_x as single = 1.0, _
		  byval scale_y as single = 1.0, byval pivot_x as single = -1.0, _
		  byval pivot_y as single = -1.0, _
		  byval mode as long = GFX3_PUT_PSET, byval alpha as ulong = 255, _
		  byval filter as long = GFX3_FILTER_NEAREST ) as long
	'' Render a repeating Mode 7 ground plane below horizon_y. Camera coordinates
	'' and height use source texels; horizon_y and focal_length use destination
	'' pixels. Camera height and focal length must be positive.
	declare function Gfx3SurfaceMode7 GFX3CALL alias "fb_Gfx3SurfaceMode7" _
		( byval destination as any ptr, byval source as any ptr, _
		  byval source_x as long, byval source_y as long, _
		  byval source_width as long, byval source_height as long, _
		  byval destination_x as long, byval destination_y as long, _
		  byval destination_width as long, byval destination_height as long, _
		  byval camera_x as single, byval camera_y as single, _
		  byval camera_height as single, byval camera_angle_degrees as single, _
		  byval horizon_y as single, byval focal_length as single, _
		  byval mode as long = GFX3_PUT_PSET, byval alpha as ulong = 255, _
		  byval filter as long = GFX3_FILTER_NEAREST ) as long
	declare function Gfx3SurfacePresent GFX3CALL alias "fb_Gfx3SurfacePresent" _
		( byval surface as any ptr, byval wait as long = false ) as long
	'' Draw a point array to the current work page when destination is null, or
	'' to an opaque render-target surface. Screen coordinates follow the current
	'' VIEW and WINDOW mapping. Repeated screen coordinates retain submission
	'' order through parallel GPU layers, which is useful when downscaling an
	'' alpha mask. Opaque-surface coordinates are absolute and must be unique.
	'' Clipping is performed by the GPU.
	declare function Gfx3DrawPoints GFX3CALL alias "fb_Gfx3DrawPoints" _
		( byval destination as any ptr, _
		  byval points as const Gfx3Point ptr, byval count as long ) as long
	declare function Gfx3RunOnRenderThread GFX3CALL _
		alias "fb_Gfx3RunOnRenderThread" _
		( byval callback as Gfx3RenderCallback ptr, _
		  byval user_data as any ptr = 0 ) as long
	end extern

end namespace

#undef GFX3CALL

#endif

'' end of fbgfx3.bi
