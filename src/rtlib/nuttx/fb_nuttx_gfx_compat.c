/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_gfx_compat.c

    Purpose:

        Provide the small hosted-runtime edge needed when the NuttX QEMU smoke
        harness links real gfxlib2 sources.

    Responsibilities:

        - provide the normal rtlib context expected by gfxlib2
        - provide single-threaded lock, mutex, TLS, and error helpers
        - expose bridge functions used by the temporary NuttX console shim
        - keep graphics PRINT routed through the normal gfxlib2 hook table
        - provide safe headless fallbacks for commands that need a real
          framebuffer or input device

    This file intentionally does NOT contain:

        - hardware drawing command implementations
        - platform framebuffer or HDMI scanout code
        - USB keyboard, mouse, or controller drivers
        - a replacement for the permanent NuttX rtlib port

    Maintenance note:

        This file is part of the transitional QEMU harness. The long-term goal
        is to make NuttX build the normal rtlib objects directly. Until then,
        keep this bridge narrow and prefer using existing rtlib/gfxlib2 code
        over copying command bodies here.
*/

#include "rtlib/fb.h"
#include "gfxlib2/fb_gfx.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
    NuttX flat builds link all enabled builtin apps into one executable.
    During bring-up, separate gfxlib and sfxlib smoke apps can each bring in a
    small runtime bridge, so these shared rtlib globals must coalesce at link
    time.  The permanent NuttX rtlib should eventually provide one strong
    definition instead.
*/
#define FB_NUTTX_WEAK __attribute__((weak))

FB_RTLIB_CTX __fb_ctx FB_NUTTX_WEAK;
char __fb_errmsg[FB_ERRMSG_SIZE] FB_NUTTX_WEAK;

#define FB_NUTTX_HEADLESS_IMAGE_MAGIC 0x4e584749u

typedef struct FB_NUTTX_HEADLESS_IMAGE {
    uint32_t magic;
    int width;
    int height;
    int bpp;
    int pitch;
    size_t size;
    unsigned char data[1];
} FB_NUTTX_HEADLESS_IMAGE;

static void *fb_nuttx_tls[FB_TLSKEYS];
static int fb_nuttx_view_toprow = -1;
static int fb_nuttx_view_botrow = -1;

int fb_GfxPageSet(int active, int visible);

int fb_GfxScreenList(int depth) FB_NUTTX_WEAK;
int fb_GfxScreenList(int depth)
{
    /*
        SCREENLIST enumerates modes reported by an initialized graphics
        backend.  The headless NuttX smoke target has no real mode database,
        so return zero to indicate an empty list.
    */
    (void)depth;

    return 0;
}

static void fb_nuttx_set_int(int *dst, int value)
{
    if (dst != NULL)
        *dst = value;
}

static void fb_nuttx_set_ssize(ssize_t *dst, ssize_t value)
{
    if (dst != NULL)
        *dst = value;
}

static void fb_nuttx_set_i64(long long *dst, long long value)
{
    if (dst != NULL)
        *dst = value;
}

static void fb_nuttx_set_float(float *dst, float value)
{
    if (dst != NULL)
        *dst = value;
}

#ifdef FB_NUTTX_QEMU_MOCK_DEVICES
static void fb_nuttx_qemu_trace_gfx_console(const void *buffer, size_t len,
    int mask)
{
    const unsigned char *bytes;
    char sample[49];
    size_t sample_len;
    size_t i;
    uint32_t checksum;

    if ((buffer == NULL) || (len == 0))
        return;

    bytes = (const unsigned char *)buffer;
    sample_len = len;

    if (sample_len >= sizeof(sample))
        sample_len = sizeof(sample) - 1;

    checksum = 2166136261u;

    for (i = 0; i < len; i++) {
        checksum ^= (uint32_t)bytes[i];
        checksum *= 16777619u;
    }

    for (i = 0; i < sample_len; i++) {
        if ((bytes[i] >= 32) && (bytes[i] <= 126) && (bytes[i] != '"'))
            sample[i] = (char)bytes[i];
        else
            sample[i] = '.';
    }

    sample[sample_len] = '\0';

    printf("FB_NUTTX_QEMU_HDMI_CONSOLE len=%lu mask=%d checksum=%08lx text=\"%s\"\n",
        (unsigned long)len, mask, (unsigned long)checksum, sample);
    fflush(stdout);
}
#endif

