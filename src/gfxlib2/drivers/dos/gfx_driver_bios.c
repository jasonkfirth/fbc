/* BIOS gfx driver */

#include "../../fb_gfx.h"
#include "fb_gfx_dos.h"
#include <go32.h>
#include <pc.h>
#include <stdint.h>

typedef void (*fnUpdate)(void);

static int driver_init(char *title, int w, int h, int depth, int refresh_rate, int flags);
static void driver_update_bpp1(void);
static void driver_update_bpp2(void);
static void driver_update_bpp4(void);
static void end_of_driver_update(void);
static int *driver_fetch_modes(int depth, int *size);
static void fb_dos_ega_set_palette(void);
static void fb_dos_cga_set_palette(void);

/* GFXDRIVER */
const GFXDRIVER fb_gfxDriverBIOS =
{
	"BIOS",                  /* char *name; */
	driver_init,             /* int (*init)(char *title, int w, int h, int depth, int refresh_rate, int flags); */
	fb_dos_exit,             /* void (*exit)(void); */
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
	driver_fetch_modes,      /* int *(*fetch_modes)(int depth, int *size); */
	NULL,                    /* void (*flip)(void); */
	NULL,                    /* void (*poll_events)(void); */
	NULL,                    /* void (*update)(void); */
	NULL                     /* int (*resize)(int width, int height); */
};

static const int res_bpp1[] = {
	SCREENLIST(640, 200),     /* 0x06 */
    SCREENLIST(640, 350),     /* 0x0F */
    SCREENLIST(640, 480),     /* 0x11 */
    0
};
static const unsigned char mode_bpp1[] = {
	0x06, 0x0F, 0x11, 0
};
static const int res_bpp2[] = {
	SCREENLIST(320, 200),     /* 0x04 */
    0
};
static const unsigned char mode_bpp2[] = {
	0x04, 0
};
static const int res_bpp4[] = {
#if 0
	SCREENLIST(320, 200),     /* 0x0D */
	SCREENLIST(640, 200),     /* 0x0E */
#endif
	SCREENLIST(640, 350),     /* 0x10 */
    SCREENLIST(640, 480),     /* 0x12 */
    0
};
static const unsigned char mode_bpp4[] = {
#if 0
    0x0D, 0x0E,
#endif
    0x10, 0x12,
    0
};

typedef struct _driver_depth_modes {
    int bit_depth;
    const int * resolutions;
    const unsigned char * modes;
    fnUpdate pfnUpdate;
} driver_depth_modes;

static const driver_depth_modes scr_modes[] = {
    { 1, res_bpp1, mode_bpp1, driver_update_bpp1 },
    { 2, res_bpp2, mode_bpp2, driver_update_bpp2 },
    { 4, res_bpp4, mode_bpp4, driver_update_bpp4 }
};

#define DRV_DEPTH_COUNT (sizeof(scr_modes)/sizeof(scr_modes[0]))

static unsigned char uchScanLineBuffer[ 640 ];

static unsigned int driver_code_size( fnUpdate start, fnUpdate end )
{
	uintptr_t start_addr = (uintptr_t)start;
	uintptr_t end_addr = (uintptr_t)end;
	uintptr_t code_size;

	/* DJGPP function pointers are code offsets in the current code segment. */
	if( end_addr < start_addr )
		return 0;
	code_size = end_addr - start_addr;
	if( code_size > UINT_MAX )
		return 0;
	return (unsigned int)code_size;
}

