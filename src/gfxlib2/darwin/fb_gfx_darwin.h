#ifndef FB_GFX_DARWIN_H
#define FB_GFX_DARWIN_H

#include "../fb_gfx.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FB_DARWIN_STATE {
	int initialized;
	int quit_posted;
	int width;
	int height;
	int depth;
	int refresh_rate;
	int flags;
	int mouse_x;
	int mouse_y;
	int mouse_z;
	int mouse_buttons;
	int mouse_clip;
	int mouse_cursor;
	int desktop_width;
	int desktop_height;
	int has_focus;
	int view_width;
	int view_height;
	int scale;
	int draw_offset_x;
	int draw_offset_y;
	int draw_width;
	int draw_height;
	void *app;
	void *window;
	void *view;
	void *main_menu;
	void *app_menu;
	void *run_loop_mode;
} FB_DARWIN_STATE;

extern FB_DARWIN_STATE fb_darwin;
extern const GFXDRIVER fb_gfxDriverDarwin;

int fb_hDarwinInit(char *title, int w, int h, int depth, int refresh_rate, int flags);
void fb_hDarwinExit(void);
void fb_hDarwinLock(void);
void fb_hDarwinUnlock(void);
void fb_hDarwinSetPalette(int index, int r, int g, int b);
void fb_hDarwinWaitVSync(void);
int fb_hDarwinGetMouse(int *x, int *y, int *z, int *buttons, int *clip);
void fb_hDarwinSetMouse(int x, int y, int cursor, int clip);
void fb_hDarwinSetWindowTitle(char *title);
int fb_hDarwinSetWindowPos(int x, int y);
int *fb_hDarwinFetchModes(int depth, int *size);
void fb_hDarwinPollEvents(void);
void fb_hDarwinUpdate(void);
int fb_hDarwinScreenInfo(ssize_t *width, ssize_t *height, ssize_t *depth, ssize_t *refresh);

#ifdef __cplusplus
}
#endif

#endif
