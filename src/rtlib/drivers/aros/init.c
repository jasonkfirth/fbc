/*
    Project: FreeBASIC runtime library
    -------------------------

    File: drivers/aros/init.c

    Purpose:

        Own the FreeBASIC runtime lifecycle on AROS.

    Responsibilities:

        - initialize the shared runtime context and AROS services
        - bind the main FreeBASIC thread
        - release files, console state, TLS, and screen devices in order
        - terminate programs after gfxlib2 and sfxlib cleanup

    This file intentionally does NOT contain: - POSIX locale environment discovery
        - AROS console or graphics implementation
        - architecture-specific startup symbol sets

    AROS locale policy:

        AROS wide-string conversion is supplied by the AROS runtime
        replacement.  Calling POSIXC setlocale() during the INIT symbol set
        would re-enter environment discovery before program startup completes.
*/

#include "../../fb.h"
#include "../../fb_private_thread.h"

FB_RTLIB_CTX fb_ctx;
static int fb_runtime_init_count = 0;

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
}

/* ------------------------------------------------------------------------- */
/* BASIC program entry and exit                                              */
/* ------------------------------------------------------------------------- */

FBCALL void fb_Init(int argc, char **argv, int lang)
{
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

/* end of drivers/aros/init.c */
