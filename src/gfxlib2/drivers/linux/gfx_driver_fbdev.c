/*
	Project: FreeBASIC GfxLib2 Linux backend
	-----------------------------------------

	File: gfx_driver_fbdev.c

	Purpose:

		Present gfxlib2 graphics through a Linux framebuffer device.

	Responsibilities:

		- select and restore fbdev display modes
		- convert gfxlib2 pixels and palettes into the device pixel format
		- provide framebuffer mouse input and vertical synchronization

	This file intentionally does NOT contain:

		- generic graphics drawing operations
		- Linux joystick input, which is handled by GETJOYSTICK
		- sound output, which is handled by sfxlib
*/

#include "../../fb_gfx.h"
#include "fb_gfx_linux.h"
#include "../../../rtlib/drivers/unix/fb_private_console.h"

#ifndef DISABLE_FBDEV

#include "fb_linux_maliit.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <linux/fb.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#ifndef FB_AUX_VGA_PLANES_VGA4
#define FB_AUX_VGA_PLANES_VGA4	0
#endif

#if defined HOST_X86 || defined HOST_X86_64
#define OUTB(port,value)	{ __asm__ __volatile__ ("outb %b0, %w1" : : "a"(value), "Nd"(port)); }
#else
#define OUTB(port,value)
#endif

typedef struct FBDEVDRIVER
{
	int w, h, depth, flags;
	ssize_t refresh_rate;
	int mouse_clip;
	int reserved;
} FBDEVDRIVER;

static FBDEVDRIVER fb_fbdev;

static int driver_init(char *title, int w, int h, int depth, int refresh_rate, int flags);
static void driver_exit(void);
static void driver_lock(void);
static void driver_unlock(void);
static void driver_set_palette(int index, int r, int g, int b);
static void driver_wait_vsync(void);
static int driver_get_mouse(int *x, int *y, int *z, int *buttons, int *clip);
static void driver_set_mouse(int x, int y, int cursor, int clip);
static int *driver_fetch_modes(int depth, int *size);

/* GFXDRIVER */
const GFXDRIVER fb_gfxDriverFBDev =
{
	"FBDev",            /* char *name; */
	driver_init,        /* int (*init)(char *title, int w, int h, int depth, int refresh_rate, int flags); */
	driver_exit,        /* void (*exit)(void); */
	driver_lock,        /* void (*lock)(void); */
	driver_unlock,      /* void (*unlock)(void); */
	driver_set_palette, /* void (*set_palette)(int index, int r, int g, int b); */
	driver_wait_vsync,  /* void (*wait_vsync)(void); */
	driver_get_mouse,   /* int (*get_mouse)(int *x, int *y, int *z, int *buttons, int *clip); */
	NULL,               /* int (*get_touch_count)(void); */
	NULL,               /* int (*get_touch)(int index, int *x, int *y, int *id); */
	driver_set_mouse,   /* void (*set_mouse)(int x, int y, int cursor, int clip); */
	NULL,               /* void (*set_window_title)(char *title); */
	NULL,               /* int (*set_window_pos)(int x, int y); */
	driver_fetch_modes, /* int *(*fetch_modes)(void); */
	NULL,               /* void (*flip)(void); */
	NULL,               /* void (*poll_events)(void); */
	NULL,               /* void (*update)(void); */
	NULL                /* int (*resize)(int width, int height); */
};


typedef struct {
	int w, h;
} GFXMODE;


static const GFXMODE standard_mode[] = {
	{ 320, 200 }, { 320, 240 }, { 400, 300 }, { 512, 384 }, { 640, 400 }, { 640, 480 },
	{ 800, 600 }, { 1024, 768 }, { 1280, 1024 }, { 1600, 1200 }, { 0, 0 }
};

static int device_fd = -1;
static struct fb_fix_screeninfo device_info;
static struct fb_var_screeninfo mode, orig_mode;
static struct fb_cmap cmap, orig_cmap;
static unsigned char *framebuffer = NULL;
static unsigned short *palette = NULL;
static unsigned char color_conv[4096];
static BLITTER *blitter;
static unsigned char *scale_buffer = NULL;
static size_t framebuffer_offset;
static int is_running = FALSE, is_active = TRUE;
static int mode_changed = FALSE, console_mode_active = FALSE;
static int sync_initialized = FALSE, driver_ready = FALSE;
static int framebuffer_mapped = FALSE, framebuffer_uses_write = FALSE;
static int framebuffer_write_error = FALSE;
static int framebuffer_scale = 1, framebuffer_scaled_w = 0, framebuffer_scaled_h = 0, scale_pitch = 0;
static __u32 vsync_flags = 0;
static int is_palette_changed = FALSE;
static int mouse_fd = -1, mouse_packet_size, mouse_shown = TRUE;
static int mouse_x, mouse_y, mouse_screen_x, mouse_screen_y, mouse_z, mouse_buttons;
static int mouse_clip = 0;
static uint64_t last_click_time = 0;
static unsigned char *vga16_line_buffer;
static size_t vga16_line_buffer_capacity;
static pthread_t thread;
static pthread_mutex_t mutex;
static pthread_cond_t cond;

static int fbdev_dimensions_fit_int(__u32 width, __u32 height)
{
	return (width > 0) && (height > 0) &&
	       (width <= (__u32)INT_MAX) && (height <= (__u32)INT_MAX);
}

static int fbdev_size_multiply(size_t left, size_t right, size_t *result)
{
	if (!result || ((right != 0) && (left > SIZE_MAX / right)))
		return -1;

	*result = left * right;
	return 0;
}

