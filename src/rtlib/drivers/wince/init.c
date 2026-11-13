/*
    Project: FreeBASIC runtime library
    -------------------------

    File: drivers/wince/init.c

    Purpose:

        Own the FreeBASIC runtime lifecycle on Windows CE.

    Responsibilities:

        - initialize the shared runtime context and Windows CE services
        - bind the main FreeBASIC thread
        - repair an empty CeGCC argv[0] on reduced Windows CE images
        - release files, console state, TLS, and screen devices in order
        - terminate programs after gfxlib2 and sfxlib cleanup

    This file intentionally does NOT contain: - desktop locale environment discovery
        - Windows CE console or graphics implementation
        - architecture-specific startup symbol sets

    Windows CE locale policy:

        Wide-string conversion uses MultiByteToWideChar() and
        WideCharToMultiByte() in the WinCE runtime replacements.  The CeGCC
        core runtime does not provide a usable locale.h, so startup does not
        call setlocale().
*/

#include "../../fb.h"
#include "../../fb_private_thread.h"

FB_RTLIB_CTX fb_ctx;
static int fb_runtime_init_count = 0;
static char *fb_wince_argv0 = NULL;

/* ------------------------------------------------------------------------- */
/* Runtime startup and shutdown                                              */
/* ------------------------------------------------------------------------- */

void fb_hRtInit(void)
{
    ++fb_runtime_init_count;
    if (fb_runtime_init_count != 1)
        return;

    memset(&fb_ctx, 0, sizeof(FB_RTLIB_CTX));
    fb_hInit();

#ifdef ENABLE_MT
    fb_TlsInit();
#endif
    fb_AllocateMainFBThread();
}

void fb_hRtExit(void)
{
    --fb_runtime_init_count;
    if (fb_runtime_init_count != 0)
        return;

    fb_FileReset();
    fb_hEnd(0);
    fb_DevScrnEnd(FB_HANDLE_SCREEN);
    fb_TlsFreeCtxTb();

#ifdef ENABLE_MT
    fb_TlsExit();
#endif

    if (fb_ctx.errmsg != NULL)
        fputs(fb_ctx.errmsg, stderr);

    free(fb_wince_argv0);
    fb_wince_argv0 = NULL;
}

/* ------------------------------------------------------------------------- */
/* BASIC program entry and exit                                              */
/* ------------------------------------------------------------------------- */

FBCALL void fb_Init(int argc, char **argv, int lang)
{
    if (argc > 0 && argv != NULL && argv[0] != NULL && argv[0][0] == '\0') {
        wchar_t wide_path[MAX_PATH + 1];

        if (fb_hWinCEGetExecutablePathWC(wide_path, ARRAY_SIZE(wide_path))) {
            fb_wince_argv0 = fb_hConvertPathFromWC(wide_path, TRUE);
            if (fb_wince_argv0 != NULL)
                argv[0] = fb_wince_argv0;
        }
    }

    fb_ctx.argc = argc;
    fb_ctx.argv = argv;
    fb_ctx.lang = lang;
}

FBCALL void fb_End(int errlevel)
{
    if (fb_ctx.exit_sfxlib != NULL)
        fb_ctx.exit_sfxlib();
    if (fb_ctx.exit_gfxlib2 != NULL)
        fb_ctx.exit_gfxlib2();

    exit(errlevel);
}

/* end of drivers/wince/init.c */