static FB_NUTTX_HEADLESS_IMAGE *fb_nuttx_headless_image(void *image)
{
    FB_NUTTX_HEADLESS_IMAGE *headless_image;

    if (image == NULL)
        return NULL;

    headless_image = (FB_NUTTX_HEADLESS_IMAGE *)image;

    if (headless_image->magic != FB_NUTTX_HEADLESS_IMAGE_MAGIC)
        return NULL;

    return headless_image;
}

void fb_nuttx_gfx_compat_init(int argc, char *argv[])
{
    memset(&__fb_ctx, 0, sizeof(__fb_ctx));

    __fb_ctx.argc = argc;
    __fb_ctx.argv = argv;
    __fb_ctx.null_desc.data = NULL;
    __fb_ctx.null_desc.len = 0;
    __fb_ctx.null_desc.size = 0;

    fb_nuttx_view_toprow = -1;
    fb_nuttx_view_botrow = -1;
}

void fb_nuttx_gfx_compat_exit(void)
{
    if (__fb_ctx.exit_gfxlib2 != NULL) {
        void (*exit_gfxlib2)(void);

        exit_gfxlib2 = __fb_ctx.exit_gfxlib2;
        __fb_ctx.exit_gfxlib2 = NULL;
        exit_gfxlib2();
    }
}

int fb_nuttx_gfx_active(void)
{
    return __fb_ctx.hooks.printbuffproc != NULL;
}

int fb_nuttx_gfx_print_buffer(const void *buffer, size_t len, int mask)
{
    if (__fb_ctx.hooks.printbuffproc == NULL)
        return 0;

#ifdef FB_NUTTX_QEMU_MOCK_DEVICES
    fb_nuttx_qemu_trace_gfx_console(buffer, len, mask);
#endif

    __fb_ctx.hooks.printbuffproc(buffer, len, mask);

    return 1;
}

FBSTRING *fb_nuttx_gfx_inkey(void)
{
    if (__fb_ctx.hooks.inkeyproc == NULL)
        return &__fb_ctx.null_desc;

    return __fb_ctx.hooks.inkeyproc();
}

int fb_nuttx_gfx_cls(int mode)
{
    if (__fb_ctx.hooks.clsproc == NULL)
        return 0;

    __fb_ctx.hooks.clsproc(mode);

    return 1;
}

int fb_nuttx_gfx_locate(int row, int col, int cursor)
{
    if (__fb_ctx.hooks.locateproc == NULL)
        return 0;

    return __fb_ctx.hooks.locateproc(row, col, cursor);
}

int fb_nuttx_gfx_get_x(void)
{
    if (__fb_ctx.hooks.getxproc == NULL)
        return 0;

    return __fb_ctx.hooks.getxproc();
}

int fb_nuttx_gfx_get_y(void)
{
    if (__fb_ctx.hooks.getyproc == NULL)
        return 0;

    return __fb_ctx.hooks.getyproc();
}

int fb_nuttx_gfx_get_size(int *cols, int *rows)
{
    if (__fb_ctx.hooks.getsizeproc == NULL)
        return 0;

    __fb_ctx.hooks.getsizeproc(cols, rows);

    return 1;
}

void fb_Lock(void)
{
}

void fb_Unlock(void)
{
}

void fb_StrLock(void)
{
}

void fb_StrUnlock(void)
{
}

void fb_GraphicsLock(void)
{
}

void fb_GraphicsUnlock(void)
{
}

void fb_MathLock(void)
{
}

void fb_MathUnlock(void)
{
}

void fb_ProfileLock(void)
{
}

void fb_ProfileUnlock(void)
{
}

FBMUTEX *fb_MutexCreate(void) __attribute__((weak));
FBMUTEX *fb_MutexCreate(void)
{
    return (FBMUTEX *)malloc(1);
}

void fb_MutexDestroy(FBMUTEX *mutex) __attribute__((weak));
void fb_MutexDestroy(FBMUTEX *mutex)
{
    free(mutex);
}

void fb_MutexLock(FBMUTEX *mutex) __attribute__((weak));
void fb_MutexLock(FBMUTEX *mutex)
{
    (void)mutex;
}

