/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------
    File: amiga/file_datetime.c
    Purpose: Read file dates through native DOS metadata.
    Responsibilities: Translate the 1978 epoch and 50 Hz ticks to BASIC dates.
    This file intentionally does NOT contain POSIX stat or timezone conversion.
*/

#include "../fb.h"
#include <proto/dos.h>

FBCALL double fb_FileDateTime(const char *filename)
{
    struct FileInfoBlock information;
    BPTR lock;
    if (filename == NULL || *filename == '\0') return 0.0;
    lock = Lock(filename, SHARED_LOCK);
    if (lock == 0) return 0.0;
    if (!Examine(lock, &information)) { UnLock(lock); return 0.0; }
    UnLock(lock);
    return (double)fb_DateSerial(1978, 1, 1) + (double)information.fib_Date.ds_Days +
        (double)information.fib_Date.ds_Minute / 1440.0 +
        (double)information.fib_Date.ds_Tick / (50.0 * 86400.0);
}

/* end of amiga/file_datetime.c */