static int fbdev_set_framebuffer_offset(int width, int height, size_t *offset)
{
	uint64_t x_offset, y_offset, row_stride, bytes_per_pixel, offset_wide;
	uint64_t visible_row_bytes, final_row_offset, end_offset;
	int pixel_bytes;

	if (!offset || (width <= 0) || (height <= 0) ||
	    (mode.xres < (__u32)width) || (mode.yres < (__u32)height))
		return -1;

	y_offset = ((uint64_t)mode.yres - (uint64_t)(unsigned int)height) >> 1;
	if (mode.bits_per_pixel == 4) {
		if (mode.xres < 8)
			return -1;
		row_stride = (uint64_t)(mode.xres >> 3);
		x_offset = ((uint64_t)mode.xres - (uint64_t)(unsigned int)width) >> 4;
		visible_row_bytes = ((uint64_t)(unsigned int)width + UINT64_C(7)) >> 3;
	} else {
		if (mode.bits_per_pixel > 32)
			return -1;
		pixel_bytes = BYTES_PER_PIXEL(mode.bits_per_pixel);
		if (pixel_bytes <= 0)
			return -1;
		bytes_per_pixel = (uint64_t)(unsigned int)pixel_bytes;
		row_stride = (uint64_t)device_info.line_length;
		x_offset = (((uint64_t)mode.xres - (uint64_t)(unsigned int)width) >> 1) *
		           bytes_per_pixel;
		visible_row_bytes = (uint64_t)(unsigned int)width * bytes_per_pixel;
	}

	if ((y_offset != 0) && (row_stride > UINT64_MAX / y_offset))
		return -1;
	offset_wide = y_offset * row_stride;
	if (x_offset > UINT64_MAX - offset_wide)
		return -1;
	offset_wide += x_offset;
	if ((height > 1) &&
	    (row_stride > UINT64_MAX / (uint64_t)(unsigned int)(height - 1)))
		return -1;
	final_row_offset = row_stride * (uint64_t)(unsigned int)(height - 1);
	if (final_row_offset > UINT64_MAX - offset_wide)
		return -1;
	end_offset = offset_wide + final_row_offset;
	if (visible_row_bytes > UINT64_MAX - end_offset)
		return -1;
	end_offset += visible_row_bytes;
	if ((x_offset > row_stride) ||
	    (visible_row_bytes > row_stride - x_offset) ||
	    (offset_wide >= (uint64_t)device_info.smem_len) ||
	    (end_offset > (uint64_t)device_info.smem_len) ||
	    (offset_wide > (uint64_t)SIZE_MAX))
		return -1;

	*offset = (size_t)offset_wide;
	return 0;
}

static int fbdev_reserve_vga16_line_buffer(size_t capacity)
{
	unsigned char *new_buffer;

	if (capacity == 0)
		return -1;
	if (capacity <= vga16_line_buffer_capacity)
		return 0;

	new_buffer = (unsigned char *)realloc(vga16_line_buffer, capacity);
	if (!new_buffer)
		return -1;

	vga16_line_buffer = new_buffer;
	vga16_line_buffer_capacity = capacity;
	return 0;
}

static int fbdev_refresh_rate_to_int(ssize_t refresh_rate)
{
	if (refresh_rate <= 0)
		return 0;
	if (refresh_rate > INT_MAX)
		return INT_MAX;
	return (int)refresh_rate;
}

static useconds_t fbdev_frame_delay(void)
{
	ssize_t refresh_rate = fb_fbdev.refresh_rate;
	uint64_t delay;

	if (refresh_rate <= 0)
		refresh_rate = 60;

	delay = UINT64_C(1000000) / (uint64_t)refresh_rate;
	if (delay == 0)
		delay = 1;

	return (useconds_t)delay;
}

static unsigned short fbdev_palette_channel(int channel)
{
	if (channel < 0)
		channel = 0;
	else if (channel > 255)
		channel = 255;

	return (unsigned short)((unsigned int)channel << 8);
}

static int fbdev_modes_match(const struct fb_var_screeninfo *left,
	const struct fb_var_screeninfo *right)
{
	return (left->xres == right->xres) &&
		(left->yres == right->yres) &&
		(left->xres_virtual == right->xres_virtual) &&
		(left->yres_virtual == right->yres_virtual) &&
		(left->xoffset == right->xoffset) &&
		(left->yoffset == right->yoffset) &&
		(left->bits_per_pixel == right->bits_per_pixel) &&
		(left->grayscale == right->grayscale) &&
		(left->red.offset == right->red.offset) &&
		(left->red.length == right->red.length) &&
		(left->red.msb_right == right->red.msb_right) &&
		(left->green.offset == right->green.offset) &&
		(left->green.length == right->green.length) &&
		(left->green.msb_right == right->green.msb_right) &&
		(left->blue.offset == right->blue.offset) &&
		(left->blue.length == right->blue.length) &&
		(left->blue.msb_right == right->blue.msb_right) &&
		(left->transp.offset == right->transp.offset) &&
		(left->transp.length == right->transp.length) &&
		(left->transp.msb_right == right->transp.msb_right) &&
		(left->pixclock == right->pixclock) &&
		(left->left_margin == right->left_margin) &&
		(left->right_margin == right->right_margin) &&
		(left->upper_margin == right->upper_margin) &&
		(left->lower_margin == right->lower_margin) &&
		(left->hsync_len == right->hsync_len) &&
		(left->vsync_len == right->vsync_len) &&
		(left->sync == right->sync) &&
		(left->vmode == right->vmode);
}

static int fbdev_set_mode(const struct fb_var_screeninfo *requested,
	struct fb_var_screeninfo *current)
{
	if (fbdev_modes_match(requested, current))
		return 0;

	if (ioctl(device_fd, FBIOPUT_VSCREENINFO, requested) < 0) {
		if (ioctl(device_fd, FBIOGET_VSCREENINFO, current) < 0) {
			mode_changed = TRUE;
			return -1;
		}
		mode_changed = !fbdev_modes_match(current, &orig_mode);
		return 0;
	}

	/* A successful set may invalidate fbdev mappings, so read back before use. */
	mode_changed = TRUE;
	if (ioctl(device_fd, FBIOGET_VSCREENINFO, current) < 0)
		return -1;

	mode_changed = !fbdev_modes_match(current, &orig_mode);
	return 0;
}

static int fbdev_use_write_access(const char *device_name)
{
	const char *access_method = getenv("FBGFX_FRAMEBUFFER_ACCESS");
	char sysfs_driver[128];
	ssize_t path_length;

	if (access_method) {
		if (!strcmp(access_method, "write"))
			return TRUE;
		if (!strcmp(access_method, "mmap"))
			return FALSE;
	}

	/*
		Some Rockchip DRM fbdev drivers expose GEM-backed memory without a
		physical start address.  On affected kernels, the legacy mmap callback
		rejects that mapping with EINVAL and can corrupt page-table state.
		Use fbdev's write callback for this framebuffer instead of probing mmap.
	*/
	if (!device_name || strcmp(device_name, "/dev/fb0") ||
	    (device_info.smem_start != 0))
		return FALSE;

	path_length = readlink("/sys/class/graphics/fb0/device/driver",
		sysfs_driver, sizeof(sysfs_driver) - 1);
	if ((path_length <= 0) || ((size_t)path_length >= sizeof(sysfs_driver) - 1))
		return FALSE;

	sysfs_driver[path_length] = '\0';
	return (strstr(sysfs_driver, "rockchip-drm") != NULL);
}