void fb_MutexUnlock(FBMUTEX *mutex) __attribute__((weak));
void fb_MutexUnlock(FBMUTEX *mutex)
{
    (void)mutex;
}

void *fb_TlsGetCtx(int index, size_t len, FB_TLS_DESTRUCTOR destructor)
{
    (void)destructor;

    if ((index < 0) || (index >= FB_TLSKEYS) || (len == 0))
        return NULL;

    if (fb_nuttx_tls[index] == NULL)
        fb_nuttx_tls[index] = calloc(1, len);

    return fb_nuttx_tls[index];
}

void fb_TlsDelCtx(int index)
{
    if ((index < 0) || (index >= FB_TLSKEYS))
        return;

    free(fb_nuttx_tls[index]);
    fb_nuttx_tls[index] = NULL;
}

void fb_TlsFreeCtxTb(void)
{
    int i;

    for (i = 0; i < FB_TLSKEYS; i++)
        fb_TlsDelCtx(i);
}

int __attribute__((weak)) fb_hStrDelTemp(FBSTRING *str)
{
    if (str == NULL)
        return FALSE;

    /*
        The NuttX mini-runtime uses size == 0 for borrowed temporary
        descriptors. Do not free those, because they often point at string
        literals emitted by the compiler.
    */
    if ((str->data != NULL) && (str->size > 0))
        free(str->data);

    str->data = NULL;
    str->len = 0;
    str->size = 0;

    return TRUE;
}

FBSTRING *fb_hStrRealloc(FBSTRING *str, ssize_t size, int preserve)
{
    char *old_data;
    char *new_data;
    size_t old_len;
    size_t new_size;

    if ((str == NULL) || (size < 0) || (size >= INT_MAX))
        return NULL;

    old_data = str->data;
    old_len = 0;

    if ((preserve != FALSE) && (old_data != NULL) && (str->len > 0))
        old_len = (size_t)str->len;

    new_size = (size_t)size + 1u;
    new_data = (char *)calloc(1u, new_size);

    if (new_data == NULL)
        return NULL;

    if ((old_len > 0) && (old_data != NULL)) {
        if (old_len > (size_t)size)
            old_len = (size_t)size;

        memcpy(new_data, old_data, old_len);
    }

    if ((old_data != NULL) && (str->size > 0))
        free(old_data);

    str->data = new_data;
    str->len = size;
    str->size = new_size;

    return str;
}

int fb_ErrorSetNum(int errnum)
{
    return errnum;
}

int fb_Getkey(void)
{
    unsigned char ch;
    ssize_t nread;

    if (__fb_ctx.hooks.getkeyproc != NULL)
        return __fb_ctx.hooks.getkeyproc();

    nread = read(STDIN_FILENO, &ch, 1);
    if (nread == 1)
        return (int)ch;

    return EOF;
}

int fb_KeyHit(void)
{
    if (__fb_ctx.hooks.keyhitproc != NULL)
        return __fb_ctx.hooks.keyhitproc();

    return 0;
}

int fb_GetMouse(int *x, int *y, int *z, int *buttons_, int *clip)
{
    if (__fb_ctx.hooks.getmouseproc != NULL)
        return __fb_ctx.hooks.getmouseproc(x, y, z, buttons_, clip);

    if (x != NULL)
        *x = -1;
    if (y != NULL)
        *y = -1;
    if (z != NULL)
        *z = -1;
    if (buttons_ != NULL)
        *buttons_ = -1;
    if (clip != NULL)
        *clip = -1;

    return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
}

int fb_SetMouse(int x, int y, int cursor, int clip)
{
    if (__fb_ctx.hooks.setmouseproc != NULL)
        return __fb_ctx.hooks.setmouseproc(x, y, cursor, clip);

    return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
}

int fb_GetMouse64(long long *x, long long *y, long long *z,
    long long *buttons_, long long *clip)
{
    int mx;
    int my;
    int mz;
    int mbuttons;
    int mclip;
    int result;

    mx = -1;
    my = -1;
    mz = -1;
    mbuttons = -1;
    mclip = -1;

    result = fb_GetMouse(&mx, &my, &mz, &mbuttons, &mclip);

    fb_nuttx_set_i64(x, (long long)mx);
    fb_nuttx_set_i64(y, (long long)my);
    fb_nuttx_set_i64(z, (long long)mz);
    fb_nuttx_set_i64(buttons_, (long long)mbuttons);
    fb_nuttx_set_i64(clip, (long long)mclip);

    return result;
}

