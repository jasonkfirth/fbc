'' Project: FreeBASIC SDL3 examples
'' File: testgputext.bas
'' Purpose: Port upstream SDL3_ttf-3.2.2/examples/testgputext.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.

'' Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>
''
'' This software is provided 'as-is', without any express or implied
'' warranty.  In no event will the authors be held liable for any damages
'' arising from the use of this software.
''
'' Permission is granted to anyone to use this software for any purpose,
'' including commercial applications, and to alter it and redistribute it
'' freely, subject to the following restrictions:
''
'' 1. The origin of this software must not be misrepresented; you must not
''    claim that you wrote the original software. If you use this software
''    in a product, an acknowledgment in the product documentation would be
''    appreciated but is not required.
'' 2. Altered source versions must be plainly marked as such, and must not be
''    misrepresented as being the original software.
'' 3. This notice may not be removed or altered from any source distribution.

#include once "SDL3/SDL.bi"
#include once "smoke.bi"
#include once "SDL3/SDL_ttf.bi"

#define MAX_VERTEX_COUNT 4000
#define MAX_INDEX_COUNT 6000
#define SUPPORTED_SHADER_FORMATS (SDL_GPU_SHADERFORMAT_SPIRV or SDL_GPU_SHADERFORMAT_DXIL or SDL_GPU_SHADERFORMAT_MSL)

#include once "testgputext/shader-data.bi"

type SDL_Vec2
	x as single
	y as single
end type

type SDL_Vec3
	x as single
	y as single
	z as single
end type

type SDL_Vec4
	x as single
	y as single
	z as single
	w as single
end type

type SDL_Mat4Values
	m00 as single
	m01 as single
	m02 as single
	m03 as single
	m10 as single
	m11 as single
	m12 as single
	m13 as single
	m20 as single
	m21 as single
	m22 as single
	m23 as single
	m30 as single
	m31 as single
	m32 as single
	m33 as single
end type

'' SDL's GPU uniforms use column-major storage. BASIC and C keep the last
'' array index contiguous; v names the same 16 floats as the array view.
type SDL_Mat4X4
	union
		v as SDL_Mat4Values
		m(0 to 3, 0 to 3) as single
	end union
end type

enum Shader
	VertexShader
	PixelShader
	PixelShader_SDF
end enum

type Vec2 as SDL_FPoint
type Vec3
	x as single
	y as single
	z as single
end type

type Vertex
	pos_ as Vec3
	colour as SDL_FColor
	uv as Vec2
end type

type Context
	device_ as SDL_GPUDevice ptr
	window_ as SDL_Window ptr
	pipeline as SDL_GPUGraphicsPipeline ptr
	vertex_buffer as SDL_GPUBuffer ptr
	index_buffer as SDL_GPUBuffer ptr
	transfer_buffer as SDL_GPUTransferBuffer ptr
	sampler as SDL_GPUSampler ptr
	cmd_buf as SDL_GPUCommandBuffer ptr
end type

type GeometryData
	vertices as Vertex ptr
	vertex_count as long
	indices as long ptr
	index_count as long
end type

