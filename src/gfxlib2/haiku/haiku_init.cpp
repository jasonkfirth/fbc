#ifndef DISABLE_HAIKU

#include "fb_gfx_haiku.h"
#include "haiku_window.h"
#include "haiku_render.h"
#include "haiku_debug.h"
#include "../fb_gfx.h"

#include <Application.h>
#include <Bitmap.h>
#include <View.h>
#include <Window.h>
#include <OS.h>
#include <stdlib.h>
#include <new>

extern BBitmap *g_bmp;
extern BWindow *g_win;
extern BView   *g_view;

extern thread_id fb_haiku_event_thread;

static BApplication *fb_app = NULL;

#define FB_HAIKU_FORCE_EXIT_DELAY_US 500000

/* ------------------------------------------------------------------------- */
/* Force process exit fallback                                               */
/* ------------------------------------------------------------------------- */

static int32 fb_hHaikuForceExit(void*)
{
    snooze(FB_HAIKU_FORCE_EXIT_DELAY_US);
    exit(0);
    return 0;
}

/* ------------------------------------------------------------------------- */

static int32 fb_hHaikuInitFail(void)
{
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

    g_win = new(std::nothrow) FBHaikuWindow(frame, title ? title : "FreeBASIC");
    if (!g_win)
        return fb_hHaikuInitFail();

    g_view = new(std::nothrow) FBHaikuView(g_win->Bounds());
    if (!g_view)
    {
        if (g_win->Lock())
            g_win->Quit();
        g_win = NULL;
        return fb_hHaikuInitFail();
    }

    g_win->AddChild(g_view);

    g_bmp = new(std::nothrow)
        BBitmap(
            BRect(0, 0, fb_haiku.width - 1, fb_haiku.height - 1),
            B_BITMAP_ACCEPTS_VIEWS,
            B_RGB32
        );

    if (!g_bmp || !g_bmp->IsValid() || !g_bmp->Bits())
    {
        if (g_bmp) { delete g_bmp; g_bmp = NULL; }
        if (g_win->Lock()) g_win->Quit();
        g_win = NULL;
        g_view = NULL;
        return fb_hHaikuInitFail();
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
    fb_haiku.gui_ready = 1;
    fb_hHaikuUnlockState();

    if (fb_haiku.gui_ready_sem >= B_OK)
        release_sem(fb_haiku.gui_ready_sem);

    if (created_app) {
        app->Run();
    } else {
        while (!fb_haiku.quitting)
            snooze(10000);
    }

    if (g_bmp) { delete g_bmp; g_bmp = NULL; }

    fb_hHaikuLockState();
    fb_haiku.gui_running = 0;
    fb_haiku.gui_ready   = 0;
    fb_haiku.window      = NULL;
    fb_haiku.view        = NULL;
    fb_haiku.bitmap      = NULL;
    fb_haiku.app         = NULL;
    fb_hHaikuUnlockState();

    g_view = NULL;
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

    if (fb_haiku.gui_failed || !fb_haiku.gui_ready || !g_win || !g_view || !g_bmp) {
        fb_hHaikuExit();
        return -1;
    }

    fb_haiku.initialized = 1;

    return 0;
}

/* ------------------------------------------------------------------------- */

void fb_hHaikuExit(void)
{
    thread_id tid = fb_haiku_event_thread;
    thread_id killer;

    fb_haiku.quitting = 1;

    if (g_win)
        g_win->PostMessage(B_QUIT_REQUESTED);

    if (fb_app)
        fb_app->PostMessage(B_QUIT_REQUESTED);

    /* fallback: force process exit if runtime does not shut down */
    killer = spawn_thread(
        fb_hHaikuForceExit,
        "fb_haiku_force_exit",
        B_NORMAL_PRIORITY,
        NULL
    );

    if (killer >= B_OK)
        resume_thread(killer);

    if (tid >= B_OK && find_thread(NULL) != tid)
    {
        if (fb_haiku.gui_exit_sem >= B_OK)
            acquire_sem(fb_haiku.gui_exit_sem);

        wait_for_thread(tid, NULL);
    }

    fb_haiku_event_thread = -1;
    fb_haiku.gui_thread   = -1;
    fb_haiku.initialized  = 0;

    fb_hHaikuDestroyLock();
    fb_hHaikuDestroyStateSync();
}

#endif
