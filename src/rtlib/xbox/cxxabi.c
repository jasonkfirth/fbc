/*
    Project: FreeBASIC Xbox Runtime Library
    ---------------------------------------

    File: cxxabi.c

    Purpose:

        Provide the small set of GCC C++ ABI symbols needed when simple
        MinGW C++ helper objects are linked into Xbox programs.

    Responsibilities:

        * sized and unsized global delete operators
        * a minimal global new operator for freestanding C++ objects
        * placeholder RTTI vtables required by Itanium ABI typeinfo records
        * a pure-virtual-call trap

    This file intentionally does NOT contain:

        * a full libsupc++ implementation
        * exception handling support
        * dynamic_cast or typeid runtime semantics
*/

#include <stdlib.h>

/* ------------------------------------------------------------------------- */
/* Itanium C++ ABI entry points                                              */
/* ------------------------------------------------------------------------- */

void fb_xbox_cxx_delete(void *ptr) __asm__("__ZdlPv");
void fb_xbox_cxx_delete_sized(void *ptr, unsigned int size) __asm__("__ZdlPvj");
void *fb_xbox_cxx_new(unsigned int size) __asm__("__Znwj");
void fb_xbox_cxx_pure_virtual(void) __asm__("___cxa_pure_virtual");

/*
    MinGW C++ emits Itanium ABI RTTI records even though the object format is
    COFF.  The FreeBASIC C++ interop tests only need those records to link;
    they do not use dynamic_cast or typeid at run time.  These placeholder
    vtables make that limited ABI surface available without pulling in a
    host libsupc++ that would depend on the Windows CRT.
*/
void * const fb_xbox_cxx_class_type_info_vtable[4]
	__asm__("__ZTVN10__cxxabiv117__class_type_infoE") =
	{ NULL, NULL, NULL, NULL };

void * const fb_xbox_cxx_si_class_type_info_vtable[4]
	__asm__("__ZTVN10__cxxabiv120__si_class_type_infoE") =
	{ NULL, NULL, NULL, NULL };

void fb_xbox_cxx_delete(void *ptr)
{
	free(ptr);
}

void fb_xbox_cxx_delete_sized(void *ptr, unsigned int size)
{
	(void)size;
	free(ptr);
}

void *fb_xbox_cxx_new(unsigned int size)
{
	void *ptr;

	if (size == 0)
		size = 1;

	ptr = malloc(size);
	if (ptr == NULL)
		abort();

	return ptr;
}

void fb_xbox_cxx_pure_virtual(void)
{
	abort();
}

/* end of cxxabi.c */