declare function SDL_Vector3 cdecl(byval x as single, byval y as single, byval z as single) as SDL_Vec3
declare function SDL_Vec3Magnitude cdecl(byval vec as SDL_Vec3) as single
declare function SDL_Vec3Normalize cdecl(byval vec as SDL_Vec3) as SDL_Vec3
declare function SDL_Vec3Add cdecl(byval vec1 as SDL_Vec3, byval vec2 as SDL_Vec3) as SDL_Vec3
declare function SDL_Vec3Sub cdecl(byval vec1 as SDL_Vec3, byval vec2 as SDL_Vec3) as SDL_Vec3
declare function SDL_Vec3MultiplyFloat cdecl(byval vec as SDL_Vec3, byval val_ as single) as SDL_Vec3
declare function SDL_Vec3Dot cdecl(byval vec1 as SDL_Vec3, byval vec2 as SDL_Vec3) as single
declare function SDL_Vec3Cross cdecl(byval vec1 as SDL_Vec3, byval vec2 as SDL_Vec3) as SDL_Vec3
declare function SDL_Matrix4X4 cdecl(byval m00 as single, byval m10 as single, byval m20 as single, byval m30 as single, byval m01 as single, byval m11 as single, byval m21 as single, byval m31 as single, byval m02 as single, byval m12 as single, byval m22 as single, byval m32 as single, byval m03 as single, byval m13 as single, byval m23 as single, byval m33 as single) as SDL_Mat4X4
declare function SDL_MatrixIdentity cdecl() as SDL_Mat4X4
declare function SDL_MatrixTranspose cdecl(byval mat as SDL_Mat4X4) as SDL_Mat4X4
declare function SDL_MatrixMultiply cdecl(byval mat1 as SDL_Mat4X4, byval mat2 as SDL_Mat4X4) as SDL_Mat4X4
declare function SDL_MatrixScaling cdecl(byval scale as SDL_Vec3) as SDL_Mat4X4
declare function SDL_MatrixTranslation cdecl(byval offset as SDL_Vec3) as SDL_Mat4X4
declare function SDL_MatrixRotationX cdecl(byval angle as single) as SDL_Mat4X4
declare function SDL_MatrixRotationY cdecl(byval angle as single) as SDL_Mat4X4
declare function SDL_MatrixRotationZ cdecl(byval angle as single) as SDL_Mat4X4
declare function SDL_MatrixOrtho cdecl(byval left_samples as single, byval right_samples as single, byval bottom as single, byval top as single, byval near as single, byval far as single) as SDL_Mat4X4
declare function SDL_MatrixPerspective cdecl(byval fovy as single, byval aspect_ratio as single, byval near as single, byval far as single) as SDL_Mat4X4
declare function SDL_MatrixLookAt cdecl(byval pos_ as SDL_Vec3, byval target as SDL_Vec3, byval up as SDL_Vec3) as SDL_Mat4X4
declare sub check_error_bool cdecl(byval res as boolean)
declare function check_error_ptr cdecl(byval ptr_ as any ptr) as any ptr
declare function load_shader cdecl(byval device_ as SDL_GPUDevice ptr, byval shader as Shader, byval sampler_count as Uint32, byval uniform_buffer_count as Uint32, byval storage_buffer_count as Uint32, byval storage_texture_count as Uint32) as SDL_GPUShader ptr
declare sub queue_text_sequence cdecl(byval geometry_data as GeometryData ptr, byval sequence as TTF_GPUAtlasDrawSequence ptr, byval colour as SDL_FColor ptr)
declare sub queue_text cdecl(byval geometry_data as GeometryData ptr, byval sequence as TTF_GPUAtlasDrawSequence ptr, byval colour as SDL_FColor ptr)
declare sub set_geometry_data cdecl(byval context as Context ptr, byval geometry_data as GeometryData ptr)
declare sub transfer_data cdecl(byval context as Context ptr, byval geometry_data as GeometryData ptr)
declare sub draw_ cdecl(byval context as Context ptr, byval matrices as SDL_Mat4X4 ptr, byval num_matrices as long, byval draw_sequence as TTF_GPUAtlasDrawSequence ptr)
declare sub free_context cdecl(byval context as Context ptr)
declare function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long

function SDL_Vector3 cdecl(byval x as single, byval y as single, byval z as single) as SDL_Vec3
	return type<SDL_Vec3>(x, y, z)
end function

function SDL_Vec3Magnitude cdecl(byval vec as SDL_Vec3) as single
	return SDL_sqrtf((((vec.x * vec.x) + (vec.y * vec.y)) + (vec.z * vec.z)))
end function

function SDL_Vec3Normalize cdecl(byval vec as SDL_Vec3) as SDL_Vec3
	dim mag as single = SDL_Vec3Magnitude(vec)
	if (mag = 0) then
		return type<SDL_Vec3>(0, 0, 0)
	else
		if (mag = 1) then
			return vec
		else
			return type<SDL_Vec3>((vec.x / mag), (vec.y / mag), (vec.z / mag))
		end if
	end if
end function

function SDL_Vec3Add cdecl(byval vec1 as SDL_Vec3, byval vec2 as SDL_Vec3) as SDL_Vec3
	return SDL_Vector3((vec1.x + vec2.x), (vec1.y + vec2.y), (vec1.z + vec2.z))
end function

function SDL_Vec3Sub cdecl(byval vec1 as SDL_Vec3, byval vec2 as SDL_Vec3) as SDL_Vec3
	return SDL_Vector3((vec1.x - vec2.x), (vec1.y - vec2.y), (vec1.z - vec2.z))
end function

function SDL_Vec3MultiplyFloat cdecl(byval vec as SDL_Vec3, byval val_ as single) as SDL_Vec3
	return SDL_Vector3((vec.x * val_), (vec.y * val_), (vec.z * val_))
end function

function SDL_Vec3Dot cdecl(byval vec1 as SDL_Vec3, byval vec2 as SDL_Vec3) as single
	return ((((vec1.x * vec2.x) + (vec1.y * vec2.y)) + (vec1.z * vec2.z)))
end function

function SDL_Vec3Cross cdecl(byval vec1 as SDL_Vec3, byval vec2 as SDL_Vec3) as SDL_Vec3
	return SDL_Vector3(((vec1.y * vec2.z) - (vec1.z * vec2.y)), ((vec1.z * vec2.x) - (vec1.x * vec2.z)), ((vec1.x * vec2.y) - (vec1.y * vec2.x)))
