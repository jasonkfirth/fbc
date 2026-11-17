/* banked VESA gfx driver */

#include "../../fb_gfx.h"
#include "fb_gfx_dos.h"
#include <limits.h>

static int driver_init(char *title, int w, int h, int depth, int refresh_rate, int flags);
static void driver_exit(void);
static void driver_update(void);
static void end_of_driver_update(void);
static void fb_dos_vesa_set_bank_int(void);
static void fb_dos_vesa_set_bank_int_end(void);
extern void fb_dos_vesa_set_bank_pm(void);
extern void fb_dos_vesa_set_bank_pm_end(void);

/* GFXDRIVER */
const GFXDRIVER fb_gfxDriverVESA =
{
	"VESA banked",           /* char *name; */
	driver_init,             /* int (*init)(char *title, int w, int h, int depth, int refresh_rate, int flags); */
	driver_exit,             /* void (*exit)(void); */
	fb_dos_lock,             /* void (*lock)(void); */
	fb_dos_unlock,           /* void (*unlock)(void); */
	fb_dos_set_palette,      /* void (*set_palette)(int index, int r, int g, int b); */
	fb_dos_vga_wait_vsync,   /* void (*wait_vsync)(void); */
	fb_dos_get_mouse,        /* int (*get_mouse)(int *x, int *y, int *z, int *buttons, int *clip); */
	NULL,                    /* int (*get_touch_count)(void); */
	NULL,                    /* int (*get_touch)(int index, int *x, int *y, int *id); */
	fb_dos_set_mouse,        /* void (*set_mouse)(int x, int y, int cursor, int clip); */
	fb_dos_set_window_title, /* void (*set_window_title)(char *title); */
	NULL,                    /* int (*set_window_pos)(int x, int y); */
	fb_dos_vesa_fetch_modes, /* int *(*fetch_modes)(int depth, int *size); */
	NULL,                    /* void (*flip)(void); */
	NULL,                    /* void (*poll_events)(void); */
	NULL,                    /* void (*update)(void); */
	NULL                     /* int (*resize)(int width, int height); */
};

static int data_locked = FALSE;
static void (*fb_dos_vesa_set_bank)(void) = NULL;
static BLITTER *blitter = NULL;
static unsigned char *buffer = NULL;
static size_t buffer_size = 0;

int fb_dos_vesa_bank_number = 0;

static int driver_init(char *title, int w, int h, int depth_arg, int refresh_rate, int flags)
{
	int depth = MAX(8, depth_arg);
	int is_rgb = FALSE;
	int bpp;
	
	fb_dos_detect();
	fb_dos_vesa_detect();
	
	if (flags & DRIVER_OPENGL)
		return -1;
	
	if (!fb_dos.vesa_ok)
		return -1;
	
	if (fb_dos_vesa_set_mode(w, h, depth, FALSE))
		return -1;
	
	vesa_get_pm_functions();
	
	if (fb_dos_vesa_pm_bank_switcher)
		fb_dos_vesa_set_bank = fb_dos_vesa_set_bank_pm;
	else
		fb_dos_vesa_set_bank = fb_dos_vesa_set_bank_int;
	
	if (refresh_rate <= 0)
		refresh_rate = 60;
	
	is_rgb = (depth > 8) && (fb_dos.vesa_mode_info.RedFieldPosition == 0);
	
	if (fb_dos.vesa_mode_info.BlueFieldPosition == 10 || fb_dos.vesa_mode_info.RedFieldPosition == 10)
		bpp = 15;
	else if (fb_dos.vesa_mode_info.BlueFieldPosition == 11 || fb_dos.vesa_mode_info.RedFieldPosition == 11)
		bpp = 16;
	else
		bpp = fb_dos.vesa_mode_info.BitsPerPixel;
	
	if( depth != bpp || is_rgb )
	{
		unsigned int video_height = fb_dos.vesa_mode_info.YResolution;
		unsigned int bytes_per_scanline = fb_dos.vesa_mode_info.BytesPerScanLine;

		blitter = fb_hGetBlitter(bpp, is_rgb);
		if( !blitter )
			return -1;

		if( video_height == 0 || bytes_per_scanline == 0 ||
		    (size_t)video_height > (size_t)-1 / (size_t)bytes_per_scanline )
			return -1;

		buffer_size = (size_t)video_height * (size_t)bytes_per_scanline;
		buffer = malloc( buffer_size );
		if( !buffer )
		{
			buffer_size = 0;
			return -1;
		}
	}
	else
	{
		blitter = NULL;
		buffer = NULL;
	}
	
	fb_dos_lock_data( &fb_dos_vesa_set_bank, sizeof(fb_dos_vesa_set_bank) );
	fb_dos_lock_data( &fb_dos_vesa_pm_info, sizeof(fb_dos_vesa_pm_info) );
	fb_dos_lock_data( &blitter, sizeof(blitter) );
	lock_code( fb_dos_vesa_set_bank_pm, fb_dos_vesa_set_bank_pm_end );
	lock_code( fb_dos_vesa_set_bank_int, fb_dos_vesa_set_bank_int_end );
	if( buffer )
	{
		fb_dos_lock_data( &buffer, sizeof(buffer) );
		fb_dos_lock_data( buffer, buffer_size );
	}
	data_locked = TRUE;

	fb_dos.update = driver_update;
	fb_dos.update_len = (unsigned int)end_of_driver_update - (unsigned int)driver_update;
	fb_dos.set_palette = fb_dos_vesa_set_palette;

	return fb_dos_init(title, w, h, depth, refresh_rate, flags);
}

