/*
 * Project: FreeBASIC runtime library
 * File: files/file_exists.c
 * Purpose: file existence testing.
 * Responsibilities: BASIC file operations and device-independent handle policy.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL int fb_FileExists 
	( 
		const char *filename 
	)
{
	FILE *fp;
	
	fp = fb_hOpenFile(filename, "r");
	if (fp) 
	{
		fclose(fp);
		return FB_TRUE;
	} 
	else
	{
		return FB_FALSE;
	}
}

/* end of files/file_exists.c */
