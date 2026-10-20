/*
 * Project: FreeBASIC runtime library
 * File: devices/dev_lpt_write.c
 * Purpose: LPTx device.
 * Responsibilities: File-device callbacks and private stream state.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

int fb_DevLptWrite( FB_FILE *handle, const void* value, size_t valuelen )
{
    int res;

    FB_LOCK();

    res = fb_PrinterWrite((DEV_LPT_INFO*) handle->opaque, value, valuelen );

    FB_UNLOCK();

	return res;
}

/* end of devices/dev_lpt_write.c */
