'' Project: FreeBASIC SDL addon bindings
'' File: SDL_gpu.bi
'' Purpose: Declare the pinned SDL2_gpu C interface.
'' Responsibilities: Preserve public types, callbacks, and header helpers.
'' This file intentionally does NOT contain: the external library implementation.
'' Translated from upstream; this is an altered source version.
''
'' Auto-detect if we're using the SDL2 API by the headers available.

#pragma once

#inclib "SDL2_gpu"
#include once "SDL.bi"


#include once "crt/long.bi"

extern "C"

type GPU_RendererImpl as GPU_RendererImpl_

#define _SDL_GPU_H__
#define _USE_MATH_DEFINES
#define SDL_GPU_USE_SDL2
const GPU_HAVE_STDC = 1
#define GPU_HAVE_C99 (GPU_HAVE_STDC andalso (__STDC_VERSION__ >= cast(clong, 199901)))
const GPU_HAVE_GNUC = 1
const GPU_HAVE_MSVC = 0
#define GPU_HAVE_MSVC18 (GPU_HAVE_MSVC andalso (_MSC_VER >= 1800))
#define GPU_bool boolean

#ifndef __FB_64BIT__
	const SDL_GPU_BITNESS = 32
#elseif defined(__FB_WIN32__) and defined(__FB_64BIT__)
	const SDL_GPU_BITNESS = 64
#endif

#ifdef __FB_64BIT__
const SDL_GPU_BITNESS = 64
#else
const SDL_GPU_BITNESS = 32
#endif

const GPU_FALSE = 0
const GPU_TRUE = 1

type GPU_Rect
	x as single
	y as single
	w as single
	h as single
end type

const GPU_RENDERER_ORDER_MAX = 10
type GPU_RendererEnum as Uint32
const GPU_RENDERER_UNKNOWN as GPU_RendererEnum = 0
const GPU_RENDERER_OPENGL_1_BASE as GPU_RendererEnum = 1
const GPU_RENDERER_OPENGL_1 as GPU_RendererEnum = 2
const GPU_RENDERER_OPENGL_2 as GPU_RendererEnum = 3
const GPU_RENDERER_OPENGL_3 as GPU_RendererEnum = 4
const GPU_RENDERER_OPENGL_4 as GPU_RendererEnum = 5
const GPU_RENDERER_GLES_1 as GPU_RendererEnum = 11
const GPU_RENDERER_GLES_2 as GPU_RendererEnum = 12
const GPU_RENDERER_GLES_3 as GPU_RendererEnum = 13
const GPU_RENDERER_D3D9 as GPU_RendererEnum = 21
const GPU_RENDERER_D3D10 as GPU_RendererEnum = 22
const GPU_RENDERER_D3D11 as GPU_RendererEnum = 23
const GPU_RENDERER_CUSTOM_0 = 1000

type GPU_RendererID
	name as const zstring ptr
	renderer as GPU_RendererEnum
	major_version as long
	minor_version as long

	#ifdef __FB_64BIT__
		_padding as zstring * 4
	#endif
end type

type GPU_ComparisonEnum as long
enum
	GPU_NEVER = &h0200
	GPU_LESS = &h0201
	GPU_EQUAL = &h0202
	GPU_LEQUAL = &h0203
	GPU_GREATER = &h0204
	GPU_NOTEQUAL = &h0205
	GPU_GEQUAL = &h0206
	GPU_ALWAYS = &h0207
end enum

type GPU_BlendFuncEnum as long
enum
	GPU_FUNC_ZERO = 0
	GPU_FUNC_ONE = 1
	GPU_FUNC_SRC_COLOR = &h0300
	GPU_FUNC_DST_COLOR = &h0306
	GPU_FUNC_ONE_MINUS_SRC = &h0301
	GPU_FUNC_ONE_MINUS_DST = &h0307
	GPU_FUNC_SRC_ALPHA = &h0302
	GPU_FUNC_DST_ALPHA = &h0304
	GPU_FUNC_ONE_MINUS_SRC_ALPHA = &h0303
	GPU_FUNC_ONE_MINUS_DST_ALPHA = &h0305
end enum

type GPU_BlendEqEnum as long
enum
	GPU_EQ_ADD = &h8006
	GPU_EQ_SUBTRACT = &h800A
	GPU_EQ_REVERSE_SUBTRACT = &h800B
end enum

type GPU_BlendMode
	source_color as GPU_BlendFuncEnum
	dest_color as GPU_BlendFuncEnum
	source_alpha as GPU_BlendFuncEnum
	dest_alpha as GPU_BlendFuncEnum
	color_equation as GPU_BlendEqEnum
	alpha_equation as GPU_BlendEqEnum
end type

type GPU_BlendPresetEnum as long
enum
	GPU_BLEND_NORMAL = 0
	GPU_BLEND_PREMULTIPLIED_ALPHA = 1
	GPU_BLEND_MULTIPLY = 2
	GPU_BLEND_ADD = 3
	GPU_BLEND_SUBTRACT = 4
	GPU_BLEND_MOD_ALPHA = 5
	GPU_BLEND_SET_ALPHA = 6
	GPU_BLEND_SET = 7
	GPU_BLEND_NORMAL_KEEP_ALPHA = 8
	GPU_BLEND_NORMAL_ADD_ALPHA = 9
	GPU_BLEND_NORMAL_FACTOR_ALPHA = 10
end enum

type GPU_FilterEnum as long
enum
	GPU_FILTER_NEAREST = 0
	GPU_FILTER_LINEAR = 1
	GPU_FILTER_LINEAR_MIPMAP = 2
end enum

type GPU_SnapEnum as long
enum
	GPU_SNAP_NONE = 0
	GPU_SNAP_POSITION = 1
	GPU_SNAP_DIMENSIONS = 2
	GPU_SNAP_POSITION_AND_DIMENSIONS = 3
end enum

type GPU_WrapEnum as long
enum
	GPU_WRAP_NONE = 0
	GPU_WRAP_REPEAT = 1
	GPU_WRAP_MIRRORED = 2
end enum

type GPU_FormatEnum as long
enum
	GPU_FORMAT_LUMINANCE = 1
	GPU_FORMAT_LUMINANCE_ALPHA = 2
	GPU_FORMAT_RGB = 3
	GPU_FORMAT_RGBA = 4
	GPU_FORMAT_ALPHA = 5
	GPU_FORMAT_RG = 6
	GPU_FORMAT_YCbCr422 = 7
	GPU_FORMAT_YCbCr420P = 8
	GPU_FORMAT_BGR = 9
	GPU_FORMAT_BGRA = 10
	GPU_FORMAT_ABGR = 11
end enum

