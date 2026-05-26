/*
    FreeBASIC runtime Wii initialization
    ------------------------------------

    File: hinit.c

    Purpose:

        Initialize the minimal libogc services needed by ordinary
        FreeBASIC programs on the Wii.

    Responsibilities:

        - initialize the VI video subsystem and console fallback
        - initialize Wii Remote and GameCube controller polling
        - initialize the default FAT filesystem for ordinary file I/O
        - make the SD card root the default working directory when present
        - provide runtime-wide mutexes for MT builds
        - expose the selected video mode to gfxlib

    This file intentionally does NOT contain:

        - gfxlib page presentation
        - sfxlib audio streaming
        - path translation for individual file APIs

    Platform notes:

        libogc programs commonly set up a simple external framebuffer before
        doing anything that might print to stdout.  FreeBASIC console programs
        can print before selecting SCREEN, so the runtime owns a small fallback
        console mode.  gfxlib is free to reconfigure video when SCREEN is used.
*/

#include "../fb.h"
#include <fat.h>
#include <ogc/mutex.h>
#include <ogc/pad.h>
#include <wiiuse/wpad.h>

static GXRModeObj *fb_wii_rmode = NULL;
static void *fb_wii_console_xfb = NULL;
static int fb_wii_video_ready = FALSE;
static int fb_wii_filesystem_ready = FALSE;

#ifdef ENABLE_MT
static mutex_t fb_wii_global_mutex = LWP_MUTEX_NULL;
static mutex_t fb_wii_string_mutex = LWP_MUTEX_NULL;
static mutex_t fb_wii_graphics_mutex = LWP_MUTEX_NULL;
static mutex_t fb_wii_math_mutex = LWP_MUTEX_NULL;
static mutex_t fb_wii_profile_mutex = LWP_MUTEX_NULL;

static void fb_WiiMutexInit(mutex_t *mutex)
{
	if (*mutex == LWP_MUTEX_NULL)
		LWP_MutexInit(mutex, TRUE);
}

static void fb_WiiMutexDestroy(mutex_t *mutex)
{
	if (*mutex != LWP_MUTEX_NULL) {
		LWP_MutexDestroy(*mutex);
		*mutex = LWP_MUTEX_NULL;
	}
}

static void fb_WiiThreadLocksInit(void)
{
	fb_WiiMutexInit(&fb_wii_global_mutex);
	fb_WiiMutexInit(&fb_wii_string_mutex);
	fb_WiiMutexInit(&fb_wii_graphics_mutex);
	fb_WiiMutexInit(&fb_wii_math_mutex);
	fb_WiiMutexInit(&fb_wii_profile_mutex);
}

static void fb_WiiThreadLocksEnd(void)
{
	fb_WiiMutexDestroy(&fb_wii_profile_mutex);
	fb_WiiMutexDestroy(&fb_wii_math_mutex);
	fb_WiiMutexDestroy(&fb_wii_graphics_mutex);
	fb_WiiMutexDestroy(&fb_wii_string_mutex);
	fb_WiiMutexDestroy(&fb_wii_global_mutex);
}

FBCALL void fb_Lock(void)            { LWP_MutexLock(fb_wii_global_mutex); }
FBCALL void fb_Unlock(void)          { LWP_MutexUnlock(fb_wii_global_mutex); }
FBCALL void fb_StrLock(void)         { LWP_MutexLock(fb_wii_string_mutex); }
FBCALL void fb_StrUnlock(void)       { LWP_MutexUnlock(fb_wii_string_mutex); }
FBCALL void fb_GraphicsLock(void)    { LWP_MutexLock(fb_wii_graphics_mutex); }
FBCALL void fb_GraphicsUnlock(void)  { LWP_MutexUnlock(fb_wii_graphics_mutex); }
FBCALL void fb_MathLock(void)        { LWP_MutexLock(fb_wii_math_mutex); }
FBCALL void fb_MathUnlock(void)      { LWP_MutexUnlock(fb_wii_math_mutex); }
FBCALL void fb_ProfileLock(void)     { LWP_MutexLock(fb_wii_profile_mutex); }
FBCALL void fb_ProfileUnlock(void)   { LWP_MutexUnlock(fb_wii_profile_mutex); }
#endif

void fb_WiiVideoInit(void)
{
	if (fb_wii_video_ready)
		return;

	VIDEO_Init();

	fb_wii_rmode = VIDEO_GetPreferredMode(NULL);
	if (fb_wii_rmode == NULL)
		return;

	fb_wii_console_xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(fb_wii_rmode));
	if (fb_wii_console_xfb == NULL)
		return;

	VIDEO_Configure(fb_wii_rmode);
	VIDEO_SetNextFramebuffer(fb_wii_console_xfb);
	VIDEO_ClearFrameBuffer(fb_wii_rmode, fb_wii_console_xfb, COLOR_BLACK);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	if (fb_wii_rmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync();

	CON_Init(fb_wii_console_xfb,
	         20,
	         20,
	         fb_wii_rmode->fbWidth,
	         fb_wii_rmode->xfbHeight,
	         fb_wii_rmode->fbWidth * VI_DISPLAY_PIX_SZ);

	fb_wii_video_ready = TRUE;
}

GXRModeObj *fb_WiiGetRenderMode(void)
{
	if (!fb_wii_video_ready)
		fb_WiiVideoInit();

	return fb_wii_rmode;
}

void *fb_WiiGetConsoleFrameBuffer(void)
{
	if (!fb_wii_video_ready)
		fb_WiiVideoInit();

	return fb_wii_console_xfb;
}

static void fb_WiiFilesystemInit(void)
{
	if (fb_wii_filesystem_ready)
		return;

	/*
	    libfat provides the SD/USB filesystem layer used by normal Wii
	    homebrew.  FreeBASIC programs commonly use relative paths such as
	    "./file/data.txt"; on real hardware those files normally live on the
	    SD card beside the application.  When an SD card is present, make that
	    convention explicit by selecting "sd:/" as the process working
	    directory.

	    If no FAT device is present, leave the default newlib state alone.
	    This keeps pure console, graphics, and sound programs working even when
	    the user has not inserted an SD card.
	*/
	if (fatInitDefault()) {
		chdir("sd:/");
		fb_wii_filesystem_ready = TRUE;
	}
}

void fb_hInit(void)
{
	fb_WiiVideoInit();
	fb_WiiFilesystemInit();
#ifdef ENABLE_MT
	fb_WiiThreadLocksInit();
#endif
	PAD_Init();
	WPAD_Init();
}

void fb_hEnd(int unused)
{
	(void)unused;
#ifdef ENABLE_MT
	fb_WiiThreadLocksEnd();
#endif
}

/* end of hinit.c */
