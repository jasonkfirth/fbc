/*
    FreeBASIC gfxlib2 NuttX backend
    --------------------------------

    File: gfx_driver.c

    Purpose:

        Provide the first NuttX graphics driver registration point for
        gfxlib2.

    Responsibilities:

        - register a NuttX driver with the normal gfxlib2 driver list
        - accept low-memory paletted framebuffer modes
        - keep framebuffer ownership and drawing commands in generic gfxlib2
        - connect the generic framebuffer to the RP2350 DVI scanout backend
        - feed gfxlib2 input from NuttX USB HID devices when available

    This file intentionally does NOT contain:

        - implementations of PSET, LINE, POINT, CIRCLE, PAINT, or GET/PUT
        - LCD or fbdev scanout code
        - touch or controller input
        - sound support

    Platform notes:

        NuttX covers very small systems.  The first useful target for this
        backend is the set of low-memory paletted modes whose current gfxlib2
        framebuffer storage fits inside the SCREEN 13 budget: 64,000 bytes per
        page.  The generic gfxlib2 core already owns the framebuffer pages,
        dirty lines, palette arrays, and drawing commands. This driver is the
        platform edge that forwards presentation and palette changes to the
        board-specific RP2350 DVI scanout implementation.
*/

#include "../fb_gfx.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define SCREENLIST(w, h) ((h) | ((w) << 16))

#define NUTTX_SCREEN_WIDTH 320
#define NUTTX_SCREEN_HEIGHT 200
#define NUTTX_MAX_FRAMEBUFFER_BYTES \
    ((size_t)NUTTX_SCREEN_WIDTH * (size_t)NUTTX_SCREEN_HEIGHT)
#define NUTTX_HID_KEYBOARD_PATH "/dev/kbda"
#define NUTTX_HID_MOUSE_PATH "/dev/mouse0"
#define NUTTX_INPUT_READ_LIMIT 16

typedef struct NUTTX_MOUSE_REPORT
{
    uint8_t buttons;
    int16_t x;
    int16_t y;
    int16_t wheel;
} NUTTX_MOUSE_REPORT;

typedef struct NUTTX_MODE
{
    int width;
    int height;
    int depth;
} NUTTX_MODE;

static const NUTTX_MODE nuttx_modes[] =
{
    { 320, 200, 2 },
    { 320, 200, 4 },
    { 320, 200, 8 }
};

static int nuttx_active;
static unsigned int nuttx_palette[256];
static int nuttx_stdin_flags_valid;
static int nuttx_stdin_flags;
static int nuttx_termios_valid;
static struct termios nuttx_termios;
static int nuttx_mouse_x;
static int nuttx_mouse_y;
static int nuttx_mouse_z;
static int nuttx_mouse_buttons;
static int nuttx_mouse_clip;
static int nuttx_dvi_ready;
static int nuttx_keyboard_fd = -1;
static int nuttx_mouse_fd = -1;
#ifdef FB_NUTTX_QEMU_MOCK_DEVICES
static unsigned int nuttx_qemu_present_count;
static uint32_t nuttx_qemu_last_checksum;
#endif

extern int fb_nuttx_dvi_start(void);
extern void fb_nuttx_dvi_set_palette(int index, unsigned int rgb);
extern void fb_nuttx_dvi_present(void);
extern void fb_nuttx_dvi_blank(void);
extern void fb_nuttx_dvi_framebuffer_lock(void);
extern void fb_nuttx_dvi_framebuffer_unlock(void);

static int clamp_int(int value, int low, int high)
{
    if (value < low)
        return low;

    if (value > high)
        return high;

    return value;
}

static int nuttx_is_paletted_depth(int depth)
{
    return (depth == 1) || (depth == 2) || (depth == 4) || (depth == 8);
}

static int nuttx_framebuffer_fits(int w, int h, int depth)
{
    size_t bytes_per_pixel;
    size_t bytes_per_line;

    if ((w <= 0) || (h <= 0))
        return FALSE;

    if (!nuttx_is_paletted_depth(depth))
        return FALSE;

    /*
        gfxlib2 currently stores 1, 2, 4, and 8 bit paletted modes as one byte
        per pixel internally.  The DVI scanout layer can pack or expand later,
        but this guard has to match the actual allocation made by gfx_screen.c.
    */
    bytes_per_pixel = (size_t)BYTES_PER_PIXEL(depth);

    if ((size_t)w > (NUTTX_MAX_FRAMEBUFFER_BYTES / bytes_per_pixel))
        return FALSE;

    bytes_per_line = (size_t)w * bytes_per_pixel;

    if ((size_t)h > (NUTTX_MAX_FRAMEBUFFER_BYTES / bytes_per_line))
        return FALSE;

    return TRUE;
}