type GPU_FileFormatEnum as long
enum
	GPU_FILE_AUTO = 0
	GPU_FILE_PNG
	GPU_FILE_BMP
	GPU_FILE_TGA
end enum

type GPU_Renderer as GPU_Renderer_
type GPU_Target as GPU_Target_

type GPU_Image
	renderer as GPU_Renderer ptr
	context_target as GPU_Target ptr
	target as GPU_Target ptr
	data as any ptr
	w as Uint16
	h as Uint16
	format as GPU_FormatEnum
	num_layers as long
	bytes_per_pixel as long
	base_w as Uint16
	base_h as Uint16
	texture_w as Uint16
	texture_h as Uint16
	anchor_x as single
	anchor_y as single
	color as SDL_Color
	blend_mode as GPU_BlendMode
	filter_mode as GPU_FilterEnum
	snap_mode as GPU_SnapEnum
	wrap_mode_x as GPU_WrapEnum
	wrap_mode_y as GPU_WrapEnum
	refcount as long
	using_virtual_resolution as byte
	has_mipmaps as byte
	use_blending as byte
	is_alias as byte
end type

type GPU_TextureHandle as uinteger

type GPU_Camera
	x as single
	y as single
	z as single
	angle as single
	zoom_x as single
	zoom_y as single
	z_near as single
	z_far as single
	use_centered_origin as byte

	#ifdef __FB_64BIT__
		_padding as zstring * 7
	#else
		_padding as zstring * 3
	#endif
end type

type GPU_ShaderBlock
	position_loc as long
	texcoord_loc as long
	color_loc as long
	modelViewProjection_loc as long
end type

const GPU_MODEL = 0
const GPU_VIEW = 1
const GPU_PROJECTION = 2

type GPU_MatrixStack
	storage_size as ulong
	size as ulong
	matrix as single ptr ptr
end type

type GPU_Context
	context as any ptr
	active_target as GPU_Target ptr
	current_shader_block as GPU_ShaderBlock
	default_textured_shader_block as GPU_ShaderBlock
	default_untextured_shader_block as GPU_ShaderBlock
	windowID as Uint32
	window_w as long
	window_h as long
	drawable_w as long
	drawable_h as long
	stored_window_w as long
	stored_window_h as long
	default_textured_vertex_shader_id as Uint32
	default_textured_fragment_shader_id as Uint32
	default_untextured_vertex_shader_id as Uint32
	default_untextured_fragment_shader_id as Uint32
	current_shader_program as Uint32
	default_textured_shader_program as Uint32
	default_untextured_shader_program as Uint32
	shapes_blend_mode as GPU_BlendMode
	line_thickness as single
	refcount as long
	data as any ptr
	failed as byte
	use_texturing as byte
	shapes_use_blending as byte

	#ifdef __FB_64BIT__
		_padding as zstring * 5
	#else
		_padding as zstring * 1
	#endif
end type

type GPU_Target_
	renderer as GPU_Renderer ptr
	context_target as GPU_Target ptr
	image as GPU_Image ptr
	data as any ptr
	w as Uint16
	h as Uint16
	base_w as Uint16
	base_h as Uint16
	clip_rect as GPU_Rect
	color as SDL_Color
	viewport as GPU_Rect
	matrix_mode as long
	projection_matrix as GPU_MatrixStack
	view_matrix as GPU_MatrixStack
	model_matrix as GPU_MatrixStack
	camera as GPU_Camera
	using_virtual_resolution as byte
	use_clip_rect as byte
	use_color as byte
	use_camera as byte
	depth_function as GPU_ComparisonEnum
	context as GPU_Context ptr
	refcount as long
	use_depth_test as byte
	use_depth_write as byte
	is_alias as byte
	_padding as zstring * 1
end type

type GPU_FeatureEnum as Uint32
dim shared GPU_FEATURE_NON_POWER_OF_TWO as const GPU_FeatureEnum = &h1
dim shared GPU_FEATURE_RENDER_TARGETS as const GPU_FeatureEnum = &h2
dim shared GPU_FEATURE_BLEND_EQUATIONS as const GPU_FeatureEnum = &h4
dim shared GPU_FEATURE_BLEND_FUNC_SEPARATE as const GPU_FeatureEnum = &h8
dim shared GPU_FEATURE_BLEND_EQUATIONS_SEPARATE as const GPU_FeatureEnum = &h10
dim shared GPU_FEATURE_GL_BGR as const GPU_FeatureEnum = &h20
dim shared GPU_FEATURE_GL_BGRA as const GPU_FeatureEnum = &h40
dim shared GPU_FEATURE_GL_ABGR as const GPU_FeatureEnum = &h80
dim shared GPU_FEATURE_VERTEX_SHADER as const GPU_FeatureEnum = &h100
dim shared GPU_FEATURE_FRAGMENT_SHADER as const GPU_FeatureEnum = &h200
dim shared GPU_FEATURE_PIXEL_SHADER as const GPU_FeatureEnum = &h200
dim shared GPU_FEATURE_GEOMETRY_SHADER as const GPU_FeatureEnum = &h400
dim shared GPU_FEATURE_WRAP_REPEAT_MIRRORED as const GPU_FeatureEnum = &h800
dim shared GPU_FEATURE_CORE_FRAMEBUFFER_OBJECTS as const GPU_FeatureEnum = &h1000

#define GPU_FEATURE_ALL_BASE GPU_FEATURE_RENDER_TARGETS
#define GPU_FEATURE_ALL_BLEND_PRESETS (GPU_FEATURE_BLEND_EQUATIONS or GPU_FEATURE_BLEND_FUNC_SEPARATE)
#define GPU_FEATURE_ALL_GL_FORMATS ((GPU_FEATURE_GL_BGR or GPU_FEATURE_GL_BGRA) or GPU_FEATURE_GL_ABGR)
#define GPU_FEATURE_BASIC_SHADERS (GPU_FEATURE_FRAGMENT_SHADER or GPU_FEATURE_VERTEX_SHADER)
#define GPU_FEATURE_ALL_SHADERS ((GPU_FEATURE_FRAGMENT_SHADER or GPU_FEATURE_VERTEX_SHADER) or GPU_FEATURE_GEOMETRY_SHADER)
type GPU_WindowFlagEnum as Uint32
type GPU_InitFlagEnum as Uint32

