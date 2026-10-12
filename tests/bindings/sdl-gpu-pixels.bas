'' Project: FreeBASIC native library regression checks
'' File: sdl-gpu-pixels.bas
'' Purpose: Check SDL_gpu's rendered pixels through its framebuffer API.
'' Responsibilities: Exercise SDL1/SDL2 colors and release the copied surface.
'' This file intentionally contains no window-system screen capture.

#lang "fb"
#if SDL_ADDON_API = 1
	#include once "SDL/SDL_gpu.bi"
#else
	#include once "SDL2/SDL_gpu.bi"
#endif

Function pixel_matches(ByVal surface As SDL_Surface Ptr, ByVal x As Long, ByVal y As Long, _
	ByVal red As UByte, ByVal green As UByte, ByVal blue As UByte) As Boolean
	'' CopySurfaceFromTarget uses a 32-bit surface. Reject another format
	'' rather than interpreting its pixels with an incorrect byte stride.
	If surface->format = 0 OrElse surface->format->BytesPerPixel <> 4 Then Return False
	If x < 0 OrElse y < 0 OrElse x >= surface->w OrElse y >= surface->h Then Return False
	If surface->pixels = 0 OrElse surface->pitch < surface->w * 4 Then Return False
	Dim As ULong pixel = *CPtr(ULong Ptr, CPtr(UByte Ptr, surface->pixels) + y * surface->pitch + x * 4)
	Dim As UByte actual_red, actual_green, actual_blue
	SDL_GetRGB(pixel, surface->format, @actual_red, @actual_green, @actual_blue)
	Return actual_red = red AndAlso actual_green = green AndAlso actual_blue = blue
End Function

Dim As GPU_Target Ptr target = GPU_Init(640, 480, 0)
If target = 0 Then End 1
GPU_ClearRGBA(target, 15, 25, 40, 255)
GPU_RectangleFilled(target, 70, 70, 180, 180, Type<SDL_Color>(40, 180, 240, 255))
Dim As SDL_Surface Ptr surface = GPU_CopySurfaceFromTarget(target)
Dim As Integer exit_status = 1
If surface <> 0 Then
	If SDL_LockSurface(surface) = 0 Then
		If pixel_matches(surface, 20, 20, 15, 25, 40) AndAlso _
			pixel_matches(surface, 100, 100, 40, 180, 240) Then exit_status = 0
		SDL_UnlockSurface(surface)
	End If
	SDL_FreeSurface(surface)
End If
GPU_Quit()
If exit_status = 0 Then Print "SDL_GPU_PIXELS_PASS"
End exit_status

'' end of sdl-gpu-pixels.bas
