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

	if( sysctl( mib, 2, &vmt, &size, NULL, 0 ) )
		return 0;

	return vmt.t_free * sysconf( _SC_PAGE_SIZE );
}

/* end of drivers/dragonfly/sys_fmem.c */