static int driver_open_nonblocking(const char *path)
{
    int fd;
    int flags;

    fd = open(path, O_RDONLY | O_NONBLOCK);

    if (fd < 0)
        return -1;

    flags = fcntl(fd, F_GETFL, 0);

    if (flags >= 0)
        (void)fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    return fd;
}

static void driver_open_hid_devices(void)
{
    if (nuttx_keyboard_fd < 0)
        nuttx_keyboard_fd = driver_open_nonblocking(NUTTX_HID_KEYBOARD_PATH);

    if (nuttx_mouse_fd < 0)
        nuttx_mouse_fd = driver_open_nonblocking(NUTTX_HID_MOUSE_PATH);

#ifdef FB_NUTTX_QEMU_MOCK_DEVICES
    if (nuttx_keyboard_fd >= 0) {
        printf("FB_NUTTX_QEMU_HID_KEYBOARD_OPEN %s\n", NUTTX_HID_KEYBOARD_PATH);
        fflush(stdout);
    }

    if (nuttx_mouse_fd >= 0) {
        printf("FB_NUTTX_QEMU_HID_MOUSE_OPEN %s\n", NUTTX_HID_MOUSE_PATH);
        fflush(stdout);
    }
#endif
}

static void driver_close_hid_devices(void)
{
    if (nuttx_keyboard_fd >= 0) {
        close(nuttx_keyboard_fd);
        nuttx_keyboard_fd = -1;
    }

    if (nuttx_mouse_fd >= 0) {
        close(nuttx_mouse_fd);
        nuttx_mouse_fd = -1;
    }
}

#ifdef FB_NUTTX_QEMU_MOCK_DEVICES
static uint32_t nuttx_qemu_checksum_framebuffer(const unsigned char *data,
    size_t bytes)
{
    uint32_t hash;
    size_t i;

    if ((data == NULL) || (bytes == 0))
        return 0;

    /*
        FNV-1a is small, deterministic, and good enough for a smoke harness.
        The goal is not cryptographic certainty; it is to prove that QEMU ran
        the same framebuffer path that the RP2350 DVI driver will consume.
    */
    hash = 2166136261u;

    for (i = 0; i < bytes; i++) {
        hash ^= (uint32_t)data[i];
        hash *= 16777619u;
    }

    return hash;
}

static unsigned int nuttx_qemu_pixel_at(const unsigned char *data, int x, int y)
{
    const unsigned char *row;

    if ((x < 0) || (y < 0) || (x >= __fb_gfx->w) || (y >= __fb_gfx->h))
        return 0;

    row = data + ((size_t)y * (size_t)__fb_gfx->pitch);
    return (unsigned int)row[x];
}