int fb_Multikey(int scancode)
{
    if (__fb_ctx.hooks.multikeyproc != NULL)
        return __fb_ctx.hooks.multikeyproc(scancode);

    return FB_FALSE;
}

FBSTRING *fb_hMakeInkeyStr(int ch)
{
    static char text[2];
    static FBSTRING result;

    if ((ch <= 0) || (ch > 255))
        return &__fb_ctx.null_desc;

    text[0] = (char)ch;
    text[1] = '\0';

    result.data = text;
    result.len = 1;
    result.size = 0;

    return &result;
}

unsigned int fb_CpuDetect(void)
{
    return 0;
}

void fb_Beep(void) __attribute__((weak));
void fb_Beep(void)
{
}

void fb_Delay(int msecs)
{
    if (msecs <= 0)
        return;

    usleep((useconds_t)msecs * 1000u);
}

char *fb_hGetExeName(char *dst, ssize_t maxlen)
{
    static const char name[] = "fb-nuttx";
    size_t len;

    if ((dst == NULL) || (maxlen <= 0))
        return NULL;

    len = strlen(name);

    if (len > (size_t)maxlen)
        len = (size_t)maxlen;

    memcpy(dst, name, len);
    dst[len] = '\0';

    return dst;
}

void fb_DevScrnMaybeUpdateWidth(void)
{
    FB_HANDLE_SCREEN->line_length = 0;
}

int fb_SetPos(FB_FILE *handle, int line_length)
{
    if (handle != NULL)
        handle->line_length = line_length;

    return fb_ErrorSetNum(FB_RTERROR_OK);
}

int fb_ConsoleGetTopRow(void)
{
    if (fb_nuttx_view_toprow < 0)
        fb_nuttx_view_toprow = 0;

    return fb_nuttx_view_toprow;
}

int fb_ConsoleGetBotRow(void)
{
    if (fb_nuttx_view_botrow < 0) {
        int rows;

        rows = 25;

        if (__fb_ctx.hooks.getsizeproc != NULL)
            __fb_ctx.hooks.getsizeproc(NULL, &rows);

        if (rows <= 0)
            rows = 25;

        fb_nuttx_view_botrow = rows - 1;
    }

    return fb_nuttx_view_botrow;
}

void fb_ConsoleSetTopBotRows(int top, int bot)
{
    fb_nuttx_view_toprow = top;
    fb_nuttx_view_botrow = bot;
}

void fb_ConsoleGetView(int *toprow, int *botrow)
{
    if (toprow != NULL)
        *toprow = fb_ConsoleGetTopRow() + 1;

    if (botrow != NULL)
        *botrow = fb_ConsoleGetBotRow() + 1;
}

int fb_ConsoleViewEx(int toprow, int botrow, int set_cursor)
{
    int rows;

    (void)set_cursor;

    if ((toprow <= 0) && (botrow <= 0)) {
        fb_ConsoleSetTopBotRows(-1, -1);
        return fb_ErrorSetNum(FB_RTERROR_OK);
    }

    rows = 25;

    if (__fb_ctx.hooks.getsizeproc != NULL)
        __fb_ctx.hooks.getsizeproc(NULL, &rows);

    if (toprow <= 0)
        toprow = 1;

    if (botrow <= 0)
        botrow = rows;

    if ((toprow > botrow) || (toprow > rows))
        return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);

    if (botrow > rows)
        botrow = rows;

    fb_ConsoleSetTopBotRows(toprow - 1, botrow - 1);

    return fb_ErrorSetNum(FB_RTERROR_OK);
}

int fb_ConsoleView(int toprow, int botrow)
{
    return fb_ConsoleViewEx(toprow, botrow, TRUE);
}

/* ------------------------------------------------------------------------- */
/* Headless graphics command fallbacks                                       */
/* ------------------------------------------------------------------------- */

