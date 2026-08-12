/*
    FreeBASIC runtime Wii serial device stubs
    -----------------------------------------

    File: io_serial.c

    Purpose:

        Provide defined failures for unsupported OPEN COM operations.

    Responsibilities:

        - export the serial hooks required by the generic COM device layer
        - report FB_RTERROR_ILLEGALFUNCTIONCALL for every operation

    This file intentionally does NOT contain:

        - USB serial adapter drivers
        - hardware UART configuration
        - asynchronous serial I/O
*/

#include "../fb.h"

int fb_SerialOpen
	(
		FB_FILE *handle,
		int iPort,
		FB_SERIAL_OPTIONS *options,
		const char *pszDevice,
		void **ppvHandle
	)
{
	(void)handle;
	(void)iPort;
	(void)options;
	(void)pszDevice;
	(void)ppvHandle;

	return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
}

int fb_SerialGetRemaining(FB_FILE *handle, void *pvHandle, fb_off_t *pLength)
{
	(void)handle;
	(void)pvHandle;
	(void)pLength;

	return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
}

int fb_SerialWrite(FB_FILE *handle, void *pvHandle, const void *data, size_t length)
{
	(void)handle;
	(void)pvHandle;
	(void)data;
	(void)length;

	return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
}

int fb_SerialRead(FB_FILE *handle, void *pvHandle, void *data, size_t *pLength)
{
	(void)handle;
	(void)pvHandle;
	(void)data;
	(void)pLength;

	return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
}

int fb_SerialClose(FB_FILE *handle, void *pvHandle)
{
	(void)handle;
	(void)pvHandle;

	return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
}

/* end of io_serial.c */