static void nuttx_qemu_trace_present(void)
{
    size_t bytes;
    int x;
    int y;
    int min_x;
    int min_y;
    int max_x;
    int max_y;
    unsigned int nonzero;
    uint32_t checksum;
    const unsigned char *framebuffer;

    if ((__fb_gfx == NULL) || (__fb_gfx->framebuffer == NULL))
        return;

    if ((__fb_gfx->w <= 0) || (__fb_gfx->h <= 0) || (__fb_gfx->pitch <= 0))
        return;

    framebuffer = __fb_gfx->framebuffer;
    bytes = (size_t)__fb_gfx->pitch * (size_t)__fb_gfx->h;
    checksum = nuttx_qemu_checksum_framebuffer(framebuffer, bytes);

    if ((nuttx_qemu_present_count > 0) &&
        (checksum == nuttx_qemu_last_checksum))
        return;

    nuttx_qemu_present_count++;
    nuttx_qemu_last_checksum = checksum;

    printf("FB_NUTTX_QEMU_GFX_PRESENT frame=%u w=%d h=%d depth=%d pitch=%d bytes=%lu checksum=%08lx\n",
        nuttx_qemu_present_count,
        __fb_gfx->w,
        __fb_gfx->h,
        __fb_gfx->depth,
        __fb_gfx->pitch,
        (unsigned long)bytes,
        (unsigned long)checksum);

    min_x = __fb_gfx->w;
    min_y = __fb_gfx->h;
    max_x = -1;
    max_y = -1;
    nonzero = 0;

    /*
        The visual signature gives the QEMU lab a stronger graphics contract
        without dumping an entire 64 KB framebuffer into the serial log.
    */
    for (y = 0; y < __fb_gfx->h; y++) {
        const unsigned char *row;

        row = framebuffer + ((size_t)y * (size_t)__fb_gfx->pitch);

        for (x = 0; x < __fb_gfx->w; x++) {
            if (row[x] == 0)
                continue;

            nonzero++;

            if (x < min_x)
                min_x = x;

            if (y < min_y)
                min_y = y;

            if (x > max_x)
                max_x = x;

            if (y > max_y)
                max_y = y;
        }
    }

    if (nonzero == 0) {
        min_x = 0;
        min_y = 0;
        max_x = 0;
        max_y = 0;
    }

    printf("FB_NUTTX_QEMU_GFX_VISUAL frame=%u nonzero=%u bounds=%d,%d-%d,%d p2_2=%u p3_3=%u p10_10=%u center=%u\n",
        nuttx_qemu_present_count,
        nonzero,
        min_x,
        min_y,
        max_x,
        max_y,
        nuttx_qemu_pixel_at(framebuffer, 2, 2),
        nuttx_qemu_pixel_at(framebuffer, 3, 3),
        nuttx_qemu_pixel_at(framebuffer, 10, 10),
        nuttx_qemu_pixel_at(framebuffer, __fb_gfx->w / 2, __fb_gfx->h / 2));
    fflush(stdout);
}
#endif

static void driver_make_stdin_nonblocking(void)
{
    int flags;
    struct termios raw_termios;

    flags = fcntl(STDIN_FILENO, F_GETFL, 0);

    if (flags >= 0) {
        nuttx_stdin_flags = flags;
        nuttx_stdin_flags_valid = TRUE;

        (void)fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    }

    if (tcgetattr(STDIN_FILENO, &nuttx_termios) != 0)
        return;

    raw_termios = nuttx_termios;
    raw_termios.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    raw_termios.c_iflag &= (tcflag_t)~(IXON | ICRNL);
    raw_termios.c_cc[VMIN] = 0;
    raw_termios.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw_termios) == 0)
        nuttx_termios_valid = TRUE;
}

static void driver_post_key(unsigned char ch)
{
    int key;

    key = (int)ch;

    if (key == '\n')
        key = 13;

    if ((key <= 0) || (key >= 128))
        return;

    if (__fb_gfx->key != NULL)
        __fb_gfx->key[key] = TRUE;

    fb_hPostKey(key);
}

static void driver_poll_keyboard_fd(int fd)
{
    struct pollfd pfd;
    unsigned char ch;
    ssize_t got;
    int reads_left;

    if (fd < 0)
        return;

    pfd.fd = fd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    reads_left = NUTTX_INPUT_READ_LIMIT;

    while (reads_left > 0) {
        if (poll(&pfd, 1, 0) <= 0)
            break;

        if ((pfd.revents & POLLIN) == 0)
            break;

        got = read(fd, &ch, 1);

        if (got == 1) {
            reads_left--;
            driver_post_key(ch);
            continue;
        }

        if ((got < 0) && ((errno == EAGAIN) || (errno == EWOULDBLOCK)))
            break;

        break;
    }
}

static void driver_poll_mouse_fd(void)
{
    struct pollfd pfd;
    NUTTX_MOUSE_REPORT report;
    ssize_t got;
    int reads_left;

    if ((nuttx_mouse_fd < 0) || (__fb_gfx == NULL))
        return;

    pfd.fd = nuttx_mouse_fd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    reads_left = NUTTX_INPUT_READ_LIMIT;

    while (reads_left > 0) {
        if (poll(&pfd, 1, 0) <= 0)
            break;

        if ((pfd.revents & POLLIN) == 0)
            break;

        got = read(nuttx_mouse_fd, &report, sizeof(report));

        if (got == (ssize_t)sizeof(report)) {
            reads_left--;
            nuttx_mouse_x = clamp_int((int)report.x, 0, __fb_gfx->w - 1);
            nuttx_mouse_y = clamp_int((int)report.y, 0, __fb_gfx->h - 1);
            nuttx_mouse_z = (int)report.wheel;
            nuttx_mouse_buttons = (int)report.buttons;
            continue;
        }

        if ((got < 0) && ((errno == EAGAIN) || (errno == EWOULDBLOCK)))
            break;

        break;
    }
}