void fb_GfxLine(void *target, float x1, float y1, float x2, float y2,
    unsigned int color, int type, unsigned int style, int coord_type)
{
    (void)target;
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
    (void)color;
    (void)type;
    (void)style;
    (void)coord_type;
}

void fb_GfxEllipse(void *target, float x, float y, float radius,
    unsigned int color, float aspect, float start, float end, int fill,
    int coord_type)
{
    (void)target;
    (void)x;
    (void)y;
    (void)radius;
    (void)color;
    (void)aspect;
    (void)start;
    (void)end;
    (void)fill;
    (void)coord_type;
}

void fb_GfxPaint(void *target, float fx, float fy, unsigned int color,
    unsigned int border_color, FBSTRING *pattern, int mode, int coord_type)
{
    (void)target;
    (void)fx;
    (void)fy;
    (void)color;
    (void)border_color;
    (void)pattern;
    (void)mode;
    (void)coord_type;
}

void fb_GfxDraw(void *target, FBSTRING *command)
{
    (void)target;
    (void)command;
}

int fb_GfxDrawString(void *target, float fx, float fy, int coord_type,
    FBSTRING *string, unsigned int color, void *font, int mode,
    PUTTER *putter, BLENDER *blender, void *param)
{
    (void)target;
    (void)fx;
    (void)fy;
    (void)coord_type;
    (void)string;
    (void)color;
    (void)font;
    (void)mode;
    (void)putter;
    (void)blender;
    (void)param;

    return fb_ErrorSetNum(FB_RTERROR_OK);
}

void fb_GfxWindow(float x1, float y1, float x2, float y2, int screen)
{
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
    (void)screen;
}

void *fb_GfxImageCreate(int width, int height, unsigned int color, int depth,
    int flags)
{
    FB_NUTTX_HEADLESS_IMAGE *image;
    size_t pixels;
    size_t bytes_per_pixel;
    size_t data_size;
    size_t total_size;

    (void)color;
    (void)flags;

    if ((width <= 0) || (height <= 0))
        return NULL;

    if (depth <= 0)
        depth = 32;

    bytes_per_pixel = (size_t)((depth + 7) / 8);

    if (bytes_per_pixel == 0)
        bytes_per_pixel = 1;

    pixels = (size_t)width * (size_t)height;

    if ((width != 0) && (pixels / (size_t)width != (size_t)height))
        return NULL;

    data_size = pixels * bytes_per_pixel;

    if ((pixels != 0) && (data_size / pixels != bytes_per_pixel))
        return NULL;

    if (data_size > (SIZE_MAX - sizeof(*image)))
        return NULL;

    total_size = sizeof(*image) + data_size;
    image = (FB_NUTTX_HEADLESS_IMAGE *)calloc(1u, total_size);

    if (image == NULL)
        return NULL;

    image->magic = FB_NUTTX_HEADLESS_IMAGE_MAGIC;
    image->width = width;
    image->height = height;
    image->bpp = (int)bytes_per_pixel;
    image->pitch = width * (int)bytes_per_pixel;
    image->size = data_size;

    return image;
}

void *fb_GfxImageCreateQB(int width, int height, unsigned int color, int depth,
    int flags)
{
    return fb_GfxImageCreate(width, height, color, depth, flags);
}

void fb_GfxImageDestroy(void *image)
{
    FB_NUTTX_HEADLESS_IMAGE *headless_image;

    headless_image = fb_nuttx_headless_image(image);

    if (headless_image != NULL) {
        headless_image->magic = 0;
        free(headless_image);
    }
}

int fb_GfxImageInfo(void *img, ssize_t *width, ssize_t *height, ssize_t *bpp,
    ssize_t *pitch, void **imgdata, ssize_t *size)
{
    FB_NUTTX_HEADLESS_IMAGE *image;

    image = fb_nuttx_headless_image(img);

    if (image == NULL)
        return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);

    fb_nuttx_set_ssize(width, (ssize_t)image->width);
    fb_nuttx_set_ssize(height, (ssize_t)image->height);
    fb_nuttx_set_ssize(bpp, (ssize_t)image->bpp);
    fb_nuttx_set_ssize(pitch, (ssize_t)image->pitch);

    if (imgdata != NULL)
        *imgdata = image->data;

    fb_nuttx_set_ssize(size, (ssize_t)image->size);

    return fb_ErrorSetNum(FB_RTERROR_OK);
}

