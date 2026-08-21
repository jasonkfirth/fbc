/*
    FreeBASIC Runtime Library support for Windows CE
    ------------------------------------------------

    File: io_printer.c

    Purpose:

        Provide deterministic OPEN LPT and LPRINT behavior on Windows CE
        systems that do not expose the desktop Windows print spooler API.

    Responsibilities:

        - reject printer opens with the normal file-not-found result
        - reject writes and closes on unavailable printer devices
        - avoid importing desktop winspool functions into CE executables

    This file intentionally does NOT contain:

        - desktop printer enumeration
        - raw spooler jobs
        - device-specific Windows CE printer-driver integration
*/

#include "../fb.h"

int fb_PrinterOpen( DEV_LPT_INFO *dev_info,
                    int port,
                    const char *device )
{
    (void)dev_info;
    (void)port;
    (void)device;

    return fb_ErrorSetNum( FB_RTERROR_FILENOTFOUND );
}

int fb_PrinterWrite( DEV_LPT_INFO *dev_info,
                     const void *data,
                     size_t length )
{
    (void)dev_info;
    (void)data;
    (void)length;

    return fb_ErrorSetNum( FB_RTERROR_FILEIO );
}

int fb_PrinterWriteWstr( DEV_LPT_INFO *dev_info,
                         const FB_WCHAR *data,
                         size_t length )
{
    (void)dev_info;
    (void)data;
    (void)length;

    return fb_ErrorSetNum( FB_RTERROR_FILEIO );
}

int fb_PrinterClose( DEV_LPT_INFO *dev_info )
{
    (void)dev_info;

    return fb_ErrorSetNum( FB_RTERROR_FILEIO );
}

/* end of io_printer.c */
