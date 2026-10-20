/*
 * Project: FreeBASIC runtime library
 * File: fb_private_hdynload.h
 * Purpose: Provide fb private hdynload runtime support.
 * Responsibilities: Shared runtime types, ABI declarations and internal contracts.
 * This file intentionally does NOT contain: Platform implementations or compiler parsing.
 */
#if defined HOST_UNIX || defined HOST_WIN32

#ifdef HOST_WIN32
	#include <windows.h>
	typedef HANDLE FB_DYLIB;
#else
	typedef void *FB_DYLIB;
#endif

FB_DYLIB fb_hDynLoad    (const char *libname, const char *const *funcname, void **funcptr);
int      fb_hDynLoadAlso(FB_DYLIB lib, const char *const *funcname, void **funcptr, ssize_t count);
void     fb_hDynUnload  (FB_DYLIB *lib);

#endif

/* end of fb_private_hdynload.h */
