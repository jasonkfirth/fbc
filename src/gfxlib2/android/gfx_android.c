#include "fb_gfx_android.h"

#include <android/api-level.h>
#include <android/keycodes.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FB_ANDROID_LOG_TAG "FreeBASIC"
#define FB_ANDROID_CONSOLE_LINES 256
#define FB_ANDROID_CONSOLE_COLS 256
#define FB_ANDROID_FONT_W 6
#define FB_ANDROID_FONT_H 8
#define FB_ANDROID_KEYBOARD_BUTTON_W 56
#define FB_ANDROID_KEYBOARD_BUTTON_H 40

typedef struct FB_ANDROID_GFX_STATE
{
	pthread_mutex_t mutex;
	ANativeActivity *activity;
	ANativeWindow *window;
	int active;
	int width;
	int height;
	int depth;
	int refresh_rate;
	int mouse_x;
	int mouse_y;
	int mouse_z;
	int mouse_buttons;
	int window_width;
	int window_height;
	BLITTER *blitter;
	char console[FB_ANDROID_CONSOLE_LINES][FB_ANDROID_CONSOLE_COLS];
	int console_line;
	int console_col;
	int keyboard_button_down;
	int keyboard_visible;
} FB_ANDROID_GFX_STATE;

static FB_ANDROID_GFX_STATE fb_android =
{
	PTHREAD_MUTEX_INITIALIZER,
	NULL,
	NULL,
	0,
	0,
	0,
	0,
	60,
	0,
	0,
	0,
	0,
	0,
	0,
	NULL,
	{{0}},
	0,
	0,
	0,
	0
};

static void android_log(const char *text)
{
	if (text)
		__android_log_write(ANDROID_LOG_INFO, FB_ANDROID_LOG_TAG, text);
}

static uint32_t rgba(unsigned r, unsigned g, unsigned b)
{
	return 0xff000000u | ((r & 0xffu) << 16) | ((g & 0xffu) << 8) | (b & 0xffu);
}

static void sleep_ms(int ms)
{
	struct timespec req;

	req.tv_sec = ms / 1000;
	req.tv_nsec = (long)(ms % 1000) * 1000000L;
	nanosleep(&req, NULL);
}

static void update_window_size_locked(void)
{
	if (fb_android.window)
	{
		fb_android.window_width = ANativeWindow_getWidth(fb_android.window);
		fb_android.window_height = ANativeWindow_getHeight(fb_android.window);
	}
	else
	{
		fb_android.window_width = 0;
		fb_android.window_height = 0;
	}
}

static int keyboard_button_rect_locked(int *x0, int *y0, int *x1, int *y1)
{
	int w = fb_android.window_width > 0 ? fb_android.window_width : fb_android.width;

	if (w <= 0)
		return 0;

	*x1 = w - 8;
	*y0 = 8;
	*x0 = *x1 - FB_ANDROID_KEYBOARD_BUTTON_W;
	*y1 = *y0 + FB_ANDROID_KEYBOARD_BUTTON_H;
	return 1;
}

int fb_hAndroidKeyboardButtonHit(float x, float y)
{
	int x0, y0, x1, y1;
	int hit = 0;

	pthread_mutex_lock(&fb_android.mutex);
	if (keyboard_button_rect_locked(&x0, &y0, &x1, &y1))
		hit = ((int)x >= x0 && (int)x < x1 && (int)y >= y0 && (int)y < y1);
	pthread_mutex_unlock(&fb_android.mutex);

	return hit;
}

void fb_hAndroidToggleKeyboard(void)
{
	pthread_mutex_lock(&fb_android.mutex);
	if (fb_android.activity)
	{
		if (fb_android.keyboard_visible)
		{
			ANativeActivity_hideSoftInput(fb_android.activity, 0);
			fb_android.keyboard_visible = 0;
		}
		else
		{
			ANativeActivity_showSoftInput(fb_android.activity, 0);
			fb_android.keyboard_visible = 1;
		}
	}
	pthread_mutex_unlock(&fb_android.mutex);
}

void fb_hAndroidSetActivity(ANativeActivity *activity)
{
	pthread_mutex_lock(&fb_android.mutex);
	fb_android.activity = activity;
	pthread_mutex_unlock(&fb_android.mutex);
}

void fb_hAndroidSetWindow(ANativeWindow *window)
{
	ANativeWindow *old_window;

	if (window)
		ANativeWindow_acquire(window);

	pthread_mutex_lock(&fb_android.mutex);
	old_window = fb_android.window;
	fb_android.window = window;
	update_window_size_locked();
	pthread_mutex_unlock(&fb_android.mutex);

	if (old_window)
		ANativeWindow_release(old_window);
}

