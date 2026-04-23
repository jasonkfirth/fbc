#include "../fb_gfx.h"
#include "fb_gfx_darwin.h"

#ifdef HOST_DARWIN

static int driver_init(char *title, int w, int h, int depth, int refresh_rate, int flags)
{
	return fb_hDarwinInit(title, w, h, depth, refresh_rate, flags);
}

const GFXDRIVER fb_gfxDriverDarwin = {
	"Darwin",
	driver_init,
	fb_hDarwinExit,
	fb_hDarwinLock,
	fb_hDarwinUnlock,
	fb_hDarwinSetPalette,
	fb_hDarwinWaitVSync,
	fb_hDarwinGetMouse,
	fb_hDarwinSetMouse,
	fb_hDarwinSetWindowTitle,
	fb_hDarwinSetWindowPos,
	fb_hDarwinFetchModes,
	NULL,
	fb_hDarwinPollEvents,
	fb_hDarwinUpdate
};

#endif