static int driver_init(char *title, int w, int h, int depth, int refresh_rate, int flags)
{
    int i;
    unsigned char uchNewMode;
    const driver_depth_modes *depth_modes = NULL;
    int iResCount, iFoundRes;

    if( (fb_gfx == NULL) || (fb_gfx->scanline_size <= 0) ||
        (w <= 0) || (w > SHRT_MAX) || (h <= 0) || (h > USHRT_MAX) )
        return -1;

    fb_dos_detect();

    /* Remove this dumb "fake" scan line size */
    h /= fb_gfx->scanline_size;

	if (flags & DRIVER_OPENGL)
		return -1;

    for( i=0; i!=DRV_DEPTH_COUNT; ++i ) {
        if( scr_modes[i].bit_depth == depth ) {
            depth_modes = scr_modes + i;
            break;
        }
    }

    if (depth_modes == NULL)
        return -1;

    iResCount = 0;
    while( depth_modes->resolutions[ iResCount ]!=0 )
        ++iResCount;

    iFoundRes = iResCount;
    for( i=0; i!=iResCount; ++i ) {
        if( depth_modes->resolutions[ i ]==SCREENLIST( w, h ) ) {
            iFoundRes = i;
            break;
        }
    }

    if( iFoundRes==iResCount )
        return -1;

    fb_dos.regs.h.ah = 0x00;
    fb_dos.regs.h.al = depth_modes->modes[ iFoundRes ];
    __dpmi_int(0x10, &fb_dos.regs);

    _movedatab( _dos_ds, 0x449, (unsigned int)_my_ds(),
                (unsigned int)(uintptr_t)&uchNewMode, 1U );
    if( uchNewMode!=depth_modes->modes[ iFoundRes ] )
        return -1;

    /* We only need a scanline_size of 1 because the doubling will be done
     * by the graphics card itself. */
    fb_gfx->scanline_size = 1;
    refresh_rate = 70;

    fb_dos.bios_mode = depth_modes->modes[ iFoundRes ];
	fb_dos.update = depth_modes->pfnUpdate;
	fb_dos.update_len = driver_code_size( fb_dos.update, end_of_driver_update );
    if( fb_dos.bios_mode >= 0x11 ) {
        /* 16 or 256 colors out of 64*64*64 (262144) */
        fb_dos.set_palette = fb_dos_vga_set_palette;
    } else if( fb_dos.bios_mode >= 0x0D ) {
        /* EGA exposes 16 palette registers selected from a 64 color set. */
        fb_dos.set_palette = fb_dos_ega_set_palette;
    } else {
        /* CGA can only choose the background and one of its fixed palettes. */
        fb_dos.set_palette = fb_dos_cga_set_palette;
    }

	return fb_dos_init(title, w, h, depth, refresh_rate, flags);

}

static unsigned char fb_dos_ega_quantize(int component)
{
	if( component <= 10 )
		return 0;
	if( component <= 31 )
		return 1;
	if( component <= 52 )
		return 2;
	return 3;
}

static unsigned char fb_dos_ega_color(int r, int g, int b)
{
	unsigned char rr, gg, bb;

	rr = fb_dos_ega_quantize(r);
	gg = fb_dos_ega_quantize(g);
	bb = fb_dos_ega_quantize(b);

	return (unsigned char)(((bb & 1) << 0) |
	                       ((gg & 1) << 1) |
	                       ((rr & 1) << 2) |
	                       ((bb & 2) << 2) |
	                       ((gg & 2) << 3) |
	                       ((rr & 2) << 4));
}

static void fb_dos_ega_set_palette(void)
{
	int i;
	unsigned int color_count;
	if( (fb_dos.depth < 1) || (fb_dos.depth > 4) ||
	    (fb_dos.pal_first < 0) || (fb_dos.pal_first > 255) ||
	    (fb_dos.pal_last < 0) || (fb_dos.pal_last > 255) )
		return;

	color_count = 1U << (unsigned int)fb_dos.depth;
	if( color_count > (unsigned int)fb_dos.pal_last + 1U )
		color_count = (unsigned int)fb_dos.pal_last + 1U;
	if( color_count > 16U )
		color_count = 16U;

	for (i = fb_dos.pal_first; (unsigned int)i < color_count; i++) {
		fb_dos.regs.x.ax = 0x1000;
		fb_dos.regs.h.bl = (unsigned char)i;
		fb_dos.regs.h.bh = fb_dos_ega_color(fb_dos.pal[i].r, fb_dos.pal[i].g, fb_dos.pal[i].b);
		__dpmi_int(0x10, &fb_dos.regs);
	}

	fb_dos.pal_dirty = FALSE;
	fb_dos.pal_last = 0;
	fb_dos.pal_first = 256;
}

static const unsigned char cga_rgb[16][3] = {
	{  0,  0,  0 }, {  0,  0, 42 }, {  0, 42,  0 }, {  0, 42, 42 },
	{ 42,  0,  0 }, { 42,  0, 42 }, { 42, 21,  0 }, { 42, 42, 42 },
	{ 21, 21, 21 }, { 21, 21, 63 }, { 21, 63, 21 }, { 21, 63, 63 },
	{ 63, 21, 21 }, { 63, 21, 63 }, { 63, 63, 21 }, { 63, 63, 63 }
};

