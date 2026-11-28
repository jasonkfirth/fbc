/*
 * Project: FreeBASIC runtime library
 * File: drivers/dragonfly/sys_fmem.c
 * Purpose: fre() function.
 * Responsibilities: Target-specific runtime support for dragonfly.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <sys/sysctl.h>
#include <sys/vmmeter.h>
#include <vm/vm_param.h>

FBCALL size_t fb_GetMemAvail( int mode )
{
	int mib[2] = { CTL_VM, VM_METER };
	struct vmtotal vmt;
	size_t size = sizeof(struct vmtotal);
	long page_size;

	(void)mode;

	if( sysctl( mib, 2, &vmt, &size, NULL, 0 ) )
		return 0;

	page_size = sysconf( _SC_PAGE_SIZE );
	if( (page_size <= 0) || (vmt.t_free < 0) )
		return 0;
	if( (size_t)vmt.t_free > (size_t)-1 / (size_t)page_size )
		return 0;

	return (size_t)vmt.t_free * (size_t)page_size;
}

/* end of drivers/dragonfly/sys_fmem.c */
