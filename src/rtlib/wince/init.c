/*
    FreeBASIC runtime library
    -------------------------

    File: wince/init.c

    Purpose:

        Own the FreeBASIC runtime lifecycle on Windows CE.

    Responsibilities:

        - initialize the shared runtime context and Windows CE services
        - bind the main FreeBASIC thread
        - repair an empty CeGCC argv[0] on reduced Windows CE images
        - release files, console state, TLS, and screen devices in order
        - terminate programs after gfxlib2 and sfxlib cleanup

    This file intentionally does NOT contain:

        - desktop locale environment discovery
        - Windows CE console or graphics implementation
        - architecture-specific startup symbol sets

    Windows CE locale policy:

        Wide-string conversion uses MultiByteToWideChar() and
        WideCharToMultiByte() in the WinCE runtime replacements.  The CeGCC
        core runtime does not provide a usable locale.h, so startup does not
        call setlocale().
*/

#include "../fb.h"
#include "../fb_private_thread.h"

FB_RTLIB_CTX __fb_ctx;
static int __fb_is_inicnt = 0;
static char *fb_wince_argv0 = NULL;

/* ------------------------------------------------------------------------- */
/* Runtime startup and shutdown                                              */
/* ------------------------------------------------------------------------- */

void fb_hRtInit(void)
{
    ++__fb_is_inicnt;
    if (__fb_is_inicnt != 1)
        return;

    memset(&__fb_ctx, 0, sizeof(FB_RTLIB_CTX));
    fb_hInit();

#ifdef ENABLE_MT
    fb_TlsInit();
#endif
    fb_AllocateMainFBThread();
}

void fb_hRtExit(void)
{
    --__fb_is_inicnt;
    if (__fb_is_inicnt != 0)
        return;

    fb_FileReset();
    fb_hEnd(0);
    fb_DevScrnEnd(FB_HANDLE_SCREEN);
    fb_TlsFreeCtxTb();

#ifdef ENABLE_MT
    fb_TlsExit();
#endif

    if (__fb_ctx.errmsg != NULL)
        fputs(__fb_ctx.errmsg, stderr);

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

    __fb_ctx.argc = argc;
    __fb_ctx.argv = argv;
    __fb_ctx.lang = lang;
}

FBCALL void fb_End(int errlevel)
{
    if (__fb_ctx.exit_sfxlib != NULL)
        __fb_ctx.exit_sfxlib();
    if (__fb_ctx.exit_gfxlib2 != NULL)
        __fb_ctx.exit_gfxlib2();

    exit(errlevel);
}

/* end of wince/init.c */