static const unsigned char cga_palettes[4][3] = {
	{  2,  4,  6 },
	{ 10, 12, 14 },
	{  3,  5,  7 },
	{ 11, 13, 15 }
};

static int fb_dos_cga_color_distance(int pal_index, int cga_index)
{
	int dr, dg, db;

	dr = fb_dos.pal[pal_index].r - cga_rgb[cga_index][0];
	dg = fb_dos.pal[pal_index].g - cga_rgb[cga_index][1];
	db = fb_dos.pal[pal_index].b - cga_rgb[cga_index][2];

	return (dr * dr) + (dg * dg) + (db * db);
}

static unsigned char fb_dos_cga_nearest_color(int pal_index)
{
	int i, best, best_distance, distance;

	best = 0;
	best_distance = fb_dos_cga_color_distance(pal_index, 0);

	for (i = 1; i < 16; ++i) {
		distance = fb_dos_cga_color_distance(pal_index, i);
		if (distance < best_distance) {
			best = i;
			best_distance = distance;
		}
	}

	return (unsigned char)best;
}

static unsigned char fb_dos_cga_best_palette(void)
{
	int pal, color, score, best, best_score;

	best = 0;
	best_score = 0x7FFFFFFF;

	for (pal = 0; pal < 4; ++pal) {
		score = 0;
		for (color = 1; color < 4; ++color)
			score += fb_dos_cga_color_distance(color, cga_palettes[pal][color - 1]);
		if (score < best_score) {
			best = pal;
			best_score = score;
		}
	}

	return (unsigned char)best;
}

static void fb_dos_cga_set_palette(void)
{
	unsigned char background;
	unsigned char palette;
	unsigned char control;

	background = fb_dos_cga_nearest_color(0);
	palette = fb_dos_cga_best_palette();
	control = (unsigned char)((palette >= 2) ? 1 : 0);
	if (palette & 1)
		control |= 0x10;

	fb_dos.regs.h.ah = 0x0B;
	fb_dos.regs.h.bh = 0x00;
	fb_dos.regs.h.bl = background;
	__dpmi_int(0x10, &fb_dos.regs);

	fb_dos.regs.h.ah = 0x0B;
	fb_dos.regs.h.bh = 0x01;
	fb_dos.regs.h.bl = control;
	__dpmi_int(0x10, &fb_dos.regs);

	fb_dos.pal_dirty = FALSE;
	fb_dos.pal_last = 0;
	fb_dos.pal_first = 256;
}

static void init_planar_access( void )
{
    /* Write mask: Allow all bits */
    fb_gfx_dos_outportb(0x3CE, 0x08);
    fb_gfx_dos_outportb(0x3CF, 0xFF);

    /* Write mode: 0 */
    fb_gfx_dos_outportb(0x3CE, 0x05);
    fb_gfx_dos_outportb(0x3CF, 0x00);

    /* Overwrite mode, no rotation */
    fb_gfx_dos_outportb(0x3CE, 0x03);
    fb_gfx_dos_outportb(0x3CF, 0x00);

    /* Enable write access for plane "plane" */
    fb_gfx_dos_outportb(0x3C4, 0x02);
}

