/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: gpiopins.c

    Purpose:

        Provide the NuttX backend for the common FreeBASIC GPIO pin API.

    Responsibilities:

        - open GPIO pins registered by NuttX as /dev/gpioN devices
        - read and write digital pin values through the NuttX GPIO ioctl API
        - query and change the NuttX GPIO pin type when the board enables it

    This file intentionally does NOT contain:

        - board-specific pin numbering tables
        - pin mux setup
        - interrupt callback registration
        - GPIO implementations for Linux, ESP-IDF, or other host platforms

    This file is included by fb_nuttx_minrt.c while the target is still
    being brought up. Keeping the command bodies in smaller files makes
    the code easier to compare with the normal rtlib layout.
*/

/* ------------------------------------------------------------------------- */
/* NuttX GPIO helpers                                                        */
/* ------------------------------------------------------------------------- */

/*
    GPIO availability on NuttX is board and configuration dependent.

    Some builds expose the header path but do not enable the GPIO ioctl
    command set used by the character-device GPIO driver. Treat each group
    of operations as optional so the runtime can still build on small QEMU
    configurations that have no GPIO devices at all.
*/

#ifndef FB_NUTTX_HAVE_GPIO_READWRITE
#if FB_NUTTX_HAVE_GPIO && defined(GPIOC_READ) && defined(GPIOC_WRITE)
#define FB_NUTTX_HAVE_GPIO_READWRITE 1
#else
#define FB_NUTTX_HAVE_GPIO_READWRITE 0
#endif
#endif

#ifndef FB_NUTTX_HAVE_GPIO_PINTYPE
#if FB_NUTTX_HAVE_GPIO && defined(GPIOC_PINTYPE)
#define FB_NUTTX_HAVE_GPIO_PINTYPE 1
#else
#define FB_NUTTX_HAVE_GPIO_PINTYPE 0
#endif
#endif

#ifndef FB_NUTTX_HAVE_GPIO_SETPINTYPE
#if FB_NUTTX_HAVE_GPIO && defined(GPIOC_SETPINTYPE)
#define FB_NUTTX_HAVE_GPIO_SETPINTYPE 1
#else
#define FB_NUTTX_HAVE_GPIO_SETPINTYPE 0
#endif
#endif

/*
    NuttX exposes GPIO pins to applications as character devices such as
    /dev/gpio0, /dev/gpio1, and so on. Unlike normal files, those devices are
    controlled through ioctl() calls. The public include in inc/gpiopins.bi
    provides a small friendly wrapper over these runtime symbols.

    These helpers intentionally return simple integer status values because
    that keeps the first microcontroller API easy to use from BASIC:

        -1  operation failed
         0  false, low, or success depending on the call
         1  true or high
*/

int32 fb_GpioPinOpen(const int32 pin)
{
    char path[32];

    if ((pin < 0) || (pin > 9999)) {
        errno = EINVAL;
        return -1;
    }

    snprintf(path, sizeof(path), "/dev/gpio%" PRId32, pin);
    return open(path, O_RDWR);
}

int32 fb_GpioPinOpenDevice(const char *device)
{
    if ((device == NULL) || (device[0] == '\0')) {
        errno = EINVAL;
        return -1;
    }

    return open(device, O_RDWR);
}

int32 fb_GpioPinClose(const int32 handle)
{
    if (handle < 0) {
        errno = EBADF;
        return -1;
    }

    return close(handle);
}

int32 fb_GpioPinRead(const int32 handle)
{
#if FB_NUTTX_HAVE_GPIO_READWRITE
    bool value;

    if (handle < 0) {
        errno = EBADF;
        return -1;
    }

    value = false;

    if (ioctl(handle, GPIOC_READ, (unsigned long)(uintptr_t)&value) < 0)
        return -1;

    return value ? 1 : 0;
#else
    (void)handle;
    errno = ENOSYS;
    return -1;
#endif
}

int32 fb_GpioPinWrite(const int32 handle, const int32 value)
{
#if FB_NUTTX_HAVE_GPIO_READWRITE
    if (handle < 0) {
        errno = EBADF;
        return -1;
    }

    if (ioctl(handle, GPIOC_WRITE, value ? 1 : 0) < 0)
        return -1;

    return 0;
#else
    (void)handle;
    (void)value;
    errno = ENOSYS;
    return -1;
#endif
}

int32 fb_GpioPinGetType(const int32 handle)
{
#if FB_NUTTX_HAVE_GPIO_PINTYPE
    int pin_type;

    if (handle < 0) {
        errno = EBADF;
        return -1;
    }

    if (ioctl(handle, GPIOC_PINTYPE, (unsigned long)(uintptr_t)&pin_type) < 0)
        return -1;

    return (int32)pin_type;
#else
    (void)handle;
    errno = ENOSYS;
    return -1;
#endif
}

int32 fb_GpioPinSetType(const int32 handle, const int32 pin_type)
{
#if FB_NUTTX_HAVE_GPIO_SETPINTYPE
    if ((handle < 0) || (pin_type < 0)) {
        errno = EINVAL;
        return -1;
    }

    if (ioctl(handle, GPIOC_SETPINTYPE, (unsigned long)pin_type) < 0)
        return -1;

    return 0;
#else
    (void)handle;
    (void)pin_type;
    errno = ENOSYS;
    return -1;
#endif
}

/* end of gpiopins.c */
