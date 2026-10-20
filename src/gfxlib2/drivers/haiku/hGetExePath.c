/*
    FreeBASIC gfxlib2 Haiku backend
    --------------------------------

    File: hGetExePath.c

    Purpose:

        Retrieve the directory path of the currently running executable.

    Responsibilities:

        • locate the running application image
        • extract the filesystem path of that image
        • strip the executable name to obtain the directory path
        • return the result in a caller-supplied buffer

    This file intentionally does NOT contain:

        • graphics driver logic
        • window management
        • rendering code
        • input handling

    Design notes:

        Many parts of the runtime need to locate resources relative to
        the executable location.

        Haiku exposes information about loaded images through the
        get_next_image_info() API. By iterating through the loaded images
        and locating the entry with type B_APP_IMAGE, we can identify
        the executable that started the application.

        The returned value is the directory containing the executable,
        not the full executable path.
*/

#ifndef DISABLE_HAIKU

#include <image.h>
#include <OS.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* ------------------------------------------------------------------------- */
/* Debug helper                                                              */
/* ------------------------------------------------------------------------- */

static int haiku_exepath_debug = -1;

static int fb_hHaikuExePathDebugEnabled(void)
{
    if (haiku_exepath_debug == -1)
    {
        const char *env = getenv("HAIKU_GFX_DEBUG");
        haiku_exepath_debug = (env && *env) ? 1 : 0;
    }

    return haiku_exepath_debug;
}

#define HAIKU_EXEPATH_DEBUG(fmt, ...) \
    do { \
        if (fb_hHaikuExePathDebugEnabled()) \
            fprintf(stderr, "HAIKU_GFX: " fmt "\n", ##__VA_ARGS__); \
    } while (0)


/* ------------------------------------------------------------------------- */
/* Retrieve executable directory                                             */
/* ------------------------------------------------------------------------- */

/*
    fb_hGetExePath()

    Parameters:

        dst     - caller-supplied buffer
        maxlen  - size of buffer

    Returns:

        pointer to dst on success
        NULL on failure

    Behaviour:

        The function searches the loaded image list for the application
        image and extracts the directory path from the full executable
        filename.
*/

char *fb_hGetExePath(char *dst, ssize_t maxlen)
{
    image_info info;
    int32 cookie = 0;

    if (!dst || maxlen <= 0)
        return NULL;

    dst[0] = '\0';

    while (get_next_image_info(0, &cookie, &info) == B_OK)
    {
        if (info.type == B_APP_IMAGE)
        {
            strncpy(dst, info.name, maxlen - 1);
            dst[maxlen - 1] = '\0';

            /*
                Strip the executable name from the path.

                Example:

                    /boot/home/apps/myprogram

                becomes

                    /boot/home/apps
            */

            char *p = strrchr(dst, '/');

            if (p)
                *p = '\0';

            HAIKU_EXEPATH_DEBUG("Executable path: %s", dst);

            return dst;
        }
    }

    HAIKU_EXEPATH_DEBUG("Executable path not found");

    return NULL;
}

#endif

/* end of hGetExePath.c */
