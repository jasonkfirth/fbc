/*
 * Project: FreeBASIC runtime library
 * File: drivers/android/sys_getexepath.c
 * Purpose: Locate packaged app assets or an Android command-line executable.
 * Responsibilities: A bounded launcher path and the normal proc-fs fallback.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <sys/stat.h>
#include <limits.h>

/* The APK launcher sets this once before calling BASIC. Copy the path rather
 * than retaining an Activity-owned pointer. User threads only read it. */
static char app_files_path[PATH_MAX];

void fb_hAndroidSetAppFilesPath( const char *path )
{
	size_t length = path ? strlen( path ) : 0;
	app_files_path[0] = '\0';
	if( length == 0 || length >= sizeof(app_files_path) || path[0] != '/' )
		return;
	memcpy( app_files_path, path, length + 1 );
}

char *fb_hGetExePath( char *dst, ssize_t maxlen )
{
	char *p;
	struct stat finfo;
	ssize_t len;

	if( dst == NULL || maxlen < 2 )
		return NULL;
	if( app_files_path[0] != '\0' ) {
		size_t length = strlen( app_files_path );
		if( length >= (size_t)maxlen ) {
			dst[0] = '\0';
			return NULL;
		}
		memcpy( dst, app_files_path, length + 1 );
		return dst;
	}

	if ((stat("/proc/self/exe", &finfo) == 0) && ((len = readlink("/proc/self/exe", dst, maxlen - 1)) > -1)) {
		/* Command-line Android programs retain their actual executable path. */
		dst[len] = '\0';
		p = strrchr(dst, '/');
		if (p == dst) /* keep the "/" rather than returning "" */
			*(p + 1) = '\0';
		else if (p)
			*p = '\0';
		else
			dst[0] = '\0';
	} else {
		p = NULL;
	}

	return p;
}

/* end of drivers/android/sys_getexepath.c */
