/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------
    File: amiga/file_copy.c
    Purpose: Copy regular files through native DOS handles.
    Responsibilities: Check opened-file identity before truncating and copy fully.
    This file intentionally does NOT contain directory or metadata copying.

    MODE_READWRITE opens the destination without truncation. DupLockFromFH and
    SameLock identify aliases using the opened objects, before SetFileSize can
    destroy source data. Callers synchronize concurrent content changes.
*/

#include "../fb.h"
#include <proto/dos.h>

FBCALL int fb_FileCopy(const char *source, const char *destination)
{
    BPTR input = 0, output = 0, input_lock = 0, output_lock = 0;
    struct FileInfoBlock information;
    unsigned char buffer[16384];
    LONG count, offset, written;
    int result = FB_RTERROR_ILLEGALFUNCTIONCALL;

    if (source == NULL || destination == NULL || *source == '\0' || *destination == '\0')
        goto done;
    input = Open(source, MODE_OLDFILE);
    if (input == 0) goto done;
    input_lock = DupLockFromFH(input);
    if (input_lock == 0 || !Examine(input_lock, &information) || information.fib_DirEntryType >= 0)
        goto done;
    output = Open(destination, MODE_READWRITE);
    if (output == 0) goto done;
    output_lock = DupLockFromFH(output);
    if (output_lock == 0 || !Examine(output_lock, &information) || information.fib_DirEntryType >= 0 ||
        SameLock(input_lock, output_lock) == LOCK_SAME)
        goto done;
    if (SetFileSize(output, 0, OFFSET_BEGINNING) == -1) goto done;
    while ((count = Read(input, buffer, sizeof(buffer))) > 0) {
        offset = 0;
        while (offset < count) {
            written = Write(output, buffer + offset, count - offset);
            if (written <= 0) goto done;
            offset += written;
        }
    }
    if (count < 0) goto done;
    result = FB_RTERROR_OK;
done:
    if (output_lock != 0) UnLock(output_lock);
    if (input_lock != 0) UnLock(input_lock);
    if (output != 0 && !Close(output)) result = FB_RTERROR_FILEIO;
    if (input != 0) Close(input);
    return fb_ErrorSetNum(result);
}

/* end of amiga/file_copy.c */
