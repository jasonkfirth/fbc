/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/sys_fmem.c
 * Purpose: fre() function.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <dpmi.h>

FBCALL size_t fb_GetMemAvail( int mode )
{
	__dpmi_free_mem_info info;
	(void)__dpmi_get_free_memory_information(&info);
	return info.total_number_of_free_pages * 4096;
}

/* end of drivers/dos/sys_fmem.c */