static int fbdev_write_framebuffer(void)
{
	size_t offset = 0;
	size_t remaining = (size_t)device_info.smem_len;

	while (remaining > 0) {
		ssize_t bytes_written = pwrite(device_fd, framebuffer + offset,
			remaining, (off_t)offset);

		if (bytes_written < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		if (bytes_written == 0) {
			errno = EIO;
			return -1;
		}

		offset += (size_t)bytes_written;
		remaining -= (size_t)bytes_written;
	}

	return 0;
}

#if defined HOST_X86 || defined HOST_X86_64

static void vga16_blitter(unsigned char *dest, int pitch)
{
	unsigned int color;
	unsigned char *buffer, pattern;
	unsigned char *s, *source = fb_gfx->framebuffer;
	int x, y, plane, i, offset;
	size_t line_buffer_size;

	if (fb_fbdev.w <= 0)
		return;
	line_buffer_size = (size_t)fb_fbdev.w;
	if (fbdev_reserve_vga16_line_buffer(line_buffer_size) != 0)
		return;
	buffer = vga16_line_buffer;

	OUTB(0x3CE, 0x03);
	OUTB(0x3CF, 0x00);

	OUTB(0x3CE, 0x05);
	OUTB(0x3CF, 0x00);

	OUTB(0x3CE, 0x01);
	OUTB(0x3CF, 0x00);

	OUTB(0x3CE, 0x08);
	OUTB(0x3CF, 0xFF);

	for (y = 0; y < fb_fbdev.h; y++) {
		if (fb_gfx->dirty[y]) {
			offset = 0;
			s = source;
			for (x = 0; x < fb_fbdev.w; x += 8) {
				for (plane = 0; plane < 4; plane++) {
					pattern = 0;
					for (i = 0; i < 8; i++) {
						if (fb_gfx->depth == 8) {
							color = fb_gfx->device_palette[s[i]];
							color = color_conv[((color & 0xF0) >> 4) | ((color & 0xF000) >> 8) | ((color & 0xF00000) >> 12)];
						}
						else {
							color = s[i];
						}

						if (color & (1 << plane))
							pattern |= 1 << (7 - i);
					}
					buffer[((fb_fbdev.w >> 3) * plane) + offset] = pattern;
				}
				offset++;
				s += 8;
			}
			for (plane = 0; plane < 4; plane++) {
				OUTB(0x3C4, 0x02);
				OUTB(0x3C5, (1 << plane));
				fb_hMemCpy(dest, buffer + ((fb_fbdev.w >> 3) * plane),
				           (size_t)(fb_fbdev.w >> 3));
			}
		}
		dest += pitch;
		source += fb_gfx->pitch;
	}
}

#else /* !( defined HOST_X86 || defined HOST_X86_64 ) */

static void vga16_blitter(unsigned char *dest, int pitch)
{
	unsigned int c, color[2];
	unsigned char *buffer;
	unsigned char *s, *source = fb_gfx->framebuffer;
	int x, y, offset, i;
	size_t line_buffer_size;

	if (fb_fbdev.w <= 0)
		return;
	line_buffer_size = (size_t)fb_fbdev.w;
	if (fbdev_reserve_vga16_line_buffer(line_buffer_size) != 0)
		return;
	buffer = vga16_line_buffer;

	for (y = 0; y < fb_fbdev.h; y++) {
		if (fb_gfx->dirty[y]) {
			offset = 0;
			s = source;
			for (x = 0; x < fb_fbdev.w; x += 2) {
				if (fb_gfx->depth == 8) {
					for( i = 0; i < 2; i++ )
					{
						c = fb_gfx->device_palette[s[i]];
						color[i] = color_conv[((c & 0xF0) >> 4) | ((c & 0xF000) >> 8) | ((c & 0xF00000) >> 12)];
					}
					buffer[offset] = (color[0] << 4) | color[1];
				}
				else
				{
					buffer[offset] = ((s[0] << 4) & 0xF0) | (s[1] & 0xF);
				}

				offset++;
				s += 2;
			}

			fb_hMemCpy(dest, buffer, (size_t)(fb_fbdev.w >> 1));
		}
		dest += pitch;
		source += fb_gfx->pitch;
	}
}

#endif /* defined HOST_X86 || defined HOST_X86_64 */

static void scaled_blitter(unsigned char *dest, int pitch)
{
	unsigned char *src, *src_pixel, *dst, *dst_pixel;
	int bytes_per_pixel, x, y, sx, sy;

	if ((framebuffer_scale <= 1) || (!scale_buffer)) {
		blitter(dest, pitch);
		return;
	}

	bytes_per_pixel = BYTES_PER_PIXEL(mode.bits_per_pixel);
	if ((bytes_per_pixel <= 0) || (scale_pitch <= 0)) {
		blitter(dest, pitch);
		return;
	}

	/*
	 * Reuse the normal blitter for color conversion.
	 *
	 * The generic blitters already know how to translate gfxlib's internal
	 * framebuffer into the active fbdev pixel format.  The scaled path first
	 * asks the normal blitter to build an unscaled device-format image, then
	 * expands only the dirty lines into the real framebuffer.
	 */
	blitter(scale_buffer, scale_pitch);

	for (y = 0; y < fb_fbdev.h; y++) {
		if (!fb_gfx->dirty[y])
			continue;

		src = scale_buffer + ((size_t)y * (size_t)scale_pitch);
		for (sy = 0; sy < framebuffer_scale; sy++) {
			dst = dest + ((((size_t)y * (size_t)framebuffer_scale) +
			               (size_t)sy) * (size_t)pitch);
			for (x = 0; x < fb_fbdev.w; x++) {
				src_pixel = src + ((size_t)x * (size_t)bytes_per_pixel);
				for (sx = 0; sx < framebuffer_scale; sx++) {
					dst_pixel = dst + ((((size_t)x * (size_t)framebuffer_scale) +
					                    (size_t)sx) * (size_t)bytes_per_pixel);
					fb_hMemCpy(dst_pixel, src_pixel, (size_t)bytes_per_pixel);
				}
			}
		}
	}
}

static void fbdev_sync_mouse_screen_pos(void)
{
	if (framebuffer_scale > 1) {
		mouse_screen_x = (mouse_x * framebuffer_scale) + (framebuffer_scale / 2);
		mouse_screen_y = (mouse_y * framebuffer_scale) + (framebuffer_scale / 2);
		mouse_screen_x = MID(0, mouse_screen_x, framebuffer_scaled_w - 1);
		mouse_screen_y = MID(0, mouse_screen_y, framebuffer_scaled_h - 1);
	} else {
		mouse_screen_x = mouse_x;
		mouse_screen_y = mouse_y;
	}
}

static void fbdev_apply_mouse_delta(int dx, int dy, EVENT *e)
{
	int old_x = mouse_x;
	int old_y = mouse_y;

	if (framebuffer_scale > 1) {
		mouse_screen_x = MID(0, mouse_screen_x + dx, framebuffer_scaled_w - 1);
		mouse_screen_y = MID(0, mouse_screen_y + dy, framebuffer_scaled_h - 1);
		mouse_x = MID(0, mouse_screen_x / framebuffer_scale, fb_gfx->w - 1);
		mouse_y = MID(0, mouse_screen_y / framebuffer_scale, fb_gfx->h - 1);
	} else {
		mouse_x = MID(0, mouse_x + dx, fb_gfx->w - 1);
		mouse_y = MID(0, mouse_y + dy, fb_gfx->h - 1);
		mouse_screen_x = mouse_x;
		mouse_screen_y = mouse_y;
	}

	e->x = mouse_x;
	e->y = mouse_y;
	e->dx = mouse_x - old_x;
	e->dy = mouse_y - old_y;

	if( fb_gfx->scanline_size != 1 ) {
		e->y /= fb_gfx->scanline_size;
		e->dy /= fb_gfx->scanline_size;
	}
}

static void *driver_thread(void *arg)
{
	struct fb_vblank vblank;
	unsigned int count;
	uint64_t cur_time;
	fd_set set;
	struct timeval cur_tv, tv = { 0, 0 };
	unsigned char buffer[1024];
	int buttons;
	size_t bytes_left = 0, bytes_consumed;
	ssize_t bytes_read;
	EVENT e;

	(void)arg;

	pthread_mutex_lock(&mutex);
	is_running = TRUE;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);

	for (;;) {
		pthread_mutex_lock(&mutex);
		if (!is_running) {
			pthread_mutex_unlock(&mutex);
			break;
		}

		if ((mouse_fd >= 0) && (mouse_fd < FD_SETSIZE)) {
			FD_ZERO(&set);
			FD_SET(mouse_fd, &set);
			if (select(FD_SETSIZE, &set, NULL, NULL, &tv) > 0) {
				bytes_read = read(mouse_fd, &buffer[bytes_left], sizeof(buffer) - bytes_left);
				if (bytes_read > 0) {
					bytes_left += (size_t)bytes_read;
					while (bytes_left >= (size_t)mouse_packet_size) {
						if (((mouse_packet_size == 3) && ((buffer[0] & 0xC0) != 0x00)) ||
						   ((mouse_packet_size == 4) && ((buffer[0] & 0xC8) != 0x08)))
							bytes_consumed = 1U;
						else {
							fbdev_apply_mouse_delta(
									(int)buffer[1] - ((int)(buffer[0] & 0x10) << 4),
									-(int)buffer[2] + ((int)(buffer[0] & 0x20) << 3),
								&e
							);
							if (e.dx || e.dy) {
								e.type = EVENT_MOUSE_MOVE;
								fb_hPostEvent(&e);
							}
							buttons = mouse_buttons;
							mouse_buttons = buffer[0] & 0x7;
							if ((mouse_packet_size == 4) && (buffer[3] & 0xF)) {
								mouse_z += (((buffer[3] & 0xF) - 7) >> 3);
								e.type = EVENT_MOUSE_WHEEL;
								e.z = mouse_z;
								fb_hPostEvent(&e);
							}
							buttons = (mouse_buttons ^ buttons) & 0x7;
							for (e.button = 0x4; e.button; e.button >>= 1) {
								if (buttons & e.button) {
									if (mouse_buttons & e.button) {
										if ((gettimeofday(&cur_tv, NULL) == 0) &&
										    (cur_tv.tv_sec >= 0)) {
											cur_time = ((uint64_t)cur_tv.tv_sec * UINT64_C(1000)) +
											           (uint64_t)(cur_tv.tv_usec / 1000);
											if (cur_time - last_click_time < DOUBLE_CLICK_TIME)
												e.type = EVENT_MOUSE_DOUBLE_CLICK;
											else
												e.type = EVENT_MOUSE_BUTTON_PRESS;
											last_click_time = cur_time;
										} else {
											e.type = EVENT_MOUSE_BUTTON_PRESS;
										}
									}
									else
										e.type = EVENT_MOUSE_BUTTON_RELEASE;
									fb_hPostEvent(&e);
								}
							}
							bytes_consumed = (size_t)mouse_packet_size;
						}
						bytes_left -= bytes_consumed;
						memmove(buffer, buffer + bytes_consumed, bytes_left);
					}
				}
			}
		}

		if (vsync_flags & (FB_VBLANK_HAVE_VBLANK | FB_VBLANK_HAVE_VCOUNT)) {
			if (vsync_flags & FB_VBLANK_HAVE_VCOUNT) {
				ioctl(device_fd, FBIOGET_VBLANK, &vblank);
				do {
					count = vblank.vcount;
				} while ((ioctl(device_fd, FBIOGET_VBLANK, &vblank) == 0) && (vblank.vcount >= count));
			}
			else {
				while ((ioctl(device_fd, FBIOGET_VBLANK, &vblank) == 0) && (vblank.flags & FB_VBLANK_VBLANKING))
					;
				while ((ioctl(device_fd, FBIOGET_VBLANK, &vblank) == 0) && (!(vblank.flags & FB_VBLANK_VBLANKING)))
					;
			}
		}
		pthread_cond_signal(&cond);

		if (is_active) {
			if (is_palette_changed) {
				if (device_info.type != FB_TYPE_VGA_PLANES)
					ioctl(device_fd, FBIOPUTCMAP, &cmap);
				else
					fb_hMemSet(fb_gfx->dirty, TRUE, (size_t)fb_fbdev.h);
				if (mouse_fd >= 0)
					fb_hSoftCursorPaletteChanged();
				is_palette_changed = FALSE;
			}
			if ((mouse_fd >= 0) && (mouse_shown))
				fb_hSoftCursorPut(mouse_x, mouse_y);
			scaled_blitter(framebuffer + framebuffer_offset, (int)device_info.line_length);
			if (framebuffer_uses_write && !framebuffer_write_error &&
			    (fbdev_write_framebuffer() < 0))
				framebuffer_write_error = TRUE;
			fb_hMemSet(fb_gfx->dirty, FALSE, (size_t)fb_fbdev.h);
			if ((mouse_fd >= 0) && (mouse_shown))
				fb_hSoftCursorUnput(mouse_x, mouse_y);
		}

		pthread_mutex_unlock(&mutex);

		if (vsync_flags & (FB_VBLANK_HAVE_VBLANK | FB_VBLANK_HAVE_VCOUNT))
			usleep(8000);
		else
			usleep(fbdev_frame_delay());
	}

	return NULL;
}