static void driver_restore_stdin(void)
{
    if (nuttx_termios_valid) {
        (void)tcsetattr(STDIN_FILENO, TCSANOW, &nuttx_termios);
        nuttx_termios_valid = FALSE;
    }

    if (!nuttx_stdin_flags_valid)
        return;

    (void)fcntl(STDIN_FILENO, F_SETFL, nuttx_stdin_flags);
    nuttx_stdin_flags_valid = FALSE;
}

static int driver_init(char *title, int w, int h, int depth,
    int refresh_rate, int flags)
{
    (void)title;
    (void)refresh_rate;

    if (flags & DRIVER_OPENGL)
        return -1;

    if ((w <= 0) || (h <= 0))
        return -1;

    /*
        The RP2350-PiZero target has to leave most of its 520 KB SRAM for
        NuttX, USB, SD, sound buffers, DVI scanout, and the BASIC program.
        Keep each gfxlib page capped at the classic SCREEN 13 budget:
        320 * 200 bytes.
    */
    if (!nuttx_framebuffer_fits(w, h, depth))
        return -1;

    nuttx_active = TRUE;
    memset(nuttx_palette, 0, sizeof(nuttx_palette));
    nuttx_mouse_x = w / 2;
    nuttx_mouse_y = h / 2;
    nuttx_mouse_z = 0;
    nuttx_mouse_buttons = 0;
    nuttx_mouse_clip = 0;
    nuttx_dvi_ready = (fb_nuttx_dvi_start() == 0);
#ifdef FB_NUTTX_QEMU_MOCK_DEVICES
    nuttx_qemu_present_count = 0;
    nuttx_qemu_last_checksum = 0;

    printf("FB_NUTTX_QEMU_GFX_INIT w=%d h=%d depth=%d dvi=%d\n",
        w,
        h,
        depth,
        nuttx_dvi_ready ? 1 : 0);
    fflush(stdout);
#endif

    if (nuttx_dvi_ready)
        fb_nuttx_dvi_present();

    driver_open_hid_devices();
    driver_make_stdin_nonblocking();

    return 0;
}

static void driver_exit(void)
{
    fb_nuttx_dvi_framebuffer_lock();

    if (nuttx_dvi_ready)
        fb_nuttx_dvi_blank();

    driver_close_hid_devices();
    driver_restore_stdin();
    nuttx_dvi_ready = FALSE;
    nuttx_active = FALSE;
    fb_nuttx_dvi_framebuffer_unlock();
}

static void driver_lock(void)
{
    fb_nuttx_dvi_framebuffer_lock();
}

static void driver_unlock(void)
{
    if (!nuttx_active || (__fb_gfx == NULL)) {
        fb_nuttx_dvi_framebuffer_unlock();
        return;
    }

    /*
        The RP2350 encoder reads the framebuffer directly on core 1.  Clear
        gfxlib's dirty markers after drawing while continuous scanout samples
        the current framebuffer contents.
    */
    if (__fb_gfx->dirty)
        fb_hMemSet(__fb_gfx->dirty, FALSE,
            __fb_gfx->h * __fb_gfx->scanline_size);

#ifdef FB_NUTTX_QEMU_MOCK_DEVICES
    nuttx_qemu_trace_present();
#endif

    if (nuttx_dvi_ready)
        fb_nuttx_dvi_present();

    fb_nuttx_dvi_framebuffer_unlock();
}

static void driver_set_palette(int index, int r, int g, int b)
{
    unsigned int red;
    unsigned int green;
    unsigned int blue;

    if ((index < 0) || (index >= 256))
        return;

    /* gfxlib's driver interface already supplies 8-bit RGB components. */

    red = (unsigned int)clamp_int(r, 0, 255);
    green = (unsigned int)clamp_int(g, 0, 255);
    blue = (unsigned int)clamp_int(b, 0, 255);

    nuttx_palette[index] = (red << 16) | (green << 8) | blue;

    if (nuttx_dvi_ready)
        fb_nuttx_dvi_set_palette(index, nuttx_palette[index]);
}

