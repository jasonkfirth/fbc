/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------

    File: amiga/fb_amiga.h

    Purpose:
        Define the native C runtime and DOS boundary for AmigaOS 3.x.

    Responsibilities:
        - describe calling convention, console limits, and file offsets
        - provide checked adapters for newlib's 32-bit stream positioning

    This file intentionally does NOT contain:
        - library ownership, graphics, sound, or generic m68k ABI policy
*/

#ifndef __FB_AMIGA_H__
#define __FB_AMIGA_H__

#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <limits.h>

#define FBCALL
#define FB_NEWLINE "\n"
#define FB_NEWLINE_WSTR _LC("\n")
#define FB_BINARY_NEWLINE "\r\n"
#define FB_BINARY_NEWLINE_WSTR _LC("\r\n")
#define FB_LL_FMTMOD "ll"
#define FB_CONSOLE_MAXPAGES 1

/* NDK names the BCPL null handle as zero; AROS also provides BNULL. Native
   replacement sources use this spelling to make handle ownership explicit. */
#ifndef BNULL
#define BNULL 0
#endif

#undef alloca
#define alloca(size) __builtin_alloca(size)

/* Keep BASIC's 64-bit file positions intact until the checked DOS boundary.
   Amiga newlib provides fseek/ftell, with signed 32-bit offsets. Silently
   casting a larger BASIC position would seek to an unrelated file location. */
typedef long long fb_off_t;

static __inline__ int fb_hAmigaFseek(FILE *stream, fb_off_t offset, int whence)
{
	if( offset < LONG_MIN || offset > LONG_MAX ) {
		errno = EOVERFLOW;
		return -1;
	}
	return fseek(stream, (long)offset, whence);
}

#define fseeko(stream, offset, whence) fb_hAmigaFseek(stream, offset, whence)
#define ftello(stream) ((fb_off_t)ftell(stream))

FBCALL void fb_BgLock(void);
FBCALL void fb_BgUnlock(void);
#define BG_LOCK() fb_BgLock()
#define BG_UNLOCK() fb_BgUnlock()

void fb_hAmigaDebug(const char *message);
void fb_hAmigaDebugNumber(const char *message, unsigned long value);

/* File-device callers hold FB_LOCK while consulting newlib's descriptor table. */
int fb_hAmigaGetFileHandle(FILE *stream, unsigned long *handle);

#endif

/* end of amiga/fb_amiga.h */
