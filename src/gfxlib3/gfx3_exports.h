/*
 * Project: FreeBASIC gfxlib3
 * File: gfx3_exports.h
 * Purpose: Declare the C entry points implemented by the gfxlib3 runtime.
 * Responsibilities: Check exported definitions against one typed ABI contract.
 * This file does not implement rendering or define platform driver interfaces.
 */
#ifndef FB_GFX3_EXPORTS_H
#define FB_GFX3_EXPORTS_H

#include "gfx3_image.h"

/* The public point record is defined by the surface owner. Only its address
   crosses these declarations; its byte layout is unchanged. */
struct FB_GFX3_PUBLIC_POINT;

#ifdef __cplusplus
extern "C" {
#endif

/* api/gfx3_api.c */
FBCALL int fb_GfxScreenRes(int width, int height, int depth, int page_count,
	int flags, int refresh_rate);
FBCALL int fb_GfxScreen(int mode, int depth, int page_count, int flags,
	int refresh_rate);
FBCALL int fb_GfxScreenQB(int mode, int visible_page, int active_page);
FBCALL void fb_GfxSetWindowTitle(FBSTRING_ACCESS *title);
FBCALL void fb_GfxPset(void *target, float x, float y, unsigned int color,
	int flags, int preset);
FBCALL unsigned int fb_GfxPoint(void *target, float x, float y);
FBCALL void fb_GfxLine(void *target, float x1, float y1, float x2, float y2,
	unsigned int color, int type, unsigned int style, int flags);
FBCALL void fb_GfxEllipse(void *target, float x, float y, float radius,
	unsigned int color, float aspect, float start, float end, int filled,
	int flags);
FBCALL void fb_GfxView(int x1, int y1, int x2, int y2,
	unsigned int fill_color, unsigned int border_color, int flags);
FBCALL void fb_GfxWindow(float x1, float y1, float x2, float y2, int screen);
FBCALL float fb_GfxPMap(float coordinate, int function);
int fb_GfxPageSet(int work_page, int visible_page);
int fb_GfxPageCopy(int from_page, int to_page);
FBCALL int fb_GfxFlip(int from_page, int to_page);

/* api/gfx3_draw_api.c */
FBCALL void fb_GfxDraw(void *target, FBSTRING_ACCESS *command);

/* api/gfx3_file_api.c */
FBCALL int fb_GfxBsaveEx(FBSTRING_ACCESS *filename, void *source,
	unsigned int size, void *palette, int bits_per_pixel);
FBCALL int fb_GfxBsave(FBSTRING_ACCESS *filename, void *source,
	unsigned int size, void *palette);
FBCALL int fb_GfxBload(FBSTRING_ACCESS *filename, void *destination,
	void *palette);
FBCALL int fb_GfxBloadQB(FBSTRING_ACCESS *filename, void *destination,
	void *palette);

/* api/gfx3_image_api.c */
FBCALL void *fb_GfxImageCreate(int width, int height, unsigned int color,
	int depth, int flags);
FBCALL void *fb_GfxImageCreateQB(int width, int height, unsigned int color,
	int depth, int flags);
FBCALL void fb_GfxImageDestroy(void *image);
FBCALL int fb_GfxImageInfo(void *image, ssize_t *width, ssize_t *height,
	ssize_t *bytes_per_pixel, ssize_t *pitch, void **pixels, ssize_t *size);
FBCALL int fb_GfxImageInfo32(void *image, int *width, int *height,
	int *bytes_per_pixel, int *pitch, void **pixels, int *size);
FBCALL int fb_GfxImageInfo64(void *image, long long *width,
	long long *height, long long *bytes_per_pixel, long long *pitch,
	void **pixels, long long *size);
FBCALL int fb_GfxGet(void *target, float x1, float y1, float x2, float y2,
	unsigned char *destination, int flags, FBARRAY_ACCESS *array);
FBCALL int fb_GfxGetQB(void *target, float x1, float y1, float x2, float y2,
	unsigned char *destination, int flags, FBARRAY_ACCESS *array);
FBCALL int fb_Gfx3ScreenCapture(void *destination, int x, int y,
	int width, int height);
FBCALL int fb_GfxPut(void *target, float x, float y, unsigned char *source,
	int x1, int y1, int x2, int y2, int flags, int mode, PUTTER *putter,
	int alpha, BLENDER *blender, void *parameter);
FBCALL void fb_GfxImageConvertRow(const unsigned char *source,
	int source_depth, unsigned char *destination, int destination_depth,
	int width, int source_is_rgb);
void fb_hPutTrans(unsigned char *source, unsigned char *destination, int width,
	int height, int source_pitch, int destination_pitch, int alpha,
	BLENDER *blender, void *parameter);
void fb_hPutPSet(unsigned char *source, unsigned char *destination, int width,
	int height, int source_pitch, int destination_pitch, int alpha,
	BLENDER *blender, void *parameter);
void fb_hPutPReset(unsigned char *source, unsigned char *destination, int width,
	int height, int source_pitch, int destination_pitch, int alpha,
	BLENDER *blender, void *parameter);
void fb_hPutAnd(unsigned char *source, unsigned char *destination, int width,
	int height, int source_pitch, int destination_pitch, int alpha,
	BLENDER *blender, void *parameter);
void fb_hPutOr(unsigned char *source, unsigned char *destination, int width,
	int height, int source_pitch, int destination_pitch, int alpha,
	BLENDER *blender, void *parameter);
void fb_hPutXor(unsigned char *source, unsigned char *destination, int width,
	int height, int source_pitch, int destination_pitch, int alpha,
	BLENDER *blender, void *parameter);
void fb_hPutAlpha(unsigned char *source, unsigned char *destination, int width,
	int height, int source_pitch, int destination_pitch, int alpha,
	BLENDER *blender, void *parameter);
void fb_hPutAdd(unsigned char *source, unsigned char *destination, int width,
	int height, int source_pitch, int destination_pitch, int alpha,
	BLENDER *blender, void *parameter);
void fb_hPutBlend(unsigned char *source, unsigned char *destination, int width,
	int height, int source_pitch, int destination_pitch, int alpha,
	BLENDER *blender, void *parameter);
void fb_hPutCustom(unsigned char *source, unsigned char *destination, int width,
	int height, int source_pitch, int destination_pitch, int alpha,
	BLENDER *blender, void *parameter);

/* api/gfx3_input_api.c */
FBSTRING_ACCESS *fb_GfxInkey(void);
int fb_GfxGetkey(void);
int fb_GfxKeyHit(void);
int fb_GfxMultikey(int scancode);
int fb_GfxGetMouse(int *x, int *y, int *z, int *buttons, int *clip);
int fb_GfxSetMouse(int x, int y, int cursor, int clip);
FBCALL int fb_GfxDropEnable(int enabled);
FBCALL int fb_GfxDropGet(int identifier, int index, FBSTRING_ACCESS *output);
FBCALL int fb_GfxEvent(EVENT *event);
int fb_GfxIsRedir(int is_input);
void fb_GfxSleep(int milliseconds);

/* api/gfx3_interop_api.c */
FBCALL int fb_Gfx3RunOnRenderThread(FB_GFX3_INTEROP_CALLBACK callback,
	void *user_data);
FBCALL int fb_Gfx3SetExternalPresentation(int enabled);

/* api/gfx3_paint_api.c */
FBCALL void fb_GfxPaint(void *target, float x, float y, unsigned int color,
	unsigned int border_color, FBSTRING_ACCESS *pattern, int paint_mode, int flags);
FBCALL int fb_GfxPaintPattern(void *target, float x, float y, FBSTRING_ACCESS *pattern,
	unsigned int foreground, unsigned int background, unsigned int border,
	int relative);

/* api/gfx3_query_api.c */
FBCALL void fb_GfxScreenInfo(ssize_t *width, ssize_t *height,
	ssize_t *depth, ssize_t *bytes_per_pixel, ssize_t *pitch,
	ssize_t *refresh, FBSTRING_ACCESS *driver);
FBCALL void fb_GfxScreenInfo32(int *width, int *height, int *depth,
	int *bytes_per_pixel, int *pitch, int *refresh, FBSTRING_ACCESS *driver);
FBCALL void fb_GfxScreenInfo64(long long *width, long long *height,
	long long *depth, long long *bytes_per_pixel, long long *pitch,
	long long *refresh, FBSTRING_ACCESS *driver);
FBCALL int fb_GfxScreenList(int depth);
FBCALL void fb_GfxLock(void);
FBCALL void fb_GfxUnlock(int start_line, int end_line);
FBCALL void *fb_GfxScreenPtr(void);
FBCALL int fb_GfxWaitVSync(void);
FBCALL void fb_GfxControl_s(int what, FBSTRING_ACCESS *parameter);
FBCALL void fb_GfxControl_i(int what, ssize_t *parameter1,
	ssize_t *parameter2, ssize_t *parameter3, ssize_t *parameter4);
FBCALL void fb_GfxControl_i32(int what, int *parameter1, int *parameter2,
	int *parameter3, int *parameter4);
FBCALL void fb_GfxControl_i64(int what, long long *parameter1,
	long long *parameter2, long long *parameter3, long long *parameter4);
FBCALL void *fb_GfxGetGLProcAddress(const char *procedure);
FBCALL void fb_GfxPalette(int index, int red, int green, int blue);
FBCALL void fb_GfxPaletteGet(int index, int *red, int *green, int *blue);
FBCALL void fb_GfxPaletteGet64(int index, long long *red,
	long long *green, long long *blue);
FBCALL void fb_GfxPaletteUsing(int *data);
FBCALL void fb_GfxPaletteUsing64(long long *data);
FBCALL void fb_GfxPaletteGetUsing(int *data);
FBCALL void fb_GfxPaletteGetUsing64(long long *data);
FBCALL int fb_GfxGetJoystick(int id, ssize_t *buttons, float *axis1,
	float *axis2, float *axis3, float *axis4, float *axis5, float *axis6,
	float *axis7, float *axis8);
FBCALL int fb_GfxGetXPad(int id, ssize_t *buttons, float *left_x,
	float *left_y, float *right_x, float *right_y, float *left_trigger,
	float *right_trigger, ssize_t *dpad);
FBCALL ssize_t fb_GfxGetTouchCount(void);
FBCALL ssize_t fb_GfxGetTouch(ssize_t index, ssize_t *x, ssize_t *y,
	ssize_t *id);
FBCALL ssize_t fb_GfxGetTouchHit(ssize_t x1, ssize_t y1, ssize_t x2,
	ssize_t y2);
FBCALL ssize_t fb_GfxGetTouchHitCircle(ssize_t x, ssize_t y,
	ssize_t radius);

/* api/gfx3_text_api.c */
FBCALL int fb_GfxDrawStringSize(FBSTRING_ACCESS *string, int *width, int *height,
	void *font_image);
FBCALL int fb_GfxDrawString(void *target, float x, float y, int flags,
	FBSTRING_ACCESS *string, unsigned int color, void *font_image, int mode,
	PUTTER *putter, BLENDER *blender, void *parameter);

/* api/gfx3_vga_api.c */
int fb_GfxIn(unsigned short port);
int fb_GfxOut(unsigned short port, unsigned char value);

/* compat/gfx3_blender_compat.c */
void *fb_hPixelSetAlpha4(void *destination, int color, size_t count);
int fb_hSimdAvailable(void);
void *fb_hPixelSetAlpha4SIMD(void *destination, int color, size_t count);

/* compat/gfx3_line_input.c */
char *fb_GfxReadStr(char *buffer, ssize_t maximum_length);
int fb_GfxLineInput(FBSTRING_ACCESS *text, void *destination,
	ssize_t destination_length, int fill_remaining, int add_question,
	int add_newline);
int fb_GfxLineInputWstr(const FB_WCHAR *text, FB_WCHAR *destination,
	ssize_t maximum_characters, int add_question, int add_newline);

/* compat/gfx3_qb_input.c */
FBCALL int fb_GfxStickQB(int index);
FBCALL int fb_GfxStrigQB(int index);

/* core/gfx3_gpu_surface.c */
FBCALL void *fb_Gfx3SurfaceCreate(int width, int height, int depth,
	unsigned int usage, unsigned int clear_color);
FBCALL void *fb_Gfx3SurfaceLoad(FBSTRING_ACCESS *filename, int depth,
	unsigned int usage);
FBCALL void *fb_Gfx3SurfaceLoadWstr(const FB_WCHAR *filename, int depth,
	unsigned int usage);
FBCALL int fb_Gfx3SurfaceDestroy(void *surface_pointer);
FBCALL int fb_Gfx3SurfaceInfo(void *surface_pointer, int *width, int *height,
	int *depth, unsigned int *usage);
FBCALL int fb_Gfx3SurfaceUpload(void *surface_pointer, int x, int y,
	int width, int height, int pitch, const void *pixels);
FBCALL int fb_Gfx3SurfaceDownload(void *surface_pointer, int x, int y,
	int width, int height, int pitch, void *pixels);
FBCALL int fb_Gfx3SurfaceMap(void *surface_pointer, unsigned int access,
	void **pixels, int *pitch);
FBCALL int fb_Gfx3SurfaceMapRect(void *surface_pointer, int x, int y,
	int width, int height, unsigned int access, void **pixels, int *pitch);
FBCALL int fb_Gfx3SurfaceUnmap(void *surface_pointer);
FBCALL int fb_Gfx3SurfaceClear(void *surface_pointer,
	unsigned int clear_color);
FBCALL int fb_Gfx3SurfaceBlit(void *destination_pointer,
	void *source_pointer, int source_x, int source_y, int width, int height,
	int destination_x, int destination_y, int mode, unsigned int alpha);
FBCALL int fb_Gfx3SurfaceCopyFromScreen(void *destination_pointer,
	int source_x, int source_y, int width, int height,
	int destination_x, int destination_y);
FBCALL int fb_Gfx3SurfaceBlitScaled(void *destination_pointer,
	void *source_pointer, int source_x, int source_y, int source_width,
	int source_height, int destination_x, int destination_y,
	int destination_width, int destination_height, int mode,
	unsigned int alpha, int filter);
FBCALL int fb_Gfx3SurfaceBlitRotated(void *destination_pointer,
	void *source_pointer, int source_x, int source_y, int source_width,
	int source_height, float destination_x, float destination_y,
	float angle_degrees, float scale_x, float scale_y, float pivot_x,
	float pivot_y, int mode, unsigned int alpha, int filter);
FBCALL int fb_Gfx3SurfaceMode7(void *destination_pointer,
	void *source_pointer, int source_x, int source_y, int source_width,
	int source_height, int destination_x, int destination_y,
	int destination_width, int destination_height, float camera_x,
	float camera_y, float camera_height, float camera_angle_degrees,
	float horizon_y, float focal_length, int mode, unsigned int alpha,
	int filter);
FBCALL int fb_Gfx3SurfacePresent(void *surface_pointer, int wait);
FBCALL int fb_Gfx3DrawPoints(void *destination_pointer,
	const struct FB_GFX3_PUBLIC_POINT *public_points, int count);
FBCALL int fb_Gfx3DrawLines(const FB_GFX3_COMPAT_LINE *lines, int count);
FBCALL int fb_Gfx3DrawGlyphs(const FB_GFX3_GLYPH *glyphs, int count);
FBCALL int fb_Gfx3DrawRectangles(
	const FB_GFX3_COMPAT_RECTANGLE *rectangles, int count);

#ifdef __cplusplus
}
#endif

#endif

/* end of gfx3_exports.h */
