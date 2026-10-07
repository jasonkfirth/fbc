/*
    FreeBASIC gfxlib2 Haiku backend
    File: haiku_init.cpp
    Purpose: Own native GUI startup, shutdown and readiness synchronization.
    Responsibilities: Create and release the application, window and views.
    This file contains no drawing primitives or software page allocation.
*/

#ifndef DISABLE_HAIKU

#include "fb_gfx_haiku.h"
#include "haiku_window.h"
#include "haiku_render.h"
#include "haiku_debug.h"
#include "../fb_gfx.h"
#ifndef DISABLE_OPENGL
#include "../fb_gfx_gl.h"
#endif

#include <Application.h>
#include <Bitmap.h>
#include <View.h>
#include <Window.h>
#include <Screen.h>
#include <OS.h>
#include <stdlib.h>
#include <new>

extern BBitmap *g_bmp;
extern BWindow *g_win;
extern BView   *g_view;
#ifndef DISABLE_OPENGL
extern BGLView *g_gl_view;
#endif

extern thread_id fb_haiku_event_thread;

static BApplication *fb_app = NULL;

#ifndef DISABLE_OPENGL
static uint32 fb_hHaikuOpenGLViewOptions(void)
{
    uint32 options = BGL_RGB | BGL_DOUBLE;

    if (__fb_gl_params.color_alpha_bits > 0)
        options |= BGL_ALPHA;
    if (__fb_gl_params.depth_bits > 0)
        options |= BGL_DEPTH;
    if (__fb_gl_params.stencil_bits > 0)
        options |= BGL_STENCIL;
    if (__fb_gl_params.accum_bits > 0)
        options |= BGL_ACCUM;

    return options;
}
#endif

static int32 fb_hHaikuInitFail(BApplication *app = NULL, int created_app = 0)
{
    if (created_app)
    {
        HAIKU_DEBUG("Destroying owned application");
        delete app;
        HAIKU_DEBUG("Owned application destroyed");
    }
    fb_app = NULL;
    fb_hHaikuLockState();
    fb_haiku.gui_failed  = 1;
    fb_haiku.gui_ready   = 0;
    fb_haiku.gui_running = 0;
    fb_hHaikuUnlockState();

    if (fb_haiku.gui_ready_sem >= B_OK)
        release_sem(fb_haiku.gui_ready_sem);

    if (fb_haiku.gui_exit_sem >= B_OK)
        release_sem(fb_haiku.gui_exit_sem);

    return -1;
}

/* ------------------------------------------------------------------------- */

static int32 fb_haiku_event_thread_func(void *userdata)
{
    char *title = (char*)userdata;
    BApplication *app = NULL;
    int created_app = 0;

    fb_hHaikuLockState();
    fb_haiku.gui_running = 1;
    fb_haiku.gui_ready   = 0;
    fb_haiku.gui_failed  = 0;
    fb_hHaikuUnlockState();

    if (be_app) {
        app = (BApplication*)be_app;
        created_app = 0;
    } else {
        app = new(std::nothrow) BApplication("application/x-vnd.FreeBASIC-gfx");
        if (!app)
            return fb_hHaikuInitFail();
        created_app = 1;
    }

    fb_app = app;

    fb_hHaikuLockState();
    fb_haiku.app = app;
    fb_haiku.created_app = created_app;
    fb_hHaikuUnlockState();

    BRect frame(
        100,
        100,
        100 + fb_haiku.width  - 1,
        100 + fb_haiku.height - 1
    );

    if (fb_haiku.flags & DRIVER_FULLSCREEN)
    {
        BScreen screen;
        frame = screen.Frame();
    }

    g_win = new(std::nothrow) FBHaikuWindow(frame, title ? title : "FreeBASIC");
    if (!g_win)
        return fb_hHaikuInitFail(app, created_app);

#ifndef DISABLE_OPENGL
    if (fb_haiku.flags & DRIVER_OPENGL)
    {
        g_gl_view = new(std::nothrow)
            FBHaikuGLView(g_win->Bounds(), fb_hHaikuOpenGLViewOptions());
        g_view = g_gl_view;
    }
    else
#endif
    {
        g_view = new(std::nothrow) FBHaikuView(g_win->Bounds());
    }

    if (!g_view)
    {
        if (g_win->Lock())
            g_win->Quit();
        g_win = NULL;
        return fb_hHaikuInitFail(app, created_app);
    }

    g_win->AddChild(g_view);

    if (!(fb_haiku.flags & DRIVER_OPENGL))
    {
        g_bmp = fb_hHaikuCreateBitmap(fb_haiku.width, fb_haiku.height);

        if (!g_bmp || !g_bmp->IsValid() || !g_bmp->Bits())
        {
            if (g_bmp) { delete g_bmp; g_bmp = NULL; }
            if (g_win->Lock()) g_win->Quit();
            g_win = NULL;
            g_view = NULL;
            return fb_hHaikuInitFail(app, created_app);
        }
    }

    g_win->Show();

    if (g_win->Lock()) {
        g_view->MakeFocus(true);
        g_win->Activate(true);
        g_win->Unlock();
    }

    fb_hInitScancodes();

    fb_hHaikuLockState();
    fb_haiku.window = g_win;
    fb_haiku.view   = g_view;
    fb_haiku.bitmap = g_bmp;
#ifndef DISABLE_OPENGL
    fb_haiku.gl_view = g_gl_view;
#endif
    fb_haiku.gui_ready = 1;
    fb_hHaikuUnlockState();

    if (fb_haiku.gui_ready_sem >= B_OK)
        release_sem(fb_haiku.gui_ready_sem);

    if (created_app) {
        HAIKU_DEBUG("Application loop starting");
        app->Run();
        HAIKU_DEBUG("Application loop returned");
    } else {
        while (!fb_haiku.quitting)
            snooze(10000);
    }

    /* The native window is joined before shutdown reaches this point. Release
       bitmaps while BApplication's app_server connection is still available;
       BBitmap destruction communicates through that connection. */
    if (g_bmp) { delete g_bmp; g_bmp = NULL; }

    /* Run() returning does not destroy BApplication. Its destructor clears
       be_app and joins remaining windows; leaving it allocated makes the next
       mode reuse an application whose message loop has already stopped. */
    if (created_app)
    {
        HAIKU_DEBUG("Destroying application after Run");
        delete app;
        HAIKU_DEBUG("Application destroyed after Run");
    }

    fb_hHaikuLockState();
    fb_haiku.gui_running = 0;
    fb_haiku.gui_ready   = 0;
    fb_haiku.window      = NULL;
    fb_haiku.view        = NULL;
    fb_haiku.bitmap      = NULL;
    fb_haiku.gl_view     = NULL;
    fb_haiku.app         = NULL;
    fb_hHaikuUnlockState();

    g_view = NULL;
#ifndef DISABLE_OPENGL
    g_gl_view = NULL;
#endif
    g_win  = NULL;
    fb_app = NULL;

    if (fb_haiku.gui_exit_sem >= B_OK)
        release_sem(fb_haiku.gui_exit_sem);

    return 0;
}

