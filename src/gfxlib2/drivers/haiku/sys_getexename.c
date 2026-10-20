/*
    FreeBASIC runtime support
    -------------------------

    File: sys_getexename.c

    Purpose:

        Retrieve the executable file name of the currently running program.

    Responsibilities:

        • locate the running application image
        • extract the executable name from the full path
        • return the name in a caller-provided buffer

    This file intentionally does NOT contain:

        • path resolution logic
        • filesystem manipulation
        • runtime initialization

    Notes:

        The Haiku operating system provides image inspection APIs that allow
        applications to enumerate loaded program images. The main application
        binary is reported with the type B_APP_IMAGE.

        By scanning loaded images and locating B_APP_IMAGE, we can determine
        the path of the currently executing program.
*/

#ifndef DISABLE_HAIKU

#include <image.h>
#include <OS.h>

#include <stdio.h>
#include <string.h>


/* ------------------------------------------------------------------------- */
/* Get executable name                                                       */
/* ------------------------------------------------------------------------- */

/*
    fb_hGetExeName()

    Retrieve the base file name of the currently running executable.

    Parameters:

        dst     - destination buffer
        maxlen  - size of the destination buffer

    Returns:

        dst on success
        NULL on failure

    Defensive behavior:

        • ensures the destination buffer is valid
        • guarantees null termination
*/

char *fb_hGetExeName(char *dst, ssize_t maxlen)
{
    image_info info;
    int32 cookie = 0;

    if (!dst || maxlen <= 0)
        return NULL;

    dst[0] = '\0';

    /*
        Iterate through loaded images until the application image is found.
    */

    while (get_next_image_info(0, &cookie, &info) == B_OK)
    {
        if (info.type == B_APP_IMAGE)
        {
            const char *path = info.name;
            const char *base;

            /*
                Extract the file name from the full path.

                Example:

                    /boot/home/myapp

                becomes:

                    myapp
            */

            base = strrchr(path, '/');

            if (base)
                base++;
            else
                base = path;

            strncpy(dst, base, maxlen - 1);
            dst[maxlen - 1] = '\0';

            return dst;
        }
    }

    /*
        If the application image cannot be located, return failure.
        This should rarely occur in normal operation.
    */

    return NULL;
}

#endif

/* end of sys_getexename.c */