int fb_GfxImageInfo32(void *img, int *width, int *height, int *bpp,
    int *pitch, void **imgdata, int *size)
{
    FB_NUTTX_HEADLESS_IMAGE *image;

    image = fb_nuttx_headless_image(img);

    if (image == NULL)
        return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);

    fb_nuttx_set_int(width, image->width);
    fb_nuttx_set_int(height, image->height);
    fb_nuttx_set_int(bpp, image->bpp);
    fb_nuttx_set_int(pitch, image->pitch);

    if (imgdata != NULL)
        *imgdata = image->data;

    fb_nuttx_set_int(size, (int)image->size);

    return fb_ErrorSetNum(FB_RTERROR_OK);
}

int fb_GfxImageInfo64(void *img, long long *width, long long *height,
    long long *bpp, long long *pitch, void **imgdata, long long *size)
{
    FB_NUTTX_HEADLESS_IMAGE *image;

    image = fb_nuttx_headless_image(img);

    if (image == NULL)
        return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);

    fb_nuttx_set_i64(width, (long long)image->width);
    fb_nuttx_set_i64(height, (long long)image->height);
    fb_nuttx_set_i64(bpp, (long long)image->bpp);
    fb_nuttx_set_i64(pitch, (long long)image->pitch);

    if (imgdata != NULL)
        *imgdata = image->data;

    fb_nuttx_set_i64(size, (long long)image->size);

    return fb_ErrorSetNum(FB_RTERROR_OK);
}

int fb_GfxGet(void *target, float x1, float y1, float x2, float y2,
    unsigned char *dest, int coord_type, FBARRAY *array)
{
    (void)target;
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
    (void)dest;
    (void)coord_type;
    (void)array;

    return fb_ErrorSetNum(FB_RTERROR_OK);
}

int fb_GfxGetQB(void *target, float x1, float y1, float x2, float y2,
    unsigned char *dest, int coord_type, FBARRAY *array)
{
    return fb_GfxGet(target, x1, y1, x2, y2, dest, coord_type, array);
}

int fb_GfxPut(void *target, float x, float y, unsigned char *src, int x1,
    int y1, int x2, int y2, int coord_type, int mode, PUTTER *putter,
    int alpha, BLENDER *blender, void *param)
{
    (void)target;
    (void)x;
    (void)y;
    (void)src;
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
    (void)coord_type;
    (void)mode;
    (void)putter;
    (void)alpha;
    (void)blender;
    (void)param;

    return fb_ErrorSetNum(FB_RTERROR_OK);
}

int fb_GfxBload(FBSTRING *filename, void *dest, void *pal)
{
    (void)filename;
    (void)dest;
    (void)pal;

    return fb_ErrorSetNum(FB_RTERROR_OK);
}

int fb_GfxBloadQB(FBSTRING *filename, void *dest, void *pal)
{
    return fb_GfxBload(filename, dest, pal);
}

int fb_GfxBsaveEx(FBSTRING *filename, void *src, unsigned int size, void *pal,
    int bitsperpixel)
{
    (void)filename;
    (void)src;
    (void)size;
    (void)pal;
    (void)bitsperpixel;

    return fb_ErrorSetNum(FB_RTERROR_OK);
}

int fb_GfxBsave(FBSTRING *filename, void *src, unsigned int size, void *pal)
{
    return fb_GfxBsaveEx(filename, src, size, pal, 0);
}

void fb_GfxControl_s(int what, FBSTRING *param)
{
    (void)what;
    (void)param;
}

void fb_GfxControl_i(int what, ssize_t *param1, ssize_t *param2,
    ssize_t *param3, ssize_t *param4)
{
    (void)what;
    fb_nuttx_set_ssize(param1, 0);
    fb_nuttx_set_ssize(param2, 0);
    fb_nuttx_set_ssize(param3, 0);
    fb_nuttx_set_ssize(param4, 0);
}

void fb_GfxControl_i32(int what, int *param1, int *param2, int *param3,
    int *param4)
{
    (void)what;
    fb_nuttx_set_int(param1, 0);
    fb_nuttx_set_int(param2, 0);
    fb_nuttx_set_int(param3, 0);
    fb_nuttx_set_int(param4, 0);
}

