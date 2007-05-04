/*
 *  libgfx2 - FreeBASIC's alternative gfx library
 *	Copyright (C) 2005 Angelo Mottola (a.mottola@libero.it)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 * vesa_lin.c -- linear VESA gfx driver
 *
 * chng: jul/2006 written [DrV]
 *
 */

#include "fb_gfx_dos.h"

static int driver_init(char *title, int w, int h, int depth, int refresh_rate, int flags);
static void driver_exit(void);
static void driver_update(void);
static void end_of_driver_update(void);

GFXDRIVER fb_gfxDriverVESAlinear =
{
	"VESA linear",           /* char *name; */
	driver_init,             /* int (*init)(char *title, int w, int h, int depth, int refresh_rate, int flags); */
	driver_exit,             /* void (*exit)(void); */
	fb_dos_lock,             /* void (*lock)(void); */
	fb_dos_unlock,           /* void (*unlock)(void); */
	fb_dos_set_palette,      /* void (*set_palette)(int index, int r, int g, int b); */
	fb_dos_vga_wait_vsync,   /* void (*wait_vsync)(void); */
	fb_dos_get_mouse,        /* int (*get_mouse)(int *x, int *y, int *z, int *buttons); */
	fb_dos_set_mouse,        /* void (*set_mouse)(int x, int y, int cursor); */
	fb_dos_set_window_title, /* void (*set_window_title)(char *title); */
	NULL,                    /* int (*set_window_pos)(int x, int y); */
	fb_dos_vesa_fetch_modes, /* int *(*fetch_modes)(int depth, int *size); */
	NULL,                    /* void (*flip)(void); */
	NULL                     /* void (*poll_events)(void); */
};


static __dpmi_meminfo mapping = {0};
static unsigned char *video;
static BLITTER *blitter;
static int nearptr_enabled = FALSE;
static int data_locked = FALSE;


/*:::::*/
static int driver_init(char *title, int w, int h, int depth_arg, int refresh_rate, int flags)
{
	int depth = MAX(8, depth_arg);
	int is_rgb, bpp;
	
	fb_dos_detect();
	fb_dos_vesa_detect();
	
	if (flags & DRIVER_OPENGL)
		return -1;
	
	if (!fb_dos.nearptr_ok)
		return -1;
	
	if (!fb_dos.vesa_ok)
		return -1;
	
	if (fb_dos_vesa_set_mode(w, h, depth, TRUE))
		return -1;
	
	refresh_rate = 60; /* FIXME */
	
	fb_dos_lock_data(&video, sizeof(video));
	fb_dos_lock_data(&blitter, sizeof(blitter));
	data_locked = TRUE;

	is_rgb = (depth > 8) && (fb_dos.vesa_mode_info.LinRedFieldPosition != 0);
	
	if (fb_dos.vesa_mode_info.LinBlueFieldPosition == 10 || fb_dos.vesa_mode_info.LinRedFieldPosition == 10)
		bpp = 15;
	else if (fb_dos.vesa_mode_info.LinBlueFieldPosition == 11 || fb_dos.vesa_mode_info.LinRedFieldPosition == 11)
		bpp = 16;
	else
		bpp = fb_dos.vesa_mode_info.BitsPerPixel;
	
	blitter = fb_hGetBlitter(fb_dos.vesa_mode_info.BitsPerPixel, is_rgb); /* FIXME */
	if (!blitter)
		return -1;
	
	fb_dos.update = driver_update;
	fb_dos.update_len = (unsigned int)end_of_driver_update - (unsigned int)driver_update;
	fb_dos.set_palette = fb_dos_vga_set_palette; /* FIXME */
	
	__djgpp_nearptr_enable();
	nearptr_enabled = TRUE;
	
	mapping.address = fb_dos.vesa_mode_info.PhysBasePtr;
	mapping.size = fb_dos.vesa_info.total_memory << 16;
	if (__dpmi_physical_address_mapping(&mapping) != 0)
		return -1;
	
	video = (unsigned char *)(mapping.address - __djgpp_base_address);
	
	return fb_dos_init(title, w, h, depth, refresh_rate, flags);
}


/*:::::*/
static void driver_exit(void)
{
	if (mapping.address != 0)
	{
		__dpmi_free_physical_address_mapping(&mapping);
		mapping.address = 0;
	}
	
	if (nearptr_enabled)
	{
		__djgpp_nearptr_disable();
		nearptr_enabled = FALSE;
	}
	
	if (data_locked)
	{
		fb_dos_unlock_data(&video, sizeof(video));
		fb_dos_unlock_data(&blitter, sizeof(blitter));
		data_locked = FALSE;
	}
	
	fb_dos_exit();
}


/*:::::*/
static void driver_update(void)
{
	blitter(video, fb_dos.vesa_mode_info.BytesPerScanLine);

}

static void end_of_driver_update(void) { /* do not remove */ }