dim shared GPU_INIT_ENABLE_VSYNC as const GPU_InitFlagEnum = &h1
dim shared GPU_INIT_DISABLE_VSYNC as const GPU_InitFlagEnum = &h2
dim shared GPU_INIT_DISABLE_DOUBLE_BUFFER as const GPU_InitFlagEnum = &h4
dim shared GPU_INIT_DISABLE_AUTO_VIRTUAL_RESOLUTION as const GPU_InitFlagEnum = &h8
dim shared GPU_INIT_REQUEST_COMPATIBILITY_PROFILE as const GPU_InitFlagEnum = &h10
dim shared GPU_INIT_USE_ROW_BY_ROW_TEXTURE_UPLOAD_FALLBACK as const GPU_InitFlagEnum = &h20
dim shared GPU_INIT_USE_COPY_TEXTURE_UPLOAD_FALLBACK as const GPU_InitFlagEnum = &h40
const GPU_DEFAULT_INIT_FLAGS = 0
dim shared GPU_NONE as const Uint32 = &h0
type GPU_PrimitiveEnum as Uint32
dim shared GPU_POINTS as const GPU_PrimitiveEnum = &h0
dim shared GPU_LINES as const GPU_PrimitiveEnum = &h1
dim shared GPU_LINE_LOOP as const GPU_PrimitiveEnum = &h2
dim shared GPU_LINE_STRIP as const GPU_PrimitiveEnum = &h3
dim shared GPU_TRIANGLES as const GPU_PrimitiveEnum = &h4
dim shared GPU_TRIANGLE_STRIP as const GPU_PrimitiveEnum = &h5
dim shared GPU_TRIANGLE_FAN as const GPU_PrimitiveEnum = &h6
type GPU_BatchFlagEnum as Uint32
dim shared GPU_BATCH_XY as const GPU_BatchFlagEnum = &h1
dim shared GPU_BATCH_XYZ as const GPU_BatchFlagEnum = &h2
dim shared GPU_BATCH_ST as const GPU_BatchFlagEnum = &h4
dim shared GPU_BATCH_RGB as const GPU_BatchFlagEnum = &h8
dim shared GPU_BATCH_RGBA as const GPU_BatchFlagEnum = &h10
dim shared GPU_BATCH_RGB8 as const GPU_BatchFlagEnum = &h20
dim shared GPU_BATCH_RGBA8 as const GPU_BatchFlagEnum = &h40

#define GPU_BATCH_XY_ST (GPU_BATCH_XY or GPU_BATCH_ST)
#define GPU_BATCH_XYZ_ST (GPU_BATCH_XYZ or GPU_BATCH_ST)
#define GPU_BATCH_XY_RGB (GPU_BATCH_XY or GPU_BATCH_RGB)
#define GPU_BATCH_XYZ_RGB (GPU_BATCH_XYZ or GPU_BATCH_RGB)
#define GPU_BATCH_XY_RGBA (GPU_BATCH_XY or GPU_BATCH_RGBA)
#define GPU_BATCH_XYZ_RGBA (GPU_BATCH_XYZ or GPU_BATCH_RGBA)
#define GPU_BATCH_XY_ST_RGBA ((GPU_BATCH_XY or GPU_BATCH_ST) or GPU_BATCH_RGBA)
#define GPU_BATCH_XYZ_ST_RGBA ((GPU_BATCH_XYZ or GPU_BATCH_ST) or GPU_BATCH_RGBA)
#define GPU_BATCH_XY_RGB8 (GPU_BATCH_XY or GPU_BATCH_RGB8)
#define GPU_BATCH_XYZ_RGB8 (GPU_BATCH_XYZ or GPU_BATCH_RGB8)
#define GPU_BATCH_XY_RGBA8 (GPU_BATCH_XY or GPU_BATCH_RGBA8)
#define GPU_BATCH_XYZ_RGBA8 (GPU_BATCH_XYZ or GPU_BATCH_RGBA8)
#define GPU_BATCH_XY_ST_RGBA8 ((GPU_BATCH_XY or GPU_BATCH_ST) or GPU_BATCH_RGBA8)
#define GPU_BATCH_XYZ_ST_RGBA8 ((GPU_BATCH_XYZ or GPU_BATCH_ST) or GPU_BATCH_RGBA8)
type GPU_FlipEnum as Uint32

dim shared GPU_FLIP_NONE as const GPU_FlipEnum = &h0
dim shared GPU_FLIP_HORIZONTAL as const GPU_FlipEnum = &h1
dim shared GPU_FLIP_VERTICAL as const GPU_FlipEnum = &h2
type GPU_TypeEnum as Uint32
dim shared GPU_TYPE_BYTE as const GPU_TypeEnum = &h1400
dim shared GPU_TYPE_UNSIGNED_BYTE as const GPU_TypeEnum = &h1401
dim shared GPU_TYPE_SHORT as const GPU_TypeEnum = &h1402
dim shared GPU_TYPE_UNSIGNED_SHORT as const GPU_TypeEnum = &h1403
dim shared GPU_TYPE_INT as const GPU_TypeEnum = &h1404
dim shared GPU_TYPE_UNSIGNED_INT as const GPU_TypeEnum = &h1405
dim shared GPU_TYPE_FLOAT as const GPU_TypeEnum = &h1406
dim shared GPU_TYPE_DOUBLE as const GPU_TypeEnum = &h140A

type GPU_ShaderEnum as long
enum
	GPU_VERTEX_SHADER = 0
	GPU_FRAGMENT_SHADER = 1
	GPU_PIXEL_SHADER = 1
	GPU_GEOMETRY_SHADER = 2
end enum

type GPU_ShaderLanguageEnum as long
enum
	GPU_LANGUAGE_NONE = 0
	GPU_LANGUAGE_ARB_ASSEMBLY = 1
	GPU_LANGUAGE_GLSL = 2
	GPU_LANGUAGE_GLSLES = 3
	GPU_LANGUAGE_HLSL = 4
	GPU_LANGUAGE_CG = 5
end enum

type GPU_AttributeFormat
	num_elems_per_value as long
	as GPU_TypeEnum type
	stride_bytes as long
	offset_bytes as long
	is_per_sprite as byte
	normalize as byte
	_padding as zstring * 2
end type

type GPU_Attribute
	values as any ptr
	format as GPU_AttributeFormat
	location as long

	#ifdef __FB_64BIT__
		_padding as zstring * 4
	#endif
end type

type GPU_AttributeSource
	next_value as any ptr
	per_vertex_storage as any ptr
	num_values as long
	per_vertex_storage_stride_bytes as long
	per_vertex_storage_offset_bytes as long
	per_vertex_storage_size as long
	attribute as GPU_Attribute
	enabled as byte

	#ifdef __FB_64BIT__
		_padding as zstring * 7
	#else
		_padding as zstring * 3
	#endif
end type

type GPU_ErrorEnum as long
enum
	GPU_ERROR_NONE = 0
	GPU_ERROR_BACKEND_ERROR = 1
	GPU_ERROR_DATA_ERROR = 2
	GPU_ERROR_USER_ERROR = 3
	GPU_ERROR_UNSUPPORTED_FUNCTION = 4
	GPU_ERROR_NULL_ARGUMENT = 5
	GPU_ERROR_FILE_NOT_FOUND = 6
