/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: gpiopins.c

    Purpose:

        Provide the generic fallback implementation for the shared GPIO pin
        API.

    Responsibilities:

        - export the fb_GpioPin* runtime symbols for targets without a native
          GPIO backend
        - fail GPIO operations with a clear errno value
        - let platform-specific backends override this file by using the same
          basename under src/rtlib/<target>/

    This file intentionally does NOT contain:

        - board-specific pin numbering tables
        - Linux GPIO character-device or sysfs access
        - NuttX GPIO ioctl handling
        - emulated GPIO state
*/

#include "fb.h"

#include <errno.h>

#ifndef ENOSYS
#define ENOSYS EINVAL
#endif

#ifndef EBADF
#define EBADF EINVAL
#endif

/* ------------------------------------------------------------------------- */
/* Unsupported GPIO fallback                                                 */
/* ------------------------------------------------------------------------- */

/*
    The public gpiopins.bi header is intentionally platform-neutral. Targets
    that do not yet have a hardware backend still need these symbols so a
    program can be compiled and fail at runtime in a predictable way.
*/

int fb_GpioPinOpen(const int pin)
{
    if (pin < 0)
        errno = EINVAL;
    else
        errno = ENOSYS;

    return -1;
}

int fb_GpioPinOpenDevice(const char *device)
{
    if ((device == NULL) || (device[0] == '\0'))
        errno = EINVAL;
    else
        errno = ENOSYS;

    return -1;
}

int fb_GpioPinClose(const int handle)
{
    if (handle < 0)
        errno = EBADF;
    else
        errno = ENOSYS;

    return -1;
}

int fb_GpioPinRead(const int handle)
{
    if (handle < 0)
        errno = EBADF;
    else
        errno = ENOSYS;

    return -1;
}

int fb_GpioPinWrite(const int handle, const int value)
{
    (void)value;

    if (handle < 0)
        errno = EBADF;
    else
        errno = ENOSYS;

    return -1;
}

int fb_GpioPinGetType(const int handle)
{
    if (handle < 0)
        errno = EBADF;
    else
        errno = ENOSYS;

    return -1;
}

int fb_GpioPinSetType(const int handle, const int pin_type)
{
    (void)pin_type;

    if (handle < 0)
        errno = EBADF;
    else
        errno = ENOSYS;

    return -1;
}

/* end of gpiopins.c */