static void driver_update_planar_ega_vga( unsigned int planes )
{
    /* EGA/VGA planar mode */
    unsigned int y;
    unsigned int w, h, w8, w32;
    unsigned char *buffer;
    unsigned screen_offset = 0xA0000;
    static const unsigned char vga_bitmap[8] = {
        128, 64, 32, 16, 8, 4, 2, 1
    };
    static const unsigned char bitmask[4] = {
        1, 2, 4, 8
    };
    unsigned int uiDestOffset[4];

    if( (fb_gfx == NULL) || (fb_gfx->framebuffer == NULL) ||
        (fb_gfx->dirty == NULL) || (fb_dos.w <= 0) || (fb_dos.w > 640) ||
        (fb_dos.h <= 0) || (fb_dos.w % 8 != 0) ||
        (planes == 0) || (planes > 4) )
        return;

    w = (unsigned int)fb_dos.w;
    h = (unsigned int)fb_dos.h;
    w8 = w >> 3;
    w32 = w >> 5;
    buffer = fb_gfx->framebuffer;
    uiDestOffset[0] = 0;
    uiDestOffset[1] = w8;
    uiDestOffset[2] = w8 * 2U;
    uiDestOffset[3] = w8 * 3U;

    init_planar_access();

    for (y = 0;
         y != h;
         ++y)
    {
        if (fb_gfx->dirty[y]) {
            unsigned x, uiDestIndex = 0;
            unsigned char *pFrameBuffer = buffer;
            unsigned plane;

            /* Split "per pixel" color values into a plane oriented
             * representation */
            for( x=0; x!=w; x+=8 ) {
                unsigned i;
                for( plane=0; plane!=planes; ++plane ) {
                    unsigned char color_mask = bitmask[plane];
                    unsigned char pattern = 0;
                    for( i=0; i!=8; ++i ) {
                        if (pFrameBuffer[i] & color_mask)
                            pattern |= vga_bitmap[i];
                    }
                    uchScanLineBuffer[ uiDestOffset[plane] + uiDestIndex ] = pattern;
                }
                pFrameBuffer += 8;
                ++uiDestIndex;
            }
            for( plane=0; plane!=planes; ++plane ) {
                fb_gfx_dos_outportb(0x3C5, bitmask[plane] ); /* Select plane */
                _movedatal((unsigned int)_my_ds(),
                           (unsigned long)((uintptr_t)uchScanLineBuffer +
                                           (uintptr_t)uiDestOffset[ plane ]),
                           _dos_ds,
                           screen_offset,
                           (unsigned long)w32);
            }
        }
        buffer += w;
        screen_offset += w8;
    }
}

static void driver_update_bpp1(void)
{
    unsigned int w, h, w4, w8, w32;
    unsigned char *buffer;
    if( (fb_gfx == NULL) || (fb_gfx->framebuffer == NULL) ||
        (fb_gfx->dirty == NULL) || (fb_dos.w <= 0) || (fb_dos.w > 640) ||
        (fb_dos.h <= 0) || (fb_dos.w % 32 != 0) )
        return;
    w = (unsigned int)fb_dos.w;
    h = (unsigned int)fb_dos.h;
    w4 = w >> 2;
    w8 = w >> 3;
    w32 = w >> 5;
    buffer = fb_gfx->framebuffer;

    if( fb_dos.bios_mode==0x06 ) {
        /* CGA interlaced mode */
        unsigned int y;
        unsigned int screen_even = 0xB8000;
        unsigned int screen_odd  = 0xBA000;

        for (y = 0;
             y != h;
             ++y)
        {
            unsigned int *pScreenOffset = ( ((y & 1)==0) ? &screen_even : &screen_odd );
            if (fb_gfx->dirty[y]) {
                unsigned dx = w8;
                unsigned x = w4;
                unsigned char uchDst = 0;
                while( x-- ) {
                    unsigned int value;
                    memcpy( &value, buffer + (size_t)x * sizeof(value), sizeof(value) );
                    if( x & 1 ) {
                        uchDst = (unsigned char)
                            (
                             ((value & 0x01000000) >> 24) +
                             ((value & 0x00010000) >> 15) +
                             ((value & 0x00000100) >>  6) +
                             ((value & 0x00000001) <<  3)
                            );
                    } else {
                        uchScanLineBuffer[dx--] = (unsigned char)
                            (
                             uchDst +
                             (
                              ((value & 0x01000000) >> 20) +
                              ((value & 0x00010000) >> 11) +
                              ((value & 0x00000100) >>  2) +
                              ((value & 0x00000001) <<  7)
                             )
                            );
                    }
                }
                _movedatal((unsigned int)_my_ds(), (unsigned long)(uintptr_t)uchScanLineBuffer,
                           _dos_ds, *pScreenOffset,
                           (unsigned long)w32);
            }
            buffer += w;
            *pScreenOffset += w8;
        }
    } else {
        /* EGA/VGA linear mode */
        unsigned int y;
        unsigned int screen_offset = 0xA0000;

        init_planar_access();
        fb_gfx_dos_outportb(0x3C5, 0x0F); /* All planes */

        for (y = 0;
             y != h;
             ++y)
        {
            if (fb_gfx->dirty[y]) {
                unsigned dx = w8;
                unsigned x = w4;
                unsigned char uchDst = 0;
                while( x-- ) {
                    unsigned int value;
                    memcpy( &value, buffer + (size_t)x * sizeof(value), sizeof(value) );
                    if( x & 1 ) {
                        uchDst = (unsigned char)
                            (
                             ((value & 0x01000000) >> 24) +
                             ((value & 0x00010000) >> 15) +
                             ((value & 0x00000100) >>  6) +
                             ((value & 0x00000001) <<  3)
                            );
                    } else {
                        uchScanLineBuffer[dx--] = (unsigned char)
                            (
                             uchDst +
                             (
                              ((value & 0x01000000) >> 20) +
                              ((value & 0x00010000) >> 11) +
                              ((value & 0x00000100) >>  2) +
                              ((value & 0x00000001) <<  7)
                             )
                            );
                    }
                }
                _movedatal((unsigned int)_my_ds(), (unsigned long)(uintptr_t)uchScanLineBuffer,
                           _dos_ds, screen_offset,
                           (unsigned long)w32);
            }
            buffer += w;
            screen_offset += w8;
        }
    }
}