static void driver_wait_vsync(void)
{
    struct timespec delay;

    delay.tv_sec = 0;
    delay.tv_nsec = 16666666L;
    nanosleep(&delay, NULL);
}

static void driver_poll_events(void)
{
    if (!nuttx_active || (__fb_gfx == NULL))
        return;

    /*
        Poll the NuttX USB HID keyboard when the board exposes it, while still
        accepting serial console input.  That keeps QEMU smoke tests and
        recovery consoles usable until the harness can inject USB HID events.
    */
    if (__fb_gfx->key != NULL)
        fb_hMemSet(__fb_gfx->key, FALSE, 128);

    if (nuttx_keyboard_fd >= 0)
        driver_poll_keyboard_fd(nuttx_keyboard_fd);

    driver_poll_keyboard_fd(STDIN_FILENO);

    driver_poll_mouse_fd();
}

static int driver_get_mouse(int *x, int *y, int *z, int *buttons, int *clip)
{
    if (!nuttx_active)
        return -1;

    *x = nuttx_mouse_x;
    *y = nuttx_mouse_y;
    *z = nuttx_mouse_z;
    *buttons = nuttx_mouse_buttons;
    *clip = nuttx_mouse_clip;

    return 0;
}

static void driver_set_mouse(int x, int y, int cursor, int clip)
{
    (void)cursor;

    if (!nuttx_active || (__fb_gfx == NULL))
        return;

    if (x >= 0)
        nuttx_mouse_x = clamp_int(x, 0, __fb_gfx->w - 1);

    if (y >= 0)
        nuttx_mouse_y = clamp_int(y, 0, __fb_gfx->h - 1);

    if (clip >= 0)
        nuttx_mouse_clip = (clip != 0);
}

static int *driver_fetch_modes(int depth, int *size)
{
    int *modes;
    int count;
    int i;

    if (size == NULL)
        return NULL;

    *size = 0;

    if ((depth != 0) && !nuttx_is_paletted_depth(depth))
        return NULL;

    count = 0;

    for (i = 0; i < (int)(sizeof(nuttx_modes) / sizeof(nuttx_modes[0])); i++) {
        if (((depth == 0) || (depth == nuttx_modes[i].depth)) &&
            nuttx_framebuffer_fits(nuttx_modes[i].width, nuttx_modes[i].height,
                nuttx_modes[i].depth))
            count++;
    }

    if (count == 0)
        return NULL;

    modes = (int *)malloc(sizeof(int) * count);

    if (modes == NULL)
        return NULL;

    count = 0;

    for (i = 0; i < (int)(sizeof(nuttx_modes) / sizeof(nuttx_modes[0])); i++) {
        if (((depth == 0) || (depth == nuttx_modes[i].depth)) &&
            nuttx_framebuffer_fits(nuttx_modes[i].width, nuttx_modes[i].height,
                nuttx_modes[i].depth)) {
            modes[count] = SCREENLIST(nuttx_modes[i].width,
                nuttx_modes[i].height);
            count++;
        }
    }

    *size = count;

    return modes;
}

static const GFXDRIVER fb_gfxDriverNuttX =
{
    "nuttx",
    driver_init,
    driver_exit,
    driver_lock,
    driver_unlock,
    driver_set_palette,
    driver_wait_vsync,
    driver_get_mouse,
    NULL,
    NULL,
    driver_set_mouse,
    NULL,
    NULL,
    driver_fetch_modes,
    NULL,
    driver_poll_events,
    NULL,
    NULL
};

const GFXDRIVER *__fb_gfx_drivers_list[] =
{
    &fb_gfxDriverNuttX,
    &__fb_gfxDriverNull,
    NULL
};

void fb_hScreenInfo(ssize_t *width, ssize_t *height, ssize_t *depth,
    ssize_t *refresh)
{
    *width = NUTTX_SCREEN_WIDTH;
    *height = NUTTX_SCREEN_HEIGHT;
    *depth = 8;
    *refresh = 60;
}

/* end of gfx_driver.c */