void fb_GfxControl_i64(int what, long long *param1, long long *param2,
    long long *param3, long long *param4)
{
    (void)what;
    fb_nuttx_set_i64(param1, 0);
    fb_nuttx_set_i64(param2, 0);
    fb_nuttx_set_i64(param3, 0);
    fb_nuttx_set_i64(param4, 0);
}

int fb_GfxEvent(EVENT *event)
{
    (void)event;

    return FB_FALSE;
}

int fb_GfxGetJoystick(int id, ssize_t *buttons, float *a1, float *a2,
    float *a3, float *a4, float *a5, float *a6, float *a7, float *a8)
{
    (void)id;
    fb_nuttx_set_ssize(buttons, 0);
    fb_nuttx_set_float(a1, 0.0f);
    fb_nuttx_set_float(a2, 0.0f);
    fb_nuttx_set_float(a3, 0.0f);
    fb_nuttx_set_float(a4, 0.0f);
    fb_nuttx_set_float(a5, 0.0f);
    fb_nuttx_set_float(a6, 0.0f);
    fb_nuttx_set_float(a7, 0.0f);
    fb_nuttx_set_float(a8, 0.0f);

    return FB_FALSE;
}

int fb_GfxGetXPad(int id, ssize_t *buttons, float *lstick_x,
    float *lstick_y, float *rstick_x, float *rstick_y, float *ltrigger,
    float *rtrigger, ssize_t *dpad)
{
    (void)id;
    fb_nuttx_set_ssize(buttons, 0);
    fb_nuttx_set_float(lstick_x, 0.0f);
    fb_nuttx_set_float(lstick_y, 0.0f);
    fb_nuttx_set_float(rstick_x, 0.0f);
    fb_nuttx_set_float(rstick_y, 0.0f);
    fb_nuttx_set_float(ltrigger, 0.0f);
    fb_nuttx_set_float(rtrigger, 0.0f);
    fb_nuttx_set_ssize(dpad, 0);

    return FB_FALSE;
}

ssize_t fb_GfxGetTouchCount(void)
{
    return 0;
}

ssize_t fb_GfxGetTouch(ssize_t index, ssize_t *x, ssize_t *y, ssize_t *id)
{
    (void)index;
    fb_nuttx_set_ssize(x, -1);
    fb_nuttx_set_ssize(y, -1);
    fb_nuttx_set_ssize(id, -1);

    return -1;
}

ssize_t fb_GfxGetTouchHit(ssize_t x1, ssize_t y1, ssize_t x2, ssize_t y2)
{
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;

    return FB_FALSE;
}

ssize_t fb_GfxGetTouchHitCircle(ssize_t x, ssize_t y, ssize_t radius)
{
    (void)x;
    (void)y;
    (void)radius;

    return FB_FALSE;
}

void *fb_GfxGetGLProcAddress(const char *proc)
{
    (void)proc;

    return NULL;
}

void fb_GfxPaletteUsing(int *data)
{
    (void)data;
}

void fb_GfxPaletteUsing64(long long *data)
{
    (void)data;
}

void fb_GfxPaletteGet(int index, int *r, int *g, int *b)
{
    (void)index;
    fb_nuttx_set_int(r, 0);
    fb_nuttx_set_int(g, 0);
    fb_nuttx_set_int(b, 0);
}

void fb_GfxPaletteGet64(int index, long long *r, long long *g, long long *b)
{
    (void)index;
    fb_nuttx_set_i64(r, 0);
    fb_nuttx_set_i64(g, 0);
    fb_nuttx_set_i64(b, 0);
}

void fb_GfxPaletteGetUsing(int *data)
{
    (void)data;
}

void fb_GfxPaletteGetUsing64(long long *data)
{
    (void)data;
}

void fb_GfxImageConvertRow(const unsigned char *src, int src_bpp,
    unsigned char *dest, int dst_bpp, int width, int isrgb)
{
    size_t src_bytes;
    size_t dst_bytes;

    (void)isrgb;

    if ((src == NULL) || (dest == NULL) || (width <= 0))
        return;

    src_bytes = (size_t)((src_bpp + 7) / 8);
    dst_bytes = (size_t)((dst_bpp + 7) / 8);

    if ((src_bytes == 0) || (dst_bytes == 0))
        return;

    if (src_bytes == dst_bytes) {
        memcpy(dest, src, (size_t)width * src_bytes);
    } else {
        memset(dest, 0, (size_t)width * dst_bytes);
    }
}

