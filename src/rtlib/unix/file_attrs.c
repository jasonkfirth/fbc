/*
    FreeBASIC Runtime Library
    File: unix/file_attrs.c
    Purpose: Map pathname attributes onto POSIX file metadata.
    Responsibilities: Independent queries and conservative read-only changes.
    This file does not change Dir state, rename hidden files, or emulate ACLs.
*/

#include "../fb.h"
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

static int attribute_error( void )
{
	if( errno == ENOENT || errno == ENOTDIR )
		return FB_RTERROR_FILENOTFOUND;
	if( errno == EACCES || errno == EPERM || errno == EROFS )
		return FB_RTERROR_NOPRIVILEGES;
	return FB_RTERROR_FILEIO;
}

FBCALL int fb_FileGetAttr( const char *filename )
{
	struct stat info;
	const char *base, *end;
	int attributes = 0;
	mode_t write_mask;

	if( filename == NULL || filename[0] == '\0' ) {
		fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
		return -1;
	}
	/* Like Dir(), stat follows symbolic links. This is a metadata snapshot,
	   not a promise that a later open or write will be allowed. */
	if( stat( filename, &info ) != 0 ) {
		fb_ErrorSetNum( attribute_error() );
		return -1;
	}
	if( info.st_uid == geteuid() )
		write_mask = S_IWUSR;
	else if( info.st_gid == getegid() )
		write_mask = S_IWGRP;
	else
		write_mask = S_IWOTH;
	if( (info.st_mode & write_mask) == 0 )
		attributes |= FB_FILE_ATTR_READONLY;

	/* Ignore trailing separators when finding the basename; never treat the
	   special . and .. directory components themselves as hidden files. */
	end = filename + strlen( filename );
	while( end > filename && end[-1] == '/' )
		--end;
	base = end;
	while( base > filename && base[-1] != '/' )
		--base;
	if( end - base > 1 && base[0] == '.' &&
	    !(end - base == 2 && base[1] == '.') )
		attributes |= FB_FILE_ATTR_HIDDEN;
	if( S_ISCHR( info.st_mode ) || S_ISBLK( info.st_mode ) ||
	    S_ISFIFO( info.st_mode ) || S_ISSOCK( info.st_mode ) )
		attributes |= FB_FILE_ATTR_SYSTEM;
	if( S_ISDIR( info.st_mode ) )
		attributes |= FB_FILE_ATTR_DIRECTORY;
	else
		attributes |= FB_FILE_ATTR_ARCHIVE;
	fb_ErrorSetNum( FB_RTERROR_OK );
	return attributes;
}

FBCALL int fb_FileSetAttr( const char *filename, int attributes )
{
	struct stat info;
	mode_t mode;

	/* Hidden is a naming convention and archive/system are synthesized by
	   GetAttr. Reject those writes rather than silently dropping their bits. */
	if( filename == NULL || filename[0] == '\0' ||
	    (attributes & ~FB_FILE_ATTR_READONLY) != 0 )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	if( stat( filename, &info ) != 0 )
		return fb_ErrorSetNum( attribute_error() );
	if( !S_ISREG( info.st_mode ) && !S_ISDIR( info.st_mode ) )
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );

	/* Never grant group/other write permission when clearing read-only.
	   Preserve read, execute and special mode bits. No process-global umask
	   changes are needed. The caller owns synchronization with path changes. */
	mode = info.st_mode & 07777;
	if( attributes & FB_FILE_ATTR_READONLY )
		mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
	else
		mode |= S_IWUSR;
	if( chmod( filename, mode ) != 0 )
		return fb_ErrorSetNum( attribute_error() );
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of unix/file_attrs.c */
