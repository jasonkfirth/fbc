/*
 * Project: FreeBASIC Wii pathname metadata
 * File: drivers/wii/file_attrs.c
 * Purpose: Query FAT attributes without changing directory enumeration state.
 * Responsibilities: Checked pathname queries and runtime error translation.
 * This file does not mutate FAT attributes or simulate supported writes.
 *
 * libfat exposes the DOS attribute bits used by FreeBASIC directly. A mounted
 * volume root may have no FAT directory entry, so stat supplies its directory
 * classification. Queries are snapshots, not permission or lifetime leases.
 */
#include "../../fb.h"
#include <errno.h>
#include <fat.h>
#include <sys/stat.h>

FBCALL int fb_FileGetAttr(const char *filename)
{
    int attributes;
    struct stat information;
    if (filename == NULL || filename[0] == '\0') {
        fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
        return -1;
    }
    attributes = FAT_getAttr(filename);
    if (attributes >= 0) {
        fb_ErrorSetNum(FB_RTERROR_OK);
        return attributes & (FB_FILE_ATTR_READONLY | FB_FILE_ATTR_HIDDEN |
                             FB_FILE_ATTR_SYSTEM |
                             FB_FILE_ATTR_DIRECTORY | FB_FILE_ATTR_ARCHIVE);
    }
    if (stat(filename, &information) == 0 && S_ISDIR(information.st_mode)) {
        fb_ErrorSetNum(FB_RTERROR_OK);
        return FB_FILE_ATTR_DIRECTORY;
    }
    if (errno == ENOENT || errno == ENOTDIR)
        fb_ErrorSetNum(FB_RTERROR_FILENOTFOUND);
    else if (errno == EACCES || errno == EPERM || errno == EROFS)
        fb_ErrorSetNum(FB_RTERROR_NOPRIVILEGES);
    else
        fb_ErrorSetNum(FB_RTERROR_FILEIO);
    return -1;
}

FBCALL int fb_FileSetAttr(const char *filename, int attributes)
{
    (void)filename;
    (void)attributes;
    return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
}
/* end of drivers/wii/file_attrs.c */
