/* X11 gfx driver */

#ifndef DISABLE_X11

#include "../fb_gfx.h"
#include "fb_gfx_x11.h"
#include <limits.h>
#include <sys/shm.h>

static int driver_init(char *title, int w, int h, int depth, int refresh_rate, int flags);
static int driver_resize(int width, int height);

/* GFXDRIVER */
const GFXDRIVER fb_gfxDriverX11 =
{
	"X11",                  /* char *name; */
	driver_init,            /* int (*init)(char *title, int w, int h, int depth, int refresh_rate, int flags); */
	fb_hX11Exit,            /* void (*exit)(void); */
	fb_hX11Lock,            /* void (*lock)(void); */
	fb_hX11Unlock,          /* void (*unlock)(void); */
	fb_hX11SetPalette,      /* void (*set_palette)(int index, int r, int g, int b); */
	fb_hX11WaitVSync,       /* void (*wait_vsync)(void); */
	fb_hX11GetMouse,        /* int (*get_mouse)(int *x, int *y, int *z, int *buttons, int *clip); */
	NULL,                   /* int (*get_touch_count)(void); */
	NULL,                   /* int (*get_touch)(int index, int *x, int *y, int *id); */
	fb_hX11SetMouse,        /* void (*set_mouse)(int x, int y, int cursor, int clip); */
	fb_hX11SetWindowTitle,  /* void (*set_window_title)(char *title); */
	fb_hX11SetWindowPos,    /* int (*set_window_pos)(int x, int y); */
	fb_hX11FetchModes,      /* int *(*fetch_modes)(void); */
	NULL,                   /* void (*flip)(void); */
	NULL,                   /* void (*poll_events)(void); */
	NULL,                   /* void (*update)(void); */
	driver_resize           /* int (*resize)(int width, int height); */
};

static XImage *image, *shape_image;
static XImage *scaled_image;
static Pixmap shape_pixmap;
static GC shape_gc;
static XShmSegmentInfo shm_info;
static BLITTER *blitter;
static int is_shm;
static int needs_byte_swap;
static void (*update_mask)(unsigned char *src, unsigned char *mask, int w, int h);

static int host_byte_order(void)
{
	const unsigned int value = 1;

	if (*(const unsigned char *)&value)
		return LSBFirst;
	else
		return MSBFirst;
}

static void byte_swap_16(unsigned char *data, int pitch, int w, int h)
{
	int x;
	unsigned char temp;

	for (; h; h--) {
		for (x = 0; x < w; x++) {
			temp = data[(x * 2) + 0];
			data[(x * 2) + 0] = data[(x * 2) + 1];
			data[(x * 2) + 1] = temp;
		}
		data += pitch;
	}
}

static void byte_swap_32(unsigned char *data, int pitch, int w, int h)
{
	int x;
	unsigned char temp;

	for (; h; h--) {
		for (x = 0; x < w; x++) {
			temp = data[(x * 4) + 0];
			data[(x * 4) + 0] = data[(x * 4) + 3];
			data[(x * 4) + 3] = temp;

			temp = data[(x * 4) + 1];
			data[(x * 4) + 1] = data[(x * 4) + 2];
			data[(x * 4) + 2] = temp;
		}
		data += pitch;
	}
}

static void byte_swap_image_region(int y, int h)
{
	unsigned char *data;

	if (!needs_byte_swap)
		return;

	data = (unsigned char *)image->data + (y * image->bytes_per_line);

	if (image->bits_per_pixel == 16)
		byte_swap_16(data, image->bytes_per_line, fb_x11.w, h);
	else if (image->bits_per_pixel == 32)
		byte_swap_32(data, image->bytes_per_line, fb_x11.w, h);
}

static void update_mask_8(unsigned char *pixel, unsigned char *mask, int w, int h)
{
	int x, b;
	unsigned char *p = pixel;
	
	for(; h; h--) {
		b = 0;
		for (x = 0; x < w; x++) {
			if (*p++ != 0)
				b |= 1 << (x & 0x7);
			if ((x & 0x7) == 0x7) {
				*mask++ = b;
				b = 0;
			}
		}
		if (w & 0x7)
			*mask++ = b;
	}
}

static void update_mask_16(unsigned char *pixel, unsigned char *mask, int w, int h)
{
	int x, b;
	unsigned short *p = (unsigned short *)pixel;
	
	for(; h; h--) {
		b = 0;
		for (x = 0; x < w; x++) {
			if (*p++ != MASK_COLOR_16)
				b |= 1 << (x & 0x7);
			if ((x & 0x7) == 0x7) {
				*mask++ = b;
				b = 0;
			}
		}
		if (w & 0x7)
			*mask++ = b;
	}
}

