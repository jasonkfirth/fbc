/*
    FreeBASIC runtime Wii internationalization hook
    -----------------------------------------------

    File: drv_intl_get.c

    Purpose:

        Report that platform locale strings are not available through
        libogc.

    Responsibilities:

        - return NULL so the shared datetime runtime uses defaults

    This file intentionally does NOT contain:

        - locale database handling
        - system language detection
*/

#include "../fb.h"

const char *fb_DrvIntlGet(eFbIntlIndex Index)
{
	return NULL;
}

/* end of drv_intl_get.c */