end function

function SDL_Matrix4X4 cdecl(byval m00 as single, byval m10 as single, byval m20 as single, byval m30 as single, byval m01 as single, byval m11 as single, byval m21 as single, byval m31 as single, byval m02 as single, byval m12 as single, byval m22 as single, byval m32 as single, byval m03 as single, byval m13 as single, byval m23 as single, byval m33 as single) as SDL_Mat4X4
	dim result as SDL_Mat4X4
	result.v.m00 = m00
	result.v.m01 = m01
	result.v.m02 = m02
	result.v.m03 = m03
	result.v.m10 = m10
	result.v.m11 = m11
	result.v.m12 = m12
	result.v.m13 = m13
	result.v.m20 = m20
	result.v.m21 = m21
	result.v.m22 = m22
	result.v.m23 = m23
	result.v.m30 = m30
	result.v.m31 = m31
	result.v.m32 = m32
	result.v.m33 = m33
	return result
end function

function SDL_MatrixIdentity cdecl() as SDL_Mat4X4
	return SDL_Matrix4X4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
end function

function SDL_MatrixTranspose cdecl(byval mat as SDL_Mat4X4) as SDL_Mat4X4
	dim res as SDL_Mat4X4
	scope
		dim i as long = 0
		do while (i < 4)
			scope
				dim j as long = 0
				do while (j < 4)
					res.m(j, i) = mat.m(i, j)
					j += 1
				loop
			end scope
			i += 1
		loop
	end scope
	return res
end function

function SDL_MatrixMultiply cdecl(byval mat1 as SDL_Mat4X4, byval mat2 as SDL_Mat4X4) as SDL_Mat4X4
	dim res as SDL_Mat4X4
	scope
		dim i as long = 0
		do while (i < 4)
			scope
				dim j as long = 0
				do while (j < 4)
					dim sum as single = 0
					scope
						dim x as long = 0
						do while (x < 4)
							sum += ((@mat1.m(0, 0) + (x) * 4)[j] * (@mat2.m(0, 0) + (i) * 4)[x])
							x += 1
						loop
					end scope
					(@res.m(0, 0) + (i) * 4)[j] = sum
					j += 1
				loop
			end scope
			i += 1
		loop
	end scope
	return res
end function

function SDL_MatrixScaling cdecl(byval scale as SDL_Vec3) as SDL_Mat4X4
	dim x as single = scale.x
	dim y as single = scale.y
	dim z as single = scale.z
	return SDL_Matrix4X4(x, 0, 0, 0, 0, y, 0, 0, 0, 0, z, 0, 0, 0, 0, 1)
end function

function SDL_MatrixTranslation cdecl(byval offset as SDL_Vec3) as SDL_Mat4X4
	return SDL_Matrix4X4(1, 0, 0, offset.x, 0, 1, 0, offset.y, 0, 0, 1, offset.z, 0, 0, 0, 1)
end function

function SDL_MatrixRotationX cdecl(byval angle as single) as SDL_Mat4X4
	dim cos_ as single = cast(single, SDL_cos(cast(double, angle)))
	dim sin_ as single = cast(single, SDL_sin(cast(double, angle)))
	return SDL_Matrix4X4(1, 0, 0, 0, 0, cos_, (-sin_), 0, 0, sin_, cos_, 0, 0, 0, 0, 1)
end function

function SDL_MatrixRotationY cdecl(byval angle as single) as SDL_Mat4X4
	dim cos_ as single = cast(single, SDL_cos(cast(double, angle)))
	dim sin_ as single = cast(single, SDL_sin(cast(double, angle)))
	return SDL_Matrix4X4(cos_, 0, sin_, 0, 0, 1, 0, 0, (-sin_), 0, cos_, 0, 0, 0, 0, 1)
end function

