/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/io_printer.c
 * Purpose: printer access stubs.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_PrinterOpen( DEV_LPT_INFO *devInfo, int iPort, const char *pszDevice )
{
	return fb_ErrorSetNum( FB_RTERROR_FILENOTFOUND );
}

int fb_PrinterWrite( DEV_LPT_INFO *devInfo, const void *data, size_t length )
{
	return fb_ErrorSetNum( FB_RTERROR_FILEIO );
}

int fb_PrinterWriteWstr( DEV_LPT_INFO *devInfo, const FB_WCHAR *data, size_t length )
{
	return fb_ErrorSetNum( FB_RTERROR_FILEIO );
}

int fb_PrinterClose( DEV_LPT_INFO *devInfo )
{
	return fb_ErrorSetNum( FB_RTERROR_FILEIO );
}

/* end of drivers/js/io_printer.c */