static void driver_update_bpp2(void)
{
	unsigned int y, w, h, w4, w16;
	unsigned char *buffer;
    unsigned int screen_even = 0xB8000;
    unsigned int screen_odd  = 0xBA000;
	if( (fb_gfx == NULL) || (fb_gfx->framebuffer == NULL) ||
	    (fb_gfx->dirty == NULL) || (fb_dos.w <= 0) || (fb_dos.w > 640) ||
	    (fb_dos.h <= 0) || (fb_dos.w % 16 != 0) )
		return;
	w = (unsigned int)fb_dos.w;
	h = (unsigned int)fb_dos.h;
	w4 = w >> 2;
	w16 = w >> 4;
	buffer = fb_gfx->framebuffer;

    for (y = 0;
         y != h;
         ++y)
    {
        unsigned int *pScreenOffset = ( ((y & 1)==0) ? &screen_even : &screen_odd );
        if (fb_gfx->dirty[y]) {
            unsigned x = w4;
            while( x-- ) {
                unsigned int value;
                memcpy( &value, buffer + (size_t)x * sizeof(value), sizeof(value) );
                uchScanLineBuffer[x] = (unsigned char)
                    (
                     ((value & 0x03000000) >> 24) +
                     ((value & 0x00030000) >> 14) +
                     ((value & 0x00000300) >>  4) +
                     ((value & 0x00000003) <<  6)
                    );
            }
            _movedatal((unsigned int)_my_ds(), (unsigned long)(uintptr_t)uchScanLineBuffer,
                       _dos_ds, *pScreenOffset,
                       (unsigned long)w16);
        }
        buffer += w;
        *pScreenOffset += w4;
    }
}

static void driver_update_bpp4(void)
{
    if( fb_dos.bios_mode >= 0x10 ) {
        driver_update_planar_ega_vga( 4 );
    }
}
static void end_of_driver_update(void) { /* do not remove */ }

static int *driver_fetch_modes(int depth, int *size)
{
    int i;
    const driver_depth_modes *depth_modes = NULL;
    int iResCount;
    size_t allocation_size;
    int *modes;
#ifdef DEBUG
    int iModeCount;
#endif
    for( i=0; i!=DRV_DEPTH_COUNT; ++i ) {
        if( scr_modes[i].bit_depth == depth ) {
            depth_modes = scr_modes + i;
            break;
        }
    }

    if ((depth_modes == NULL) || (size == NULL))
        return NULL;

    iResCount = 0;
    while( depth_modes->resolutions[ iResCount ]!=0 )
        ++iResCount;

#ifdef DEBUG
    iModeCount = 0;
    while( depth_modes->modes[ iModeCount ]!=0 )
        ++iModeCount;
    DBG_ASSERT( iModeCount==iResCount );
#endif

	if( (iResCount <= 0) ||
	    ((size_t)iResCount > (size_t)-1 / sizeof(int)) )
		return NULL;
	allocation_size = (size_t)iResCount * sizeof(int);
	modes = (int *)malloc( allocation_size );
	if( modes == NULL )
		return NULL;
	memcpy( modes, depth_modes->resolutions, allocation_size );
	*size = iResCount;
	return modes;
}