function SDL_MatrixRotationZ cdecl(byval angle as single) as SDL_Mat4X4
	dim cos_ as single = cast(single, SDL_cos(cast(double, angle)))
	dim sin_ as single = cast(single, SDL_sin(cast(double, angle)))
	return SDL_Matrix4X4(cos_, (-sin_), 0, 0, sin_, cos_, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
end function

function SDL_MatrixOrtho cdecl(byval left_samples as single, byval right_samples as single, byval bottom as single, byval top as single, byval near as single, byval far as single) as SDL_Mat4X4
	dim l as single = left_samples
	dim r as single = right_samples
	dim b as single = bottom
	dim t as single = top
	dim n as single = near
	dim f as single = far
	dim dx as single = ((-((r + l))) / ((r - l)))
	dim dy_ as single = ((-((t + b))) / ((t - b)))
	dim dz as single = ((-((f + n))) / ((f - n)))
	return SDL_Matrix4X4((2 / ((r - l))), 0, 0, dx, 0, (2 / ((t - b))), 0, dy_, 0, 0, (2 / ((f - n))), dz, 0, 0, 0, 1)
end function

function SDL_MatrixPerspective cdecl(byval fovy as single, byval aspect_ratio as single, byval near as single, byval far as single) as SDL_Mat4X4
	dim n as single = near
	dim f as single = far
	dim t as single = (SDL_tanf((fovy / 2.0f)) * n)
	dim b as single = (-t)
	dim r as single = (t * aspect_ratio)
	dim l as single = (-r)
	return SDL_Matrix4X4((((2 * n)) / ((r - l))), 0, (((r + l)) / ((r - l))), 0, 0, (((2 * n)) / ((t - b))), (((t + b)) / ((t - b))), 0, 0, 0, ((-((f + n))) / ((f - n))), ((-(((2 * n) * f))) / ((f - n))), 0, 0, (-1), 1)
end function

function SDL_MatrixLookAt cdecl(byval pos_ as SDL_Vec3, byval target as SDL_Vec3, byval up as SDL_Vec3) as SDL_Mat4X4
	dim d as SDL_Vec3 = SDL_Vec3Normalize(SDL_Vec3Sub(target, pos_))
	dim u as SDL_Vec3 = SDL_Vec3Normalize(up)
	dim r as SDL_Vec3 = SDL_Vec3Normalize(SDL_Vec3Cross(u, d))
	u = SDL_Vec3Cross(r, d)
	return SDL_Matrix4X4(r.x, r.y, r.z, (-SDL_Vec3Dot(r, pos_)), u.x, u.y, u.z, (-SDL_Vec3Dot(u, pos_)), (-d.x), (-d.y), (-d.z), SDL_Vec3Dot(d, pos_), 0, 0, 0, 1)
end function

sub check_error_bool cdecl(byval res as boolean)
	if (res = 0) then
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, strptr("%s"), SDL_GetError())
	end if
end sub

function check_error_ptr cdecl(byval ptr_ as any ptr) as any ptr
	if (ptr_ = 0) then
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, strptr("%s"), SDL_GetError())
	end if
	return ptr_
end function

function load_shader cdecl(byval device_ as SDL_GPUDevice ptr, byval shader as Shader, byval sampler_count as Uint32, byval uniform_buffer_count as Uint32, byval storage_buffer_count as Uint32, byval storage_texture_count as Uint32) as SDL_GPUShader ptr
	dim createinfo as SDL_GPUShaderCreateInfo
	createinfo.num_samplers = sampler_count
	createinfo.num_storage_buffers = storage_buffer_count
	createinfo.num_storage_textures = storage_texture_count
	createinfo.num_uniform_buffers = uniform_buffer_count
	createinfo.props = 0
	dim format_ as SDL_GPUShaderFormat = SDL_GetGPUShaderFormats(device_)
	if (format_ and (SDL_GPU_SHADERFORMAT_DXIL)) then
		createinfo.format = (SDL_GPU_SHADERFORMAT_DXIL)
		select case shader
			case (VertexShader)
				goto switch_case_6
			case (PixelShader)
				goto switch_case_7
			case (PixelShader_SDF)
				goto switch_case_8
			case else
				goto switch_done_9
		end select
		switch_case_6:
		scope
			createinfo.code = @shader_vert_dxil(0)
			createinfo.code_size = shader_vert_dxil_len
			createinfo.entrypoint = strptr("VSMain")
			goto switch_done_9
		end scope
		switch_case_7:
		scope
			createinfo.code = @shader_frag_dxil(0)
			createinfo.code_size = shader_frag_dxil_len
			createinfo.entrypoint = strptr("PSMain")
			goto switch_done_9
		end scope
		switch_case_8:
		scope
			createinfo.code = @shader_sdf_frag_dxil(0)
			createinfo.code_size = shader_sdf_frag_dxil_len
			createinfo.entrypoint = strptr("PSMain")
			goto switch_done_9
		end scope
		switch_done_9:
	else
		if (format_ and (SDL_GPU_SHADERFORMAT_MSL)) then
			createinfo.format = (SDL_GPU_SHADERFORMAT_MSL)
			select case shader
				case (VertexShader)
					goto switch_case_10
				case (PixelShader)
					goto switch_case_11
				case (PixelShader_SDF)
					goto switch_case_12
				case else
					goto switch_done_13
			end select
			switch_case_10:
			scope
				createinfo.code = @shader_vert_msl(0)
				createinfo.code_size = shader_vert_msl_len
				createinfo.entrypoint = strptr("main0")
				goto switch_done_13
			end scope
			switch_case_11:
			scope
				createinfo.code = @shader_frag_msl(0)
				createinfo.code_size = shader_frag_msl_len
				createinfo.entrypoint = strptr("main0")
				goto switch_done_13
			end scope
			switch_case_12:
			scope
				createinfo.code = @shader_sdf_frag_msl(0)
				createinfo.code_size = shader_sdf_frag_msl_len
				createinfo.entrypoint = strptr("main0")
				goto switch_done_13
			end scope
			switch_done_13:
		else
			createinfo.format = (SDL_GPU_SHADERFORMAT_SPIRV)
			select case shader
				case (VertexShader)
					goto switch_case_14
				case (PixelShader)
					goto switch_case_15
				case (PixelShader_SDF)
					goto switch_case_16
				case else
					goto switch_done_17
			end select
			switch_case_14:
			scope
				createinfo.code = @shader_vert_spv(0)
				createinfo.code_size = shader_vert_spv_len
				createinfo.entrypoint = strptr("main")
				goto switch_done_17
			end scope
			switch_case_15:
			scope
				createinfo.code = @shader_frag_spv(0)
				createinfo.code_size = shader_frag_spv_len
				createinfo.entrypoint = strptr("main")
				goto switch_done_17
			end scope
			switch_case_16:
			scope
				createinfo.code = @shader_sdf_frag_spv(0)
				createinfo.code_size = shader_sdf_frag_spv_len
				createinfo.entrypoint = strptr("main")
				goto switch_done_17
			end scope
			switch_done_17:
		end if
	end if
	if (shader = VertexShader) then
		createinfo.stage = SDL_GPU_SHADERSTAGE_VERTEX
	else
		createinfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT
	end if
	return SDL_CreateGPUShader(device_, @(createinfo))
