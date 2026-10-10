'' Project: FreeBASIC SDL addon bindings
'' File: SDL_shadercross.bi
'' Purpose: Declare the pinned SDL3_shadercross C interface.
'' Responsibilities: Preserve public types, callbacks, and header helpers.
'' This file intentionally does NOT contain: the external library implementation.
'' Translated from upstream; this is an altered source version.
''
'' Simple DirectMedia Layer Shader Cross Compiler
''   Copyright (C) 2024 Sam Lantinga <slouken@libsdl.org>
''
''   This software is provided 'as-is', without any express or implied
''   warranty.  In no event will the authors be held liable for any damages
''   arising from the use of this software.
''
''   Permission is granted to anyone to use this software for any purpose,
''   including commercial applications, and to alter it and redistribute it
''   freely, subject to the following restrictions:
''
''   1. The origin of this software must not be misrepresented; you must not
''      claim that you wrote the original software. If you use this software
''      in a product, an acknowledgment in the product documentation would be
''      appreciated but is not required.
''   2. Altered source versions must be plainly marked as such, and must not be
''      misrepresented as being the original software.
''   3. This notice may not be removed or altered from any source distribution.

#pragma once

#inclib "SDL3_shadercross"
#include once "SDL.bi"


#include once "SDL3/SDL_gpu.bi"

extern "C"

#define SDL_SHADERCROSS_H
const SDL_SHADERCROSS_MAJOR_VERSION = 3
const SDL_SHADERCROSS_MINOR_VERSION = 0
const SDL_SHADERCROSS_MICRO_VERSION = 0

type SDL_ShaderCross_IOVarType as long
enum
	SDL_SHADERCROSS_IOVAR_TYPE_UNKNOWN
	SDL_SHADERCROSS_IOVAR_TYPE_INT8
	SDL_SHADERCROSS_IOVAR_TYPE_UINT8
	SDL_SHADERCROSS_IOVAR_TYPE_INT16
	SDL_SHADERCROSS_IOVAR_TYPE_UINT16
	SDL_SHADERCROSS_IOVAR_TYPE_INT32
	SDL_SHADERCROSS_IOVAR_TYPE_UINT32
	SDL_SHADERCROSS_IOVAR_TYPE_INT64
	SDL_SHADERCROSS_IOVAR_TYPE_UINT64
	SDL_SHADERCROSS_IOVAR_TYPE_FLOAT16
	SDL_SHADERCROSS_IOVAR_TYPE_FLOAT32
	SDL_SHADERCROSS_IOVAR_TYPE_FLOAT64
end enum

type SDL_ShaderCross_ShaderStage as long
enum
	SDL_SHADERCROSS_SHADERSTAGE_VERTEX
	SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT
	SDL_SHADERCROSS_SHADERSTAGE_COMPUTE
end enum

type SDL_ShaderCross_IOVarMetadata
	name as zstring ptr
	location as Uint32
	vector_type as SDL_ShaderCross_IOVarType
	vector_size as Uint32
end type

type SDL_ShaderCross_GraphicsShaderResourceInfo
	num_samplers as Uint32
	num_storage_textures as Uint32
	num_storage_buffers as Uint32
	num_uniform_buffers as Uint32
end type

type SDL_ShaderCross_GraphicsShaderMetadata
	resource_info as SDL_ShaderCross_GraphicsShaderResourceInfo
	num_inputs as Uint32
	inputs as SDL_ShaderCross_IOVarMetadata ptr
	num_outputs as Uint32
	outputs as SDL_ShaderCross_IOVarMetadata ptr
end type

type SDL_ShaderCross_ComputePipelineMetadata
	num_samplers as Uint32
	num_readonly_storage_textures as Uint32
	num_readonly_storage_buffers as Uint32
	num_readwrite_storage_textures as Uint32
	num_readwrite_storage_buffers as Uint32
	num_uniform_buffers as Uint32
	threadcount_x as Uint32
	threadcount_y as Uint32
	threadcount_z as Uint32
end type

type SDL_ShaderCross_SPIRV_Info
	bytecode as const Uint8 ptr
	bytecode_size as uinteger
	entrypoint as const zstring ptr
	shader_stage as SDL_ShaderCross_ShaderStage
	props as SDL_PropertiesID