static void update_mask_32(unsigned char *pixel, unsigned char *mask, int w, int h)
{
	int x, b;
	unsigned int *p = (unsigned int *)pixel;
	
	for(; h; h--) {
		b = 0;
		for (x = 0; x < w; x++) {
			if (((*p++) & ~MASK_A_32) != MASK_COLOR_32)
				b |= 1 << (x & 0x7);
			if ((x & 0x7) == 0x7) {
				*mask++ = b;
				b = 0;
			}
		}
		if (w & 0x7)
			*mask++ = b;
	}
}

static int x11_init(void)
{
	XGCValues values;
	int x = 0, y = 0, h, is_rgb = FALSE;
	char *display_name;
	
	image = NULL;
	scaled_image = NULL;
	shape_image = NULL;
	is_shm = FALSE;
	needs_byte_swap = FALSE;
	
	if ((fb_x11.visual_depth >= 24) && (fb_x11.visual->red_mask == 0xFF))
		is_rgb = TRUE;
	else if ((fb_x11.visual_depth >= 15) && (fb_x11.visual->red_mask == 0x1F))
		is_rgb = TRUE;
	blitter = fb_hGetBlitter(fb_x11.visual_depth, is_rgb);
	if (!blitter)
		return -1;
	
	if (!(fb_x11.flags & DRIVER_FULLSCREEN)) {
		x = (XDisplayWidth(fb_x11.display, fb_x11.screen) - fb_x11.w) >> 1;
		y = (XDisplayHeight(fb_x11.display, fb_x11.screen) - fb_x11.h) >> 1;
	}
	fb_hX11InitWindow(x, y);
	
	if (fb_x11.flags & DRIVER_SHAPED_WINDOW) {
		shape_image = XCreateImage(fb_x11.display, fb_x11.visual, 1, XYBitmap, 0, NULL, fb_x11.w, fb_x11.h, 8, 0);
		shape_image->data = calloc(1, shape_image->bytes_per_line * shape_image->height);
		shape_pixmap = XCreateBitmapFromData(fb_x11.display, fb_x11.window,
											 shape_image->data, fb_x11.w, fb_x11.h);
		values.foreground = 1;
		values.background = 0;											 
		shape_gc = XCreateGC(fb_x11.display, shape_pixmap, GCForeground | GCBackground, &values);
		if (__fb_gfx->bpp == 1)
			update_mask = update_mask_8;
		else if (__fb_gfx->bpp == 2)
			update_mask = update_mask_16;
		else
			update_mask = update_mask_32;
	}
	
	fb_x11.display_offset = 0;
	display_name = XDisplayName(NULL);
	if (((!display_name[0]) || (display_name[0] == ':') ||
	    (!strncmp(display_name, "unix:", 5))) &&
	    (XShmQueryExtension(fb_x11.display)) &&
	    !(fb_x11.flags & DRIVER_RESIZABLE)) {
		if (fb_x11.flags & DRIVER_FULLSCREEN) {
			if (fb_hX11EnterFullscreen(&h)) {
				fb_hX11LeaveFullscreen();
				return -1;
			}
			XReparentWindow(fb_x11.display, fb_x11.window, fb_x11.fswindow, 0, 0);
			fb_hX11RefreshLayout(fb_x11.w, h);
			XMoveResizeWindow(fb_x11.display, fb_x11.fswindow, 0,0,fb_x11.view_w, fb_x11.view_h);
			XMoveResizeWindow(fb_x11.display, fb_x11.window, 0, 0, fb_x11.view_w, fb_x11.view_h);
		}
		is_shm = TRUE;
		image = XShmCreateImage(fb_x11.display, fb_x11.visual, XDefaultDepth(fb_x11.display, fb_x11.screen),
					ZPixmap, 0, &shm_info, fb_x11.w, fb_x11.h);
		if (image) {
			shm_info.shmid = shmget(IPC_PRIVATE, image->bytes_per_line * image->height, IPC_CREAT | 0777);
			shm_info.shmaddr = image->data = shmat(shm_info.shmid, 0, 0);
			shm_info.readOnly = False;
			if (!XShmAttach(fb_x11.display, &shm_info)) {
				shmdt(shm_info.shmaddr);
				shmctl(shm_info.shmid, IPC_RMID, 0);
				XDestroyImage(image);
				image = NULL;
			}
		}
	}
	else if (fb_x11.flags & DRIVER_FULLSCREEN)
		return -1;
	if (!image) {
		is_shm = FALSE;
		image = XCreateImage(fb_x11.display, fb_x11.visual, XDefaultDepth(fb_x11.display, fb_x11.screen),
				     ZPixmap, 0, NULL, fb_x11.w, fb_x11.h, 32, 0);
		image->data = malloc(image->bytes_per_line * image->height);
		if (!image->data) {
			XDestroyImage(image);
			image = NULL;
		}
	}
	if (!image)
		return -1;
	
	needs_byte_swap = ((image->bits_per_pixel == 16) || (image->bits_per_pixel == 32)) &&
	                   (image->byte_order != host_byte_order());

	return 0;
}

