/*
 * Project: FreeBASIC runtime library
 * File: devices/dev_lpt_write_wstr.c
 * Purpose: LPTx device.
 * Responsibilities: File-device callbacks and private stream state.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

int fb_DevLptWriteWstr( FB_FILE *handle, const FB_WCHAR* value, size_t valuelen )
{
    int res;

    FB_LOCK();

    res = fb_PrinterWriteWstr((DEV_LPT_INFO*) handle->opaque, value, valuelen );

    FB_UNLOCK();

	return res;
}

/* end of devices/dev_lpt_write_wstr.c */
