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

        BASIC string conversion uses the common runtime. The pinned SDK uses
        newlib, so AROS's POSIXC locale initialization does not apply here.
        Native startup retains the C locale rather than importing that
        constructor and its environment-discovery dependencies.
*/

#include "../fb.h"
#include "../fb_private_thread.h"
#include <dos/dosextens.h>
#include <proto/exec.h>

FB_RTLIB_CTX __fb_ctx;
static int __fb_is_inicnt = 0;
static APTR previous_requester_window;
void fb_hAmigaInitAllocator(void);
void fb_hAmigaInitFileDescriptors(void);
void fb_hAmigaStopBgThread(void);
void fb_hAmigaClosePipes(void);
void __pthread_Exit_Func(void);

/* ------------------------------------------------------------------------- */
/* Runtime startup and shutdown                                              */
/* ------------------------------------------------------------------------- */

void fb_hRtInit(void)
{
    fb_hAmigaInitAllocator();
    fb_hAmigaInitFileDescriptors();
    fb_hAmigaDebug("rtinit");
    ++__fb_is_inicnt;
    if (__fb_is_inicnt != 1)
        return;

    /* BASIC reports filesystem failures through its return values and ERR.
       An unknown volume must not suspend CHDIR/OPEN behind an Insert Disk
       requester. This is per-process state, inherited by our command workers
       and restored before returning to a DOS loader such as the test runner. */
    struct Process *process = (struct Process *)FindTask(NULL);
    previous_requester_window = process->pr_WindowPtr;
    process->pr_WindowPtr = (APTR)-1;

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

    fb_hAmigaClosePipes();
    fb_hAmigaStopBgThread();
    __pthread_Exit_Func();

    fb_FileReset();
    fb_hEnd(0);
    fb_DevScrnEnd(FB_HANDLE_SCREEN);
    fb_TlsFreeCtxTb();

#ifdef ENABLE_MT
    fb_TlsExit();
#endif

    if (__fb_ctx.errmsg != NULL)
        fputs(__fb_ctx.errmsg, stderr);
    ((struct Process *)FindTask(NULL))->pr_WindowPtr = previous_requester_window;
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

    /* Amiga tasks share this command's code and globals. Reap them while the
       runtime and BASIC global objects still exist, before the SDK walks its
       C++/BASIC destructor list and unloads the command's Hunk segments. */
    fb_hAmigaClosePipes();
    fb_hAmigaStopBgThread();
    __pthread_Exit_Func();

    exit(errlevel);
}

/* end of amiga/init.c */
