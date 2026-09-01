/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_platform.c

    Purpose:

        Provide shared Windows platform services used by sfxlib backends.

    Responsibilities:

        • provide the Windows target teardown hook
        • isolate optional Windows API entry points from the PE import table
        • retain compatible fallbacks for older Windows releases

    This file intentionally does NOT contain:

        • audio device selection
        • sample mixing or format conversion
        • backend-specific playback or capture logic
*/

#include "fb_sfx_win32.h"

#include <objbase.h>


/* ------------------------------------------------------------------------- */
/* Optional Windows API compatibility                                        */
/* ------------------------------------------------------------------------- */

typedef HRESULT (WINAPI *FB_SFX_COINITIALIZEEX_PROC)(LPVOID, DWORD);

HRESULT fb_sfxWin32InitializeCom(void)
{
    union
    {
        FARPROC generic;
        FB_SFX_COINITIALIZEEX_PROC initialize_ex;
    } procedure;
    HMODULE ole32_module;

    procedure.generic = NULL;
    ole32_module = GetModuleHandleA("ole32.dll");

    if (ole32_module != NULL)
        procedure.generic = GetProcAddress(ole32_module, "CoInitializeEx");

    if (procedure.initialize_ex != NULL)
        return procedure.initialize_ex(NULL, COINIT_MULTITHREADED);

    /*
        Windows 95 provides CoInitialize but needs the optional DCOM update
        before it exports CoInitializeEx.  sfxlib selects WinMM on that system,
        so single-threaded COM initialization is a safe compatibility fallback
        if this helper is reached by an audio worker.
    */
    return CoInitialize(NULL);
}


/* ------------------------------------------------------------------------- */
/* Platform teardown                                                         */
/* ------------------------------------------------------------------------- */

void fb_sfxPlatformExit(void)
{
}

/* end of sfx_platform.c */
