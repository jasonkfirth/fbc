/*
    Project: FreeBASIC Runtime Library
    ----------------------------------

    File: wince/io_serial_control.c

    Purpose:

        Reuse the Win32 communications-control implementation on Windows CE.

    Responsibilities:

        - select the shared Windows HANDLE backend for the WinCE source layer

    This file intentionally does NOT contain:

        - a second copy of the Windows communications logic
        - serial opening, reads, or writes
        - desktop-only device discovery
*/

#include "../win32/io_serial_control.c"

/* end of wince/io_serial_control.c */