static void driver_save_screen(void)
{
	EVENT e;

	pthread_mutex_lock(&mutex);
	if (!driver_ready) {
		pthread_mutex_unlock(&mutex);
		return;
	}
	is_active = FALSE;
	pthread_mutex_unlock(&mutex);
	ioctl(device_fd, FBIOPUTCMAP, &orig_cmap);
	e.type = EVENT_WINDOW_LOST_FOCUS;
	fb_hPostEvent(&e);
}

static void driver_restore_screen(void)
{
	EVENT e;

	pthread_mutex_lock(&mutex);
	if (!driver_ready) {
		pthread_mutex_unlock(&mutex);
		return;
	}
	is_active = TRUE;
	is_palette_changed = TRUE;
	fb_hMemSet(framebuffer, 0, (size_t)device_info.smem_len);
	if (framebuffer_uses_write && !framebuffer_write_error &&
	    (fbdev_write_framebuffer() < 0))
		framebuffer_write_error = TRUE;
	fb_hMemSet(fb_gfx->dirty, TRUE, (size_t)fb_fbdev.h);
	pthread_mutex_unlock(&mutex);
	e.type = EVENT_WINDOW_GOT_FOCUS;
	fb_hPostEvent(&e);
}

static void driver_key_handler( int pressed, int repeated, int scancode, int key )
{
	EVENT e;
	int ready;

	pthread_mutex_lock(&mutex);
	ready = driver_ready;
	pthread_mutex_unlock(&mutex);
	if (!ready)
		return;

	if( pressed ) {
		if( repeated ) {
			e.type = EVENT_KEY_REPEAT;
		} else {
			e.type = EVENT_KEY_PRESS;
		}
	} else {
		e.type = EVENT_KEY_RELEASE;
	}

	e.scancode = scancode;

	/* Don't return extended keycodes in the ascii field */
	e.ascii = ((key < 0) || (key > 0xFF)) ? 0 : key;

	fb_hPostEvent( &e );
}

