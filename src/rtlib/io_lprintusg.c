/*
    FreeBASIC Runtime Library
    File: io_lprintusg.c
    Purpose: Initialize printer output for PRINT USING formats.
    Responsibilities: Open the printer before initializing byte or UTF-8 formats.
    This file intentionally does NOT contain format parsing or device drivers.

    Printer initialization stays in a separate archive member so ordinary
    PRINT USING does not pull in platform printer libraries when linking.
*/

#include "fb.h"

/*:::::*/
FBCALL int fb_LPrintUsingInit( FBSTRING *fmtstr )
{
    int res = fb_LPrintInit();
    if( res!=FB_RTERROR_OK )
        return res;
    return fb_PrintUsingInit( fmtstr );
}

/*:::::*/
FBCALL int fb_LPrintUsingInitUstr( FBSTRING *fmtstr )
{
    int res = fb_LPrintInit();
    if( res!=FB_RTERROR_OK )
        return res;
    return fb_PrintUsingInitUstr( fmtstr );
}

/* end of io_lprintusg.c */