int fb_hAndroidIsGraphicsActive(void)
{
	int active;

	pthread_mutex_lock(&fb_android.mutex);
	active = fb_android.active;
	pthread_mutex_unlock(&fb_android.mutex);

	return active;
}

int fb_hAndroidInit(char *title, int w, int h, int depth, int refresh_rate, int flags, int require_api26)
{
	(void)title;

	if (w <= 0 || h <= 0 || depth <= 0)
		return 0;

	if (flags & DRIVER_OPENGL)
		return -1;

	if (require_api26 && android_get_device_api_level() < 26)
		return -1;

	pthread_mutex_lock(&fb_android.mutex);

	if (!fb_android.window)
	{
		pthread_mutex_unlock(&fb_android.mutex);
		return -1;
	}

	fb_android.width = w;
	fb_android.height = h;
	fb_android.depth = depth;
	fb_android.refresh_rate = refresh_rate > 0 ? refresh_rate : 60;
	fb_android.mouse_x = w / 2;
	fb_android.mouse_y = h / 2;
	fb_android.mouse_z = 0;
	fb_android.mouse_buttons = 0;
	fb_android.blitter = fb_hGetBlitter(32, TRUE);

	if (!fb_android.blitter)
	{
		pthread_mutex_unlock(&fb_android.mutex);
		return -1;
	}

	ANativeWindow_setBuffersGeometry(fb_android.window, w, h, WINDOW_FORMAT_RGBA_8888);
	update_window_size_locked();
	fb_android.active = 1;
	__fb_gfx->refresh_rate = fb_android.refresh_rate;

	pthread_mutex_unlock(&fb_android.mutex);

	android_log("Android gfx driver initialized");
	fb_hAndroidUpdate();
	return 0;
}

void fb_hAndroidExit(void)
{
	pthread_mutex_lock(&fb_android.mutex);
	fb_android.active = 0;
	fb_android.width = 0;
	fb_android.height = 0;
	fb_android.depth = 0;
	fb_android.blitter = NULL;
	pthread_mutex_unlock(&fb_android.mutex);
}

void fb_hAndroidLock(void)
{
	pthread_mutex_lock(&fb_android.mutex);
}

void fb_hAndroidUnlock(void)
{
	pthread_mutex_unlock(&fb_android.mutex);
	fb_hAndroidUpdate();
}

void fb_hAndroidWaitVSync(void)
{
	int refresh;

	pthread_mutex_lock(&fb_android.mutex);
	refresh = fb_android.refresh_rate > 0 ? fb_android.refresh_rate : 60;
	pthread_mutex_unlock(&fb_android.mutex);

	sleep_ms(1000 / refresh);
}

int fb_hAndroidGetMouse(int *x, int *y, int *z, int *buttons, int *clip)
{
	pthread_mutex_lock(&fb_android.mutex);
	*x = fb_android.mouse_x;
	*y = fb_android.mouse_y;
	*z = fb_android.mouse_z;
	*buttons = fb_android.mouse_buttons;
	*clip = 0;
	pthread_mutex_unlock(&fb_android.mutex);

	return 0;
}

void fb_hAndroidSetMouse(int x, int y, int cursor, int clip)
{
	(void)cursor;
	(void)clip;

	pthread_mutex_lock(&fb_android.mutex);
	if (x >= 0)
		fb_android.mouse_x = x;
	if (y >= 0)
		fb_android.mouse_y = y;
	pthread_mutex_unlock(&fb_android.mutex);
}

void fb_hAndroidSetWindowTitle(char *title)
{
	(void)title;
}

int fb_hAndroidSetWindowPos(int x, int y)
{
	(void)x;
	(void)y;
	return 0;
}

int *fb_hAndroidFetchModes(int depth, int *size)
{
	int *modes;

	(void)depth;

	modes = (int *)malloc(sizeof(int) * 3);
	if (!modes)
	{
		*size = 0;
		return NULL;
	}

	modes[0] = (480 << 16) | 320;
	modes[1] = (800 << 16) | 480;
	modes[2] = (1280 << 16) | 720;
	*size = 3;
	return modes;
}

void fb_hAndroidPollEvents(void)
{
}

static void draw_rect(ANativeWindow_Buffer *buffer, int x0, int y0, int x1, int y1, uint32_t color)
{
	int x, y;

	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 > buffer->width) x1 = buffer->width;
	if (y1 > buffer->height) y1 = buffer->height;

	for (y = y0; y < y1; ++y)
	{
		uint32_t *row = (uint32_t *)buffer->bits + (y * buffer->stride);
		for (x = x0; x < x1; ++x)
			row[x] = color;
	}
}