static int driver_init(char *title, int w, int h, int depth, int refresh_rate, int flags)
{
	const char *device_name;
	int try, i, j, r, g, b, dist, best_dist, best_index = 0;
	ssize_t dummy;
	__u32 palette_len;
	int using_current_mode = FALSE;
	struct fb_var_screeninfo current_mode;
	struct fb_vblank vblank;
	const char *mouse_device[] = { "/dev/input/mice", "/dev/usbmouse", "/dev/psaux", NULL };
	const unsigned char im_init[] = { 243, 200, 243, 100, 243, 80 };

	(void)title;
	(void)refresh_rate;
	if ((w <= 0) || (h <= 0))
		return -1;

	if (flags & DRIVER_OPENGL)
		return -1;

	mode_changed = FALSE;
	console_mode_active = FALSE;
	sync_initialized = FALSE;
	driver_ready = FALSE;
	framebuffer = NULL;
	palette = NULL;
	scale_buffer = NULL;
	framebuffer_mapped = FALSE;
	framebuffer_uses_write = FALSE;
	framebuffer_write_error = FALSE;
	device_fd = -1;
	mouse_fd = -1;
	free(vga16_line_buffer);
	vga16_line_buffer = NULL;
	vga16_line_buffer_capacity = 0;
	fb_fbdev.refresh_rate = 0;
	is_running = FALSE;
	is_active = TRUE;

	fb_fbdev.w = w;
	fb_fbdev.h = h;
	fb_fbdev.flags = flags;
	framebuffer_scale = 1;
	framebuffer_scaled_w = w;
	framebuffer_scaled_h = h;
	scale_pitch = 0;
	depth = MAX(depth, 4);

	device_name = getenv("FBGFX_FRAMEBUFFER");
	if (!device_name)
		device_name = "/dev/fb0";
	device_fd = open(device_name, O_RDWR, 0);
	if (device_fd < 0)
		return -1;

	if ((ioctl(device_fd, FBIOGET_FSCREENINFO, &device_info) < 0) ||
	    (ioctl(device_fd, FBIOGET_VSCREENINFO, &orig_mode) < 0) ||
	    ((device_info.type != FB_TYPE_PACKED_PIXELS)
#if defined(i386) && defined(FB_TYPE_VGA_PLANES)
	     && (device_info.type != FB_TYPE_VGA_PLANES)
#endif
	     ) ||
	    ((device_info.visual != FB_VISUAL_PSEUDOCOLOR) &&
	     (device_info.visual != FB_VISUAL_DIRECTCOLOR) &&
	     (device_info.visual != FB_VISUAL_TRUECOLOR))) {
		return -1;
	}

	if (pthread_mutex_init(&mutex, NULL) != 0)
		return -1;
	if (pthread_cond_init(&cond, NULL) != 0) {
		pthread_mutex_destroy(&mutex);
		return -1;
	}
	sync_initialized = TRUE;

	/*
		A Linux fbdev mode switch requires a controlling virtual console.
		Acquire it before changing the hardware mode so an SSH-launched program
		cannot leave the display in a half-initialized state.
	*/
	if (fb_hConsoleGfxMode(driver_exit, driver_save_screen,
		driver_restore_screen, driver_key_handler))
		return -1;
	console_mode_active = TRUE;
	current_mode = orig_mode;

#if defined(i386) && defined(FB_TYPE_VGA_PLANES)
	if ((device_info.type == FB_TYPE_VGA_PLANES) && (device_info.type_aux == FB_AUX_VGA_PLANES_VGA4)) {
		mode = orig_mode;
		if ((orig_mode.xres >= (unsigned int)w) && (orig_mode.yres >= (unsigned int)h) && fb_console_context.has_perm && (depth <= 8)) {
			/* we are in vga16 mode, got to live with it */
			goto got_mode;
		}

		return -1;
	}
#endif

	/* tries in order:
	 *  1) wanted resolution and color depth;
	 *  2) wanted resolution and original color depth;
	 *  3) current framebuffer mode, centered/scaled if it is large enough.
	 *
	 * The fallback deliberately does not switch to the next larger standard
	 * mode.  If the requested mode cannot be set exactly, the safest fbdev
	 * behavior is to leave the display at the user's current resolution and
	 * present the logical SCREEN as a centered integer-scaled image there.
	 */
	for (try = 0; try < 2; try++) {
		mode = orig_mode;

		mode.xoffset = 0;
		mode.yoffset = 0;

		if (try == 0) {
			mode.bits_per_pixel = (__u32)depth;
			mode.grayscale = 0;
			switch (depth) {
				case 15:
					mode.red.offset   = 10; mode.red.length   = 5;
					mode.green.offset = 5;  mode.green.length = 5;
					mode.blue.offset  = 0;  mode.blue.length  = 5;
					break;
				case 16:
					mode.red.offset   = 11; mode.red.length   = 5;
					mode.green.offset = 5;  mode.green.length = 6;
					mode.blue.offset  = 0;  mode.blue.length  = 5;
					break;
				case 24:
				case 32:
					mode.red.offset   = 16; mode.red.length   = 8;
					mode.green.offset = 8;  mode.green.length = 8;
					mode.blue.offset  = 0;  mode.blue.length  = 8;
					break;
				default:
					mode.red.offset   = mode.red.length   = 0;
					mode.green.offset = mode.green.length = 0;
					mode.blue.offset  = mode.blue.length  = 0;
					break;
			}
			mode.red.msb_right = mode.green.msb_right = mode.blue.msb_right = 0;
		}

		mode.xres = mode.xres_virtual = (__u32)w;
		mode.yres = mode.yres_virtual = (__u32)h;
		/* Do not repeat FBIOPUT for the active mode: some drivers invalidate
		   their fbdev mapping even when the requested mode is unchanged. */
		if (fbdev_set_mode(&mode, &current_mode) < 0)
			return -1;
		mode = current_mode;
		/* Some fbdev drivers round the requested visible geometry. */
		if ((mode.xres == (unsigned int)w) &&
		    (mode.yres == (unsigned int)h))
			goto got_mode;
	}

	mode = orig_mode;
	mode.xoffset = 0;
	mode.yoffset = 0;
	if (fbdev_set_mode(&mode, &current_mode) < 0)
		return -1;
	mode = current_mode;
	if ((mode.xres >= (unsigned int)w) && (mode.yres >= (unsigned int)h)) {
		using_current_mode = TRUE;
		goto got_mode;
	}

	return -1;

got_mode:
	if (!fbdev_dimensions_fit_int(mode.xres, mode.yres))
		return -1;
	if (fb_hFBDevInfo(&dummy, &dummy, &dummy, &fb_fbdev.refresh_rate) < 0)
		return -1;
	fb_gfx->refresh_rate = fbdev_refresh_rate_to_int(fb_fbdev.refresh_rate);

	if (ioctl(device_fd, FBIOGET_FSCREENINFO, &device_info) < 0)
		return -1;
	if ((device_info.smem_len == 0) || (device_info.line_length > (__u32)INT_MAX))
		return -1;

	framebuffer_uses_write = fbdev_use_write_access(device_name);
	if (framebuffer_uses_write) {
		if (device_info.smem_len == 0)
			return -1;
		framebuffer = (unsigned char *)malloc((size_t)device_info.smem_len);
		if (!framebuffer)
			return -1;
	} else {
		framebuffer = mmap(NULL, (size_t)device_info.smem_len, PROT_READ | PROT_WRITE,
			MAP_SHARED, device_fd, 0);
		if (framebuffer == (unsigned char *)-1) {
			framebuffer = NULL;
			return -1;
		}
		framebuffer_mapped = TRUE;
	}

	fb_hMemSet(framebuffer, 0, (size_t)device_info.smem_len);
	if (framebuffer_uses_write && (fbdev_write_framebuffer() < 0)) {
		framebuffer_write_error = TRUE;
		return -1;
	}

	/*
	 * Some fbdev drivers cannot switch to the exact requested mode even
	 * though the current framebuffer is large enough.  In that case, keep
	 * the application's logical SCREEN size unchanged, but present it as a
	 * centered integer-scaled image.  Exact mode switches keep the old
	 * one-to-one behavior.
	 */
	if (using_current_mode && ((mode.xres != (unsigned int)w) || (mode.yres != (unsigned int)h))) {
#ifndef GFXLIB_NEVERSCALE
		int scale_x = (int)mode.xres / w;
		int scale_y = (int)mode.yres / h;
		int scale = (scale_x < scale_y) ? scale_x : scale_y;

		if ((scale > 1) && (mode.bits_per_pixel >= 8)) {
			framebuffer_scale = scale;
			framebuffer_scaled_w = w * framebuffer_scale;
			framebuffer_scaled_h = h * framebuffer_scale;
		}
#endif
	}

	if (mode.bits_per_pixel == 4) {
		framebuffer_scale = 1;
		framebuffer_scaled_w = w;
		framebuffer_scaled_h = h;
		palette_len = 16;
		if (fbdev_set_framebuffer_offset(w, h, &framebuffer_offset) != 0)
			return -1;
		blitter = vga16_blitter;
	} else {
		size_t scale_size, pitch_size;

		palette_len = 256;
		if (mode.bits_per_pixel > 32u)
			return -1;
		blitter = fb_hGetBlitter((int)mode.bits_per_pixel,
			(mode.red.offset == 0) ? TRUE : FALSE);
		if (!blitter)
			return -1;
		if (fbdev_set_framebuffer_offset(framebuffer_scaled_w, framebuffer_scaled_h,
		                                 &framebuffer_offset) != 0)
			return -1;

		if (framebuffer_scale > 1) {
			int pixel_bytes = BYTES_PER_PIXEL(mode.bits_per_pixel);
			if ((pixel_bytes > 0) &&
			    (fbdev_size_multiply((size_t)w, (size_t)pixel_bytes, &pitch_size) == 0) &&
			    (pitch_size <= (size_t)INT_MAX) &&
			    (fbdev_size_multiply(pitch_size, (size_t)h, &scale_size) == 0)) {
				scale_buffer = (unsigned char *)malloc(scale_size);
				if (scale_buffer)
					scale_pitch = (int)pitch_size;
			}
			if (!scale_buffer) {
				framebuffer_scale = 1;
				framebuffer_scaled_w = w;
				framebuffer_scaled_h = h;
				scale_pitch = 0;
				if (fbdev_set_framebuffer_offset(w, h, &framebuffer_offset) != 0)
					return -1;
			}
		}
	}

	mouse_packet_size = 3;
	for (try = 0; mouse_device[try]; try++) {
		mouse_fd = open(mouse_device[try], O_RDWR, 0);
		if ((mouse_fd >= 0) && (write(mouse_fd, im_init, sizeof(im_init)) == sizeof(im_init))) {
			mouse_packet_size++;
			break;
		}
		if (mouse_fd < 0)
			mouse_fd = open(mouse_device[try], O_RDONLY, 0);
		if (mouse_fd >= 0)
			break;
	}
	if (mouse_fd >= FD_SETSIZE) {
		close(mouse_fd);
		mouse_fd = -1;
	}
	if (mouse_fd >= 0) {
		mouse_x = w >> 1;
		mouse_y = h >> 1;
		fbdev_sync_mouse_screen_pos();
		mouse_buttons = mouse_z = 0;
		mouse_shown = TRUE;
		if (fb_hSoftCursorInit() != 0) {
			close(mouse_fd);
			mouse_fd = -1;
		}
	}

	palette = (unsigned short *)malloc(sizeof(unsigned short) * 1536);
	if (!palette)
		return -1;
	orig_cmap.start = 0;
	orig_cmap.len = palette_len;
	orig_cmap.transp = NULL;
	orig_cmap.red = palette;
	orig_cmap.green = palette + 256;
	orig_cmap.blue = palette + 512;
	ioctl(device_fd, FBIOGETCMAP, &orig_cmap);
	cmap.start = 0;
	cmap.len = palette_len;
	cmap.transp = NULL;
	cmap.red = palette + 768;
	cmap.green = palette + 1024;
	cmap.blue = palette + 1280;
	if ((mode.bits_per_pixel == 4) && (depth == 8)) {
		/* set safe palette */
		for (i = 0; i < 16; i++) {
			r = cmap.red[i]   = fb_palette[FB_PALETTE_16].data[(i * 3) + 2] << 8;
			g = cmap.green[i] = fb_palette[FB_PALETTE_16].data[(i * 3) + 1] << 8;
			b = cmap.blue[i]  = fb_palette[FB_PALETTE_16].data[(i * 3)    ] << 8;
			fb_gfx->device_palette[i] = (unsigned int)(r >> 8) |
			                            (unsigned int)g |
			                            ((unsigned int)b << 8);
		}
		ioctl(device_fd, FBIOPUTCMAP, &cmap);
		for (i = 0; i < 4096; i++) {
			best_dist = 1000000;
			r = (i & 0xF) << 4;
			g = (i & 0xF0);
			b = (i & 0xF00) >> 4;
			for (j = 0; j < 16; j++) {
				dist = fb_hColorDistance(j, r, g, b);
				if (dist < best_dist) {
					best_dist = dist;
					best_index = j;
				}
			}
			color_conv[i] = (unsigned char)best_index;
		}
	}

	if (ioctl(device_fd, FBIOGET_VBLANK, &vblank) == 0)
		vsync_flags = vblank.flags;

	pthread_mutex_lock(&mutex);
	driver_ready = TRUE;
	if (pthread_create(&thread, NULL, driver_thread, NULL)) {
		driver_ready = FALSE;
		pthread_mutex_unlock(&mutex);
		return -1;
	}
	while (!is_running)
		pthread_cond_wait(&cond, &mutex);
	pthread_mutex_unlock(&mutex);

	return 0;
}

