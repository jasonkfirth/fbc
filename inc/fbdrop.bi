'' FreeBASIC graphics: fbdrop.bi
'' Purpose: Receive native file and text drops through gfxlib2 or gfxlib3.
'' Responsibilities: Checked payload access and bounded lifetime declarations.
'' This interface does not open files, execute text or own the application loop.
#ifndef __FB_DROP_BI__
#define __FB_DROP_BI__
#if __FB_LANG__ <> "fb"
	#error fbdrop.bi requires the fb dialect
#endif
#include once "fbgfx.bi"
namespace FB
	const DROP_OK as long = 0
	const DROP_UNAVAILABLE as long = 1
	const DROP_INVALID as long = 2
	const DROP_FAILED as long = 3
	const DROP_EXPIRED as long = 4
	const DROP_FILES as long = 1
	const DROP_TEXT as long = 2
	const DROP_MAX_ITEMS as long = 64
	const DROP_MAX_BYTES as long = 65536
	const DROP_RETAINED as long = 8
end namespace
namespace FBC
	extern "rtlib"
		'' Enable after SCREENRES with 1 or TRUE; 0/FALSE disables reception.
		'' Unsupported drivers return UNAVAILABLE.
		'' Disabling or changing modes discards retained payloads.
		declare function screenDropEnable alias "fb_GfxDropEnable" _
			( byval enabled as long = 1 ) as long
		'' Call after EVENT_DROP, with item indexes 0 through drop_count - 1.
		'' The result is an owned UTF-8 STRING. Retrieve it before eight newer
		'' drops, disabling reception or changing modes; otherwise EXPIRED.
		declare function screenDropGet alias "fb_GfxDropGet" _
			( byval identifier as long, byval index as long, _
			  byref output as string ) as long
	end extern
end namespace
#endif
'' end of fbdrop.bi