end type

#define SDL_SHADERCROSS_PROP_SHADER_DEBUG_ENABLE_BOOLEAN "SDL_shadercross.spirv.debug.enable"
#define SDL_SHADERCROSS_PROP_SHADER_DEBUG_NAME_STRING "SDL_shadercross.spirv.debug.name"
#define SDL_SHADERCROSS_PROP_SHADER_CULL_UNUSED_BINDINGS_BOOLEAN "SDL_shadercross.spirv.cull_unused_bindings"
#define SDL_SHADERCROSS_PROP_SPIRV_PSSL_COMPATIBILITY_BOOLEAN "SDL_shadercross.spirv.pssl.compatibility"
#define SDL_SHADERCROSS_PROP_SPIRV_MSL_VERSION_STRING "SDL_shadercross.spirv.msl.version"
#define SDL_SHADERCROSS_PROP_HLSL_SKIP_SPIRV_ROUNDTRIP_BOOLEAN "SDL_shadercross.hlsl.skip_spirv_roundtrip"

type SDL_ShaderCross_HLSL_Define
	name as zstring ptr
	value as zstring ptr
end type

type SDL_ShaderCross_HLSL_Info
	source as const zstring ptr
	entrypoint as const zstring ptr
	include_dir as const zstring ptr
	defines as SDL_ShaderCross_HLSL_Define ptr
	shader_stage as SDL_ShaderCross_ShaderStage
	props as SDL_PropertiesID
end type

declare function SDL_ShaderCross_Init() as boolean
declare sub SDL_ShaderCross_Quit()
declare function SDL_ShaderCross_GetSPIRVShaderFormats() as SDL_GPUShaderFormat
declare function SDL_ShaderCross_TranspileMSLFromSPIRV(byval info as const SDL_ShaderCross_SPIRV_Info ptr) as any ptr
declare function SDL_ShaderCross_TranspileHLSLFromSPIRV(byval info as const SDL_ShaderCross_SPIRV_Info ptr) as any ptr
declare function SDL_ShaderCross_CompileDXBCFromSPIRV(byval info as const SDL_ShaderCross_SPIRV_Info ptr, byval size as uinteger ptr) as any ptr
declare function SDL_ShaderCross_CompileDXILFromSPIRV(byval info as const SDL_ShaderCross_SPIRV_Info ptr, byval size as uinteger ptr) as any ptr
declare function SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(byval device as SDL_GPUDevice ptr, byval info as const SDL_ShaderCross_SPIRV_Info ptr, byval resource_info as const SDL_ShaderCross_GraphicsShaderResourceInfo ptr, byval props as SDL_PropertiesID) as SDL_GPUShader ptr
declare function SDL_ShaderCross_CompileComputePipelineFromSPIRV(byval device as SDL_GPUDevice ptr, byval info as const SDL_ShaderCross_SPIRV_Info ptr, byval metadata as const SDL_ShaderCross_ComputePipelineMetadata ptr, byval props as SDL_PropertiesID) as SDL_GPUComputePipeline ptr
declare function SDL_ShaderCross_ReflectGraphicsSPIRV(byval bytecode as const Uint8 ptr, byval bytecode_size as uinteger, byval props as SDL_PropertiesID) as SDL_ShaderCross_GraphicsShaderMetadata ptr
declare function SDL_ShaderCross_ReflectComputeSPIRV(byval bytecode as const Uint8 ptr, byval bytecode_size as uinteger, byval props as SDL_PropertiesID) as SDL_ShaderCross_ComputePipelineMetadata ptr
declare function SDL_ShaderCross_GetHLSLShaderFormats() as SDL_GPUShaderFormat
declare function SDL_ShaderCross_CompileDXBCFromHLSL(byval info as const SDL_ShaderCross_HLSL_Info ptr, byval size as uinteger ptr) as any ptr
declare function SDL_ShaderCross_CompileDXILFromHLSL(byval info as const SDL_ShaderCross_HLSL_Info ptr, byval size as uinteger ptr) as any ptr
declare function SDL_ShaderCross_CompileSPIRVFromHLSL(byval info as const SDL_ShaderCross_HLSL_Info ptr, byval size as uinteger ptr) as any ptr

end extern

'' End of SDL_shadercross.bi