end function

sub queue_text_sequence cdecl(byval geometry_data as GeometryData ptr, byval sequence as TTF_GPUAtlasDrawSequence ptr, byval colour as SDL_FColor ptr)
	scope
		dim i as long = 0
		do while (i < sequence->num_vertices)
			dim vert as Vertex
			dim pos_ as SDL_FPoint = sequence->xy[i]
			vert.pos_ = type<Vec3>(pos_.x, pos_.y, 0.0f)
			vert.colour = (*colour)
			vert.uv = sequence->uv[i]
			geometry_data->vertices[(geometry_data->vertex_count + i)] = vert
			i += 1
		loop
	end scope
	SDL_memcpy(cptr(any ptr, (geometry_data->indices + geometry_data->index_count)), cptr(const any ptr, sequence->indices), (sequence->num_indices * sizeof(long)))
	geometry_data->vertex_count += sequence->num_vertices
	geometry_data->index_count += sequence->num_indices
end sub

sub queue_text cdecl(byval geometry_data as GeometryData ptr, byval sequence as TTF_GPUAtlasDrawSequence ptr, byval colour as SDL_FColor ptr)
	scope
		do while sequence
			queue_text_sequence(geometry_data, sequence, colour)
			sequence = sequence->next
		loop
	end scope
end sub

sub set_geometry_data cdecl(byval context as Context ptr, byval geometry_data as GeometryData ptr)
	dim mapped_vertices as Vertex ptr = cptr(Vertex ptr, SDL_MapGPUTransferBuffer(context->device_, context->transfer_buffer, false))
	SDL_memcpy(cptr(any ptr, mapped_vertices), cptr(const any ptr, geometry_data->vertices), (sizeof(Vertex) * geometry_data->vertex_count))
	SDL_memcpy(cptr(any ptr, (mapped_vertices + 4000)), cptr(const any ptr, geometry_data->indices), (sizeof(long) * geometry_data->index_count))
	SDL_UnmapGPUTransferBuffer(context->device_, context->transfer_buffer)
end sub

sub transfer_data cdecl(byval context as Context ptr, byval geometry_data as GeometryData ptr)
	dim copy_pass as SDL_GPUCopyPass ptr = cptr(SDL_GPUCopyPass ptr, check_error_ptr(cptr(any ptr, SDL_BeginGPUCopyPass(context->cmd_buf))))
	SDL_UploadToGPUBuffer(copy_pass, @(type<SDL_GPUTransferBufferLocation>(context->transfer_buffer, 0)), @(type<SDL_GPUBufferRegion>(context->vertex_buffer, 0, (sizeof(Vertex) * geometry_data->vertex_count))), false)
	SDL_UploadToGPUBuffer(copy_pass, @(type<SDL_GPUTransferBufferLocation>(context->transfer_buffer, (sizeof(Vertex) * 4000))), @(type<SDL_GPUBufferRegion>(context->index_buffer, 0, (sizeof(long) * geometry_data->index_count))), false)
	SDL_EndGPUCopyPass(copy_pass)
end sub