/* ------------------------------------------------------------------------- */

int fb_hHaikuInit(char *title, int w, int h, int depth, int refresh, int flags)
{
    thread_id tid;

    if (fb_haiku_event_thread >= B_OK || g_win || fb_app)
        fb_hHaikuExit();

    fb_hHaikuResetState();

    if (fb_hHaikuCreateStateSync() != 0)
        return -1;

    fb_haiku.quitting = 0;
    fb_haiku.width    = w;
    fb_haiku.height   = h;
    fb_haiku.depth    = depth;
    fb_haiku.refresh  = refresh;
    fb_haiku.flags    = flags;
    fb_haiku.scanline_size = __fb_gfx ? __fb_gfx->scanline_size : 1;

    if (__fb_gfx)
        fb_hMemSet(__fb_gfx->key, FALSE, 128);

    tid = spawn_thread(
        fb_haiku_event_thread_func,
        "fb_haiku_gui_thread",
        B_NORMAL_PRIORITY,
        (void*)title
    );

    if (tid < B_OK) {
        fb_hHaikuDestroyStateSync();
        return -1;
    }

    fb_haiku_event_thread = tid;

    fb_hHaikuLockState();
    fb_haiku.gui_thread = tid;
    fb_hHaikuUnlockState();

    if (resume_thread(tid) != B_OK) {
        fb_hHaikuDestroyStateSync();
        fb_haiku_event_thread = -1;
        return -1;
    }

    if (fb_haiku.gui_ready_sem >= B_OK)
        acquire_sem(fb_haiku.gui_ready_sem);

    if (fb_haiku.gui_failed || !fb_haiku.gui_ready || !g_win || !g_view ||
        (!(flags & DRIVER_OPENGL) && !g_bmp)) {
        fb_hHaikuExit();
        return -1;
    }

#ifndef DISABLE_OPENGL
    if (flags & DRIVER_OPENGL) {
        g_gl_view->LockGL();
        fb_haiku.gl_locked = 1;
    }
#endif

    fb_haiku.initialized = 1;

    return 0;
}

/* ------------------------------------------------------------------------- */

void fb_hHaikuExit(void)
{
    thread_id tid = fb_haiku_event_thread;

    fb_haiku.quitting = 1;

#ifndef DISABLE_OPENGL
    if (g_gl_view && fb_haiku.gl_locked && (find_thread(NULL) != tid)) {
        g_gl_view->UnlockGL();
        fb_haiku.gl_locked = 0;
    }
#endif

    /* Quit the window synchronously before releasing its bitmap or state.
       Native hooks never take the driver mutex, so they can finish while the
       runtime owns the graphics lock during a SCREENRES mode transition. */
    if (g_win && g_win->Lock())
    {
        HAIKU_DEBUG("Quitting native window");
        g_win->Quit();
        HAIKU_DEBUG("Native window quit");
        g_win = NULL;
        g_view = NULL;
    }

    if (fb_app && fb_haiku.created_app)
    {
        HAIKU_DEBUG("Requesting application shutdown");
        fb_app->PostMessage(B_QUIT_REQUESTED);
    }

    /*
        fb_hHaikuExit() is also used for normal gfx mode changes.  Do not
        force the process to exit here, or old programs that switch from
        SCREEN 13 to SCREENRES during startup will be killed by their own
        mode switch.
    */
    if (tid >= B_OK && find_thread(NULL) != tid)
    {
        if (fb_haiku.gui_exit_sem >= B_OK)
        {
            HAIKU_DEBUG("Waiting for GUI shutdown");
            acquire_sem(fb_haiku.gui_exit_sem);
        }

        wait_for_thread(tid, NULL);
    }

    fb_haiku_event_thread = -1;
    fb_haiku.gui_thread   = -1;
    fb_haiku.initialized  = 0;

    fb_hHaikuDestroyLock();
    fb_hHaikuDestroyStateSync();
}

#endif

/* end of haiku_init.cpp */