end enum

type GPU_ErrorObject
	function as zstring ptr
	details as zstring ptr
	error as GPU_ErrorEnum

	#ifdef __FB_64BIT__
		_padding as zstring * 4
	#endif
end type

type GPU_DebugLevelEnum as long
enum
	GPU_DEBUG_LEVEL_0 = 0
	GPU_DEBUG_LEVEL_1 = 1
	GPU_DEBUG_LEVEL_2 = 2
	GPU_DEBUG_LEVEL_3 = 3
	GPU_DEBUG_LEVEL_MAX = 3
end enum

type GPU_LogLevelEnum as long
enum
	GPU_LOG_INFO = 0
	GPU_LOG_WARNING
	GPU_LOG_ERROR
end enum

type GPU_Renderer_
	id as GPU_RendererID
	requested_id as GPU_RendererID
	SDL_init_flags as GPU_WindowFlagEnum
	GPU_init_flags as GPU_InitFlagEnum
	shader_language as GPU_ShaderLanguageEnum
	min_shader_version as long
	max_shader_version as long
	enabled_features as GPU_FeatureEnum
	current_context_target as GPU_Target ptr
	default_image_anchor_x as single
	default_image_anchor_y as single
	impl as GPU_RendererImpl ptr
	coordinate_mode as byte

	#ifdef __FB_64BIT__
		_padding as zstring * 7
	#else
		_padding as zstring * 3
	#endif
end type

private function GPU_GetCompiledVersion cdecl() as SDL_version
	dim version as SDL_version = (0, 12, 0)
	return version
end function

