/*
 * Project: FreeBASIC runtime library
 * File: devices/dev_lpt_close.c
 * Purpose: LPTx device.
 * Responsibilities: File-device callbacks and private stream state.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

int fb_DevLptClose( FB_FILE *handle )
{
    int res;
    DEV_LPT_INFO *devInfo;

    FB_LOCK();

    devInfo = (DEV_LPT_INFO*) handle->opaque;
    if( devInfo->uiRefCount==1 ) {

			res = fb_PrinterClose( devInfo );

      if( res==FB_RTERROR_OK ) {
          free(devInfo->pszDevice);
          free(devInfo);
      }

    } else {
        --devInfo->uiRefCount;
        res = fb_ErrorSetNum( FB_RTERROR_OK );
    }

    FB_UNLOCK();

	return res;
}

/* end of devices/dev_lpt_close.c */
