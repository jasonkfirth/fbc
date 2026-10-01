/*
    FreeBASIC runtime library
    -------------------------

    File: amiga/init.c

    Purpose:

        Own the FreeBASIC runtime lifecycle on AmigaOS.

    Responsibilities:

        - initialize the shared runtime context and AmigaOS services
        - bind the main FreeBASIC thread
        - release files, console state, TLS, and screen devices in order
        - terminate programs after gfxlib2 and sfxlib cleanup

    This file intentionally does NOT contain:

        - POSIX locale environment discovery
        - AmigaOS console or graphics implementation
        - architecture-specific startup symbol sets

    AmigaOS locale policy:

        AmigaOS wide-string conversion is supplied by the AmigaOS runtime
        replacement.  Calling POSIXC setlocale() during the INIT symbol set
        would re-enter environment discovery before program startup completes.
*/

#include "../fb.h"
#include "../fb_private_thread.h"

FB_RTLIB_CTX __fb_ctx;
static int __fb_is_inicnt = 0;
void fb_hAmigaInitAllocator(void);

/* ------------------------------------------------------------------------- */
/* Runtime startup and shutdown                                              */
/* ------------------------------------------------------------------------- */

void fb_hRtInit(void)
{
    fb_hAmigaInitAllocator();
    fb_hAmigaDebug("rtinit");
    ++__fb_is_inicnt;
    if (__fb_is_inicnt != 1)
        return;

    memset(&__fb_ctx, 0, sizeof(FB_RTLIB_CTX));
    fb_hInit();
    fb_hAmigaDebug("hinit returned");

#ifdef ENABLE_MT
    fb_TlsInit();
#endif
    fb_AllocateMainFBThread();
    fb_hAmigaDebug("main thread allocated");
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
}

/* ------------------------------------------------------------------------- */
/* BASIC program entry and exit                                              */
/* ------------------------------------------------------------------------- */

FBCALL void fb_Init(int argc, char **argv, int lang)
{
    fb_hAmigaDebug("fb_Init");
    __fb_ctx.argc = argc;
    __fb_ctx.argv = argv;
    __fb_ctx.lang = lang;
}

FBCALL void fb_End(int errlevel)
{
    fb_hAmigaDebug("fb_End");
    if (__fb_ctx.exit_sfxlib != NULL)
        __fb_ctx.exit_sfxlib();
    if (__fb_ctx.exit_gfxlib2 != NULL)
        __fb_ctx.exit_gfxlib2();

    exit(errlevel);
}

/* end of amiga/init.c */
