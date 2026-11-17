/* common VESA VBE routines
 * Partially based on Allegro src/dos/vesa.c
 */

#include "../../fb_gfx.h"
#include "fb_gfx_dos.h"
#include <go32.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/farptr.h>

VesaPMInfo *fb_dos_vesa_pm_info = NULL;
intptr_t fb_dos_vesa_pm_bank_switcher = 0;
intptr_t fb_dos_vesa_pm_set_palette = 0;

static int detected = FALSE;

static int vesa_read_u16(const unsigned char *data, size_t data_size,
                         size_t offset, unsigned short *value)
{
	if (!data || !value || offset > data_size ||
	    sizeof(*value) > data_size - offset)
		return 0;
	memcpy(value, data + offset, sizeof(*value));
	return 1;
}

static int vesa_read_u32(const unsigned char *data, size_t data_size,
                         size_t offset, unsigned long *value)
{
	uint32_t wire_value;
	if (!data || !value || offset > data_size ||
	    sizeof(wire_value) > data_size - offset)
		return 0;
	memcpy(&wire_value, data + offset, sizeof(wire_value));
	*value = (unsigned long)wire_value;
	return 1;
}

void vesa_get_pm_functions( void )
{
	VesaPMInfo *pm_info;
	unsigned char *pm_bytes;
	unsigned short table_size;
	size_t io_offset, offset;
	unsigned short entry;
	unsigned long mmio_address;
	unsigned short mmio_limit;
	__dpmi_meminfo meminfo;
	
	if (fb_dos.vesa_use_pm && fb_dos_vesa_pm_info)
		return;
	fb_dos.vesa_use_pm = FALSE;
	fb_dos_vesa_pm_info = NULL;
	fb_dos_vesa_pm_bank_switcher = 0;
	fb_dos_vesa_pm_set_palette = 0;
	
	if( fb_dos.vesa_info.vbe_version < 0x200 )
		return;
	
	fb_dos.regs.x.ax = VBE_PMODE;
	fb_dos.regs.x.bx = VBE_PMODE_GET;
	__dpmi_int( 0x10, &fb_dos.regs );
	if( fb_dos.regs.x.ax != VBE_STATUS_OK )
		return;
	table_size = fb_dos.regs.x.cx;
	if (table_size < sizeof(*pm_info))
		return;

	pm_info = malloc((size_t)table_size);
	if (!pm_info)
		return;
	dosmemget(SEGOFF_TO_RM(fb_dos.regs.x.es, fb_dos.regs.x.di),
	          (unsigned int)table_size, pm_info);
	pm_bytes = (unsigned char *)pm_info;
	if ((pm_info->setWindow >= table_size) ||
	    (pm_info->setPalette >= table_size))
		goto invalid_table;
	io_offset = (size_t)pm_info->IOPrivInfo;

	if (io_offset)
	{
		/* The VBE table stores port words before its optional MMIO record. */
		offset = io_offset;
		for (;;)
		{
			if (!vesa_read_u16(pm_bytes, (size_t)table_size, offset, &entry))
				goto invalid_table;
			offset += sizeof(entry);
			if (entry == 0xFFFFU)
				break;
		}

		if (!vesa_read_u32(pm_bytes, (size_t)table_size, offset,
		                   &mmio_address) ||
		    !vesa_read_u16(pm_bytes, (size_t)table_size,
		                   offset + sizeof(uint32_t), &mmio_limit) ||
		    !vesa_read_u16(pm_bytes, (size_t)table_size,
		                   offset + sizeof(uint32_t) + sizeof(mmio_limit),
		                   &entry) || entry != 0xFFFFU)
			goto invalid_table;

		if (mmio_address)
		{
			meminfo.address = mmio_address;
			meminfo.size = (unsigned long)mmio_limit + 1UL;
			if (__dpmi_physical_address_mapping(&meminfo) != 0)
				goto invalid_table;
			if (__dpmi_lock_linear_region(&meminfo) != 0)
			{
				__dpmi_free_physical_address_mapping(&meminfo);
				goto invalid_table;
			}

			fb_dos.vesa_mmio_linear = meminfo.address;
			fb_dos.vesa_mmio_sel = __dpmi_allocate_ldt_descriptors(1);
			if (fb_dos.vesa_mmio_sel <= 0)
			{
				fb_dos.vesa_mmio_sel = 0;
				__dpmi_unlock_linear_region(&meminfo);
				__dpmi_free_physical_address_mapping(&meminfo);
				goto invalid_table;
			}
			if (__dpmi_set_segment_base_address(fb_dos.vesa_mmio_sel,
			                                    meminfo.address) == -1 ||
			    __dpmi_set_segment_limit(fb_dos.vesa_mmio_sel,
			                             meminfo.size - 1UL) != 0)
			{
				__dpmi_free_ldt_descriptor(fb_dos.vesa_mmio_sel);
				fb_dos.vesa_mmio_sel = 0;
				__dpmi_unlock_linear_region(&meminfo);
				__dpmi_free_physical_address_mapping(&meminfo);
				goto invalid_table;
			}
		}
	}
	fb_dos_vesa_pm_info = pm_info;
	fb_dos_vesa_pm_bank_switcher = (intptr_t)((uintptr_t)pm_info +
	                                             (uintptr_t)pm_info->setWindow);
	fb_dos_vesa_pm_set_palette = (intptr_t)((uintptr_t)pm_info +
	                                        (uintptr_t)pm_info->setPalette);
	
	fb_dos.vesa_use_pm = TRUE;
	return;

invalid_table:
	free(pm_info);
}

