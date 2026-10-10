'' Project: FreeBASIC SDL addon examples
'' File: shadercross3.bas
'' Purpose: Reflect and translate a SPIR-V vertex shader through SDL_shadercross.
'' Responsibilities: Validate metadata and release allocations through SDL_free.
'' This file intentionally does NOT contain: GPU device creation or shader execution.

#define SDL_ADDON_API 3
#include once "../../SDL_common/example-common.bi"
#include once "SDL3/SDL_shadercross.bi"

function main() as integer
	dim filename as string = command(1)
	if filename = "" then
		print "usage: shadercross3 vertex.spv"
		return 1
	end if
	if not SDL_ShaderCross_Init() then
		example_error("Shadercross initialization failed")
		return 1
	end if
	dim byte_count as uinteger
	dim shader_data as Uint8 ptr = SDL_LoadFile(strptr(filename), @byte_count)
	if shader_data = 0 then
		SDL_ShaderCross_Quit()
		return 1
	end if
	dim metadata as SDL_ShaderCross_GraphicsShaderMetadata ptr
	metadata = SDL_ShaderCross_ReflectGraphicsSPIRV(shader_data, byte_count, 0)
	if metadata = 0 then
		example_error("Shader reflection failed")
		SDL_free(shader_data)
		SDL_ShaderCross_Quit()
		return 1
	end if
	dim result as integer = 1
	dim info as SDL_ShaderCross_SPIRV_Info
	info.bytecode = shader_data
	info.bytecode_size = byte_count
	info.entrypoint = strptr("main")
	info.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_VERTEX
	dim hlsl as zstring ptr = SDL_ShaderCross_TranspileHLSLFromSPIRV(@info)
	if hlsl <> 0 then
		if metadata->num_inputs > 0 and len(*hlsl) > 0 then
			dim hlsl_info as SDL_ShaderCross_HLSL_Info
			hlsl_info.source = hlsl
			hlsl_info.entrypoint = strptr("main")
			hlsl_info.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_VERTEX
			dim compiled_size as uinteger
			dim compiled as any ptr = SDL_ShaderCross_CompileSPIRVFromHLSL(@hlsl_info, @compiled_size)
			if compiled <> 0 and compiled_size > 0 then
				print "Reflected inputs: "; metadata->num_inputs
				print "Compiled HLSL bytes: "; compiled_size
				result = 0
				SDL_free(compiled)
			end if
		end if
		SDL_free(hlsl)
	end if
	SDL_free(metadata)
	SDL_free(shader_data)
	SDL_ShaderCross_Quit()
	return result
end function

end main()

'' End of shadercross3.bas
