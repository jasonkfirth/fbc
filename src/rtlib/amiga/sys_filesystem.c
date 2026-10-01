/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------

    File: amiga/sys_filesystem.c

    Purpose:
        Provide directory operations missing from the pinned newlib SDK.

    Responsibilities:
        - adapt the C path interfaces used by shared runtime sources to DOS
        - distinguish files from directories before destructive operations
        - transfer and release native directory locks explicitly

    This file intentionally does NOT contain:
        - wildcard enumeration, stream I/O, or Unix permission emulation
*/

#include "../fb.h"

#include <proto/dos.h>
#include <sys/stat.h>

static int amiga_directory_error(void)
{
    LONG error = IoErr();

    if (error == ERROR_OBJECT_NOT_FOUND || error == ERROR_DIR_NOT_FOUND)
        errno = ENOENT;
    else if (error == ERROR_OBJECT_EXISTS)
        errno = EEXIST;
    else if (error == ERROR_DIRECTORY_NOT_EMPTY)
        errno = ENOTEMPTY;
    else if (error == ERROR_NO_FREE_STORE)
        errno = ENOMEM;
    else
        errno = EACCES;
    return -1;
}

static BPTR amiga_directory_lock(const char *path)
{
    struct FileInfoBlock information;
    BPTR lock;

    if (path == NULL) {
        errno = EINVAL;
        return BNULL;
    }
    lock = Lock(path, SHARED_LOCK);
    if (lock == BNULL) {
        amiga_directory_error();
        return BNULL;
    }
    if (!Examine(lock, &information)) {
        UnLock(lock);
        amiga_directory_error();
        return BNULL;
    }
    if (information.fib_DirEntryType <= 0) {
        UnLock(lock);
        errno = ENOTDIR;
        return BNULL;
    }
    return lock;
}

int mkdir(const char *path, mode_t mode)
{
    BPTR lock;

    (void)mode;
    if (path == NULL) { errno = EINVAL; return -1; }
    lock = CreateDir(path);
    if (lock == BNULL) return amiga_directory_error();
    UnLock(lock);
    return 0;
}

int chdir(const char *path)
{
    BPTR lock = amiga_directory_lock(path);
    BPTR previous;

    if (lock == BNULL) return -1;
    previous = CurrentDir(lock);
    if (previous != BNULL) UnLock(previous);
    return 0;
}

int rmdir(const char *path)
{
    BPTR lock = amiga_directory_lock(path);

    if (lock == BNULL) return -1;
    UnLock(lock);
    if (!DeleteFile(path)) return amiga_directory_error();
    return 0;
}

char *getcwd(char *destination, size_t size)
{
    BPTR lock;
    LONG result;

    if (destination == NULL || size == 0 || size > LONG_MAX) {
        errno = EINVAL;
        return NULL;
    }
    lock = Lock("", SHARED_LOCK);
    if (lock == BNULL) { amiga_directory_error(); return NULL; }
    result = NameFromLock(lock, destination, (LONG)size);
    UnLock(lock);
    if (!result) { errno = ERANGE; return NULL; }
    return destination;
}

int rename(const char *source, const char *destination)
{
    if (source == NULL || destination == NULL) { errno = EINVAL; return -1; }
    if (!Rename(source, destination)) return amiga_directory_error();
    return 0;
}

/* end of amiga/sys_filesystem.c */