sub draw_ cdecl(byval context as Context ptr, byval matrices as SDL_Mat4X4 ptr, byval num_matrices as long, byval draw_sequence as TTF_GPUAtlasDrawSequence ptr)
	dim swapchain_texture as SDL_GPUTexture ptr
	check_error_bool(SDL_WaitAndAcquireGPUSwapchainTexture(context->cmd_buf, context->window_, @(swapchain_texture), cptr(Uint32 ptr, 0), cptr(Uint32 ptr, 0)))
	if (swapchain_texture <> cptr(SDL_GPUTexture ptr, (cptr(any ptr, 0)))) then
		dim colour_target_info as SDL_GPUColorTargetInfo
		colour_target_info.texture = swapchain_texture
		colour_target_info.clear_color = type<SDL_FColor>(0.300000012f, 0.400000006f, 0.5f, 1.0f)
		colour_target_info.load_op = SDL_GPU_LOADOP_CLEAR
		colour_target_info.store_op = SDL_GPU_STOREOP_STORE
		dim render_pass as SDL_GPURenderPass ptr = SDL_BeginGPURenderPass(context->cmd_buf, @(colour_target_info), 1, cptr(const SDL_GPUDepthStencilTargetInfo ptr, 0))
		SDL_BindGPUGraphicsPipeline(render_pass, context->pipeline)
		SDL_BindGPUVertexBuffers(render_pass, 0, @(type<SDL_GPUBufferBinding>(context->vertex_buffer, 0)), 1)
		SDL_BindGPUIndexBuffer(render_pass, @(type<SDL_GPUBufferBinding>(context->index_buffer, 0)), SDL_GPU_INDEXELEMENTSIZE_32BIT)
		SDL_PushGPUVertexUniformData(context->cmd_buf, 0, cptr(const any ptr, matrices), (sizeof(SDL_Mat4X4) * num_matrices))
		dim index_offset as long = 0
		dim vertex_offset as long = 0
		scope
			dim seq as TTF_GPUAtlasDrawSequence ptr = draw_sequence
			do while (seq <> cptr(TTF_GPUAtlasDrawSequence ptr, (cptr(any ptr, 0))))
				SDL_BindGPUFragmentSamplers(render_pass, 0, @(type<SDL_GPUTextureSamplerBinding>(seq->atlas_texture, context->sampler)), 1)
				SDL_DrawGPUIndexedPrimitives(render_pass, seq->num_indices, 1, index_offset, vertex_offset, 0)
				index_offset += seq->num_indices
				vertex_offset += seq->num_vertices
				seq = seq->next
			loop
		end scope
		SDL_EndGPURenderPass(render_pass)
	end if
end sub

sub free_context cdecl(byval context as Context ptr)
	SDL_ReleaseGPUTransferBuffer(context->device_, context->transfer_buffer)
	SDL_ReleaseGPUSampler(context->device_, context->sampler)
	SDL_ReleaseGPUBuffer(context->device_, context->vertex_buffer)
	SDL_ReleaseGPUBuffer(context->device_, context->index_buffer)
	SDL_ReleaseGPUGraphicsPipeline(context->device_, context->pipeline)
	SDL_ReleaseWindowFromGPUDevice(context->device_, context->window_)
	SDL_DestroyGPUDevice(context->device_)
	SDL_DestroyWindow(context->window_)
end sub

