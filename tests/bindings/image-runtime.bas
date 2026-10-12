'' FreeBASIC native image qualification
'' File: image-runtime.bas
'' Purpose: Exercise the original loaders with real SDKs and export their pixels.
'' Responsibilities: Original functions, owned images, bounds and repeated failures.
'' This file contains no substitute decoder or omitted loader implementation.
#define IMAGE_EXAMPLE_NO_MAIN
#include once "windows.bi"
#include once "win/psapi.bi"
#include once "../../examples/files/jpeglib/example.bas"
#if __FB_LANG__ = "fb"
#include once "../../examples/files/FreeImage/fb.bas"
#include once "../../examples/manual/extras/freeimage.bas"
#ifdef IMAGE_CHECK_GD
#include once "gd.bi"
namespace gd_example
#include once "../../examples/files/gd/gd.bas"
end namespace
#endif
#endif

private function write_pixels(byval image as any ptr, byref output_file as string) as integer
	dim w as long, h as long, bypp as long, pitch as long
	dim pixels as any ptr
	if imageinfo(image, w, h, bypp, pitch, pixels) <> 0 then return 1
	if w <= 0 or h <= 0 or bypp <> 4 or pixels = NULL or pitch < w * 4 then return 1
	dim file_number as integer = freefile
	if open(output_file, for binary, access write, as #file_number) <> 0 then return 1
	dim status as integer = put(#file_number, , w)
	if status = 0 then status = put(#file_number, , h)
	for row_index as integer = 0 to h-1
		if status <> 0 then exit for
		dim row_data as ubyte ptr = cast(ubyte ptr, pixels) + row_index * pitch
		status = put(#file_number, , *row_data, w * 4)
	next
	if close(#file_number) <> 0 then status = 1
	return status <> 0
end function

private function private_bytes() as uinteger
	dim counters as PROCESS_MEMORY_COUNTERS_EX
	counters.cb = sizeof(counters)
	if GetProcessMemoryInfo(GetCurrentProcess(), cast(PROCESS_MEMORY_COUNTERS ptr, @counters), sizeof(counters)) = 0 then end 2
	return counters.PrivateUsage
end function

dim family as string = command(1), filename as string = command(2), output_file as string = command(3)
'' Header failures return before image allocation. Measure that decoder path
'' without unrelated asynchronous window/font initialization changing handles.
if family <> "failure-loop" and family <> "gd" then
	if screenres(64, 64, 32) <> 0 then end 2
end if
dim image as any ptr
select case family
case "jpeg"
	image = imageread_jpg(filename, 32)
#if __FB_LANG__ = "fb"
case "freeimage"
	FreeImage_Initialise()
	image = loadImageFile(filename)
case "manual"
	FreeImage_Initialise()
	image = FI_Load(filename)
#ifdef IMAGE_CHECK_GD
case "gd"
	dim native_image as gdImagePtr = gdImageCreateTrueColor(32, 16)
	if native_image = NULL then end 1
	gdImageFilledRectangle(native_image, 0, 0, 15, 15, gdTrueColor(240, 30, 40))
	gdImageFilledRectangle(native_image, 16, 0, 31, 15, gdTrueColor(20, 190, 70))
	dim status as integer
	if gd_example.saveImageAs(native_image, output_file+".png", gd_example.FORMAT_PNG) = 0 then status = 1
	if gd_example.saveImageAs(native_image, output_file+".gif", gd_example.FORMAT_GIF) = 0 then status = 1
	if gd_example.saveImageAs(native_image, output_file+".jpg", gd_example.FORMAT_JPEG) = 0 then status = 1
	dim invalid_file as string = output_file+"/missing/out.png"
	if gd_example.saveImageAs(native_image, invalid_file, gd_example.FORMAT_PNG) <> 0 then status = 1
	if gd_example.saveImageAs(NULL, output_file, gd_example.FORMAT_PNG) <> 0 then status = 1
	if gd_example.saveImageAs(native_image, output_file, -1) <> 0 then status = 1
	gdImageDestroy(native_image)
	end status
#endif
#endif
case "failure-loop"
	dim handles_before as ulong, handles_after as ulong
	'' Warm both native decoder allocation paths before measuring private memory.
	for warm_cycle as integer = 1 to 100
		image = imageread_jpg(filename, 32)
		if image <> NULL then
			imagedestroy(image)
			end 1
		end if
	next
	if GetProcessHandleCount(GetCurrentProcess(), @handles_before) = 0 then end 2
	dim memory_before as uinteger = private_bytes()
	for cycle as integer = 1 to 500
		image = imageread_jpg(filename, 32)
		if image <> NULL then
			imagedestroy(image)
			end 1
		end if
	next
	dim memory_after as uinteger = private_bytes()
	if GetProcessHandleCount(GetCurrentProcess(), @handles_after) = 0 then end 2
	dim loop_status as integer = abs(handles_after <> handles_before or memory_after > memory_before+1024*1024)
	dim report_number as integer = freefile
	if open(output_file, for output, as #report_number) <> 0 then end 2
	print #report_number, "JPEG_FAILURE_LOOP="; iif(loop_status=0,"PASS","FAIL"); " cycles=500 handles="; handles_before; " -> "; handles_after; _
		" private="; memory_before; " -> "; memory_after
	close #report_number
	end loop_status
case else
	end 2
end select
dim status as integer = 1
if image <> NULL then
	status = write_pixels(image, output_file)
	imagedestroy(image)
end if
#if __FB_LANG__ = "fb"
if family = "freeimage" or family = "manual" then FreeImage_DeInitialise()
#endif
end status
'' end of image-runtime.bas
