/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/file_path.h
 * Purpose: Share Xbox path conversion with directory and rename operations.
 * Responsibilities: Declare native filesystem adapters for the Xbox driver.
 * This file intentionally does NOT implement drive selection or enumeration.
 */
#ifndef FB_XBOX_FILE_PATH_H
#define FB_XBOX_FILE_PATH_H
#include <windows.h>
#include <stddef.h>
HANDLE fb_hFindFirstFile( const char *path, WIN32_FIND_DATAA *data, char *mask, size_t mask_bytes );
BOOL fb_hFindNextFile( HANDLE handle, const char *mask, WIN32_FIND_DATAA *data );
int fb_hRenameFile( const char *oldname, const char *newname );
#endif
/* end of drivers/xbox/file_path.h */
