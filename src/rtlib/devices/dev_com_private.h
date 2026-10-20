/*
 * Project: FreeBASIC runtime library
 * File: devices/dev_com_private.h
 * Purpose: Provide dev com private runtime support.
 * Responsibilities: File-device callbacks and private stream state.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
typedef struct _DEV_COM_INFO {
	void *hSerial;
	char *pszDevice;
	int iPort;
	FB_SERIAL_OPTIONS Options;
} DEV_COM_INFO;

/* end of devices/dev_com_private.h */