static void driver_exit(void)
{
	int join_thread = FALSE;

	/* Drain input callbacks before the driver lock and mode are destroyed. */
	fb_hLinuxMaliitCancelPending();
	/* Stop and join the renderer before unmapping its framebuffer. */
	if (sync_initialized) {
		pthread_mutex_lock(&mutex);
		driver_ready = FALSE;
		if (is_running) {
			is_running = FALSE;
			join_thread = TRUE;
		}
		pthread_mutex_unlock(&mutex);
	}

	if (join_thread)
		pthread_join(thread, NULL);

	if (console_mode_active) {
		fb_hConsoleGfxMode(NULL, NULL, NULL, NULL);
		console_mode_active = FALSE;
	}

	if (mouse_fd >= 0) {
		fb_hSoftCursorExit();
		close(mouse_fd);
		mouse_fd = -1;
	}
	free(vga16_line_buffer);
	vga16_line_buffer = NULL;
	vga16_line_buffer_capacity = 0;

	if (device_fd >= 0) {
		if (framebuffer != NULL) {
			if (framebuffer_mapped)
				munmap(framebuffer, device_info.smem_len);
			else
				free(framebuffer);
			framebuffer = NULL;
		}
		framebuffer_mapped = FALSE;
		framebuffer_uses_write = FALSE;
		framebuffer_write_error = FALSE;
		if (scale_buffer) {
			free(scale_buffer);
			scale_buffer = NULL;
		}
		if (palette) {
			ioctl(device_fd, FBIOPUTCMAP, &orig_cmap);
			free(palette);
			palette = NULL;
		}
		if (mode_changed) {
			ioctl(device_fd, FBIOPUT_VSCREENINFO, &orig_mode);
			mode_changed = FALSE;
		}
		close(device_fd);
		device_fd = -1;
	}

	if (sync_initialized) {
		pthread_cond_destroy(&cond);
		pthread_mutex_destroy(&mutex);
		sync_initialized = FALSE;
	}
}