int fb_GfxStickQB(int n)
{
    (void)n;

    return 0;
}

int fb_GfxStrigQB(int n)
{
    (void)n;

    return 0;
}

void fb_hPutTrans(unsigned char *src, unsigned char *dest, int w, int h,
    int src_pitch, int dest_pitch, int alpha, BLENDER *blender, void *param)
{
    (void)src;
    (void)dest;
    (void)w;
    (void)h;
    (void)src_pitch;
    (void)dest_pitch;
    (void)alpha;
    (void)blender;
    (void)param;
}

void fb_hPutPSet(unsigned char *src, unsigned char *dest, int w, int h,
    int src_pitch, int dest_pitch, int alpha, BLENDER *blender, void *param)
{
    fb_hPutTrans(src, dest, w, h, src_pitch, dest_pitch, alpha, blender,
        param);
}

void fb_hPutPReset(unsigned char *src, unsigned char *dest, int w, int h,
    int src_pitch, int dest_pitch, int alpha, BLENDER *blender, void *param)
{
    fb_hPutTrans(src, dest, w, h, src_pitch, dest_pitch, alpha, blender,
        param);
}

void fb_hPutAnd(unsigned char *src, unsigned char *dest, int w, int h,
    int src_pitch, int dest_pitch, int alpha, BLENDER *blender, void *param)
{
    fb_hPutTrans(src, dest, w, h, src_pitch, dest_pitch, alpha, blender,
        param);
}

void fb_hPutOr(unsigned char *src, unsigned char *dest, int w, int h,
    int src_pitch, int dest_pitch, int alpha, BLENDER *blender, void *param)
{
    fb_hPutTrans(src, dest, w, h, src_pitch, dest_pitch, alpha, blender,
        param);
}

void fb_hPutXor(unsigned char *src, unsigned char *dest, int w, int h,
    int src_pitch, int dest_pitch, int alpha, BLENDER *blender, void *param)
{
    fb_hPutTrans(src, dest, w, h, src_pitch, dest_pitch, alpha, blender,
        param);
}

void fb_hPutAlpha(unsigned char *src, unsigned char *dest, int w, int h,
    int src_pitch, int dest_pitch, int alpha, BLENDER *blender, void *param)
{
    fb_hPutTrans(src, dest, w, h, src_pitch, dest_pitch, alpha, blender,
        param);
}

void fb_hPutBlend(unsigned char *src, unsigned char *dest, int w, int h,
    int src_pitch, int dest_pitch, int alpha, BLENDER *blender, void *param)
{
    fb_hPutTrans(src, dest, w, h, src_pitch, dest_pitch, alpha, blender,
        param);
}

void fb_hPutAdd(unsigned char *src, unsigned char *dest, int w, int h,
    int src_pitch, int dest_pitch, int alpha, BLENDER *blender, void *param)
{
    fb_hPutTrans(src, dest, w, h, src_pitch, dest_pitch, alpha, blender,
        param);
}

void fb_hPutCustom(unsigned char *src, unsigned char *dest, int w, int h,
    int src_pitch, int dest_pitch, int alpha, BLENDER *blender, void *param)
{
    fb_hPutTrans(src, dest, w, h, src_pitch, dest_pitch, alpha, blender,
        param);
}

int fb_PageSet(int active, int visible)
{
    return fb_GfxPageSet(active, visible);
}

int fb_GfxLineInput(FBSTRING *text, void *dst, ssize_t dst_len, int fillrem,
    int addquestion, int addnewline)
{
    (void)text;
    (void)dst;
    (void)dst_len;
    (void)fillrem;
    (void)addquestion;
    (void)addnewline;

    return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
}

int fb_GfxLineInputWstr(const FB_WCHAR *text, FB_WCHAR *dst,
    ssize_t max_chars, int addquestion, int addnewline)
{
    (void)text;
    (void)dst;
    (void)max_chars;
    (void)addquestion;
    (void)addnewline;

    return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
}

/* end of fb_nuttx_gfx_compat.c */