static int ensure_scaled_image(void)
{
	if (fb_x11.scale <= 1)
		return 0;

	if (scaled_image &&
	    (scaled_image->width == fb_x11.draw_w) &&
	    (scaled_image->height == fb_x11.draw_h))
		return 0;

	if (scaled_image) {
		XDestroyImage(scaled_image);
		scaled_image = NULL;
	}

	scaled_image = XCreateImage(fb_x11.display, fb_x11.visual,
	                            XDefaultDepth(fb_x11.display, fb_x11.screen),
	                            ZPixmap, 0, NULL, fb_x11.draw_w, fb_x11.draw_h, 32, 0);
	if (!scaled_image)
		return -1;

	scaled_image->data = malloc(scaled_image->bytes_per_line * scaled_image->height);
	if (!scaled_image->data) {
		XDestroyImage(scaled_image);
		scaled_image = NULL;
		return -1;
	}

	return 0;
}

static void scale_image_region(int y, int h)
{
	unsigned char *src_line, *dst_line, *src_pixel, *dst_pixel;
	int bytes_per_pixel, x, sx, sy;

	if (!scaled_image)
		return;

	bytes_per_pixel = image->bits_per_pixel >> 3;
	if (bytes_per_pixel <= 0)
		return;

	for (; h; h--, y++) {
		src_line = (unsigned char *)image->data + (y * image->bytes_per_line);
		for (sy = 0; sy < fb_x11.scale; sy++) {
			dst_line = (unsigned char *)scaled_image->data +
			           (((y * fb_x11.scale) + sy) * scaled_image->bytes_per_line);
			for (x = 0; x < fb_x11.w; x++) {
				src_pixel = src_line + (x * bytes_per_pixel);
				for (sx = 0; sx < fb_x11.scale; sx++) {
					dst_pixel = dst_line + (((x * fb_x11.scale) + sx) * bytes_per_pixel);
					fb_hMemCpy(dst_pixel, src_pixel, bytes_per_pixel);
				}
			}
		}
	}
}

void fb_hX11WaitUnmapped(Window w)
{
	XEvent e;
	do {
		XMaskEvent(fb_x11.display, StructureNotifyMask, &e);
	} while ((e.type != UnmapNotify) || (e.xmap.event != w));
}

static void x11_exit(void)
{
	if (fb_x11.flags & DRIVER_FULLSCREEN)
		fb_hX11LeaveFullscreen();
	XUnmapWindow(fb_x11.display, fb_x11.window);
	fb_hX11WaitUnmapped(fb_x11.window);
	if (fb_x11.flags & DRIVER_FULLSCREEN) {
		XUnmapWindow(fb_x11.display, fb_x11.fswindow);
	XSync(fb_x11.display, False);
	} else {
		if (!(fb_x11.flags & DRIVER_NO_FRAME)) {
			XUnmapWindow(fb_x11.display, fb_x11.wmwindow);
			fb_hX11WaitUnmapped(fb_x11.wmwindow);
		}
	}
	if (image) {
		if (is_shm) {
			XShmDetach(fb_x11.display, &shm_info);
			shmdt(shm_info.shmaddr);
			shmctl(shm_info.shmid, IPC_RMID, 0);
		}
		XDestroyImage(image);
	}
	if (scaled_image) {
		XDestroyImage(scaled_image);
		scaled_image = NULL;
	}
	if (shape_image) {
		XDestroyImage(shape_image);
		XFreePixmap(fb_x11.display, shape_pixmap);
	}
}