static void driver_lock(void)
{
	pthread_mutex_lock(&mutex);
}

static void driver_unlock(void)
{
	pthread_mutex_unlock(&mutex);
}

static void driver_set_palette(int index, int r, int g, int b)
{
	if (!palette || (index < 0) || ((__u32)index >= cmap.len))
		return;

	cmap.red[index] = fbdev_palette_channel(r);
	cmap.green[index] = fbdev_palette_channel(g);
	cmap.blue[index] = fbdev_palette_channel(b);
	is_palette_changed = TRUE;
}

static void driver_wait_vsync(void)
{
	pthread_mutex_lock(&mutex);
	pthread_cond_wait(&cond, &mutex);
	pthread_mutex_unlock(&mutex);
}

static int driver_get_mouse(int *x, int *y, int *z, int *buttons, int *clip)
{
	if (mouse_fd < 0)
		return -1;
	if (x) *x = mouse_x;
	if (y) *y = mouse_y;
	if (z) *z = mouse_z;
	if (buttons) *buttons = mouse_buttons;
	if (clip) *clip = mouse_clip;
	return 0;
}

static void driver_set_mouse(int x, int y, int cursor, int clip)
{
	if (x != (int)0x80000000 || y != (int)0x80000000) {
		if (x == (int)0x80000000) {
			x = mouse_x;
		}
		else if (y == (int)0x80000000) {
			y = mouse_y;
		}

		x = MID(0, x, fb_gfx->w - 1);
		y = MID(0, y, fb_gfx->h - 1);

		mouse_x = x;
		mouse_y = y;
		fbdev_sync_mouse_screen_pos();
	}
	mouse_shown = (cursor != 0);
	if (clip == 0)
		mouse_clip = FALSE;
	else if (clip > 0)
		mouse_clip = TRUE;
}

