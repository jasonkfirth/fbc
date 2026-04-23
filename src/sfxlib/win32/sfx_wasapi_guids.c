/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_wasapi_guids.c

    Purpose:

        Provide the COM interface and class identifiers needed by
        the Windows WASAPI backend.

        Some MinGW configurations declare these GUIDs in the header
        files but do not provide them from a default import library
        set during static linking. Defining them in one dedicated
        compilation unit keeps the backend self-contained and avoids
        depending on driver order or toolchain quirks.

    Responsibilities:

        • instantiate the WASAPI-related GUID symbols once
        • keep GUID ownership out of the driver implementation files

    This file intentionally does NOT contain:

        • audio driver logic
        • device enumeration code
        • playback or capture code
*/

#include <initguid.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

/* end of sfx_wasapi_guids.c */