static void x11_update(void)
{
	int i, y, h;
	
	fb_hX11RefreshLayout(fb_x11.view_w, fb_x11.view_h);
	blitter((unsigned char *)image->data, image->bytes_per_line);
	for (i = 0; i < fb_x11.h; i++) {
		if (__fb_gfx->dirty[i]) {
			for (y = i, h = 0; (i < fb_x11.h) && __fb_gfx->dirty[i]; h++, i++)
				;
			byte_swap_image_region(y, h);
			if (shape_image) {
				update_mask((unsigned char *)__fb_gfx->framebuffer + (y * __fb_gfx->pitch),
							(unsigned char *)shape_image->data + (y * shape_image->bytes_per_line), fb_x11.w, h);
				XPutImage(fb_x11.display, shape_pixmap, shape_gc, shape_image, 0, y, 0, y, fb_x11.w, h);
				XShapeCombineMask(fb_x11.display, fb_x11.window, ShapeBounding, 0, 0, shape_pixmap, ShapeSet);
			}
			if (fb_x11.scale > 1) {
				if (!ensure_scaled_image()) {
					scale_image_region(y, h);
					XPutImage(fb_x11.display, fb_x11.window, fb_x11.gc, scaled_image,
					          0, y * fb_x11.scale,
					          fb_x11.draw_offset_x, fb_x11.draw_offset_y + (y * fb_x11.scale),
					          fb_x11.draw_w, h * fb_x11.scale);
				}
			}
			else {
				if (is_shm)
					XShmPutImage(fb_x11.display, fb_x11.window, fb_x11.gc, image, 0, y,
					             fb_x11.draw_offset_x, y + fb_x11.draw_offset_y, fb_x11.w, h, False);
				else
					XPutImage(fb_x11.display, fb_x11.window, fb_x11.gc, image, 0, y,
					          fb_x11.draw_offset_x, y + fb_x11.draw_offset_y, fb_x11.w, h);
			}
		}
	}
	fb_hMemSet(__fb_gfx->dirty, FALSE, fb_x11.h);
}

static int driver_init(char *title, int w, int h, int depth_arg, int refresh_rate, int flags)
{
    int depth = MAX(8, depth_arg);
	if (flags & DRIVER_OPENGL)
		return -1;
	fb_hMemSet(&fb_x11, 0, sizeof(fb_x11));
	fb_x11.init = x11_init;
	fb_x11.exit = x11_exit;
	fb_x11.update = x11_update;
	return fb_hX11Init(title, w, h, depth, refresh_rate, flags);
}

/*
	Resizable modes deliberately use an ordinary XImage rather than MIT-SHM.
	That keeps replacement failure atomic and avoids detaching a shared segment
	while the X server may still be consuming an earlier presentation request.
*/
static int driver_resize(int width, int height)
{
	XImage *replacement;
	size_t data_size;
	int physical_height;

	if ((width <= 0) || (height <= 0) || is_shm || (image == NULL) ||
	    ((size_t)height > ((size_t)INT_MAX /
	     (size_t)__fb_gfx->scanline_size)))
		return -1;
	physical_height = height * __fb_gfx->scanline_size;
	replacement = XCreateImage(fb_x11.display, fb_x11.visual,
		XDefaultDepth(fb_x11.display, fb_x11.screen), ZPixmap, 0, NULL,
		width, physical_height, 32, 0);
	if (replacement == NULL)
		return -1;
	if ((replacement->bytes_per_line <= 0) ||
	    ((size_t)replacement->height > ((size_t)-1 /
	     (size_t)replacement->bytes_per_line))) {
		XDestroyImage(replacement);
		return -1;
	}
	data_size = (size_t)replacement->bytes_per_line *
		(size_t)replacement->height;
	replacement->data = (char *)malloc(data_size);
	if (replacement->data == NULL) {
		XDestroyImage(replacement);
		return -1;
	}
	memset(replacement->data, 0, data_size);

	XDestroyImage(image);
	image = replacement;
	if (scaled_image != NULL) {
		XDestroyImage(scaled_image);
		scaled_image = NULL;
	}
	fb_x11.w = width;
	fb_x11.h = physical_height;
	fb_x11.content_w = width;
	fb_x11.content_h = physical_height;
	fb_hX11RefreshLayout(width, physical_height);
	needs_byte_swap = ((image->bits_per_pixel == 16) ||
		(image->bits_per_pixel == 32)) &&
		(image->byte_order != host_byte_order());
	return 0;
}

#endif
