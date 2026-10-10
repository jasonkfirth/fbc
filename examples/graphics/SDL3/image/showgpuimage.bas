'' Project: FreeBASIC SDL3 examples
'' File: showgpuimage.bas
'' Purpose: Port upstream SDL3_image-3.4.8/examples/showgpuimage.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' showgpuimage:  A test application for the SDL image loading library.
'' Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>
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
'' claim that you wrote the original software. If you use this software
'' in a product, an acknowledgment in the product documentation would be
'' appreciated but is not required.
'' 2. Altered source versions must be plainly marked as such, and must not be
'' misrepresented as being the original software.
'' 3. This notice may not be removed or altered from any source distribution.

#include once "SDL3/SDL.bi"
#include once "smoke.bi"
#include once "SDL3/SDL_image.bi"

dim shared window_ as SDL_Window ptr
dim shared device_ as SDL_GPUDevice ptr
dim shared texture as SDL_GPUTexture ptr
dim shared texture_width as long
dim shared texture_height as long

declare function get_file_path cdecl(byval file_ as const zstring ptr) as const zstring ptr
declare function load_image cdecl(byval command_buffer as SDL_GPUCommandBuffer ptr, byval path as const zstring ptr) as boolean
declare function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long

function get_file_path cdecl(byval file_ as const zstring ptr) as const zstring ptr
	scope
		static path(0 to 4095) as byte
		if (((*file_) <> 47) andalso (SDL_GetPathInfo(file_, cptr(SDL_PathInfo ptr, 0)) = 0)) then
			scope
				SDL_snprintf(@path(0), (sizeof(byte) * 4096), strptr("%s%s"), SDL_GetBasePath(), file_)
				if SDL_GetPathInfo(@path(0), cptr(SDL_PathInfo ptr, 0)) then
					scope
						return @path(0)
					end scope
				end if
			end scope
		end if
		return file_
	end scope
end function

function load_image cdecl(byval command_buffer as SDL_GPUCommandBuffer ptr, byval path as const zstring ptr) as boolean
	scope
		SDL_ReleaseGPUTexture(device_, texture)
		texture = cptr(SDL_GPUTexture ptr, 0)
		dim copy_pass as SDL_GPUCopyPass ptr = SDL_BeginGPUCopyPass(command_buffer)
		if (copy_pass = 0) then
			scope
				SDL_Log_(strptr("SDL_BeginGPUCopyPass() failed: %s"), SDL_GetError())
				return false
			end scope
		end if
		texture = IMG_LoadGPUTexture(device_, copy_pass, get_file_path(path), @(texture_width), @(texture_height))
		if (texture = 0) then
			scope
				SDL_Log_(strptr("IMG_LoadGPUTexture() failed: %s"), SDL_GetError())
			end scope
		end if
		SDL_EndGPUCopyPass(copy_pass)
		return cast(boolean, (texture <> cptr(SDL_GPUTexture ptr, (cptr(any ptr, 0)))))
	end scope
end function

