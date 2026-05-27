#ifndef FB_GFX_ANDROID_H
#define FB_GFX_ANDROID_H

#include "../fb_gfx.h"

#include <android/input.h>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const GFXDRIVER fb_gfxDriverAndroidModern;
extern const GFXDRIVER fb_gfxDriverAndroidLegacy;

int fb_hAndroidInit(char *title, int w, int h, int depth, int refresh_rate, int flags, int require_api26);
void fb_hAndroidExit(void);
void fb_hAndroidLock(void);
void fb_hAndroidUnlock(void);
void fb_hAndroidSetPalette(int index, int r, int g, int b);
void fb_hAndroidWaitVSync(void);
int fb_hAndroidGetMouse(int *x, int *y, int *z, int *buttons, int *clip);
void fb_hAndroidSetMouse(int x, int y, int cursor, int clip);
void fb_hAndroidSetWindowTitle(char *title);
int fb_hAndroidSetWindowPos(int x, int y);
int *fb_hAndroidFetchModes(int depth, int *size);
void fb_hAndroidPollEvents(void);
void fb_hAndroidUpdate(void);
void fb_hAndroidScreenInfo(ssize_t *width, ssize_t *height, ssize_t *depth, ssize_t *refresh);

void fb_hAndroidSetActivity(ANativeActivity *activity);
void fb_hAndroidSetKeyboardEnabled(int enabled);
void fb_hAndroidSetWindow(ANativeWindow *window);
void fb_hAndroidGfxSetLifecycle(int started, int resumed, int focused);
int fb_hAndroidIsGraphicsActive(void);
void fb_hAndroidTouch(float x, float y, int action);
void fb_hAndroidTouchEvent(const AInputEvent *event);
int fb_hAndroidGetTouchCount(void);
int fb_hAndroidGetTouch(int index, int *x, int *y, int *id);
void fb_hAndroidKey(int32_t keycode, int action, int unicode);
void fb_hAndroidSetConsoleEnabled(int enabled);
void fb_hAndroidConsoleWrite(const char *text, size_t length);
void fb_hAndroidConsoleRender(void);
int fb_hAndroidKeyboardButtonHit(float x, float y);
void fb_hAndroidToggleKeyboard(void);
void fb_hAndroidGamepadMotion(const AInputEvent *event);
void fb_hAndroidGamepadKey(const AInputEvent *event);
int fb_hAndroidGetJoystick(int id, ssize_t *buttons,
                           float *a1, float *a2,
                           float *a3, float *a4,
                           float *a5, float *a6,
                           float *a7, float *a8);
int fb_hAndroidGetXPad(int id, ssize_t *buttons,
                       float *lstick_x, float *lstick_y,
                       float *rstick_x, float *rstick_y,
                       float *ltrigger, float *rtrigger,
                       ssize_t *dpad);

#ifdef __cplusplus
}
#endif

#endif