static int vesa_get_mode_info( int mode )
{
	if ((mode < 0) || (mode > USHRT_MAX))
		return -1;
	fb_hFarMemSet( _dos_ds, MASK_LINEAR( __tb ), 0, sizeof( VesaModeInfo ) );
	
	fb_dos.regs.x.ax = VBE_MODEINFO;
	fb_dos.regs.x.di = RM_OFFSET( __tb );
	fb_dos.regs.x.es = RM_SEGMENT( __tb );
	fb_dos.regs.x.cx = (unsigned short)mode;
	__dpmi_int( 0x10, &fb_dos.regs );
	if( fb_dos.regs.x.ax != VBE_STATUS_OK )
		return -1;
	
	dosmemget( MASK_LINEAR( __tb ), sizeof(fb_dos.vesa_mode_info), &fb_dos.vesa_mode_info );
	return 0;
}

void fb_dos_vesa_detect(void)
{
	int mode_list[256];
	int number_of_modes;
	unsigned long mode_ptr;
	int c;
	int attribs = VMI_MA_SUPPORTED | VMI_MA_COLOR | VMI_MA_GRAPHICS;
	
	if( detected )
		return;
	
	detected = TRUE;
	
	fb_dos.vesa_ok = FALSE;
	
	fb_hFarMemSet( _dos_ds, MASK_LINEAR( __tb ), 0, sizeof( VbeInfoBlock ) );
	dosmemput( "VBE2", 4, MASK_LINEAR( __tb ) ); /* get VESA 2 info if available */
	
	fb_dos.regs.x.ax = VBE_INFO;
	fb_dos.regs.x.di = RM_OFFSET( __tb );
	fb_dos.regs.x.es = RM_SEGMENT( __tb );
	__dpmi_int( 0x10, &fb_dos.regs );
	
	if( fb_dos.regs.x.ax != VBE_STATUS_OK )
	{
		return;
	}
	
	dosmemget( MASK_LINEAR( __tb ), sizeof( VbeInfoBlock ), &fb_dos.vesa_info );
	
	if( strncmp( fb_dos.vesa_info.vbe_signature, "VESA", 4 ) != 0 )
	{
		return;
	}
	
	fb_dos.vesa_ok = TRUE;
	
	/* get VESA modes */
	mode_ptr = RM_TO_LINEAR( fb_dos.vesa_info.video_mode_ptr );
	
	number_of_modes = 0;
	
	while( (number_of_modes < (int)(sizeof(mode_list) / sizeof(mode_list[0]))) &&
	       (_farpeekw( _dos_ds, mode_ptr ) != 0xFFFF ) )
	{
		mode_list[number_of_modes] = (int)_farpeekw( _dos_ds, mode_ptr );
		number_of_modes++;
		mode_ptr += 2;
	}
	
	fb_dos.num_vesa_modes = number_of_modes;
	if (number_of_modes == 0)
	{
		fb_dos.vesa_modes = NULL;
		return;
	}
	fb_dos.vesa_modes = calloc((size_t)number_of_modes,
	                            sizeof(*fb_dos.vesa_modes));
	if (!fb_dos.vesa_modes)
	{
		fb_dos.num_vesa_modes = 0;
		return;
	}
	
	for( c = 0; c < number_of_modes; c++ )
	{
		if ( (vesa_get_mode_info(mode_list[c]) == 0) &&
		     ((fb_dos.vesa_mode_info.ModeAttributes & attribs) == attribs) && /* color graphics mode and supported */
		     (fb_dos.vesa_mode_info.NumberOfPlanes == 1) &&
		     ((fb_dos.vesa_mode_info.MemoryModel == VMI_MM_PACK) ||
		      (fb_dos.vesa_mode_info.MemoryModel == VMI_MM_DIR)) &&
			 ((fb_dos.vesa_info.vbe_version < 0x200) ||
			 ((unsigned long)fb_dos.vesa_info.total_memory << 16 >=
			  (unsigned long)fb_dos.vesa_mode_info.BytesPerScanLine *
			  (unsigned long)fb_dos.vesa_mode_info.YResolution)) )
		{
			/* clobber WinFuncPtr to hold mode number */
			fb_dos.vesa_mode_info.WinFuncPtr = (unsigned long)mode_list[c];
			
			/* add to list */
			memcpy( &fb_dos.vesa_modes[c], &fb_dos.vesa_mode_info, sizeof(VesaModeInfo) );
		}
	}
}

