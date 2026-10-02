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
        - expose canonical paths and file metadata to the native compiler

    This file intentionally does NOT contain:
        - wildcard enumeration, stream I/O, or Unix permission emulation
*/

#include "../fb.h"

#include <proto/dos.h>
#include <sys/stat.h>
#include <strings.h>

/* newlib's caller-owned realpath buffer is sized by PATH_MAX. The SDK may
   omit that optional limit; keep the same bound for allocated results. */
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

static BPTR amiga_file_lock(const char *source)
{
    char *path = strdup(source);
    BPTR lock;
    if (path == NULL) { SetIoErr(ERROR_NO_FREE_STORE); return BNULL; }
    fb_hConvertPath(path);
    lock = Lock((CONST_STRPTR)path, SHARED_LOCK);
    free(path);
    return lock;
}

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

char *realpath(const char *source, char *destination)
{
    BPTR lock;
    char *result = destination;

    if (source == NULL || *source == '\0') { errno = EINVAL; return NULL; }
    /* POSIX callers name the current directory "."; native DOS uses "". */
    lock = amiga_file_lock(source);
    if (lock == BNULL) { amiga_directory_error(); return NULL; }
    if (result == NULL) result = malloc(PATH_MAX);
    if (result == NULL) { UnLock(lock); errno = ENOMEM; return NULL; }
    if (!NameFromLock(lock, result, PATH_MAX)) {
        UnLock(lock);
        if (destination == NULL) free(result);
        errno = ERANGE;
        return NULL;
    }
    UnLock(lock);
    return result;
}

int fb_hAmigaFileStat(const struct FileInfoBlock *native, struct stat *information)
{
    int64_t seconds;

    if (native->fib_Size < 0) { errno = EOVERFLOW; return -1; }
    memset(information, 0, sizeof(*information));
    information->st_mode = native->fib_DirEntryType > 0 ? S_IFDIR : S_IFREG;
    if (native->fib_DirEntryType == ST_SOFTLINK) information->st_mode = S_IFLNK;
    if (!(native->fib_Protection & FIBF_READ)) information->st_mode |= S_IRUSR;
    if (!(native->fib_Protection & FIBF_WRITE)) information->st_mode |= S_IWUSR;
    if (!(native->fib_Protection & FIBF_EXECUTE)) information->st_mode |= S_IXUSR;
    information->st_nlink = 1;
    information->st_size = native->fib_Size;
    /* Amiga's epoch is 1978-01-01, eight years after the Unix epoch. DOS has
       one modification stamp; it supplies all three newlib observations.
       Disk keys are not portable inode identities, so leave st_ino zero. */
    seconds = (int64_t)native->fib_Date.ds_Days * 86400 +
              (int64_t)native->fib_Date.ds_Minute * 60 +
              native->fib_Date.ds_Tick / TICKS_PER_SECOND + 252460800;
    if ((int64_t)(time_t)seconds != seconds) { errno = EOVERFLOW; return -1; }
    information->st_atime = information->st_mtime = information->st_ctime = (time_t)seconds;
    return 0;
}

int stat(const char *source, struct stat *information)
{
    struct FileInfoBlock native;
    BPTR lock;

    if (source == NULL || information == NULL) { errno = EINVAL; return -1; }
    lock = amiga_file_lock(source);
    if (lock == BNULL) return amiga_directory_error();
    if (!Examine(lock, &native)) {
        LONG error = IoErr();
        UnLock(lock);
        SetIoErr(error);
        return amiga_directory_error();
    }
    UnLock(lock);
    return fb_hAmigaFileStat(&native, information);
}

int lstat(const char *source, struct stat *information)
{
    struct FileInfoBlock native;
    const char *name;
    char *parent;
    size_t length;
    BPTR lock;

    if (source == NULL || information == NULL) { errno = EINVAL; return -1; }
    name = (const char *)FilePart((CONST_STRPTR)source);
    if (*name == '\0' || strcmp(name, ".") == 0) return stat(source, information);
    length = (size_t)(name - source);
    if (length != 0 && source[length - 1] == '/') --length;
    parent = malloc(length + 1);
    if (parent == NULL) return -1;
    memcpy(parent, source, length);
    parent[length] = '\0';
    lock = amiga_file_lock(parent);
    free(parent);
    if (lock == BNULL) return amiga_directory_error();
    /* Enumerating the parent observes the link entry itself. Lock(source)
       follows native soft links, so it cannot implement lstat's contract. */
    if (Examine(lock, &native)) {
        while (ExNext(lock, &native)) {
            if (strcasecmp((const char *)native.fib_FileName, name) == 0) {
                UnLock(lock);
                return fb_hAmigaFileStat(&native, information);
            }
        }
    }
    UnLock(lock);
    errno = ENOENT;
    return -1;
}

/* end of amiga/sys_filesystem.c */
