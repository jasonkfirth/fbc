/*
 * Project: FreeBASIC runtime library
 * File: drivers/freebsd/sys_fmem.c
 * Purpose: fre() function.
 * Responsibilities: Target-specific runtime support for freebsd.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <sys/sysctl.h>
#include <sys/vmmeter.h>
#include <vm/vm_param.h>
#include <stdint.h>

FBCALL size_t fb_GetMemAvail( int mode )
{
	int mib[2] = { CTL_VM, VM_TOTAL };
	struct vmtotal vmt;
	size_t size = sizeof(struct vmtotal);
	long page_size;
	uintmax_t free_pages;

	(void)mode;

	if( sysctl( mib, 2, &vmt, &size, NULL, 0 ) )
		return 0;
	if( vmt.t_free <= 0 )
		return 0;

	page_size = sysconf( _SC_PAGE_SIZE );
	if( page_size <= 0 )
		return 0;

	free_pages = (uintmax_t)vmt.t_free;
	if( free_pages > SIZE_MAX / (uintmax_t)page_size )
		return 0;

	return (size_t)free_pages * (size_t)page_size;
}

/* end of drivers/freebsd/sys_fmem.c */
