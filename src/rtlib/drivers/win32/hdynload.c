/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/hdynload.c
 * Purpose: Internal dynamic library functions loading.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "../../fb_private_hdynload.h"
#include <string.h>

#define hDylibFree( lib ) FreeLibrary( lib )

/*
    FreeBASIC stores the Win32 procedure address in its existing void* slots.
    Keep that ABI representation while avoiding a C function/object-pointer
    conversion at the GetProcAddress boundary.
*/
typedef char FB_WIN32_DYNLIB_POINTER_SIZE_CHECK[
	(sizeof(FARPROC) == sizeof(void *)) ? 1 : -1];

static void *hDylibSymbol(FB_DYLIB lib, const char *name)
{
	FARPROC procedure = GetProcAddress((HMODULE)lib, name);
	void *address = NULL;

	if (procedure != NULL)
		memcpy((void *)&address, (const void *)&procedure, sizeof(address));

	return address;
}

FB_DYLIB fb_hDynLoad(const char *libname, const char *const *funcname, void **funcptr)
{
	FB_DYLIB lib;
	ssize_t i;

	if (!(lib = LoadLibrary(libname)))
		return NULL;

	/* Load functions */
	for (i = 0; funcname[i]; i++) {
		funcptr[i] = hDylibSymbol(lib, funcname[i]);
		if (!funcptr[i]) {
			hDylibFree(lib);
			return NULL;
		}
	}

	return lib;
}

int fb_hDynLoadAlso( FB_DYLIB lib, const char *const *funcname, void **funcptr, ssize_t count )
{
	ssize_t i;

	/* Load functions */
	for (i = 0; i < count; i++) {
		funcptr[i] = hDylibSymbol(lib, funcname[i]);
		if (!funcptr[i])
			return -1;
	}

	return 0;
}

void fb_hDynUnload(FB_DYLIB *lib)
{
	if (*lib) {
		hDylibFree( *lib );
		*lib = NULL;
	}
}

/* end of drivers/win32/hdynload.c */