static int *driver_fetch_modes(int depth, int *size)
{
	const char *device_name;
	int i, fd, num_sizes = 0, *sizes = NULL;

	if (!size)
		return NULL;
	*size = 0;

	if ((depth != 8) && (depth != 15) && (depth != 16) && (depth != 24) && (depth != 32))
		return NULL;

	if (device_fd < 0) {
		device_name = getenv("FBGFX_FRAMEBUFFER");
		if (!device_name)
			device_name = "/dev/fb0";
		fd = open(device_name, O_RDWR, 0);
		if (fd < 0)
			return NULL;
	}
	else
		fd = device_fd;

	if (ioctl(fd, FBIOGET_VSCREENINFO, &mode) < 0) {
		if (device_fd < 0)
			close(fd);
		return NULL;
	}
	for (i = 0; standard_mode[i].w; i++) {
		mode.bits_per_pixel = (__u32)depth;
		mode.activate = FB_ACTIVATE_TEST;
		mode.xres = mode.xres_virtual = (__u32)standard_mode[i].w;
		mode.yres = mode.yres_virtual = (__u32)standard_mode[i].h;
		if (ioctl(fd, FBIOPUT_VSCREENINFO, &mode) == 0) {
			int *new_sizes;
			if ((mode.xres > 0x7FFFU) || (mode.yres > 0xFFFFU))
				continue;
			new_sizes = (int *)realloc(sizes,
				((size_t)num_sizes + 1U) * sizeof(*sizes));
			if (!new_sizes)
				break;
			sizes = new_sizes;
			sizes[num_sizes] = (int)(((uint32_t)mode.xres << 16) |
			                          (uint32_t)mode.yres);
			num_sizes++;
		}
	}

	if (device_fd < 0)
		close(fd);

	*size = num_sizes;
	return sizes;
}

int fb_hFBDevInfo(ssize_t *width, ssize_t *height, ssize_t *depth, ssize_t *refresh)
{
	struct fb_var_screeninfo temp, *info;
	int fd = -1, res;
	uint64_t htotal, vtotal;
	__u32 flags;
	double refresh_rate;

	if (!width || !height || !depth || !refresh)
		return -1;

	if (device_fd < 0) {
		if ((fd = open("/dev/fb0", O_RDWR, 0)) < 0)
			return -1;
		res = ioctl(fd, FBIOGET_VSCREENINFO, &temp);
		close(fd);
		if (res < 0)
			return -1;
		info = &temp;
	}
	else
		info = &mode;

	htotal = (uint64_t)info->left_margin + (uint64_t)info->xres +
	         (uint64_t)info->right_margin + (uint64_t)info->hsync_len;
	vtotal = (uint64_t)info->upper_margin + (uint64_t)info->yres +
	         (uint64_t)info->lower_margin + (uint64_t)info->vsync_len;
	flags = info->vmode & FB_VMODE_MASK;

	if (!(flags == FB_VMODE_INTERLACED))
		vtotal *= UINT64_C(2);
	if (flags == FB_VMODE_DOUBLE)
		vtotal *= UINT64_C(2);
#if SSIZE_MAX < UINT32_MAX
	if ((info->xres > (uint32_t)SSIZE_MAX) ||
	    (info->yres > (uint32_t)SSIZE_MAX) ||
	    (info->bits_per_pixel > (uint32_t)SSIZE_MAX))
		return -1;
#endif

	*width = (ssize_t)info->xres;
	*height = (ssize_t)info->yres;
	*depth = (ssize_t)info->bits_per_pixel;
	*refresh = 0;
	if (info->pixclock && htotal && vtotal) {
		refresh_rate = ((1.0e12 / (double)info->pixclock) /
		                (double)htotal / (double)vtotal) * 2.0;
		if (refresh_rate >= (double)SSIZE_MAX)
			*refresh = SSIZE_MAX;
		else
			*refresh = (ssize_t)refresh_rate;
	}

	return 0;
}

#endif

/* end of gfx_driver_fbdev.c */