function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long
	scope
		dim result as long = 0
		dim quit as boolean = false
		dim command_buffer as SDL_GPUCommandBuffer ptr = cptr(SDL_GPUCommandBuffer ptr, 0)
		dim event as SDL_Event = type<SDL_Event>(0)
		dim swapchain_texture as SDL_GPUTexture ptr = cptr(SDL_GPUTexture ptr, 0)
		dim swapchain_width as Uint32 = 0
		dim swapchain_height as Uint32 = 0
		dim blit_info as SDL_GPUBlitInfo
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				SDL_Log_(strptr("SDL_Init(SDL_INIT_VIDEO) failed: %s"), SDL_GetError())
				result = 2
				goto done
			end scope
		end if
		window_ = SDL_CreateWindow(strptr(""), 960, 720, SDL_WINDOW_RESIZABLE)
		if (window_ = 0) then
			scope
				SDL_Log_(strptr("SDL_CreateWindow() failed: %s"), SDL_GetError())
				result = 2
				goto done
			end scope
		end if
		device_ = SDL_CreateGPUDevice((((SDL_GPU_SHADERFORMAT_SPIRV) or (SDL_GPU_SHADERFORMAT_DXIL)) or (SDL_GPU_SHADERFORMAT_MSL)), true, cptr(const zstring ptr, 0))
		if (window_ = 0) then
			scope
				SDL_Log_(strptr("SDL_CreateGPUDevice() failed: %s"), SDL_GetError())
				result = 2
				goto done
			end scope
		end if
		if (SDL_ClaimWindowForGPUDevice(device_, window_) = 0) then
			scope
				SDL_Log_(strptr("SDL_ClaimWindowForGPUDevice() failed: %s"), SDL_GetError())
				result = 2
				goto done
			end scope
		end if
		if (argc > 1) then
			scope
				dim command_buffer as SDL_GPUCommandBuffer ptr = SDL_AcquireGPUCommandBuffer(device_)
				if (command_buffer = 0) then
					scope
						SDL_Log_(strptr("SDL_AcquireGPUCommandBuffer() failed: %s"), SDL_GetError())
						result = 2
						goto done
					end scope
				end if
				if (load_image(command_buffer, argv[1]) = 0) then
					scope
						result = 2
						goto done
					end scope
				end if
				SDL_SubmitGPUCommandBuffer(command_buffer)
				SDL3_ExampleSmokeFrame()
			end scope
		end if
		do
			if ((quit = 0)) = 0 then exit do
			scope
				command_buffer = SDL_AcquireGPUCommandBuffer(device_)
				if (command_buffer = 0) then
					scope
						SDL_Log_(strptr("SDL_AcquireGPUCommandBuffer() failed: %s"), SDL_GetError())
						goto loop_continue_1
					end scope
				end if
				do
					if (SDL_PollEvent(@(event))) = 0 then exit do
					scope
						select case event.type
							case SDL_EVENT_QUIT
								goto switch_case_3
							case SDL_EVENT_DROP_FILE
								goto switch_case_4
							case else
								goto switch_done_5
						end select
						switch_case_3:
						scope
							quit = true
							goto switch_done_5
						end scope
						switch_case_4:
						scope
							load_image(command_buffer, event.drop.data)
							goto switch_done_5
						end scope
						switch_done_5:
					end scope
				loop
				if quit then
					scope
						exit do
					end scope
				end if
				if (SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window_, @(swapchain_texture), @(swapchain_width), @(swapchain_height)) = 0) then
					scope
						SDL_Log_(strptr("SDL_WaitAndAcquireGPUSwapchainTexture() failed: %s"), SDL_GetError())
						SDL_CancelGPUCommandBuffer(command_buffer)
						goto loop_continue_1
					end scope
				end if
				if (((swapchain_texture = 0) orelse (swapchain_width = 0)) orelse (swapchain_height = 0)) then
					scope
						'' Not an error. Happens on minimize
						SDL_CancelGPUCommandBuffer(command_buffer)
						goto loop_continue_1
					end scope
				end if
				if texture then
					scope
						SDL_memset(cptr(any ptr, @((blit_info))), 0, sizeof(((blit_info))))
						blit_info.source.texture = texture
						blit_info.source.w = texture_width
						blit_info.source.h = texture_height
						blit_info.destination.texture = swapchain_texture
						blit_info.destination.w = swapchain_width
						blit_info.destination.h = swapchain_height
						SDL_BlitGPUTexture(command_buffer, @(blit_info))
					end scope
				end if
				SDL_SubmitGPUCommandBuffer(command_buffer)
				SDL3_ExampleSmokeFrame()
			end scope
			loop_continue_1:
		loop
		done:
		SDL_ReleaseGPUTexture(device_, texture)
		SDL_ReleaseWindowFromGPUDevice(device_, window_)
		SDL_DestroyGPUDevice(device_)
		SDL_DestroyWindow(window_)
		SDL_Quit()
		return result
	end scope
end function

end SDL_RunApp(__FB_ARGC__, __FB_ARGV__, @example_main, 0)

'' end of showgpuimage.bas