declare function GPU_GetLinkedVersion() as SDL_version
declare sub GPU_SetInitWindow(byval windowID as Uint32)
declare function GPU_GetInitWindow() as Uint32
declare sub GPU_SetPreInitFlags(byval GPU_flags as GPU_InitFlagEnum)
declare function GPU_GetPreInitFlags() as GPU_InitFlagEnum
declare sub GPU_SetRequiredFeatures(byval features as GPU_FeatureEnum)
declare function GPU_GetRequiredFeatures() as GPU_FeatureEnum
declare sub GPU_GetDefaultRendererOrder(byval order_size as long ptr, byval order as GPU_RendererID ptr)
declare sub GPU_GetRendererOrder(byval order_size as long ptr, byval order as GPU_RendererID ptr)
declare sub GPU_SetRendererOrder(byval order_size as long, byval order as GPU_RendererID ptr)
declare function GPU_Init(byval w as Uint16, byval h as Uint16, byval SDL_flags as GPU_WindowFlagEnum) as GPU_Target ptr
declare function GPU_InitRenderer(byval renderer_enum as GPU_RendererEnum, byval w as Uint16, byval h as Uint16, byval SDL_flags as GPU_WindowFlagEnum) as GPU_Target ptr
declare function GPU_InitRendererByID(byval renderer_request as GPU_RendererID, byval w as Uint16, byval h as Uint16, byval SDL_flags as GPU_WindowFlagEnum) as GPU_Target ptr
declare function GPU_IsFeatureEnabled(byval feature as GPU_FeatureEnum) as byte
declare sub GPU_CloseCurrentRenderer()
declare sub GPU_Quit()
declare sub GPU_SetDebugLevel(byval level as GPU_DebugLevelEnum)
declare function GPU_GetDebugLevel() as GPU_DebugLevelEnum
declare sub GPU_LogInfo(byval format as const zstring ptr, ...)
declare sub GPU_Log alias "GPU_LogInfo"(byval format as const zstring ptr, ...)
declare sub GPU_LogWarning(byval format as const zstring ptr, ...)
declare sub GPU_LogError(byval format as const zstring ptr, ...)
declare sub GPU_SetLogCallback(byval callback as function(byval log_level as GPU_LogLevelEnum, byval format as const zstring ptr, byval args as va_list) as long)
declare sub GPU_PushErrorCode(byval function as const zstring ptr, byval error as GPU_ErrorEnum, byval details as const zstring ptr, ...)
declare function GPU_PopErrorCode() as GPU_ErrorObject
declare function GPU_GetErrorString(byval error as GPU_ErrorEnum) as const zstring ptr
declare sub GPU_SetErrorQueueMax(byval max as ulong)
declare function GPU_MakeRendererID(byval name as const zstring ptr, byval renderer as GPU_RendererEnum, byval major_version as long, byval minor_version as long) as GPU_RendererID
declare function GPU_GetRendererID(byval renderer as GPU_RendererEnum) as GPU_RendererID
declare function GPU_GetNumRegisteredRenderers() as long
declare sub GPU_GetRegisteredRendererList(byval renderers_array as GPU_RendererID ptr)
declare sub GPU_RegisterRenderer(byval id as GPU_RendererID, byval create_renderer as function(byval request as GPU_RendererID) as GPU_Renderer ptr, byval free_renderer as sub(byval renderer as GPU_Renderer ptr))
declare function GPU_ReserveNextRendererEnum() as GPU_RendererEnum
declare function GPU_GetNumActiveRenderers() as long
declare sub GPU_GetActiveRendererList(byval renderers_array as GPU_RendererID ptr)
declare function GPU_GetCurrentRenderer() as GPU_Renderer ptr
declare sub GPU_SetCurrentRenderer(byval id as GPU_RendererID)
declare function GPU_GetRenderer(byval id as GPU_RendererID) as GPU_Renderer ptr
declare sub GPU_FreeRenderer(byval renderer as GPU_Renderer ptr)
declare sub GPU_ResetRendererState()
declare sub GPU_SetCoordinateMode(byval use_math_coords as byte)
declare function GPU_GetCoordinateMode() as byte
declare sub GPU_SetDefaultAnchor(byval anchor_x as single, byval anchor_y as single)
declare sub GPU_GetDefaultAnchor(byval anchor_x as single ptr, byval anchor_y as single ptr)
declare function GPU_GetContextTarget() as GPU_Target ptr
declare function GPU_GetWindowTarget(byval windowID as Uint32) as GPU_Target ptr
declare function GPU_CreateTargetFromWindow(byval windowID as Uint32) as GPU_Target ptr
declare sub GPU_MakeCurrent(byval target as GPU_Target ptr, byval windowID as Uint32)
declare function GPU_SetWindowResolution(byval w as Uint16, byval h as Uint16) as byte
declare function GPU_SetFullscreen(byval enable_fullscreen as byte, byval use_desktop_resolution as byte) as byte
declare function GPU_GetFullscreen() as byte
declare function GPU_GetActiveTarget() as GPU_Target ptr
declare function GPU_SetActiveTarget(byval target as GPU_Target ptr) as byte
declare sub GPU_SetShapeBlending(byval enable as byte)
declare function GPU_GetBlendModeFromPreset(byval preset as GPU_BlendPresetEnum) as GPU_BlendMode
declare sub GPU_SetShapeBlendFunction(byval source_color as GPU_BlendFuncEnum, byval dest_color as GPU_BlendFuncEnum, byval source_alpha as GPU_BlendFuncEnum, byval dest_alpha as GPU_BlendFuncEnum)
declare sub GPU_SetShapeBlendEquation(byval color_equation as GPU_BlendEqEnum, byval alpha_equation as GPU_BlendEqEnum)
declare sub GPU_SetShapeBlendMode(byval mode as GPU_BlendPresetEnum)
declare function GPU_SetLineThickness(byval thickness as single) as single
declare function GPU_GetLineThickness() as single
declare function GPU_CreateAliasTarget(byval target as GPU_Target ptr) as GPU_Target ptr
declare function GPU_LoadTarget(byval image as GPU_Image ptr) as GPU_Target ptr
declare function GPU_GetTarget(byval image as GPU_Image ptr) as GPU_Target ptr
declare sub GPU_FreeTarget(byval target as GPU_Target ptr)
declare sub GPU_SetVirtualResolution(byval target as GPU_Target ptr, byval w as Uint16, byval h as Uint16)
declare sub GPU_GetVirtualResolution(byval target as GPU_Target ptr, byval w as Uint16 ptr, byval h as Uint16 ptr)
declare sub GPU_GetVirtualCoords(byval target as GPU_Target ptr, byval x as single ptr, byval y as single ptr, byval displayX as single, byval displayY as single)
declare sub GPU_UnsetVirtualResolution(byval target as GPU_Target ptr)
declare function GPU_MakeRect(byval x as single, byval y as single, byval w as single, byval h as single) as GPU_Rect
declare function GPU_MakeColor(byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as SDL_Color
declare sub GPU_SetViewport(byval target as GPU_Target ptr, byval viewport as GPU_Rect)
declare sub GPU_UnsetViewport(byval target as GPU_Target ptr)
declare function GPU_GetDefaultCamera() as GPU_Camera
declare function GPU_GetCamera(byval target as GPU_Target ptr) as GPU_Camera
declare function GPU_SetCamera(byval target as GPU_Target ptr, byval cam as GPU_Camera ptr) as GPU_Camera
declare sub GPU_EnableCamera(byval target as GPU_Target ptr, byval use_camera as byte)
declare function GPU_IsCameraEnabled(byval target as GPU_Target ptr) as byte
declare function GPU_AddDepthBuffer(byval target as GPU_Target ptr) as byte
declare sub GPU_SetDepthTest(byval target as GPU_Target ptr, byval enable as byte)
declare sub GPU_SetDepthWrite(byval target as GPU_Target ptr, byval enable as byte)
declare sub GPU_SetDepthFunction(byval target as GPU_Target ptr, byval compare_operation as GPU_ComparisonEnum)
declare function GPU_GetPixel(byval target as GPU_Target ptr, byval x as Sint16, byval y as Sint16) as SDL_Color
declare function GPU_SetClipRect(byval target as GPU_Target ptr, byval rect as GPU_Rect) as GPU_Rect
declare function GPU_SetClip(byval target as GPU_Target ptr, byval x as Sint16, byval y as Sint16, byval w as Uint16, byval h as Uint16) as GPU_Rect
declare sub GPU_UnsetClip(byval target as GPU_Target ptr)
declare function GPU_IntersectRect(byval A as GPU_Rect, byval B as GPU_Rect, byval result as GPU_Rect ptr) as byte
declare function GPU_IntersectClipRect(byval target as GPU_Target ptr, byval B as GPU_Rect, byval result as GPU_Rect ptr) as byte
declare sub GPU_SetTargetColor(byval target as GPU_Target ptr, byval color as SDL_Color)
declare sub GPU_SetTargetRGB(byval target as GPU_Target ptr, byval r as Uint8, byval g as Uint8, byval b as Uint8)
declare sub GPU_SetTargetRGBA(byval target as GPU_Target ptr, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8)
declare sub GPU_UnsetTargetColor(byval target as GPU_Target ptr)
declare function GPU_LoadSurface(byval filename as const zstring ptr) as SDL_Surface ptr
declare function GPU_LoadSurface_RW(byval rwops as SDL_RWops ptr, byval free_rwops as byte) as SDL_Surface ptr
declare function GPU_SaveSurface(byval surface as SDL_Surface ptr, byval filename as const zstring ptr, byval format as GPU_FileFormatEnum) as byte
declare function GPU_SaveSurface_RW(byval surface as SDL_Surface ptr, byval rwops as SDL_RWops ptr, byval free_rwops as byte, byval format as GPU_FileFormatEnum) as byte
declare function GPU_CreateImage(byval w as Uint16, byval h as Uint16, byval format as GPU_FormatEnum) as GPU_Image ptr
declare function GPU_CreateImageUsingTexture(byval handle as GPU_TextureHandle, byval take_ownership as byte) as GPU_Image ptr
declare function GPU_LoadImage(byval filename as const zstring ptr) as GPU_Image ptr
declare function GPU_LoadImage_RW(byval rwops as SDL_RWops ptr, byval free_rwops as byte) as GPU_Image ptr
declare function GPU_CreateAliasImage(byval image as GPU_Image ptr) as GPU_Image ptr
declare function GPU_CopyImage(byval image as GPU_Image ptr) as GPU_Image ptr
declare sub GPU_FreeImage(byval image as GPU_Image ptr)
declare sub GPU_SetImageVirtualResolution(byval image as GPU_Image ptr, byval w as Uint16, byval h as Uint16)
declare sub GPU_UnsetImageVirtualResolution(byval image as GPU_Image ptr)
declare sub GPU_UpdateImage(byval image as GPU_Image ptr, byval image_rect as const GPU_Rect ptr, byval surface as SDL_Surface ptr, byval surface_rect as const GPU_Rect ptr)
declare sub GPU_UpdateImageBytes(byval image as GPU_Image ptr, byval image_rect as const GPU_Rect ptr, byval bytes as const ubyte ptr, byval bytes_per_row as long)
declare function GPU_ReplaceImage(byval image as GPU_Image ptr, byval surface as SDL_Surface ptr, byval surface_rect as const GPU_Rect ptr) as byte
declare function GPU_SaveImage(byval image as GPU_Image ptr, byval filename as const zstring ptr, byval format as GPU_FileFormatEnum) as byte
declare function GPU_SaveImage_RW(byval image as GPU_Image ptr, byval rwops as SDL_RWops ptr, byval free_rwops as byte, byval format as GPU_FileFormatEnum) as byte
declare sub GPU_GenerateMipmaps(byval image as GPU_Image ptr)
declare sub GPU_SetColor(byval image as GPU_Image ptr, byval color as SDL_Color)
declare sub GPU_SetRGB(byval image as GPU_Image ptr, byval r as Uint8, byval g as Uint8, byval b as Uint8)
declare sub GPU_SetRGBA(byval image as GPU_Image ptr, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8)
declare sub GPU_UnsetColor(byval image as GPU_Image ptr)
declare function GPU_GetBlending(byval image as GPU_Image ptr) as byte
declare sub GPU_SetBlending(byval image as GPU_Image ptr, byval enable as byte)
declare sub GPU_SetBlendFunction(byval image as GPU_Image ptr, byval source_color as GPU_BlendFuncEnum, byval dest_color as GPU_BlendFuncEnum, byval source_alpha as GPU_BlendFuncEnum, byval dest_alpha as GPU_BlendFuncEnum)
declare sub GPU_SetBlendEquation(byval image as GPU_Image ptr, byval color_equation as GPU_BlendEqEnum, byval alpha_equation as GPU_BlendEqEnum)
declare sub GPU_SetBlendMode(byval image as GPU_Image ptr, byval mode as GPU_BlendPresetEnum)
declare sub GPU_SetImageFilter(byval image as GPU_Image ptr, byval filter as GPU_FilterEnum)
declare sub GPU_SetAnchor(byval image as GPU_Image ptr, byval anchor_x as single, byval anchor_y as single)
declare sub GPU_GetAnchor(byval image as GPU_Image ptr, byval anchor_x as single ptr, byval anchor_y as single ptr)
declare function GPU_GetSnapMode(byval image as GPU_Image ptr) as GPU_SnapEnum
declare sub GPU_SetSnapMode(byval image as GPU_Image ptr, byval mode as GPU_SnapEnum)
declare sub GPU_SetWrapMode(byval image as GPU_Image ptr, byval wrap_mode_x as GPU_WrapEnum, byval wrap_mode_y as GPU_WrapEnum)
declare function GPU_GetTextureHandle(byval image as GPU_Image ptr) as GPU_TextureHandle
declare function GPU_CopyImageFromSurface(byval surface as SDL_Surface ptr) as GPU_Image ptr
declare function GPU_CopyImageFromSurfaceRect(byval surface as SDL_Surface ptr, byval surface_rect as GPU_Rect ptr) as GPU_Image ptr
declare function GPU_CopyImageFromTarget(byval target as GPU_Target ptr) as GPU_Image ptr
declare function GPU_CopySurfaceFromTarget(byval target as GPU_Target ptr) as SDL_Surface ptr
declare function GPU_CopySurfaceFromImage(byval image as GPU_Image ptr) as SDL_Surface ptr
declare function GPU_VectorLength(byval vec3 as const single ptr) as single
declare sub GPU_VectorNormalize(byval vec3 as single ptr)
declare function GPU_VectorDot(byval A as const single ptr, byval B as const single ptr) as single
declare sub GPU_VectorCross(byval result as single ptr, byval A as const single ptr, byval B as const single ptr)
declare sub GPU_VectorCopy(byval result as single ptr, byval A as const single ptr)
declare sub GPU_VectorApplyMatrix(byval vec3 as single ptr, byval matrix_4x4 as const single ptr)
declare sub GPU_Vector4ApplyMatrix(byval vec4 as single ptr, byval matrix_4x4 as const single ptr)
declare sub GPU_MatrixCopy(byval result as single ptr, byval A as const single ptr)
declare sub GPU_MatrixIdentity(byval result as single ptr)
declare sub GPU_MatrixOrtho(byval result as single ptr, byval left as single, byval right as single, byval bottom as single, byval top as single, byval z_near as single, byval z_far as single)
declare sub GPU_MatrixFrustum(byval result as single ptr, byval left as single, byval right as single, byval bottom as single, byval top as single, byval z_near as single, byval z_far as single)
declare sub GPU_MatrixPerspective(byval result as single ptr, byval fovy as single, byval aspect as single, byval z_near as single, byval z_far as single)
declare sub GPU_MatrixLookAt(byval matrix as single ptr, byval eye_x as single, byval eye_y as single, byval eye_z as single, byval target_x as single, byval target_y as single, byval target_z as single, byval up_x as single, byval up_y as single, byval up_z as single)
declare sub GPU_MatrixTranslate(byval result as single ptr, byval x as single, byval y as single, byval z as single)
declare sub GPU_MatrixScale(byval result as single ptr, byval sx as single, byval sy as single, byval sz as single)
declare sub GPU_MatrixRotate(byval result as single ptr, byval degrees as single, byval x as single, byval y as single, byval z as single)
declare sub GPU_MatrixMultiply(byval result as single ptr, byval A as const single ptr, byval B as const single ptr)
declare sub GPU_MultiplyAndAssign(byval result as single ptr, byval B as const single ptr)
declare function GPU_GetMatrixString(byval A as const single ptr) as const zstring ptr
declare function GPU_GetCurrentMatrix() as single ptr
declare function GPU_GetTopMatrix(byval stack as GPU_MatrixStack ptr) as single ptr
declare function GPU_GetModel() as single ptr
declare function GPU_GetView() as single ptr
declare function GPU_GetProjection() as single ptr
declare sub GPU_GetModelViewProjection(byval result as single ptr)
declare function GPU_CreateMatrixStack() as GPU_MatrixStack ptr
declare sub GPU_FreeMatrixStack(byval stack as GPU_MatrixStack ptr)
declare sub GPU_InitMatrixStack(byval stack as GPU_MatrixStack ptr)
declare sub GPU_CopyMatrixStack(byval source as const GPU_MatrixStack ptr, byval dest as GPU_MatrixStack ptr)
declare sub GPU_ClearMatrixStack(byval stack as GPU_MatrixStack ptr)
declare sub GPU_ResetProjection(byval target as GPU_Target ptr)
declare sub GPU_MatrixMode(byval target as GPU_Target ptr, byval matrix_mode as long)
declare sub GPU_SetProjection(byval A as const single ptr)
declare sub GPU_SetView(byval A as const single ptr)
declare sub GPU_SetModel(byval A as const single ptr)
declare sub GPU_SetProjectionFromStack(byval stack as GPU_MatrixStack ptr)
declare sub GPU_SetViewFromStack(byval stack as GPU_MatrixStack ptr)
declare sub GPU_SetModelFromStack(byval stack as GPU_MatrixStack ptr)
declare sub GPU_PushMatrix()
declare sub GPU_PopMatrix()
declare sub GPU_LoadIdentity()
declare sub GPU_LoadMatrix(byval matrix4x4 as const single ptr)
declare sub GPU_Ortho(byval left as single, byval right as single, byval bottom as single, byval top as single, byval z_near as single, byval z_far as single)
declare sub GPU_Frustum(byval left as single, byval right as single, byval bottom as single, byval top as single, byval z_near as single, byval z_far as single)
declare sub GPU_Perspective(byval fovy as single, byval aspect as single, byval z_near as single, byval z_far as single)
declare sub GPU_LookAt(byval eye_x as single, byval eye_y as single, byval eye_z as single, byval target_x as single, byval target_y as single, byval target_z as single, byval up_x as single, byval up_y as single, byval up_z as single)
declare sub GPU_Translate(byval x as single, byval y as single, byval z as single)
declare sub GPU_Scale(byval sx as single, byval sy as single, byval sz as single)
declare sub GPU_Rotate(byval degrees as single, byval x as single, byval y as single, byval z as single)
declare sub GPU_MultMatrix(byval matrix4x4 as const single ptr)
declare sub GPU_Clear(byval target as GPU_Target ptr)
declare sub GPU_ClearColor(byval target as GPU_Target ptr, byval color as SDL_Color)
declare sub GPU_ClearRGB(byval target as GPU_Target ptr, byval r as Uint8, byval g as Uint8, byval b as Uint8)
declare sub GPU_ClearRGBA(byval target as GPU_Target ptr, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8)
declare sub GPU_Blit(byval image as GPU_Image ptr, byval src_rect as GPU_Rect ptr, byval target as GPU_Target ptr, byval x as single, byval y as single)
declare sub GPU_BlitRotate(byval image as GPU_Image ptr, byval src_rect as GPU_Rect ptr, byval target as GPU_Target ptr, byval x as single, byval y as single, byval degrees as single)
declare sub GPU_BlitScale(byval image as GPU_Image ptr, byval src_rect as GPU_Rect ptr, byval target as GPU_Target ptr, byval x as single, byval y as single, byval scaleX as single, byval scaleY as single)
declare sub GPU_BlitTransform(byval image as GPU_Image ptr, byval src_rect as GPU_Rect ptr, byval target as GPU_Target ptr, byval x as single, byval y as single, byval degrees as single, byval scaleX as single, byval scaleY as single)
declare sub GPU_BlitTransformX(byval image as GPU_Image ptr, byval src_rect as GPU_Rect ptr, byval target as GPU_Target ptr, byval x as single, byval y as single, byval pivot_x as single, byval pivot_y as single, byval degrees as single, byval scaleX as single, byval scaleY as single)
declare sub GPU_BlitRect(byval image as GPU_Image ptr, byval src_rect as GPU_Rect ptr, byval target as GPU_Target ptr, byval dest_rect as GPU_Rect ptr)
declare sub GPU_BlitRectX(byval image as GPU_Image ptr, byval src_rect as GPU_Rect ptr, byval target as GPU_Target ptr, byval dest_rect as GPU_Rect ptr, byval degrees as single, byval pivot_x as single, byval pivot_y as single, byval flip_direction as GPU_FlipEnum)
declare sub GPU_TriangleBatch(byval image as GPU_Image ptr, byval target as GPU_Target ptr, byval num_vertices as ushort, byval values as single ptr, byval num_indices as ulong, byval indices as ushort ptr, byval flags as GPU_BatchFlagEnum)
declare sub GPU_TriangleBatchX(byval image as GPU_Image ptr, byval target as GPU_Target ptr, byval num_vertices as ushort, byval values as any ptr, byval num_indices as ulong, byval indices as ushort ptr, byval flags as GPU_BatchFlagEnum)
declare sub GPU_PrimitiveBatch(byval image as GPU_Image ptr, byval target as GPU_Target ptr, byval primitive_type as GPU_PrimitiveEnum, byval num_vertices as ushort, byval values as single ptr, byval num_indices as ulong, byval indices as ushort ptr, byval flags as GPU_BatchFlagEnum)
declare sub GPU_PrimitiveBatchV(byval image as GPU_Image ptr, byval target as GPU_Target ptr, byval primitive_type as GPU_PrimitiveEnum, byval num_vertices as ushort, byval values as any ptr, byval num_indices as ulong, byval indices as ushort ptr, byval flags as GPU_BatchFlagEnum)
declare sub GPU_FlushBlitBuffer()
declare sub GPU_Flip(byval target as GPU_Target ptr)
declare sub GPU_Pixel(byval target as GPU_Target ptr, byval x as single, byval y as single, byval color as SDL_Color)
declare sub GPU_Line(byval target as GPU_Target ptr, byval x1 as single, byval y1 as single, byval x2 as single, byval y2 as single, byval color as SDL_Color)
declare sub GPU_Arc(byval target as GPU_Target ptr, byval x as single, byval y as single, byval radius as single, byval start_angle as single, byval end_angle as single, byval color as SDL_Color)
declare sub GPU_ArcFilled(byval target as GPU_Target ptr, byval x as single, byval y as single, byval radius as single, byval start_angle as single, byval end_angle as single, byval color as SDL_Color)
declare sub GPU_Circle(byval target as GPU_Target ptr, byval x as single, byval y as single, byval radius as single, byval color as SDL_Color)
declare sub GPU_CircleFilled(byval target as GPU_Target ptr, byval x as single, byval y as single, byval radius as single, byval color as SDL_Color)
declare sub GPU_Ellipse(byval target as GPU_Target ptr, byval x as single, byval y as single, byval rx as single, byval ry as single, byval degrees as single, byval color as SDL_Color)
declare sub GPU_EllipseFilled(byval target as GPU_Target ptr, byval x as single, byval y as single, byval rx as single, byval ry as single, byval degrees as single, byval color as SDL_Color)
declare sub GPU_Sector(byval target as GPU_Target ptr, byval x as single, byval y as single, byval inner_radius as single, byval outer_radius as single, byval start_angle as single, byval end_angle as single, byval color as SDL_Color)
declare sub GPU_SectorFilled(byval target as GPU_Target ptr, byval x as single, byval y as single, byval inner_radius as single, byval outer_radius as single, byval start_angle as single, byval end_angle as single, byval color as SDL_Color)
declare sub GPU_Tri(byval target as GPU_Target ptr, byval x1 as single, byval y1 as single, byval x2 as single, byval y2 as single, byval x3 as single, byval y3 as single, byval color as SDL_Color)
declare sub GPU_TriFilled(byval target as GPU_Target ptr, byval x1 as single, byval y1 as single, byval x2 as single, byval y2 as single, byval x3 as single, byval y3 as single, byval color as SDL_Color)
declare sub GPU_Rectangle(byval target as GPU_Target ptr, byval x1 as single, byval y1 as single, byval x2 as single, byval y2 as single, byval color as SDL_Color)
declare sub GPU_Rectangle2(byval target as GPU_Target ptr, byval rect as GPU_Rect, byval color as SDL_Color)
declare sub GPU_RectangleFilled(byval target as GPU_Target ptr, byval x1 as single, byval y1 as single, byval x2 as single, byval y2 as single, byval color as SDL_Color)
declare sub GPU_RectangleFilled2(byval target as GPU_Target ptr, byval rect as GPU_Rect, byval color as SDL_Color)
declare sub GPU_RectangleRound(byval target as GPU_Target ptr, byval x1 as single, byval y1 as single, byval x2 as single, byval y2 as single, byval radius as single, byval color as SDL_Color)
declare sub GPU_RectangleRound2(byval target as GPU_Target ptr, byval rect as GPU_Rect, byval radius as single, byval color as SDL_Color)
declare sub GPU_RectangleRoundFilled(byval target as GPU_Target ptr, byval x1 as single, byval y1 as single, byval x2 as single, byval y2 as single, byval radius as single, byval color as SDL_Color)
declare sub GPU_RectangleRoundFilled2(byval target as GPU_Target ptr, byval rect as GPU_Rect, byval radius as single, byval color as SDL_Color)
declare sub GPU_Polygon(byval target as GPU_Target ptr, byval num_vertices as ulong, byval vertices as single ptr, byval color as SDL_Color)
declare sub GPU_Polyline(byval target as GPU_Target ptr, byval num_vertices as ulong, byval vertices as single ptr, byval color as SDL_Color, byval close_loop as byte)
declare sub GPU_PolygonFilled(byval target as GPU_Target ptr, byval num_vertices as ulong, byval vertices as single ptr, byval color as SDL_Color)
declare function GPU_CreateShaderProgram() as Uint32
declare sub GPU_FreeShaderProgram(byval program_object as Uint32)
declare function GPU_CompileShader_RW(byval shader_type as GPU_ShaderEnum, byval shader_source as SDL_RWops ptr, byval free_rwops as byte) as Uint32
declare function GPU_CompileShader(byval shader_type as GPU_ShaderEnum, byval shader_source as const zstring ptr) as Uint32
declare function GPU_LoadShader(byval shader_type as GPU_ShaderEnum, byval filename as const zstring ptr) as Uint32
declare function GPU_LinkShaders(byval shader_object1 as Uint32, byval shader_object2 as Uint32) as Uint32
declare function GPU_LinkManyShaders(byval shader_objects as Uint32 ptr, byval count as long) as Uint32
declare sub GPU_FreeShader(byval shader_object as Uint32)
declare sub GPU_AttachShader(byval program_object as Uint32, byval shader_object as Uint32)
declare sub GPU_DetachShader(byval program_object as Uint32, byval shader_object as Uint32)
declare function GPU_LinkShaderProgram(byval program_object as Uint32) as byte
declare function GPU_GetCurrentShaderProgram() as Uint32
declare function GPU_IsDefaultShaderProgram(byval program_object as Uint32) as byte
declare sub GPU_ActivateShaderProgram(byval program_object as Uint32, byval block as GPU_ShaderBlock ptr)
declare sub GPU_DeactivateShaderProgram()
declare function GPU_GetShaderMessage() as const zstring ptr
declare function GPU_GetAttributeLocation(byval program_object as Uint32, byval attrib_name as const zstring ptr) as long
declare function GPU_MakeAttributeFormat(byval num_elems_per_vertex as long, byval type as GPU_TypeEnum, byval normalize as byte, byval stride_bytes as long, byval offset_bytes as long) as GPU_AttributeFormat
declare function GPU_MakeAttribute(byval location as long, byval values as any ptr, byval format as GPU_AttributeFormat) as GPU_Attribute
declare function GPU_GetUniformLocation(byval program_object as Uint32, byval uniform_name as const zstring ptr) as long
declare function GPU_LoadShaderBlock(byval program_object as Uint32, byval position_name as const zstring ptr, byval texcoord_name as const zstring ptr, byval color_name as const zstring ptr, byval modelViewMatrix_name as const zstring ptr) as GPU_ShaderBlock
declare sub GPU_SetShaderBlock(byval block as GPU_ShaderBlock)
declare function GPU_GetShaderBlock() as GPU_ShaderBlock
declare sub GPU_SetShaderImage(byval image as GPU_Image ptr, byval location as long, byval image_unit as long)
declare sub GPU_GetUniformiv(byval program_object as Uint32, byval location as long, byval values as long ptr)
declare sub GPU_SetUniformi(byval location as long, byval value as long)
declare sub GPU_SetUniformiv(byval location as long, byval num_elements_per_value as long, byval num_values as long, byval values as long ptr)
declare sub GPU_GetUniformuiv(byval program_object as Uint32, byval location as long, byval values as ulong ptr)
declare sub GPU_SetUniformui(byval location as long, byval value as ulong)
declare sub GPU_SetUniformuiv(byval location as long, byval num_elements_per_value as long, byval num_values as long, byval values as ulong ptr)
declare sub GPU_GetUniformfv(byval program_object as Uint32, byval location as long, byval values as single ptr)
declare sub GPU_SetUniformf(byval location as long, byval value as single)
declare sub GPU_SetUniformfv(byval location as long, byval num_elements_per_value as long, byval num_values as long, byval values as single ptr)
declare sub GPU_GetUniformMatrixfv(byval program_object as Uint32, byval location as long, byval values as single ptr)
declare sub GPU_SetUniformMatrixfv(byval location as long, byval num_matrices as long, byval num_rows as long, byval num_columns as long, byval transpose as byte, byval values as single ptr)
declare sub GPU_SetAttributef(byval location as long, byval value as single)
declare sub GPU_SetAttributei(byval location as long, byval value as long)
declare sub GPU_SetAttributeui(byval location as long, byval value as ulong)
declare sub GPU_SetAttributefv(byval location as long, byval num_elements as long, byval value as single ptr)
declare sub GPU_SetAttributeiv(byval location as long, byval num_elements as long, byval value as long ptr)
declare sub GPU_SetAttributeuiv(byval location as long, byval num_elements as long, byval value as ulong ptr)
declare sub GPU_SetAttributeSource(byval num_values as long, byval source as GPU_Attribute)

end extern

'' End of SDL_gpu.bi