int fb_dos_vesa_set_mode( int w, int h, int depth, int linear )
{
	int i;
	unsigned short mode = 0;
	int bpp;
	int tries;
	int good_bpp;
	int success = 0;
	int palette_bytes;
	int palette_paragraphs;
	
	if( depth == 15 ) depth = 16;
	if( depth == 24 ) depth = 32;
	
	for( tries = 0; tries <= 2; tries++ )
	{
		for( i = 0; i < fb_dos.num_vesa_modes; i++ )
		{
			if( (fb_dos.vesa_modes[i].XResolution == w) && (fb_dos.vesa_modes[i].YResolution == h) )
			{
				bpp = fb_dos.vesa_modes[i].BitsPerPixel;
				if( tries == 0 )
				{
					good_bpp = (bpp == depth);
				}
				else if( tries == 1 )
				{
					good_bpp = (bpp == 24 && depth == 32) ||
					           (bpp == 15 && depth == 16);
				}
				else /*if( tries == 2 )*/
				{
					good_bpp = ((bpp == 24 || bpp == 32) && depth < 24) ||
					           ((bpp == 15 || bpp == 16) && depth < 15);
				}
				
				if( good_bpp )
				{
					if ( !linear || (fb_dos.vesa_modes[i].ModeAttributes & VMI_MA_LFB) )
					{
						memcpy( &fb_dos.vesa_mode_info, &fb_dos.vesa_modes[i], sizeof(VesaModeInfo) );
						if (fb_dos.vesa_modes[i].WinFuncPtr > USHRT_MAX)
							continue;
						mode = (unsigned short)fb_dos.vesa_modes[i].WinFuncPtr;
						break;
					}
				}
			}
		}
		
		if( !mode )
			continue;
		
		fb_dos.regs.x.ax = VBE_SETMODE;
		fb_dos.regs.x.bx = mode;
		
		if( linear )
		{
			fb_dos.regs.x.bx = (unsigned short)(fb_dos.regs.x.bx | 0x4000U);
		}
		
		__dpmi_int( 0x10, &fb_dos.regs );
		success = (fb_dos.regs.h.ah == 0);
		
		if( success )
		{
			break;
		}
	}
	
	if( success )
	{
		palette_bytes = 256 * 4;
		palette_paragraphs = (palette_bytes + 15) >> 4;
		fb_dos.palbuf_seg = __dpmi_allocate_dos_memory( palette_paragraphs, &fb_dos.palbuf_sel );
		if( fb_dos.palbuf_seg == -1 )
		{
			fb_dos.palbuf_seg = fb_dos.palbuf_sel = 0;
			return -1;
		}
	}
	
	return (success ? 0 : -1);
}