function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long
	dim font_filename as const zstring ptr = cptr(const zstring ptr, 0)
	dim use_SDF as boolean = false
	scope
		dim i as long = 1
		do while argv[i]
			if (SDL_strcasecmp(argv[i], strptr("--sdf")) = 0) then
				use_SDF = true
			else
				if ((*cptr(byte ptr, argv[i])) = 45) then
					exit do
				else
					font_filename = argv[i]
					exit do
				end if
			end if
			i += 1
		loop
	end scope
	if (font_filename = 0) then
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, strptr("Usage: testgputext [--sdf] FONT_FILENAME"))
		return 2
	end if
	check_error_bool(SDL_Init((SDL_INIT_VIDEO or SDL_INIT_EVENTS)))
	dim running as boolean = true
	dim context as Context = type<Context>(cptr(SDL_GPUDevice ptr, 0), cptr(SDL_Window ptr, 0), cptr(SDL_GPUGraphicsPipeline ptr, 0), cptr(SDL_GPUBuffer ptr, 0), cptr(SDL_GPUBuffer ptr, 0), cptr(SDL_GPUTransferBuffer ptr, 0), cptr(SDL_GPUSampler ptr, 0), cptr(SDL_GPUCommandBuffer ptr, 0))
	context.window_ = cptr(SDL_Window ptr, check_error_ptr(cptr(any ptr, SDL_CreateWindow(strptr("GPU text test"), 800, 600, 0))))
	context.device_ = cptr(SDL_GPUDevice ptr, check_error_ptr(cptr(any ptr, SDL_CreateGPUDevice((((((1u shl 1)) or ((1u shl 3))) or ((1u shl 4)))), true, cptr(const zstring ptr, 0)))))
	check_error_bool(SDL_ClaimWindowForGPUDevice(context.device_, context.window_))
	dim vertex_shader as SDL_GPUShader ptr = cptr(SDL_GPUShader ptr, check_error_ptr(cptr(any ptr, load_shader(context.device_, VertexShader, 0, 1, 0, 0))))
	dim fragment_shader as SDL_GPUShader ptr = cptr(SDL_GPUShader ptr, check_error_ptr(cptr(any ptr, load_shader(context.device_, iif(use_SDF, PixelShader_SDF, PixelShader), 1, 0, 0, 0))))
	'' The C sample uses compound array literals. Keep these descriptions in
	'' named local arrays so their addresses remain valid through pipeline creation.
	dim buffer_description(0 to 0) as SDL_GPUVertexBufferDescription
	buffer_description(0).slot = 0
	buffer_description(0).pitch = sizeof(Vertex)
	buffer_description(0).input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX
	dim attributes(0 to 2) as SDL_GPUVertexAttribute
	attributes(0) = type<SDL_GPUVertexAttribute>(0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0)
	attributes(1) = type<SDL_GPUVertexAttribute>(1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, sizeof(single) * 3)
	attributes(2) = type<SDL_GPUVertexAttribute>(2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, sizeof(single) * 7)
	dim target_description(0 to 0) as SDL_GPUColorTargetDescription
	target_description(0).format = SDL_GetGPUSwapchainTextureFormat(context.device_, context.window_)
	with target_description(0).blend_state
		.enable_blend = true
		.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA
		.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
		.color_blend_op = SDL_GPU_BLENDOP_ADD
		.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA
		.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_DST_ALPHA
		.alpha_blend_op = SDL_GPU_BLENDOP_ADD
	end with
	dim pipeline_create_info as SDL_GPUGraphicsPipelineCreateInfo
	pipeline_create_info.vertex_shader = vertex_shader
	pipeline_create_info.fragment_shader = fragment_shader
	pipeline_create_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST
	with pipeline_create_info.vertex_input_state
		.vertex_buffer_descriptions = @buffer_description(0)
		.num_vertex_buffers = 1
		.vertex_attributes = @attributes(0)
		.num_vertex_attributes = 3
	end with
	pipeline_create_info.target_info.num_color_targets = 1
	pipeline_create_info.target_info.color_target_descriptions = @target_description(0)

	context.pipeline = cptr(SDL_GPUGraphicsPipeline ptr, check_error_ptr(cptr(any ptr, SDL_CreateGPUGraphicsPipeline(context.device_, @(pipeline_create_info)))))
	SDL_ReleaseGPUShader(context.device_, vertex_shader)
	SDL_ReleaseGPUShader(context.device_, fragment_shader)
	dim vbf_info as SDL_GPUBufferCreateInfo = type<SDL_GPUBufferCreateInfo>((SDL_GPU_BUFFERUSAGE_VERTEX), (sizeof(Vertex) * 4000), 0)
	context.vertex_buffer = cptr(SDL_GPUBuffer ptr, check_error_ptr(cptr(any ptr, SDL_CreateGPUBuffer(context.device_, @(vbf_info)))))
	dim ibf_info as SDL_GPUBufferCreateInfo = type<SDL_GPUBufferCreateInfo>((SDL_GPU_BUFFERUSAGE_INDEX), (sizeof(long) * 6000), 0)
	context.index_buffer = cptr(SDL_GPUBuffer ptr, check_error_ptr(cptr(any ptr, SDL_CreateGPUBuffer(context.device_, @(ibf_info)))))
	dim tbf_info as SDL_GPUTransferBufferCreateInfo = type<SDL_GPUTransferBufferCreateInfo>(SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, (((sizeof(Vertex) * 4000)) + ((sizeof(long) * 6000))), 0)
	context.transfer_buffer = cptr(SDL_GPUTransferBuffer ptr, check_error_ptr(cptr(any ptr, SDL_CreateGPUTransferBuffer(context.device_, @(tbf_info)))))
	dim sampler_info as SDL_GPUSamplerCreateInfo
	sampler_info.min_filter = SDL_GPU_FILTER_LINEAR
	sampler_info.mag_filter = SDL_GPU_FILTER_LINEAR
	sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR
	sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
	sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
	sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
	context.sampler = cptr(SDL_GPUSampler ptr, check_error_ptr(cptr(any ptr, SDL_CreateGPUSampler(context.device_, @(sampler_info)))))
	dim geometry_data as GeometryData = type<GeometryData>(cptr(Vertex ptr, 0), 0, cptr(long ptr, 0), 0)
	geometry_data.vertices = cptr(Vertex ptr, SDL_calloc(4000, sizeof(Vertex)))
	geometry_data.indices = cptr(long ptr, SDL_calloc(6000, sizeof(long)))
	check_error_bool(TTF_Init())
	dim font as TTF_Font ptr = cptr(TTF_Font ptr, check_error_ptr(cptr(any ptr, TTF_OpenFont(font_filename, 50))))
	'' Preferably use a Monospaced font
	if (font = 0) then
		running = false
	end if
	SDL_Log_(strptr("SDF %s"), iif(use_SDF, strptr("enabled"), strptr("disabled")))
	TTF_SetFontSDF(font, use_SDF)
	TTF_SetFontWrapAlignment(font, TTF_HORIZONTAL_ALIGN_CENTER)
	dim engine as TTF_TextEngine ptr = cptr(TTF_TextEngine ptr, check_error_ptr(cptr(any ptr, TTF_CreateGPUTextEngine(context.device_))))
	dim str_ as zstring * 18 = !"     \nSDL is cool"
	dim text as TTF_Text ptr = cptr(TTF_Text ptr, check_error_ptr(cptr(any ptr, TTF_CreateText(engine, font, strptr(str_), 0))))
	dim matrices(0 to 1) as SDL_Mat4X4
	matrices(0) = SDL_MatrixPerspective(SDL_PI_F / 2.0f, 800.0f / 600.0f, 0.1f, 100.0f)
	matrices(1) = SDL_MatrixIdentity()
	dim rot_angle as single = 0
	dim colour as SDL_FColor = type<SDL_FColor>(1.0f, 1.0f, 0.0f, 1.0f)
	do
		if (running) = 0 then exit do
		dim event as SDL_Event
		do
			if (SDL_PollEvent(@(event))) = 0 then exit do
			select case event.type
				case SDL_EVENT_KEY_UP
					goto switch_case_24
				case SDL_EVENT_QUIT
					goto switch_case_25
				case else
					goto switch_done_26
			end select
			switch_case_24:
			scope
				if (event.key.key = 27u) then
					running = false
				end if
				goto switch_done_26
			end scope
			switch_case_25:
			scope
				running = false
				goto switch_done_26
			end scope
			switch_done_26:
		loop
		scope
			dim i as long = 0
			do while (i < 5)
				cptr(byte ptr, strptr(str_))[i] = (65 + SDL_rand(26))
				i += 1
			loop
		end scope
		TTF_SetTextString(text, strptr(str_), 0)
		dim tw as long
		dim th as long
		check_error_bool(TTF_GetTextSize(text, @(tw), @(th)))
		rot_angle = SDL_fmodf(cast(single, (cast(double, rot_angle) + 0.01)), (2 * SDL_PI_F))
		'' Create a model matrix to make the text rotate
		dim model as SDL_Mat4X4
		model = SDL_MatrixIdentity()
		model = SDL_MatrixMultiply(model, SDL_MatrixTranslation(type<SDL_Vec3>(0.0f, 0.0f, (-80.0f))))
		model = SDL_MatrixMultiply(model, SDL_MatrixScaling(type<SDL_Vec3>(0.300000012f, 0.300000012f, 0.300000012f)))
		model = SDL_MatrixMultiply(model, SDL_MatrixRotationY(rot_angle))
		model = SDL_MatrixMultiply(model, SDL_MatrixTranslation(type<SDL_Vec3>(((-tw) / 2.0f), (th / 2.0f), 0.0f)))
		matrices(1) = model
		'' Get the text data and queue the text in a buffer for drawing later
		dim sequence as TTF_GPUAtlasDrawSequence ptr = TTF_GetGPUTextDrawData(text)
		queue_text(@(geometry_data), sequence, @(colour))
		set_geometry_data(@(context), @(geometry_data))
		context.cmd_buf = cptr(SDL_GPUCommandBuffer ptr, check_error_ptr(cptr(any ptr, SDL_AcquireGPUCommandBuffer(context.device_))))
		transfer_data(@(context), @(geometry_data))
		draw_(@context, @matrices(0), 2, sequence)
		SDL_SubmitGPUCommandBuffer(context.cmd_buf)
		SDL3_ExampleSmokeFrame()
		geometry_data.vertex_count = 0
		geometry_data.index_count = 0
	loop
	SDL_free(cptr(any ptr, geometry_data.vertices))
	SDL_free(cptr(any ptr, geometry_data.indices))
	TTF_DestroyText(text)
	TTF_DestroyGPUTextEngine(engine)
	TTF_CloseFont(font)
	TTF_Quit()
	free_context(@(context))
	SDL_Quit()
	return 0
end function

end SDL_RunApp(__FB_ARGC__, __FB_ARGV__, @example_main, 0)

'' end of testgputext.bas
