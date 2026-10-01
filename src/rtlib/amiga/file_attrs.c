/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------
    File: amiga/file_attrs.c
    Purpose: Translate BASIC file attributes to native DOS metadata.
    Responsibilities: Query files independently and preserve other protections.
    This file intentionally does NOT contain wildcard iteration or Unix user IDs.
*/

#include "../fb.h"
#include <proto/dos.h>

FBCALL int fb_FileGetAttr(const char *filename)
{
    BPTR lock;
    struct FileInfoBlock information;
    int attributes;
    if (filename == NULL || *filename == '\0') {
        fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL); return -1;
    }
    lock = Lock(filename, SHARED_LOCK);
    if (lock == 0) { fb_ErrorSetNum(FB_RTERROR_FILENOTFOUND); return -1; }
    if (!Examine(lock, &information)) {
        UnLock(lock); fb_ErrorSetNum(FB_RTERROR_FILEIO); return -1;
    }
    UnLock(lock);
    attributes = information.fib_DirEntryType > 0 ? FB_FILE_ATTR_DIRECTORY : FB_FILE_ATTR_ARCHIVE;
    if (information.fib_Protection & FIBF_WRITE) attributes |= FB_FILE_ATTR_READONLY;
    if (information.fib_FileName[0] == '.') attributes |= FB_FILE_ATTR_HIDDEN;
    fb_ErrorSetNum(FB_RTERROR_OK);
    return attributes;
}

FBCALL int fb_FileSetAttr(const char *filename, int attributes)
{
    BPTR lock;
    struct FileInfoBlock information;
    if (filename == NULL || *filename == '\0' || (attributes & ~FB_FILE_ATTR_READONLY) != 0)
        return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
    lock = Lock(filename, SHARED_LOCK);
    if (lock == 0) return fb_ErrorSetNum(FB_RTERROR_FILENOTFOUND);
    if (!Examine(lock, &information)) {
        UnLock(lock); return fb_ErrorSetNum(FB_RTERROR_FILEIO);
    }
    UnLock(lock);
    if (attributes & FB_FILE_ATTR_READONLY) information.fib_Protection |= FIBF_WRITE;
    else information.fib_Protection &= ~FIBF_WRITE;
    if (!SetProtection(filename, information.fib_Protection))
        return fb_ErrorSetNum(FB_RTERROR_NOPRIVILEGES);
    return fb_ErrorSetNum(FB_RTERROR_OK);
}

/* end of amiga/file_attrs.c */