static unsigned glyph_row(char c, int row)
{
	static const unsigned blank[7] = {0, 0, 0, 0, 0, 0, 0};
	static const unsigned dash[7] = {0, 0, 0, 31, 0, 0, 0};
	static const unsigned colon[7] = {0, 4, 4, 0, 4, 4, 0};
	static const unsigned dot[7] = {0, 0, 0, 0, 0, 12, 12};
	static const unsigned zero[7] = {14, 17, 19, 21, 25, 17, 14};
	static const unsigned one[7] = {4, 12, 4, 4, 4, 4, 14};
	static const unsigned two[7] = {14, 17, 1, 2, 4, 8, 31};
	static const unsigned three[7] = {30, 1, 1, 14, 1, 1, 30};
	static const unsigned four[7] = {2, 6, 10, 18, 31, 2, 2};
	static const unsigned five[7] = {31, 16, 30, 1, 1, 17, 14};
	static const unsigned six[7] = {6, 8, 16, 30, 17, 17, 14};
	static const unsigned seven[7] = {31, 1, 2, 4, 8, 8, 8};
	static const unsigned eight[7] = {14, 17, 17, 14, 17, 17, 14};
	static const unsigned nine[7] = {14, 17, 17, 15, 1, 2, 12};
	static const unsigned letters[26][7] =
	{
		{14,17,17,31,17,17,17}, {30,17,17,30,17,17,30}, {14,17,16,16,16,17,14},
		{30,17,17,17,17,17,30}, {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
		{14,17,16,23,17,17,15}, {17,17,17,31,17,17,17}, {14,4,4,4,4,4,14},
		{7,2,2,2,18,18,12}, {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
		{17,27,21,21,17,17,17}, {17,25,21,19,17,17,17}, {14,17,17,17,17,17,14},
		{30,17,17,30,16,16,16}, {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
		{15,16,16,14,1,1,30}, {31,4,4,4,4,4,4}, {17,17,17,17,17,17,14},
		{17,17,17,17,17,10,4}, {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
		{17,17,10,4,4,4,4}, {31,1,2,4,8,16,31}
	};
	const unsigned *g = blank;

	if (row <= 0 || row >= 8)
		return 0;

	row--;
	if (c >= 'a' && c <= 'z')
		c = (char)(c - 'a' + 'A');

	if (c >= 'A' && c <= 'Z')
		g = letters[c - 'A'];
	else if (c >= '0' && c <= '9')
	{
		static const unsigned *digits[10] = {zero, one, two, three, four, five, six, seven, eight, nine};
		g = digits[c - '0'];
	}
	else if (c == '-')
		g = dash;
	else if (c == ':')
		g = colon;
	else if (c == '.')
		g = dot;

	return g[row];
}

static void draw_char(ANativeWindow_Buffer *buffer, int px, int py, char ch, uint32_t fg, uint32_t bg)
{
	int x, y;

	draw_rect(buffer, px, py, px + FB_ANDROID_FONT_W, py + FB_ANDROID_FONT_H, bg);

	for (y = 0; y < 8; ++y)
	{
		unsigned bits = glyph_row(ch, y);
		int yy = py + y;

		if (yy < 0 || yy >= buffer->height)
			continue;

		for (x = 0; x < 5; ++x)
		{
			int xx = px + x;
			if (xx < 0 || xx >= buffer->width)
				continue;
			if (bits & (1u << (4 - x)))
				*((uint32_t *)buffer->bits + yy * buffer->stride + xx) = fg;
		}
	}
}

static void draw_text(ANativeWindow_Buffer *buffer, int x, int y, const char *text, uint32_t fg, uint32_t bg)
{
	while (*text)
	{
		draw_char(buffer, x, y, *text, fg, bg);
		x += FB_ANDROID_FONT_W;
		text++;
	}
}

static void draw_keyboard_button_locked(ANativeWindow_Buffer *buffer)
{
	int x0, y0, x1, y1;
	uint32_t fill;

	if (!keyboard_button_rect_locked(&x0, &y0, &x1, &y1))
		return;

	fill = fb_android.keyboard_visible ? rgba(40, 120, 180) : rgba(54, 59, 68);
	draw_rect(buffer, x0, y0, x1, y1, rgba(16, 18, 24));
	draw_rect(buffer, x0 + 2, y0 + 2, x1 - 2, y1 - 2, fill);
	draw_text(buffer, x0 + 10, y0 + 16, "KB", rgba(245, 246, 250), fill);
}

void fb_hAndroidUpdate(void)
{
	ANativeWindow_Buffer buffer;
	ANativeWindow *window;
	BLITTER *blitter;

	pthread_mutex_lock(&fb_android.mutex);
	window = fb_android.window;
	blitter = fb_android.blitter;
	if (!fb_android.active || !window || !blitter || !__fb_gfx || !__fb_gfx->framebuffer)
	{
		pthread_mutex_unlock(&fb_android.mutex);
		return;
	}

	if (ANativeWindow_lock(window, &buffer, NULL) != 0)
	{
		pthread_mutex_unlock(&fb_android.mutex);
		return;
	}

	blitter((unsigned char *)buffer.bits, buffer.stride * 4);
	draw_keyboard_button_locked(&buffer);
	ANativeWindow_unlockAndPost(window);
	pthread_mutex_unlock(&fb_android.mutex);
}

void fb_hAndroidConsoleRender(void)
{
	ANativeWindow_Buffer buffer;
	ANativeWindow *window;
	int cols, rows, first_line, i, line;

	pthread_mutex_lock(&fb_android.mutex);
	window = fb_android.window;
	if (fb_android.active || !window)
	{
		pthread_mutex_unlock(&fb_android.mutex);
		return;
	}

	ANativeWindow_setBuffersGeometry(window, 0, 0, WINDOW_FORMAT_RGBA_8888);
	update_window_size_locked();
	if (ANativeWindow_lock(window, &buffer, NULL) != 0)
	{
		pthread_mutex_unlock(&fb_android.mutex);
		return;
	}

	draw_rect(&buffer, 0, 0, buffer.width, buffer.height, rgba(9, 12, 18));
	cols = buffer.width / FB_ANDROID_FONT_W;
	rows = buffer.height / FB_ANDROID_FONT_H;
	if (cols > FB_ANDROID_CONSOLE_COLS - 1)
		cols = FB_ANDROID_CONSOLE_COLS - 1;

	first_line = fb_android.console_line - rows + 2;
	if (first_line < 0)
		first_line += FB_ANDROID_CONSOLE_LINES;

	for (i = 0; i < rows - 1; ++i)
	{
		line = (first_line + i) % FB_ANDROID_CONSOLE_LINES;
		fb_android.console[line][cols] = '\0';
		draw_text(&buffer, 6, 6 + i * FB_ANDROID_FONT_H, fb_android.console[line],
			rgba(224, 235, 245), rgba(9, 12, 18));
	}

	draw_keyboard_button_locked(&buffer);
	ANativeWindow_unlockAndPost(window);
	pthread_mutex_unlock(&fb_android.mutex);
}

void fb_hAndroidConsoleWrite(const char *text, size_t length)
{
	size_t i;

	if (!text || length == 0)
		return;

	pthread_mutex_lock(&fb_android.mutex);
	for (i = 0; i < length; ++i)
	{
		unsigned char ch = (unsigned char)text[i];

		if (ch == '\r')
			continue;

		if (ch == '\n')
		{
			fb_android.console_line = (fb_android.console_line + 1) % FB_ANDROID_CONSOLE_LINES;
			fb_android.console_col = 0;
			memset(fb_android.console[fb_android.console_line], 0, FB_ANDROID_CONSOLE_COLS);
			continue;
		}

		if (ch == '\b')
		{
			if (fb_android.console_col > 0)
				fb_android.console[fb_android.console_line][--fb_android.console_col] = '\0';
			continue;
		}

		if (fb_android.console_col >= FB_ANDROID_CONSOLE_COLS - 1)
		{
			fb_android.console_line = (fb_android.console_line + 1) % FB_ANDROID_CONSOLE_LINES;
			fb_android.console_col = 0;
			memset(fb_android.console[fb_android.console_line], 0, FB_ANDROID_CONSOLE_COLS);
		}

		if (ch >= 32 && ch < 127)
			fb_android.console[fb_android.console_line][fb_android.console_col++] = (char)ch;
	}
	pthread_mutex_unlock(&fb_android.mutex);

	fb_hAndroidConsoleRender();
}

void fb_hAndroidTouch(float x, float y, int action)
{
	EVENT e;
	int w, h, mapped_x, mapped_y;

	if (fb_hAndroidKeyboardButtonHit(x, y))
	{
		if (action == AMOTION_EVENT_ACTION_DOWN)
			fb_android.keyboard_button_down = 1;
		else if (action == AMOTION_EVENT_ACTION_UP && fb_android.keyboard_button_down)
		{
			fb_android.keyboard_button_down = 0;
			fb_hAndroidToggleKeyboard();
			fb_hAndroidConsoleRender();
			fb_hAndroidUpdate();
		}
		return;
	}

	pthread_mutex_lock(&fb_android.mutex);
	w = fb_android.window_width > 0 ? fb_android.window_width : fb_android.width;
	h = fb_android.window_height > 0 ? fb_android.window_height : fb_android.height;
	mapped_x = (w > 0 && fb_android.width > 0) ? (int)(x * fb_android.width / w) : (int)x;
	mapped_y = (h > 0 && fb_android.height > 0) ? (int)(y * fb_android.height / h) : (int)y;
	fb_android.mouse_x = mapped_x;
	fb_android.mouse_y = mapped_y;

	if (action == AMOTION_EVENT_ACTION_DOWN)
		fb_android.mouse_buttons |= BUTTON_LEFT;
	else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL)
		fb_android.mouse_buttons &= ~BUTTON_LEFT;

	if (!fb_android.active)
	{
		pthread_mutex_unlock(&fb_android.mutex);
		return;
	}
	pthread_mutex_unlock(&fb_android.mutex);

	memset(&e, 0, sizeof(e));
	if (action == AMOTION_EVENT_ACTION_MOVE)
	{
		e.type = EVENT_MOUSE_MOVE;
		e.x = mapped_x;
		e.y = mapped_y;
	}
	else if (action == AMOTION_EVENT_ACTION_DOWN)
	{
		e.type = EVENT_MOUSE_BUTTON_PRESS;
		e.button = BUTTON_LEFT;
	}
	else if (action == AMOTION_EVENT_ACTION_UP)
	{
		e.type = EVENT_MOUSE_BUTTON_RELEASE;
		e.button = BUTTON_LEFT;
	}
	else
		return;
	fb_hPostEvent(&e);
}

static int android_key_to_scancode(int32_t keycode)
{
	if (keycode >= AKEYCODE_A && keycode <= AKEYCODE_Z)
		return SC_A + (keycode - AKEYCODE_A);
	if (keycode >= AKEYCODE_1 && keycode <= AKEYCODE_9)
		return SC_1 + (keycode - AKEYCODE_1);
	if (keycode == AKEYCODE_0)
		return SC_0;

	switch (keycode)
	{
	case AKEYCODE_ESCAPE: return SC_ESCAPE;
	case AKEYCODE_DEL: return SC_BACKSPACE;
	case AKEYCODE_TAB: return SC_TAB;
	case AKEYCODE_ENTER: return SC_ENTER;
	case AKEYCODE_SPACE: return SC_SPACE;
	case AKEYCODE_MINUS: return SC_MINUS;
	case AKEYCODE_EQUALS: return SC_EQUALS;
	case AKEYCODE_DPAD_LEFT: return SC_LEFT;
	case AKEYCODE_DPAD_RIGHT: return SC_RIGHT;
	case AKEYCODE_DPAD_UP: return SC_UP;
	case AKEYCODE_DPAD_DOWN: return SC_DOWN;
	default: return 0;
	}
}

void fb_hAndroidKey(int32_t keycode, int action, int unicode)
{
	EVENT e;
	int scancode;

	pthread_mutex_lock(&fb_android.mutex);
	if (!fb_android.active)
	{
		pthread_mutex_unlock(&fb_android.mutex);
		return;
	}
	pthread_mutex_unlock(&fb_android.mutex);

	scancode = android_key_to_scancode(keycode);
	if (!scancode && unicode <= 0)
		return;

	memset(&e, 0, sizeof(e));
	if (action == AKEY_EVENT_ACTION_DOWN)
		e.type = EVENT_KEY_PRESS;
	else if (action == AKEY_EVENT_ACTION_UP)
		e.type = EVENT_KEY_RELEASE;
	else
		return;

	e.scancode = scancode;
	e.ascii = (unicode >= 32 && unicode < 127) ? unicode : 0;
	fb_hPostEvent(&e);
}

void fb_hAndroidScreenInfo(ssize_t *width, ssize_t *height, ssize_t *depth, ssize_t *refresh)
{
	pthread_mutex_lock(&fb_android.mutex);
	update_window_size_locked();
	*width = fb_android.window_width > 0 ? fb_android.window_width : 800;
	*height = fb_android.window_height > 0 ? fb_android.window_height : 480;
	*depth = 32;
	*refresh = 60;
	pthread_mutex_unlock(&fb_android.mutex);
}