int *fb_dos_vesa_fetch_modes(int depth, int *size)
{
	int *modes;
	int count, i;
	int bpp;

	if (!size)
		return NULL;
	*size = 0;
	if (!fb_dos.detected) fb_dos_detect();
	if (!detected) fb_dos_vesa_detect();
	if (!fb_dos.vesa_modes || (fb_dos.num_vesa_modes <= 0))
		return NULL;

	for (i = 0, count = 0; i < fb_dos.num_vesa_modes; i++)
	{
		if( fb_dos.vesa_modes[i].XResolution != 0 )
		{
			bpp = fb_dos.vesa_modes[i].BitsPerPixel;
			if( (bpp == depth) ||
			    (bpp == 15 && depth == 16) ||
			    (bpp == 16 && depth == 15) ||
			    (bpp == 24 && depth == 32) ||
			    (bpp == 32 && depth == 24) )
			{
				count++;
			}
		}
	}
	if (count == 0)
		return NULL;
	modes = malloc((size_t)count * sizeof(*modes));
	if (!modes)
		return NULL;
	for (i = 0, count = 0; i < fb_dos.num_vesa_modes; i++)
	{
		if (fb_dos.vesa_modes[i].XResolution != 0)
		{
			bpp = fb_dos.vesa_modes[i].BitsPerPixel;
			if ((bpp == depth) ||
			    (bpp == 15 && depth == 16) ||
			    (bpp == 16 && depth == 15) ||
			    (bpp == 24 && depth == 32) ||
			    (bpp == 32 && depth == 24))
			{
				modes[count++] = SCREENLIST(fb_dos.vesa_modes[i].XResolution,
				                            fb_dos.vesa_modes[i].YResolution);
			}
		}
	}
	*size = count;
	return modes;
}

static void fb_dos_vesa_set_palette_int10(void)
{
	unsigned int color_count;
	unsigned int palette_count;
	unsigned int first;
	unsigned int last;

	if ((fb_dos.depth < 0) ||
	    (fb_dos.pal_first < 0) || (fb_dos.pal_first > 255) ||
	    (fb_dos.pal_last < fb_dos.pal_first) || (fb_dos.pal_last > 255) ||
	    (fb_dos.palbuf_seg < 0) || (fb_dos.palbuf_seg > USHRT_MAX) ||
	    (fb_dos.palbuf_sel <= 0))
		return;
	first = (unsigned int)fb_dos.pal_first;
	last = (unsigned int)fb_dos.pal_last;
	palette_count = fb_dos.depth >= 8 ? 256U : (1U << (unsigned int)fb_dos.depth);
	if (first >= palette_count)
		return;
	color_count = last - first + 1U;
	if (color_count > palette_count - first)
		color_count = palette_count - first;
	if (color_count == 0)
		return;
	
	movedata((unsigned int)_my_ds(), (unsigned int)(uintptr_t)fb_dos.pal,
	         (unsigned int)fb_dos.palbuf_sel, first * 4U, color_count * 4U);
	
	fb_dos.regs.x.ax = VBE_SETGETPAL;
	fb_dos.regs.x.bx = (fb_dos.vesa_info.capabilities & VIB_BLANK
	                    ? VBE_SETGETPAL_SET_BLANK
	                    : VBE_SETGETPAL_SET);
	fb_dos.regs.x.cx = (unsigned short)color_count;
	fb_dos.regs.x.dx = (unsigned short)first;
	
	fb_dos.regs.x.es = (unsigned short)fb_dos.palbuf_seg;
	fb_dos.regs.x.di = 0;
	
	__dpmi_int( 0x10, &fb_dos.regs );
	
	if( fb_dos.regs.x.ax != VBE_STATUS_OK )
	{
		fb_dos.set_palette = fb_dos_vga_set_palette;
		fb_dos_vga_set_palette( );
		return;
	}
	
	fb_dos.pal_dirty = FALSE;
	fb_dos.pal_last = 0;
	fb_dos.pal_first = 256;
}

void fb_dos_vesa_set_palette( void )
{
	fb_dos.set_palette = fb_dos_vesa_set_palette_int10;
	fb_dos.set_palette( );
}
void fb_dos_vesa_set_palette_end(void){}
