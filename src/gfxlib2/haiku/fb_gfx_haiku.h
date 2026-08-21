#ifndef FB_GFX_HAIKU_H
#define FB_GFX_HAIKU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../fb_gfx.h"

#ifdef __cplusplus
}
#endif

#include <stdint.h>
#include <OS.h>

/* ------------------------------------------------------------------------- */
/* C++ GUI headers                                                           */
/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
#include <Application.h>
#include <Bitmap.h>
#include <Window.h>
#include <View.h>
#include <Locker.h>
#ifndef DISABLE_OPENGL
#include <GLView.h>
#endif
#endif

/* ------------------------------------------------------------------------- */
/* Backend state                                                             */
/* ------------------------------------------------------------------------- */

typedef struct FB_HAIKU_STATE
{
    /* display parameters */
    int width;
    int height;
    int depth;
    int refresh;
    int flags;

    /* lifecycle */
    int initialized;
    int quitting;
    int gl_locked;

    /*
        Thread that called into the graphics backend from the runtime side.
        This is the host/main thread we may need to force down when the user
        confirms quit from the window close button.
    */
    thread_id host_thread;

    /* mouse */
    int mouse_x;
    int mouse_y;
    int mouse_z;

    int mouse_buttons;
    int mouse_latched_buttons;
    int mouse_visible;
    int mouse_clip;

    /* desktop info */
    int desktop_width;
    int desktop_height;

#ifdef __cplusplus
    BApplication *app;
    BWindow *window;
    BView *view;
    BBitmap *bitmap;
#ifndef DISABLE_OPENGL
    BGLView *gl_view;
#else
    void *gl_view;
#endif
    BLocker *backend_lock;
#else
    void *app;
    void *window;
    void *view;
    void *bitmap;
    void *gl_view;
    void *backend_lock;
#endif

    thread_id gui_thread;

    int gui_running;
    int gui_ready;
    int gui_failed;

    int created_app;

    sem_id gui_ready_sem;
    sem_id gui_exit_sem;

} FB_HAIKU_STATE;

/* ------------------------------------------------------------------------- */
/* Global backend state                                                      */
/* ------------------------------------------------------------------------- */

extern FB_HAIKU_STATE fb_haiku;

/*
    Transitional global for code paths that still expect a standalone
    GUI/event thread identifier.
*/
extern thread_id fb_haiku_event_thread;

/* ------------------------------------------------------------------------- */
/* GUI objects (C++ only)                                                    */
/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
extern BBitmap *g_bmp;
extern BWindow *g_win;
extern BView   *g_view;
#ifndef DISABLE_OPENGL
extern BGLView *g_gl_view;
#endif
#endif

/* ------------------------------------------------------------------------- */
/* Backend lifecycle                                                         */
/* ------------------------------------------------------------------------- */

int  fb_hHaikuInit(char *title, int w, int h, int depth, int refresh_rate, int flags);
void fb_hHaikuExit(void);

/* ------------------------------------------------------------------------- */
/* Rendering                                                                 */
/* ------------------------------------------------------------------------- */

void fb_hHaikuUpdate(void);
void fb_hHaikuPollEvents(void);
void fb_hHaikuWaitVSync(void);
void fb_hHaikuSetPalette(int index, int r, int g, int b);
#ifndef DISABLE_OPENGL
void fb_hHaikuOpenGLFlip(void);
#endif

/* ------------------------------------------------------------------------- */
/* Window helpers                                                            */
/* ------------------------------------------------------------------------- */

void fb_hHaikuSetWindowTitle(char *title);
int  fb_hHaikuSetWindowPos(int x, int y);

/* ------------------------------------------------------------------------- */
/* Driver interface                                                          */
/* ------------------------------------------------------------------------- */

void fb_hHaikuLock(void);
void fb_hHaikuUnlock(void);

int  fb_hHaikuGetMouse(int *x, int *y, int *z, int *buttons, int *clip);
int  fb_hHaikuGetTouchCount(void);
int  fb_hHaikuGetTouch(int index, int *x, int *y, int *id);
void fb_hHaikuSetMouse(int x, int y, int cursor, int clip);

int *fb_hHaikuFetchModes(int depth, int *size);

/* ------------------------------------------------------------------------- */
/* Event bridge                                                              */
/* ------------------------------------------------------------------------- */

/*
    The event bridge does not inspect the originating BView.  Keep that
    argument opaque so the software and OpenGL views share one C++ signature
    without exposing a Haiku interface type through this common header.
*/
void fb_hHaikuHandleKeyDown(void *view, const char *bytes, int32_t numBytes);
void fb_hHaikuHandleKeyUp(void *view, const char *bytes, int32_t numBytes);

void fb_hHaikuHandleMouseMoved(void *view, int x, int y);
void fb_hHaikuHandleMouseDown(void *view, int x, int y, int buttons);
void fb_hHaikuHandleMouseUp(void *view, int x, int y, int buttons);

void fb_hHaikuPostQuitEvent(void);

/* ------------------------------------------------------------------------- */
/* Scancode translation                                                      */
/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

unsigned char fb_hHaikuTranslateScancode(unsigned char key);
int fb_hInitScancodes(void);

#ifdef __cplusplus
}
#endif

/* ------------------------------------------------------------------------- */
/* Platform helpers                                                          */
/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

void fb_hHaikuQueryDesktop(void);

#ifdef __cplusplus
}
#endif

void fb_hHaikuResetState(void);

int  fb_hHaikuCreateStateSync(void);
void fb_hHaikuDestroyStateSync(void);

void fb_hHaikuLockState(void);
void fb_hHaikuUnlockState(void);

void fb_hHaikuDestroyLock(void);

#endif
