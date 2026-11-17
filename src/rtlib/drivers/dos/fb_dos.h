/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/fb_dos.h
 * Purpose: FreeBASIC DOS runtime: fb_dos.h Platform ABI and shared DOS hardware helpers. No scheduler implementation.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
/* FreeBASIC DOS runtime: fb_dos.h
 * Platform ABI and shared DOS hardware helpers. No scheduler implementation.
 */
#ifndef FB_DOS_H
#define FB_DOS_H

#include <stdint.h>

#define FBCALL

/* newline for console/file I/O */
#define FB_NEWLINE "\r\n"
#define FB_NEWLINE_WSTR FB_WCHAR_LITERAL("\r\n")

/* newline for printer I/O */
#define FB_BINARY_NEWLINE "\r\n"
#define FB_BINARY_NEWLINE_WSTR FB_WCHAR_LITERAL("\r\n")

#define FB_LL_FMTMOD "ll"

#define FB_CONSOLE_MAXPAGES 8

#define FB_DYLIB void*

/* In DJGPP we don't use fseeko() at all, the DJGPP semi-2.04 setup used for
   FB doesn't even seem to have it. */
typedef long fb_off_t;
#define fseeko(stream, offset, whence) fseek(stream, offset, whence)
#define ftello(stream)                 ftell(stream)

#define FB_COLOR_BLACK     (0)
#define FB_COLOR_BLUE      (1)
#define FB_COLOR_GREEN     (2)
#define FB_COLOR_CYAN      (3)
#define FB_COLOR_RED       (4)
#define FB_COLOR_MAGENTA   (5)
#define FB_COLOR_BROWN     (6)
#define FB_COLOR_WHITE     (7)
#define FB_COLOR_GREY      (8)
#define FB_COLOR_LBLUE     (9)
#define FB_COLOR_LGREEN    (10)
#define FB_COLOR_LCYAN     (11)
#define FB_COLOR_LRED      (12)
#define FB_COLOR_LMAGENTA  (13)
#define FB_COLOR_YELLOW    (14)
#define FB_COLOR_BWHITE    (15)

typedef struct {
    unsigned long edi;
    unsigned long esi;
    unsigned long ebp;
    unsigned long res;
    unsigned long ebx;
    unsigned long edx;
    unsigned long ecx;
    unsigned long eax;
} __dpmi_regs_d;

extern char *__fb_startup_cwd;

typedef int (*FnIntHandler)(unsigned irq_number);

unsigned long fb_hGetPageAddr( int pg, int cols, int rows );
int fb_isr_set( unsigned irq_number,
                FnIntHandler pfnIntHandler,
                size_t fn_size,
                size_t stack_size );
int fb_isr_reset( unsigned irq_number );
FnIntHandler fb_isr_get( unsigned irq_number );
/* To allow recursive CLI/STI */
int fb_dos_cli( void );
int fb_dos_sti( void );
int fb_dos_lock_data(const volatile void *address, size_t size);
int fb_dos_lock_code(uintptr_t address, size_t size);
int fb_dos_unlock_data(const volatile void *address, size_t size);
int fb_dos_unlock_code(uintptr_t address, size_t size);

/* DPMI locks linear address ranges. Integer addresses let code labels and
   data symbols share range checks without converting function pointers to
   object pointers or subtracting unrelated C pointers. */
static __inline__ size_t fb_dos_address_range_size(uintptr_t start, uintptr_t end)
{
	uintptr_t size;

	if( end < start )
		return 0;

	size = end - start;
#if defined(SIZE_MAX) && defined(UINTPTR_MAX) && SIZE_MAX < UINTPTR_MAX
	if( size > (uintptr_t)SIZE_MAX )
		return 0;
#endif
	return (size_t)size;
}

#define lock_var(var)           fb_dos_lock_data( &(var), sizeof(var) )
#define lock_array(array)       fb_dos_lock_data( (array), sizeof(array) )
#define lock_proc(proc)         fb_dos_lock_code( (uintptr_t)(proc), fb_dos_address_range_size( (uintptr_t)(proc), (uintptr_t)end_##proc ) )
#define lock_data(start, end)   fb_dos_lock_data( &(start), fb_dos_address_range_size( (uintptr_t)&(start), (uintptr_t)&(end) ) )
#define lock_code(start, end)   fb_dos_lock_code( (uintptr_t)(start), fb_dos_address_range_size( (uintptr_t)(start), (uintptr_t)(end) ) )

#define unlock_var(var)         fb_dos_unlock_data( &(var), sizeof(var) )
#define unlock_array(array)     fb_dos_unlock_data( (array), sizeof(array) )
#define unlock_proc(proc)       fb_dos_unlock_code( (uintptr_t)(proc), fb_dos_address_range_size( (uintptr_t)(proc), (uintptr_t)end_##proc ) )
#define unlock_data(start, end) fb_dos_unlock_data( &(start), fb_dos_address_range_size( (uintptr_t)&(start), (uintptr_t)&(end) ) )
#define unlock_code(start, end) fb_dos_unlock_code( (uintptr_t)(start), fb_dos_address_range_size( (uintptr_t)(start), (uintptr_t)(end) ) )

/* multikey() declarations also used by gfxlib2 */
#define END_OF_FUNCTION(proc)               void end_##proc (void) { }
#define END_OF_STATIC_FUNCTION(proc) static void end_##proc (void) { }
extern void (*__fb_dos_multikey_hook)(int scancode, int flags);
#define KB_PRESS    0x00000001
#define KB_REPEAT   0x00000002
#define KB_SHIFT    0x00000004
#define KB_CTRL     0x00000008
#define KB_ALT      0x00000010
#define KB_CAPSLOCK 0x00000020
#define KB_NUMLOCK  0x00000040
#define KB_EXTENDED 0x00000080

void fb_hFarMemSet ( unsigned short selector, unsigned long dest, unsigned char char_to_set, size_t bytes );
void fb_hFarMemSetW( unsigned short selector, unsigned long dest, unsigned short word_to_set, size_t words );

#endif

/* end of drivers/dos/fb_dos.h */