static void driver_exit(void)
{
	if( data_locked )
	{
			fb_dos_unlock_data( &fb_dos_vesa_set_bank, sizeof(fb_dos_vesa_set_bank) );
			fb_dos_unlock_data( &fb_dos_vesa_pm_info, sizeof(fb_dos_vesa_pm_info) );
			fb_dos_unlock_data( &blitter, sizeof(blitter) );
			unlock_code( fb_dos_vesa_set_bank_pm, fb_dos_vesa_set_bank_pm_end );
			unlock_code( fb_dos_vesa_set_bank_int, fb_dos_vesa_set_bank_int_end );
			data_locked = FALSE;
	}
	
	if( buffer )
	{
		fb_dos_unlock_data( &buffer, sizeof(buffer) );
		fb_dos_unlock_data( buffer, buffer_size );
		free( buffer );
		buffer = NULL;
		buffer_size = 0;
	}
	
	fb_dos_exit();
}

static void fb_dos_vesa_set_bank_int(void)
{
	/* set write bank of window A based on fb_dos_vesa_bank_number */
	if( fb_dos_vesa_bank_number < 0 || fb_dos_vesa_bank_number > USHRT_MAX )
		return;

	fb_dos.regs.x.ax = 0x4F05;
	fb_dos.regs.x.bx = 0;
	fb_dos.regs.x.dx = (unsigned short)fb_dos_vesa_bank_number;
	__dpmi_int(0x10, &fb_dos.regs);
}
static void fb_dos_vesa_set_bank_int_end(void) { }

static void driver_update(void)
{
	int curr_bank = -1;
	unsigned char *memory_buffer;
	unsigned char *framebuffer;
	unsigned long bank_number;
	unsigned long framebuffer_start;
	unsigned long bank_size;
	unsigned long bank_granularity;
	unsigned long bank_add;
	unsigned long todo;
	size_t copy_size;
	unsigned int height;
	unsigned int y1, y2;

	if( fb_gfx == NULL || fb_gfx->dirty == NULL || fb_dos.h <= 0 ||
	    fb_gfx->pitch <= 0 || fb_dos.vesa_mode_info.WinSize == 0 ||
	    fb_dos.vesa_mode_info.WinGranularity == 0 )
		return;
	height = (unsigned int)fb_dos.h;

	if( blitter )
	{
		blitter(buffer, fb_dos.vesa_mode_info.BytesPerScanLine);
		framebuffer = buffer;
	}
	else
	{
		framebuffer = fb_gfx->framebuffer;
	}
	if( framebuffer == NULL || fb_dos_vesa_set_bank == NULL )
		return;

	bank_size = (unsigned long)fb_dos.vesa_mode_info.WinSize * 1024ul;
	bank_granularity = (unsigned long)fb_dos.vesa_mode_info.WinGranularity * 1024ul;
	bank_add = bank_size / bank_granularity;
	if( bank_add == 0 )
		return;
	
	for (y1 = 0; y1 < height; y1++) {
		if (fb_gfx->dirty[y1]) {
			y2 = height;
			while( y2 > y1 && !fb_gfx->dirty[y2 - 1] )
				--y2;
			if( y2 <= y1 )
				return;

			bank_number = ((unsigned long)y1 * (unsigned long)fb_gfx->pitch) / bank_granularity;
			if( bank_number > USHRT_MAX )
				return;
			fb_dos_vesa_bank_number = (int)bank_number;

			framebuffer_start = bank_number * bank_granularity;
			todo = ((unsigned long)y2 * (unsigned long)fb_gfx->pitch) - framebuffer_start;
			memory_buffer = framebuffer + (size_t)framebuffer_start;
			
			while (todo > 0) {
				/* select the appropriate bank */
				if (curr_bank != fb_dos_vesa_bank_number) {
					curr_bank = fb_dos_vesa_bank_number;
					if( curr_bank < 0 || curr_bank > USHRT_MAX )
						return;
					fb_dos_vesa_set_bank();
				}
				
				/* how much can we copy in one go? */
				if (todo > bank_size)
					copy_size = (size_t)bank_size;
				else
					copy_size = (size_t)todo;
				if( copy_size == 0 )
					return;
				
				/* copy a bank of data to the screen */
				dosmemput(memory_buffer, copy_size, 0xA0000);
				
				/* move on to the next bank of data */
				todo -= (unsigned long)copy_size;
				memory_buffer += copy_size;
				if( todo > 0 ) {
					bank_number += bank_add;
					if( bank_number > USHRT_MAX )
						return;
					fb_dos_vesa_bank_number = (int)bank_number;
				}
			}
			
			break;
		}
	}
}
static void end_of_driver_update(void) { /* do not remove */ }
