'' Project: FreeBASIC SDL addon examples
'' File: gpu-common.bi
'' Purpose: Share a small SDL_gpu scene between its SDL1 and SDL2 builds.
'' Responsibilities: Check context creation, render primitives, and release the GPU target.
'' This file intentionally does NOT contain: shader compilation or image loading.

#pragma once
#include once "example-common.bi"
#if SDL_ADDON_API = 1
	#include once "SDL/SDL_gpu.bi"
#else
	#include once "SDL2/SDL_gpu.bi"
#endif

function main() as integer
	dim compiled as SDL_version = GPU_GetCompiledVersion()
	if compiled.major <> 0 or compiled.minor <> 12 then return 1
	dim target as GPU_Target ptr = GPU_Init(640, 480, 0)
	if target = 0 then
		example_error("GPU_Init failed")
		return 1
	end if
	do
		GPU_ClearRGBA(target, 15, 25, 40, 255)
		GPU_Circle(target, 320.0, 240.0, 100.0, type<SDL_Color>(255, 220, 0, 255))
		GPU_RectangleFilled(target, 70.0, 70.0, 180.0, 180.0, type<SDL_Color>(40, 180, 240, 255))
		GPU_Line(target, 100.0, 370.0, 540.0, 370.0, type<SDL_Color>(240, 80, 80, 255))
		GPU_Flip(target)
	loop until example_done()
	GPU_Quit()
	return 0
end function

'' End of gpu-common.bi
